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
build_path=${BASE_DIR}/build_debwithrelinfo_gcc
model_config=${BASE_DIR}/config_files/model_helpernode_config.json
model_provider_path=${BASE_DIR}/data/ModelProvider
debug_ModelProv=${BASE_DIR}/logs/ModelProvider_logs
smpc_config_path=${BASE_DIR}/config_files/smpc-helpernode-config.json
smpc_config=`cat $smpc_config_path`
#--------------------------------- Inputs ------------------------------------------------------------------#
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
cs0_port_model_receiver=`echo $smpc_config | jq -r .cs0_port_model_receiver`
cs1_port_model_receiver=`echo $smpc_config | jq -r .cs1_port_model_receiver`
# Port on which final output talks to image provider 
cs0_port_cs1_output_receiver=`echo $smpc_config | jq -r .cs0_port_cs1_output_receiver`

fractional_bits=`echo $smpc_config | jq -r .fractional_bits`

#number of splits
splits=`echo "$smpc_config" | jq -r .splits`

if [ ! -d "$debug_ModelProv" ];
then
	mkdir -p $debug_ModelProv
fi

#########################Weights Provider ############################################################################################
# Creates the weights and bias shares and sends it to server 0 and server 1
echo "Weight Provider starts."

$build_path/bin/CNN_weights_provider_genr --compute-server0-ip $cs0_host --compute-server0-port $cs0_port_model_receiver --compute-server1-ip $cs1_host --compute-server1-port $cs1_port_model_receiver --fractional-bits $fractional_bits --filepath $model_provider_path --config-file-path $model_config > $debug_ModelProv/weights_provider.txt &
pid1=$!

wait $pid1 

echo "Weight shares received"
#########################Share generators end ############################################################################################
