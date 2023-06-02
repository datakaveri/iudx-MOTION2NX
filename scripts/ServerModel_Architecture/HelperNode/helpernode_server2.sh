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
debug_2=${BASE_DIR}/logs/helpernode/
smpc_config_path=${BASE_DIR}/config_files/smpc-remote-config.json
smpc_config=`cat $smpc_config_path`

# #####################Inputs##########################################################################################################

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

fractional_bits=13
##########################################################################################################################################


if [ ! -d "$debug_2" ];
then
        mkdir -p $debug_2
fi

echo "Helper node starts"

############################ Inputs for inferencing tasks #######################################################################################
layer_id=1
# ####################################### Matrix multiplication layer 1 ###########################################################################

$build_path/bin/server2 --party 0,$cs0_host,$cs0_port_inference --party 1,$cs1_host,$cs1_port_inference --helper_node $helpernode_host,$helpernode_port_inference> $debug_2/helpernode_layer${layer_id}.txt &
pid1=$!
wait $pid1
check_exit_statuses $?
echo "Helper node layer 1 is done"

####################### Inputs for layer 2 ###################################################################################################
((layer_id++))
####################################### Matrix multiplication layer 2 ###########################################################################

$build_path/bin/server2 --party 0,$cs0_host,$cs0_port_inference --party 1,$cs1_host,$cs1_port_inference --helper_node $helpernode_host,$helpernode_port_inference > $debug_2/helpernode_layer${layer_id}.txt &
pid2=$!
wait $pid2
check_exit_statuses $?
echo "Helper node layer ${layer_id} is done"

#To catch any extra processes
wait