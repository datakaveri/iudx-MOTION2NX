#! /bin/bash
# paths required to run cpp files
model_config=${BASE_DIR}/config_files/file_config_model
build_path=${BASE_DIR}/build_debwithrelinfo_gcc
debug_2=$build_path/helpernode/debug_files


# #####################Inputs##########################################################################################################

# cs0_ip is the ip of server0, cs1_ip is the ip of server1
cs0_ip=127.0.0.1
cs1_ip=127.0.0.1


# Ports on which server0 and server1 of the inferencing tasks talk to each other
port0_inference=7003
port1_inference=7004
helpernode_port=7005

fractional_bits=13
##########################################################################################################################################


if [ ! -d "$debug_2" ];
then
	mkdir -p $debug_2
fi

 
echo "Helper node starts"

############################Inputs for inferencing tasks #######################################################################################
layer_id=1
input_config=" "
image_share="remote_image_shares"
if [ $layer_id -eq 1 ];
then
    input_config="remote_image_shares"
fi

#######################################Matrix multiplication layer 1 ###########################################################################

$build_path/bin/server2 --my_port $helpernode_port --party_0 $cs0_ip,$port0_inference --party_1 $cs1_ip,$port1_inference >> $build_path/helpernode/debug_files/helpernode_layer1.txt &
pid1=$!

wait $pid1 
echo "Helper node layer 1 is done"

#######################Next layer, layer 2, inputs for layer 2 ###################################################################################################
# ((layer_id++))

# # #Updating the config file for layers 2 and above. 
# if [ $layer_id -gt 1 ];
# then
#     input_config="outputshare"
# fi

#######################################Matrix multiplication layer 2 ###########################################################################
# $build_path/bin/tensor_gt_mul_test --my-id 1 --party 0,$cs0_ip,$port0_inference --party 1,$cs1_ip,$port1_inference --arithmetic-protocol beavy --boolean-protocol yao --fractional-bits $fractional_bits --config-file-input $input_config --config-file-model file_config_model1 --layer-id $layer_id --current-path $build_path > $build_path/server1/debug_files/tensor_gt_mul1_layer2.txt &
# pid1=$!

# wait $pid1 
# echo "layer 2 - matrix multiplication and addition  is done"


# wait 







