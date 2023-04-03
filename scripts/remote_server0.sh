#! /bin/bash
# paths required to run cpp files
image_config=${BASE_DIR}/config_files/file_config_input
model_config=${BASE_DIR}/config_files/file_config_model
build_path=${BASE_DIR}/build_debwithrelinfo_gcc
build_path_model=${BASE_DIR}/Dataprovider/weights_provider
image_path=${BASE_DIR}/Dataprovider/image_provider
image_provider_path=${BASE_DIR}/Dataprovider/image_provider/Final_Output_Shares
debug_0=$build_path/server0/debug_files
debug_1=$build_path/server1/debug_files


# #####################Inputs##########################################################################################################

# cs0_ip is the ip of server0, cs1_ip is the ip of server1
cs0_ip=172.30.9.43
cs1_ip=192.168.1.141

# Ports on which weights recceiver talk
cs0_port=3390
cs1_port=4567

# Ports on which image provider talks
cs0_port_image=7000
cs1_port_image=7001


# Ports on which server0 and server1 of the inferencing tasks talk to each other
port0_inference=3390
port1_inference=4567

fractional_bits=13

# Index of the image for which inferencing task is run
image_ids=(11)

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

#########################Weights Share Receiver ############################################################################################
echo "Weight Shares Receiver starts"
$build_path/bin/Weights_Share_Receiver --my-id 0 --port $cs0_port --file-names $model_config --current-path $build_path >> $debug_0/Weights_Share_Receiver0.txt &
pid2=$!
wait $pid2
echo "Weight Shares received"

#########################Image Share Receiver ############################################################################################
echo "Image Shares Receiver starts"

for i in "${image_ids[@]}"
do

$build_path/bin/Image_Share_Receiver --my-id 0 --port $cs0_port_image --fractional-bits $fractional_bits --file-names $image_config --index $i --current-path $build_path >> $debug_0/Image_Share_Receiver0.txt &
pid1=$!

#########################Image Share Provider ############################################################################################
echo "Image provider start"
$build_path/bin/image_provider_iudx --compute-server0-ip $cs0_ip --compute-server0-port $cs0_port_image --compute-server1-ip $cs1_ip --compute-server1-port $cs1_port_image --fractional-bits $fractional_bits --index $i --filepath $image_path >> $debug_1/image_provider.txt &
pid3=$!

wait $pid3 $pid1

done
echo "Image shares received"
#########################Share generators end ############################################################################################

########################Inferencing task starts ###############################################################################################

for i in "${image_ids[@]}"
do 

echo "Inferencing task of example X$i"

############################Inputs for inferencing tasks #######################################################################################
layer_id=1
input_config=" "
if [ $layer_id -eq 1 ];
then
    input_config="ip$i"
fi

#######################################Matrix multiplication layer 1 ###########################################################################
$build_path/bin/tensor_gt_mul_test --my-id 0 --party 0,$cs0_ip,$port0_inference  --party 1,$cs1_ip,$port1_inference --arithmetic-protocol beavy --boolean-protocol yao --fractional-bits $fractional_bits --config-file-input $input_config --config-file-model file_config_model0 --layer-id $layer_id --current-path $build_path > $build_path/server0/debug_files/tensor_gt_mul0_layer1.txt &
pid1=$!

wait $pid1
echo "layer 1 - matrix multiplicationa and addition is done"


#######################################Output share receivers ###########################################################################
$build_path/bin/output_shares_receiver --my-id 0 --listening-port 8899 --current-path $image_provider_path --index $i> $build_path/server0/debug_files/output_shares_receiver0.txt &
pid5=$!

$build_path/bin/output_shares_receiver --my-id 1 --listening-port 2233 --current-path $image_provider_path --index $i> $build_path/server1/debug_files/output_shares_receiver1.txt &
pid6=$!

echo "Image Provider listening for the inferencing result"

#######################################ReLu layer 1 ####################################################################################
$build_path/bin/tensor_gt_relu --my-id 0 --party 0,$cs0_ip,$port0_inference --party 1,$cs1_ip,$port1_inference --arithmetic-protocol beavy --boolean-protocol yao --fractional-bits $fractional_bits --filepath file_config_input0 --current-path $build_path > $build_path/server0/debug_files/tensor_gt_relu1_layer0.txt &
pid1=$!

wait $pid1 

echo "layer 1 - ReLu is done"

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

echo "layer 2 - matrix multiplicationa and addition is done"

####################################### Argmax  ###########################################################################
$build_path/bin/argmax --my-id 0 --party 0,$cs0_ip,$port0_inference --party 1,$cs1_ip,$port1_inference --arithmetic-protocol beavy --boolean-protocol beavy --repetitions 1 --config-filename file_config_input0 --config-input X$i --current-path $build_path  > $build_path/server0/debug_files/argmax0_layer2.txt &
pid1=$!

wait $pid1 

echo "layer 2 - argmax is done"

####################################### Final output provider  ###########################################################################
$build_path/bin/final_output_provider --my-id 0 --connection-port 8899 --config-input X$i --current-path $build_path > $build_path/server0/debug_files/final_output_provider0.txt &
pid3=$!

echo "Output shares of server 0 sent to the Image provider"


wait $pid5 $pid6 

echo "Output shares of server 0 received to the Image provider"
echo "Output shares of server 1 received to the Image provider"

############################            Reconstruction       ##################################################################################
echo "Reconstruction Starts"
$build_path/bin/Reconstruct --current-path $image_provider_path --index $i

wait 

done
