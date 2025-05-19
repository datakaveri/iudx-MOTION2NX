#!/bin/bash
check_exit_statuses() {
   for status in "$@";
   do
      if [ $status -ne 0 ]; then
         echo "Exiting due to error."
         exit 1  # Exit the script with a non-zero exit code
      fi
   done
}
# paths required to run cpp files
build_path=${BASE_DIR}/build_debwithrelinfo_gcc
debug_2=${BASE_DIR}/logs/helpernode_cnn
scripts_path=${BASE_DIR}/scripts
smpc_config_path=${BASE_DIR}/config_files/example-smpc-helpernode-config.json
smpc_config=`cat $smpc_config_path`

#####################################################################################################################################
cd $build_path

if [ -f AverageMemoryDetails2 ]; then
   rm AverageMemoryDetails2
fi

if [ -f AverageTimeDetails2 ]; then
   rm AverageTimeDetails2
fi

if [ -f MemoryDetails2 ]; then
   rm MemoryDetails2
fi

cd $build_path/stats/

if [ -f ackStats2 ]; then
   rm ackStats2
fi

cd $build_path

#####################################################################################################################################
#####################Inputs##########################################################################################################

# Do dns resolution or not
cs0_dns_resolve=`echo $smpc_config | jq -r .cs0_dns_resolve`
cs1_dns_resolve=`echo $smpc_config | jq -r .cs1_dns_resolve`
helpernode_dns_resolve=`echo $smpc_config | jq -r .helpernode_dns_resolve`


# cs0_host is the ip/domain of server0, cs1_host is the ip/domain of server1
cs0_host=`echo $smpc_config | jq -r .cs0_host`
cs1_host=`echo $smpc_config | jq -r .cs1_host`
helpernode_host=`echo $smpc_config | jq -r .helpernode_host`

if [[ $cs0_dns_resolve == "true" ]];
then
cs0_host=`dig +short $cs0_host | grep '^[.0-9]*$' | head -n 1`
fi

if [[ $cs1_dns_resolve == "true" ]];
then
cs1_host=`dig +short $cs1_host | grep '^[.0-9]*$' | head -n 1`
fi

if [[ $helpernode_dns_resolve == "true" ]];
then
helpernode_host=`dig +short $helpernode_host | grep '^[.0-9]*$' | head -n 1`
fi

# Ports on which server0 and server1 of the inferencing tasks talk to each other
cs0_port_inference=`echo $smpc_config | jq -r .cs0_port_inference`
cs1_port_inference=`echo $smpc_config | jq -r .cs1_port_inference`
helpernode_port_inference=`echo $smpc_config | jq -r .helpernode_port_inference`
relu0_port_inference=`echo $smpc_config | jq -r .relu0_port_inference`
relu1_port_inference=`echo $smpc_config | jq -r .relu1_port_inference`

number_of_layers=`echo $smpc_config | jq -r .number_of_layers`
layer_types=($(echo "$smpc_config" | jq -r '.layer_types | @sh'))
fractional_bits=13
##########################################################################################################################################

if [ ! -d "$debug_2" ];
then
        mkdir -p $debug_2
fi

echo "Helper node starts"
echo "Number of layers: $number_of_layers"

new_layer_types=(${layer_types[@]})
echo "${new_layer_types[@]}"

# layer_types=($(cat "$build_path/layer_types0"))

############################ Inputs for inferencing tasks #######################################################################################
# ####################################### Matrix multiplication layer 1 ###########################################################################

  $build_path/bin/3PC_CNN_NN_debug_2 --layers $number_of_layers --layer-types ${new_layer_types[@]} --party 0,$cs0_host,$cs0_port_inference --party 1,$cs1_host,$cs1_port_inference --helper_node $helpernode_host,$helpernode_port_inference --current-path $build_path &
  pid1=$!
  wait $pid1
  check_exit_statuses $?

wait

echo "Helper node completed"