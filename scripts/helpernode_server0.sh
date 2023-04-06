#! /bin/bash
# paths required to run cpp files
image_config=${BASE_DIR}/config_files/file_config_input_remote
model_config=${BASE_DIR}/config_files/file_config_model
build_path=${BASE_DIR}/build_debwithrelinfo_gcc
# build_path_model=${BASE_DIR}/Dataprovider/weights_provider # Add this to model_config and remove it
image_path=${BASE_DIR}/Dataprovider/image_provider  # Add this to image_config and remove it
image_provider_path=${BASE_DIR}/Dataprovider/image_provider/Final_Output_Shares # Add this to image_config and remove it
debug_0=$build_path/server0/debug_files
debug_1=$build_path/server1/debug_files


# #####################Inputs##########################################################################################################

# cs0_ip is the ip of server0, cs1_ip is the ip of server1, helpernode_ip is the ip of the helpernode
cs0_ip=127.0.0.1 
cs1_ip=127.0.0.1
helpernode_ip=127.0.0.1
# Ports on which weights receiver talk
cs0_port=3390
cs1_port=4567

# Ports on which image provider talks
cs0_port_image=3390
cs1_port_image=4567


# Ports on which server0 and server1 of the inferencing tasks talk to each other
port0_inference=7003
port1_inference=7005
helpernode_port=7004

fractional_bits=13

# Index of the image for which inferencing task is run
image_id=27

##########################################################################################################################################


if [ ! -d "$debug_0" ];
then
	# Recursively create the required directories
	mkdir -p $debug_0
fi

######################### Weights Share Receiver ############################################################################################
echo "Weight Shares Receiver starts"
$build_path/bin/Weights_Share_Receiver --my-id 0 --port $cs0_port --file-names $model_config --current-path $build_path > $debug_0/Weights_Share_Receiver0.txt &
pid2=$!
wait $pid2
echo "Weight Shares received"

#########################Image Share Receiver ############################################################################################
echo "Image Shares Receiver starts"

$build_path/bin/Image_Share_Receiver --my-id 0 --port $cs0_port_image --fractional-bits $fractional_bits --file-names $image_config --current-path $build_path > $debug_0/Image_Share_Receiver0.txt &
pid1=$!

#########################Image Share Provider ############################################################################################
echo "Image provider start"
$build_path/bin/image_provider_iudx --compute-server0-ip $cs0_ip --compute-server0-port $cs0_port_image --compute-server1-ip $cs1_ip --compute-server1-port $cs1_port_image --fractional-bits $fractional_bits --index $image_id --filepath $image_path > $debug_1/image_provider.txt &
pid3=$!

wait $pid3 $pid1

echo "Image shares received"
########################Share generators end ############################################################################################

########################Inferencing task starts ###############################################################################################


echo "Inferencing task of the image shared starts"

############################Inputs for inferencing tasks #######################################################################################
layer_id=1
# input_config=" "
image_share="remote_image_shares"
if [ $layer_id -eq 1 ];
then
    input_config="remote_image_shares"
fi

#######################################Matrix multiplication layer 1 ###########################################################################

$build_path/bin/server0 --WB_file file_config_model0 --input_file $input_config  --my_port $port0_inference --party_1 $cs1_ip,$port1_inference --helper_node $helpernode_ip,$helpernode_port --current-path $build_path --layer-id $layer_id --fractional-bits $fractional_bits > $debug_0/server0_layer${layer_id}.txt &
pid1=$!

wait $pid1 #$pid2 #$pid3 
echo "Layer 1: Matrix multiplication and addition is done"


# #######################################Output share receivers ###########################################################################
$build_path/bin/output_shares_receiver --my-id 0 --listening-port 8899 --current-path $image_provider_path > $debug_0/output_shares_receiver0.txt &
pid5=$!

$build_path/bin/output_shares_receiver --my-id 1 --listening-port 2233 --current-path $image_provider_path > $build_path/server1/debug_files/output_shares_receiver1.txt &
pid6=$!

echo "Image Provider is listening for the inferencing result"

# #######################################ReLu layer 1 ####################################################################################
$build_path/bin/tensor_gt_relu --my-id 0 --party 0,$cs0_ip,$port0_inference --party 1,$cs1_ip,$port1_inference --arithmetic-protocol beavy --boolean-protocol yao --fractional-bits $fractional_bits --filepath file_config_input0 --current-path $build_path > $debug_0/tensor_gt_relu1_layer0.txt &
pid1=$!

wait $pid1 

echo "layer 1 - ReLU is done"

#######################Next layer, layer 2, inputs for layer 2 ###################################################################################################
((layer_id++))

# #Updating the config file for layers 2 and above. 
if [ $layer_id -gt 1 ];
then
    input_config="outputshare"
fi

#######################################Matrix multiplication layer 2 ###########################################################################
$build_path/bin/server0 --WB_file file_config_model0 --input_file $input_config  --my_port $port0_inference --party_1 $cs1_ip,$port1_inference --helper_node $helpernode_ip,$helpernode_port --current-path $build_path --layer-id $layer_id --fractional-bits $fractional_bits > $debug_0/server0_layer${layer_id}.txt &
pid1=$!

wait $pid1 

echo "Layer 2 - matrix multiplication and addition is done"

####################################### Argmax  ###########################################################################
$build_path/bin/argmax --my-id 0 --party 0,$cs0_ip,$port0_inference --party 1,$cs1_ip,$port1_inference --arithmetic-protocol beavy --boolean-protocol beavy --repetitions 1 --config-filename file_config_input0 --config-input $image_share --current-path $build_path  > $debug_0/argmax0_layer2.txt &
pid1=$!

wait $pid1 

echo "layer 2 - argmax is done"

####################################### Final output provider  ###########################################################################
$build_path/bin/final_output_provider --my-id 0 --connection-port 8899 --config-input $image_share --current-path $build_path > $debug_0/final_output_provider0.txt &
pid3=$!

echo "Output shares of server 0 sent to the Image provider"


wait $pid5 $pid6 

echo "Output shares of server 0 received at the Image provider"
echo "Output shares of server 1 received at the Image provider"

############################            Reconstruction       ##################################################################################
echo "Reconstruction Starts"
$build_path/bin/Reconstruct --current-path $image_provider_path 

wait 







