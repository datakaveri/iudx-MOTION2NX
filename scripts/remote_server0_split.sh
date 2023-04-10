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


# #####################Inputs##########################################################################################################

# cs0_ip is the ip of server0, cs1_ip is the ip of server1
cs0_ip=172.30.9.43
cs1_ip=172.30.9.43

# Ports on which weights recceiver talk
cs0_port=3390
cs1_port=4567

# Ports on which image provider talks
cs0_port_image=3390
cs1_port_image=4567

# Ports on which server0 and server1 of the inferencing tasks talk to each other
port0_inference=3390
port1_inference=4567

fractional_bits=13

# Index of the image for which inferencing task is run
image_id=24

##########################################################################################################################################


if [ ! -d "$debug_0" ];
then
	# Recursively create the required directories
	mkdir -p $debug_0
fi
if [ ! -d "$debug_1" ];
then
	mkdir -p $debug_1
fi


cd $build_path

if [ -f finaloutput_0 ]; then
   rm finaloutput_0
   # echo "final output 0 is removed"
fi

if [ -f MemoryDetails0 ]; then
   rm MemoryDetails0
   # echo "Memory Details0 are removed"
fi

if [ -f AverageMemoryDetails0 ]; then
   rm AverageMemoryDetails0
   # echo "Average Memory Details0 are removed"
fi

if [ -f AverageMemory0 ]; then
   rm AverageMemory0
   # echo "Average Memory Details0 are removed"
fi

if [ -f AverageTimeDetails0 ]; then
   rm AverageTimeDetails0
   # echo "AverageTimeDetails0 is removed"
fi

if [ -f AverageTime0 ]; then
   rm AverageTime0
   # echo "AverageTime0 is removed"
fi

#########################Weights Share Receiver ############################################################################################
echo "Weight Shares Receiver starts"
$build_path/bin/Weights_Share_Receiver --my-id 0 --port $cs0_port --file-names $model_config --current-path $build_path >> $debug_0/Weights_Share_Receiver0.txt &
pid2=$!
wait $pid2
echo "Weight shares received"

#########################Image Share Receiver ############################################################################################
echo "Image Shares Receiver starts"

$build_path/bin/Image_Share_Receiver --my-id 0 --port $cs0_port_image --fractional-bits $fractional_bits --file-names $image_config --current-path $build_path >> $debug_0/Image_Share_Receiver0.txt &
pid1=$!

#########################Image Share Provider ############################################################################################
echo "Image Provider starts"
$build_path/bin/image_provider_iudx --compute-server0-ip $cs0_ip --compute-server0-port $cs0_port_image --compute-server1-ip $cs1_ip --compute-server1-port $cs1_port_image --fractional-bits $fractional_bits --index $image_id --filepath $image_path >> $debug_1/image_provider.txt &
pid3=$!

wait $pid3 $pid1

# done
echo "Image shares received"
#########################Share generators end ############################################################################################

########################Inferencing task starts ###############################################################################################
echo "Inferencing task of the image shared starts"

# echo "image_id $image_id" >> MemoryDetails0
#number of splits
splits=4
echo "Number of splits for layer 1 matrix multiplication: $splits"
x=$((256/splits))
for  (( m = 1; m <= $splits; m++ )) 
  do 

############################Inputs for inferencing tasks #######################################################################################
   layer_id=1
   input_config=" "
   image_share="remote_image_shares"
   if [ $layer_id -eq 1 ];
   then
      input_config="remote_image_shares"
   fi
    
	let l=$((m-1)) 
	let a=$(((m-1)*x+1)) 
	let b=$((m*x)) 
	let r=$((l*x)) 
    #######################################Matrix multiplication layer 1 ###########################################################################
    #Layer 1
    $build_path/bin/tensor_gt_mul_split --my-id 0 --party 0,$cs0_ip,$port0_inference --party 1,$cs1_ip,$port1_inference --arithmetic-protocol beavy --boolean-protocol yao --fractional-bits $fractional_bits --config-file-input $input_config --config-file-model file_config_model0 --layer-id $layer_id --row_start $a --row_end $b --split $splits --current-path $build_path > $build_path/server0/debug_files/tensor_gt_mul0_layer1_split.txt &
    pid1=$!
   
    wait $pid1 
    echo "Layer 1, split $m - multiplication is done."
    if [ $m -eq 1 ];then
      touch finaloutput_0
      printf "$x 1\n" >> finaloutput_0
      $build_path/bin/appendfile 0
      pid1=$!
      wait $pid1 
      
    else 
      
      $build_path/bin/appendfile 0
      pid1=$!
      wait $pid1 
    fi

	sed -i "1s/${r} 1/${b} 1/" finaloutput_0

done

cp finaloutput_0  $build_path/server0/outputshare_0 

   
#######################################Output share receivers ###########################################################################
$build_path/bin/output_shares_receiver --my-id 0 --listening-port 8899 --current-path $image_provider_path > $build_path/server0/debug_files/output_shares_receiver0.txt &
pid5=$!

$build_path/bin/output_shares_receiver --my-id 1 --listening-port 2233 --current-path $image_provider_path > $build_path/server1/debug_files/output_shares_receiver1.txt &
pid6=$!

echo "Image Provider listening for the inferencing result"

#######################################ReLu layer 1 ####################################################################################
$build_path/bin/tensor_gt_relu --my-id 0 --party 0,$cs0_ip,$port0_inference --party 1,$cs1_ip,$port1_inference --arithmetic-protocol beavy --boolean-protocol yao --fractional-bits $fractional_bits --filepath file_config_input0 --current-path $build_path > $build_path/server0/debug_files/tensor_gt_relu1_layer0.txt &
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
$build_path/bin/tensor_gt_mul_test --my-id 0 --party 0,$cs0_ip,$port0_inference --party 1,$cs1_ip,$port1_inference --arithmetic-protocol beavy --boolean-protocol yao --fractional-bits $fractional_bits --config-file-input $input_config --config-file-model file_config_model0 --layer-id $layer_id --current-path $build_path > $build_path/server0/debug_files/tensor_gt_mul0_layer2.txt &
pid1=$!

wait $pid1 

echo "Layer 2: Matrix multiplication and addition is done"

####################################### Argmax  ###########################################################################
$build_path/bin/argmax --my-id 0 --party 0,$cs0_ip,$port0_inference --party 1,$cs1_ip,$port1_inference --arithmetic-protocol beavy --boolean-protocol beavy --repetitions 1 --config-filename file_config_input0 --config-input $image_share --current-path $build_path  > $build_path/server0/debug_files/argmax0_layer2.txt &
pid1=$!

wait $pid1 

echo "Layer 2: Argmax is done"

####################################### Final output provider  ###########################################################################
$build_path/bin/final_output_provider --my-id 0 --connection-port 8899 --config-input $image_share --current-path $build_path > $build_path/server0/debug_files/final_output_provider0.txt &
pid3=$!

echo "Output shares of server 0 sent to the image provider"


wait $pid5 $pid6 

echo "Output shares of server 0 received to the Image provider"
echo "Output shares of server 1 received to the Image provider"

############################            Reconstruction       ##################################################################################
echo "Reconstruction Starts"
$build_path/bin/Reconstruct --current-path $image_provider_path 
wait 


awk '{ sum += $1 } END { print sum }' AverageTimeDetails0 >> AverageTime0
#  > AverageTimeDetails0 #clearing the contents of the file

sort -r -g AverageMemoryDetails0 | head  -1 >> AverageMemory0
#  > AverageMemoryDetails0 #clearing the contents of the file

echo -e "\nInferencing Finished"

Mem=`cat AverageMemory0`
Time=`cat AverageTime0`

Mem=$(printf "%.2f" $Mem) 
Convert_KB_to_GB=$(printf "%.14f" 9.5367431640625E-7)
Mem2=$(echo "$Convert_KB_to_GB * $Mem" | bc -l)

Memory=$(printf "%.3f" $Mem2)

echo "Memory requirement:" `printf "%.3f" $Memory` "GB"
echo "Time taken by inferencing task:" $Time "ms"


cd $scripts_path 
