#! /bin/bash
# paths required to run cpp files
image_config=${BASE_DIR}/config_files/file_config_input_remote
model_config=${BASE_DIR}/config_files/file_config_model
build_path=${BASE_DIR}/build_debwithrelinfo_gcc
build_path_model=${BASE_DIR}/Dataprovider/weights_provider
image_path=${BASE_DIR}/Dataprovider/image_provider
image_provider_path=${BASE_DIR}/Dataprovider/image_provider/Final_Output_Shares
debug_0=$build_path/server0/debug_files
debug_1=$build_path/server1/debug_files
scripts_path=${BASE_DIR}/scripts
#####################################################################################################################################
cd $build_path

if [ -f MemoryDetails1 ]; then
   rm MemoryDetails1
   # echo "Memory Details0 are removed"
fi

if [ -f AverageMemoryDetails1 ]; then
   rm AverageMemoryDetails1
   # echo "Average Memory Details0 are removed"
fi

if [ -f AverageMemory1 ]; then
   rm AverageMemory1
   # echo "Average Memory Details0 are removed"
fi

if [ -f AverageTimeDetails1 ]; then
   rm AverageTimeDetails1
   # echo "AverageTimeDetails0 is removed"
fi

if [ -f AverageTime1 ]; then
   rm AverageTime1
   # echo "AverageTime0 is removed"
fi

# #####################Inputs##########################################################################################################
# cs0_ip is the ip of server0, cs1_ip is the ip of server1, helpernode_ip is the ip of the helpernode
cs0_ip=
cs1_ip=
helpernode_ip=

# Ports on which server0 and server1 of the inferencing tasks talk to each other
port0_inference=7003
port1_inference=7004
helpernode_port=7005
# Ports on which weights receiver talk
cs0_port=3390
cs1_port=4567

# Ports on which image provider talks
cs0_port_image=3390
cs1_port_image=4567

fractional_bits=13

##########################################################################################################################################


if [ ! -d "$debug_1" ];
then
	mkdir -p $debug_1
fi

######################### Weights Share Receiver ############################################################################################
echo "Weight Shares Receiver starts"
$build_path/bin/Weights_Share_Receiver --my-id 1 --port $cs1_port --file-names $model_config --current-path $build_path > $debug_1/Weights_Share_Receiver1.txt &
pid2=$!

#########################Weights Provider ############################################################################################
echo "Weight Provider starts"
$build_path/bin/weights_provider --compute-server0-ip $cs0_ip --compute-server0-port $cs0_port --compute-server1-ip $cs1_ip --compute-server1-port $cs1_port --dp-id 0 --fractional-bits $fractional_bits --filepath $build_path_model > $debug_1/weights_provider.txt &
pid3=$!

wait $pid3
wait $pid2 
echo "Weight Shares received"

#########################Image Share Receiver ############################################################################################
echo "Image Shares Receiver starts"

$build_path/bin/Image_Share_Receiver --my-id 1 --port $cs1_port_image --fractional-bits $fractional_bits --file-names $image_config --current-path $build_path > $debug_1/Image_Share_Receiver1.txt &
pid2=$!

wait $pid2

echo "Image shares received"
########################Share generators end ############################################################################################


########################Inferencing task starts ###############################################################################################
 
echo "Inferencing task of the image shared starts"

############################Inputs for inferencing tasks #######################################################################################
layer_id=1
input_config=" "
image_share="remote_image_shares"
if [ $layer_id -eq 1 ];
then
    input_config="remote_image_shares"
fi

#######################################Matrix multiplication layer 1 ###########################################################################

$build_path/bin/server1 --WB_file file_config_model1 --input_file $input_config  --party 0,$cs0_ip,$port0_inference --party 1,$cs1_ip,$port1_inference --helper_node $helpernode_ip,$helpernode_port --current-path $build_path --layer-id $layer_id --fractional-bits $fractional_bits > $debug_1/server1_layer${layer_id}.txt &

pid1=$!

wait $pid1 
echo "Layer 1: Matrix multiplication and addition is done"

# #######################################ReLu layer 1 ####################################################################################
$build_path/bin/tensor_gt_relu --my-id 1 --party 0,$cs0_ip,$port0_inference --party 1,$cs1_ip,$port1_inference --arithmetic-protocol beavy --boolean-protocol yao --fractional-bits $fractional_bits --filepath file_config_input1 --current-path $build_path > $debug_1/tensor_gt_relu1_layer1.txt &
pid1=$!

wait $pid1 
echo "Layer 1: ReLU is done"

#######################Next layer, layer 2, inputs for layer 2 ###################################################################################################
((layer_id++))

# #Updating the config file for layers 2 and above. 
if [ $layer_id -gt 1 ];
then
    input_config="outputshare"
fi

#######################################Matrix multiplication layer 2 ###########################################################################

$build_path/bin/server1 --WB_file file_config_model1 --input_file $input_config  --party 0,$cs0_ip,$port0_inference --party 1,$cs1_ip,$port1_inference --helper_node $helpernode_ip,$helpernode_port --current-path $build_path --layer-id $layer_id --fractional-bits $fractional_bits > $debug_1/server1_layer${layer_id}.txt &
pid1=$!

wait $pid1 
echo "Layer 2: Matrix multiplication and addition is done"

####################################### Argmax  ###########################################################################

$build_path/bin/argmax --my-id 1 --party 0,$cs0_ip,$port0_inference --party 1,$cs1_ip,$port1_inference --arithmetic-protocol beavy --boolean-protocol beavy --repetitions 1 --config-filename file_config_input1 --config-input $image_share --current-path $build_path  > $debug_1/argmax1_layer2.txt &
pid1=$!


wait $pid1 
echo "Layer 2: Argmax is done"

####################################### Final output provider  ###########################################################################

$build_path/bin/final_output_provider --my-id 1 --connection-port 2233 --connection-ip $cs0_ip --config-input $image_share --current-path $build_path > $debug_1/final_output_provider1.txt &
pid4=$!

wait $pid4 
echo "Output shares of server 1 sent to the Image provider"

wait 

awk '{ sum += $1 } END { print sum }' AverageTimeDetails1 >> AverageTime1
#  > AverageTimeDetails1 #clearing the contents of the file

sort -r -g AverageMemoryDetails1 | head  -1 >> AverageMemory1
#  > AverageMemoryDetails1 #clearing the contents of the file

echo -e "\nInferencing Finished."

Mem=`cat AverageMemory1`
Time=`cat AverageTime1`

Mem=$(printf "%.2f" $Mem) 
Convert_KB_to_GB=$(printf "%.14f" 9.5367431640625E-7)
Mem2=$(echo "$Convert_KB_to_GB * $Mem" | bc -l)

Memory=$(printf "%.3f" $Mem2)

echo "Memory requirement:" `printf "%.3f" $Memory` "GB"
echo "Time taken by inferencing task:" $Time "ms"

cd $scripts_path