#! /bin/bash
# paths required to run cpp files
model_config=${BASE_DIR}/config_files/file_config_model
build_path=${BASE_DIR}/build_debwithrelinfo_gcc
debug_2=$build_path/helpernode/debug_files


##################### Inputs ##########################################################################################################

# cs0_ip is the ip of server0, cs1_ip is the ip of server1
cs0_ip=
cs1_ip=
helpernode_ip=
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

############################ Inputs for inferencing tasks #######################################################################################
layer_id=1
input_config=" "
image_share="remote_image_shares"
if [ $layer_id -eq 1 ];
then
    input_config="remote_image_shares"
fi

# ####################################### Matrix multiplication layer 1 ###########################################################################

$build_path/bin/server2 --party 0,$cs0_ip,$port0_inference --party 1,$cs1_ip,$port1_inference --helper_node $helpernode_ip,$helpernode_port > $debug_2/helpernode_layer${layer_id}.txt &
pid1=$!

wait $pid1 
echo "Helper node layer ${layer_id} is done"

####################### Next layer, layer 2, inputs for layer 2 ###################################################################################################
((layer_id++))
#######################################Matrix multiplication layer 2 ###########################################################################

$build_path/bin/server2 --party 0,$cs0_ip,$port0_inference --party 1,$cs1_ip,$port1_inference --helper_node $helpernode_ip,$helpernode_port > $debug_2/helpernode_layer${layer_id}.txt &
pid1=$!

wait $pid1 
echo "Helper node layer ${layer_id} is done"

wait 