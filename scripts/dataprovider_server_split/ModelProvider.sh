#! /bin/bash
image_config=${BASE_DIR}/config_files/file_config_input_remote
model_config=${BASE_DIR}/config_files/file_config_model
build_path=${BASE_DIR}/build_debwithrelinfo_gcc
model_provider_path=${BASE_DIR}/data/ModelProvider
debug_ModelProv=${BASE_DIR}/logs/ModelProvider_logs/debug_files
smpc_config_path=${BASE_DIR}/config_files/smpc-remote-config.json
smpc_config=`cat $smpc_config_path`
# #####################Inputs##########################################################################################################
# Do dns resolution or not 
cs0_dns_resolve=`echo $smpc_config | jq -r .cs0_dns_resolve`
cs1_dns_resolve=`echo $smpc_config | jq -r .cs1_dns_resolve`

# cs0_host is the ip/domain of server0, cs1_host is the ip/domain of server1
cs0_host=`echo $smpc_config | jq -r .cs0_host`
cs1_host=`echo $smpc_config | jq -r .cs1_host`
if [[ $cs0_dns_resolve == "true" ]];
then 
cs0_host=`dig +short $cs0_host | grep '^[.0-9]*$' | head -n 1`
fi
if [[ $cs1_dns_resolve == "true" ]];
then 
cs1_host=`dig +short $cs1_host | grep '^[.0-9]*$' | head -n 1`
fi


# Ports on which weights,image provider  receiver listens/talks
cs0_port_model_receiver=`echo $smpc_config | jq -r .cs0_port_data_receiver`
cs1_port_model_receiver=`echo $smpc_config | jq -r .cs1_port_data_receiver`
cs0_port_image_receiver=`echo $smpc_config | jq -r .cs0_port_image_receiver`
cs1_port_image_receiver=`echo $smpc_config | jq -r .cs1_port_image_receiver`   
# Port on which final output talks to image provider 
cs0_port_cs1_output_receiver=`echo $smpc_config | jq -r .cs0_port_cs1_output_receiver`


# # Ports on which server0 and server1 of the inferencing tasks talk to each other
# cs0_port_inference=`echo $smpc_config | jq -r .cs0_port_inference`
# cs1_port_inference=`echo $smpc_config | jq -r .cs1_port_inference`

fractional_bits=`echo $smpc_config | jq -r .fractional_bits`

#number of splits
splits=`echo "$smpc_config" | jq -r .splits`

# echo all input variables
#echo "cs0_host $cs0_host"
#echo "cs1_host $cs1_host"
#echo "cs0_port_data_receiver $cs0_port_data_receiver"
#echo "cs1_port_data_receiver $cs1_port_data_receiver"
#echo "cs0_port_cs1_output_receiver $cs0_port_cs1_output_receiver"
#echo "cs0_port_inference $cs0_port_inference"
#echo "cs1_port_inference $cs1_port_inference"
#echo "fractional bits: $fractional_bits"
#echo "no. of splits: $splits"
##########################################################################################################################################

if [ ! -d "$debug_ModelProv" ];
then
	mkdir -p $debug_ModelProv
fi

# cd $build_path


# if [ -f finaloutput_1 ]; then
#    rm finaloutput_1
#    # echo "final output 1 is removed"
# fi

# if [ -f MemoryDetails1 ]; then
#    rm MemoryDetails1
#    # echo "Memory Details1 are removed"
# fi

# if [ -f AverageMemoryDetails1 ]; then
#    rm AverageMemoryDetails1
#    # echo "Average Memory Details1 are removed"
# fi
# if [ -f AverageMemory1 ]; then
#    rm AverageMemory1
#    # echo "Average Memory Details1 are removed"
# fi

# if [ -f AverageTimeDetails1 ]; then
#    rm AverageTimeDetails1
#    # echo "AverageTimeDetails1 is removed"
# fi

# if [ -f AverageTime1 ]; then
#    rm AverageTime1
#    # echo "AverageTime1 is removed"
# fi

# #########################Weights Share Receiver ############################################################################################
# echo "Weight Shares Receiver starts"
# $build_path/bin/Weights_Share_Receiver --my-id 1 --port $cs1_port_data_receiver --file-names $model_config --current-path $build_path > $debug_ModelProv/Weights_Share_Receiver.txt &
# pid2=$!


#########################Weights Provider ############################################################################################
# Creates the weights and bias shares and sends it to server 0 and server 1
echo "Weight Provider starts."

$build_path/bin/weights_provider --compute-server0-ip $cs0_host --compute-server0-port $cs0_port_model_receiver --compute-server1-ip $cs1_host --compute-server1-port $cs1_port_model_receiver --dp-id 0 --fractional-bits $fractional_bits --filepath $model_provider_path > $debug_ModelProv/weights_provider.txt &
pid1=$!
wait $pid1 
echo "Weight shares sent."

#########################Image Share Receiver ############################################################################################
# # echo "Image Shares Receiver starts"

# # $build_path/bin/Image_Share_Receiver --my-id 1 --port $cs1_port_data_receiver --fractional-bits $fractional_bits --file-names $image_config --current-path $build_path > $debug_ModelProv/Image_Share_Receiver.txt &
# # pid1=$!

# # wait $pid1

# echo "Image shares received"
#########################Share generators end ############################################################################################