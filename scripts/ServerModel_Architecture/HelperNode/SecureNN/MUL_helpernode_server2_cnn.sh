#!/bin/bash
while :
do
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
smpc_config_path=${BASE_DIR}/config_files/smpc-helpernode-config.json
smpc_config=`cat $smpc_config_path`

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

layer_types_array=(${layer_types[@]})
echo "$layer_types_array[@]"

#################################################
cd $build_path

if [ -f AverageMemory2 ]; then
   rm AverageMemory2
   # echo "Memory Details0 are removed"
fi

if [ -f AverageTime2 ]; then
   rm AverageTime2
   # echo "Average Memory Details0 are removed"
fi

################################################

echo "Helper node starts"

# layer_types=($(cat "$build_path/layer_types0"))
start=$(date +%s)

############################ Inputs for inferencing tasks #######################################################################################
# ####################################### Matrix multiplication layer 1 ###########################################################################
   $build_path/bin/3PC_CNN_NN_ver_2 --layers $number_of_layers --layer-types ${layer_types_array[@]} --party 0,$cs0_host,$cs0_port_inference --party 1,$cs1_host,$cs1_port_inference --helper_node $helpernode_host,$helpernode_port_inference --current-path ${BASE_DIR}/build_debwithrelinfo_gcc > $debug_2/test2.txt &
   pid1=$!
   wait $pid1
   check_exit_statuses $?
   wait

  end=$(date +%s)

awk '{ sum += $1 } END { print sum }' AverageTimeDetails2 >> AverageTime2
#  > AverageTimeDetails1 #clearing the contents of the file

sort -r -g AverageMemoryDetails2 | head  -1 >> AverageMemory2
#  > AverageMemoryDetails1 #clearing the contents of the file

echo -e "Inferencing Finished"

Mem=`cat AverageMemory2`
Time=`cat AverageTime2`

Mem=$(printf "%.2f" $Mem)
Convert_KB_to_GB=$(printf "%.14f" 9.5367431640625E-7)
Mem2=$(echo "$Convert_KB_to_GB * $Mem" | bc -l)

Memory=$(printf "%.3f" $Mem2)

echo "Memory requirement:" `printf "%.3f" $Memory` "GB"
echo "Elapsed Time: $(($end-$start)) seconds"

sleep 3
done