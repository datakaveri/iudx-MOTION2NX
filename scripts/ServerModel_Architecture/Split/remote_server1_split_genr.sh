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
image_config=${BASE_DIR}/config_files/file_config_input_remote
build_path=${BASE_DIR}/build_debwithrelinfo_gcc
model_provider_path=${BASE_DIR}/data/ModelProvider
debug_1=${BASE_DIR}/logs/server1
smpc_config_path=${BASE_DIR}/config_files/smpc-split-config.json
smpc_config=`cat $smpc_config_path`
# #####################Inputs##########################################################################################################
# Do dns resolution or not 
cs0_dns_resolve=`echo $smpc_config | jq -r .cs0_dns_resolve`
cs1_dns_resolve=`echo $smpc_config | jq -r .cs1_dns_resolve`
reverse_ssh_dns_resolve=`echo $smpc_config | jq -r .reverse_ssh_dns_resolve`

# cs0_host is the ip/domain of server0, cs1_host is the ip/domain of server1
cs0_host=`echo $smpc_config | jq -r .cs0_host`
cs1_host=`echo $smpc_config | jq -r .cs1_host`
reverse_ssh_host=`echo $smpc_config | jq -r .reverse_ssh_host`

if [[ $cs0_dns_resolve == "true" ]];
then 
cs0_host=`dig +short $cs0_host | grep '^[.0-9]*$' | head -n 1`
fi

if [[ $cs1_dns_resolve == "true" ]];
then 
cs1_host=`dig +short $cs1_host | grep '^[.0-9]*$' | head -n 1`
fi

if [[ $reverse_ssh_dns_resolve == "true" ]];
then 
reverse_ssh_host=`dig +short $reverse_ssh_host | grep '^[.0-9]*$' | head -n 1`
fi

# Ports on which weights provider  receiver listens/talks
cs0_port_model_receiver=`echo $smpc_config | jq -r .cs0_port_model_receiver`
cs1_port_model_receiver=`echo $smpc_config | jq -r .cs1_port_model_receiver`

# Ports on which image provider  receiver listens/talks
cs0_port_image_receiver=`echo $smpc_config | jq -r .cs0_port_image_receiver`
cs1_port_image_receiver=`echo $smpc_config | jq -r .cs1_port_image_receiver`

# Port on which final output talks to image provider 
cs0_port_cs1_output_receiver=`echo $smpc_config | jq -r .cs0_port_cs1_output_receiver`


# Ports on which server0 and server1 of the inferencing tasks talk to each other
cs0_port_inference=`echo $smpc_config | jq -r .cs0_port_inference`
cs1_port_inference=`echo $smpc_config | jq -r .cs1_port_inference`
relu0_port_inference=`echo $smpc_config | jq -r .relu0_port_inference`
relu1_port_inference=`echo $smpc_config | jq -r .relu1_port_inference`

number_of_layers=`echo $smpc_config | jq -r .number_of_layers`
fractional_bits=`echo $smpc_config | jq -r .fractional_bits`

#number of splits
# splits=`echo "$smpc_config" | jq -r .splits`

# echo all input variables
#echo "cs0_host $cs0_host"
#echo "cs1_host $cs1_host"
#echo "cs0_port_model_receiver $cs0_port_model_receiver"
#echo "cs1_port_model_receiver $cs1_port_model_receiver"
#echo "cs0_port_cs1_output_receiver $cs0_port_cs1_output_receiver"
#echo "cs0_port_inference $cs0_port_inference"
#echo "cs1_port_inference $cs1_port_inference"
#echo "fractional bits: $fractional_bits"
#echo "no. of splits: $splits"
##########################################################################################################################################

if [ ! -d "$debug_1" ];
then
	mkdir -p $debug_1
fi

cd $build_path


if [ -f finaloutput_1 ]; then
   rm finaloutput_1
   # echo "final output 1 is removed"
fi

if [ -f MemoryDetails1 ]; then
   rm MemoryDetails1
   # echo "Memory Details1 are removed"
fi

if [ -f AverageMemoryDetails1 ]; then
   rm AverageMemoryDetails1
   # echo "Average Memory Details1 are removed"
fi
if [ -f AverageMemory1 ]; then
   rm AverageMemory1
   # echo "Average Memory Details1 are removed"
fi

if [ -f AverageTimeDetails1 ]; then
   rm AverageTimeDetails1
   # echo "AverageTimeDetails1 is removed"
fi

if [ -f AverageTime1 ]; then
   rm AverageTime1
   # echo "AverageTime1 is removed"
fi

#########################Weights Share Receiver ############################################################################################
echo "Weight shares receiver starts"
$build_path/bin/weight_share_receiver_genr --my-id 1 --port $cs1_port_model_receiver --current-path $build_path > $debug_1/Weights_Share_Receiver.txt &
pid1=$!

#########################Image Share Receiver ############################################################################################
echo "Image shares receiver starts"
$build_path/bin/Image_Share_Receiver --my-id 1 --port $cs1_port_image_receiver --fractional-bits $fractional_bits --file-names $image_config --current-path $build_path > $debug_1/Image_Share_Receiver.txt &
pid2=$!

wait $pid1
check_exit_statuses $? 
wait $pid2
check_exit_statuses $?

echo "Weight shares received"
echo "Image shares received"
#########################Share generators end ############################################################################################


########################Inferencing task starts ###############################################################################################

#  echo "image_ids X"$image_id > MemoryDetails1 
echo "Inferencing task of the image shared starts"

layer_id=1

input_config=" "
   image_share="remote_image_shares"
   if [ $layer_id -eq 1 ];
   then
      input_config="remote_image_shares"
   fi

for split_layer in $(echo "$smpc_config" | jq -r '.split_layers_genr[] | @base64'); do
   rows=$(echo ${split_layer} | base64 --decode | jq -r '.rows')
   num_splits=$(echo ${split_layer} | base64 --decode | jq -r '.splits')

# echo "image_id $image_id" >> MemoryDetails0
   echo "Number of splits for layer $layer_id matrix multiplication: $rows::$num_splits"

   x=$(($rows/$num_splits))
   # echo "split value: $x"

   start=$(date +%s)
   for(( m = 1; m <= $num_splits; m++ )); do
   

   if [ $layer_id -gt 1 ];
   then
    input_config="mult_output"
   fi

   let l=$((m-1)) 
   let a=$(((m-1)*x+1)) 
   let b=$((m*x)) 
   let r=$((l*x)) 

#######################################Matrix multiplication layer 1 ###########################################################################

   #Layer 1   
   $build_path/bin/tensor_gt_mul_split --my-id 1 --party 0,$cs0_host,$cs0_port_inference --party 1,$cs1_host,$cs1_port_inference --arithmetic-protocol beavy --boolean-protocol yao --fractional-bits 13 --config-file-input $input_config --config-file-model file_config_model1 --layer-id $layer_id --row_start $a --row_end $b --split $num_splits --current-path $build_path  > $debug_1/tensor_gt_mul1_layer${layer_id}_split.txt &
   pid1=$!
   wait $pid1  
   check_exit_statuses $? 
   echo "Layer $layer_id, split $m: Matrix multiplication and addition is done"
   
   if [ $m -eq 1 ];then

      touch finaloutput_1

      printf "$x 1\n" >> finaloutput_1

      $build_path/bin/appendfile 1
      pid1=$!
      wait $pid1 
      check_exit_statuses $? 
      else 
      
      $build_path/bin/appendfile 1
      pid1=$!
      wait $pid1 
      check_exit_statuses $? 
    fi

	sed -i "1s/${r} 1/${b} 1/" finaloutput_1

done

cp finaloutput_1  $build_path/server1/outputshare_1
check_exit_statuses $? 

#######################################ReLu layer 1 ####################################################################################
$build_path/bin/tensor_gt_relu --my-id 1 --party 0,$cs0_host,$cs0_port_inference --party 1,$cs1_host,$cs1_port_inference --arithmetic-protocol beavy --boolean-protocol yao --fractional-bits $fractional_bits --filepath file_config_input1 --current-path $build_path > $debug_1/tensor_gt_relu1_layer${layer_id}.txt &
pid1=$!
wait $pid1 
check_exit_statuses $? 
echo "Layer $layer_id: ReLU is done"

if [ -f finaloutput_1 ]; then
   rm finaloutput_1
   # echo "final output 1 is removed"
fi

cp $build_path/server1/outputshare_1  $build_path/server1/mult_output_1
check_exit_statuses $? 
((layer_id++))

done

if [ $layer_id -gt 1 ];
   then
    input_config="outputshare"
   fi


#######################################Matrix multiplication layer 2 ###########################################################################
for((; layer_id<$number_of_layers; layer_id++))
do

if [ $layer_id -gt 1 ];
   then
    input_config="outputshare"
   fi

$build_path/bin/tensor_gt_mul_test --my-id 1 --party 0,$cs0_host,$cs0_port_inference --party 1,$cs1_host,$cs1_port_inference --arithmetic-protocol beavy --boolean-protocol yao --fractional-bits $fractional_bits --config-file-input $input_config --config-file-model file_config_model1 --layer-id $layer_id --current-path $build_path > $debug_1/tensor_gt_mul1_layer${layer_id}.txt &
pid1=$!

wait $pid1 
check_exit_statuses $? 
echo "Layer $layer_id: Matrix multiplication and addition is done"

#######################################ReLu layer 1 ####################################################################################
$build_path/bin/tensor_gt_relu --my-id 1 --party 0,$cs0_host,$cs0_port_inference --party 1,$cs1_host,$cs1_port_inference --arithmetic-protocol beavy --boolean-protocol yao --fractional-bits $fractional_bits --filepath file_config_input1 --current-path $build_path > $debug_1/tensor_gt_relu1_layer1.txt &
pid1=$!

wait $pid1 
check_exit_statuses $? 
echo "Layer $layer_id: ReLU is done"

done

#######################################Matrix multiplication layer 5 ###########################################################################

$build_path/bin/tensor_gt_mul_test --my-id 1 --party 0,$cs0_host,$cs0_port_inference --party 1,$cs1_host,$cs1_port_inference --arithmetic-protocol beavy --boolean-protocol yao --fractional-bits $fractional_bits --config-file-input $input_config --config-file-model file_config_model1 --layer-id $layer_id --current-path $build_path > $debug_1/tensor_gt_mul1_layer${layer_id}.txt &
pid1=$!
wait $pid1 
check_exit_statuses $? 
echo "Layer $layer_id: Matrix multiplication and addition is done"


####################################### Argmax  ###########################################################################

$build_path/bin/argmax --my-id 1 --threads 1 --party 0,$cs0_host,$cs0_port_inference --party 1,$cs1_host,$cs1_port_inference --arithmetic-protocol beavy --boolean-protocol beavy --repetitions 1 --config-filename file_config_input1 --config-input $image_share --current-path $build_path  > $debug_1/argmax1_layer${layer_id}.txt &
pid1=$!

wait $pid1 
check_exit_statuses $? 
end=$(date +%s)

echo "Layer $layer_id: Argmax is done"

####################################### Final output provider  ###########################################################################

$build_path/bin/final_output_provider --my-id 1 --connection-ip $reverse_ssh_host --connection-port $cs0_port_cs1_output_receiver --config-input $image_share --current-path $build_path > $debug_1/final_output_provider.txt &
pid4=$!
wait $pid4
check_exit_statuses $?  
echo "Output shares of server 1 sent to the Image provider"

wait 
#kill $pid5 $pid6

 awk '{ sum += $1 } END { print sum }' AverageTimeDetails1 >> AverageTime1
#  > AverageTimeDetails1 #clearing the contents of the file

  sort -r -g AverageMemoryDetails1 | head  -1 >> AverageMemory1
#  > AverageMemoryDetails1 #clearing the contents of the file

echo -e "\nInferencing Finished"

Mem=`cat AverageMemory1`
Time=`cat AverageTime1`

Mem=$(printf "%.2f" $Mem) 
Convert_KB_to_GB=$(printf "%.14f" 9.5367431640625E-7)
Mem2=$(echo "$Convert_KB_to_GB * $Mem" | bc -l)

Memory=$(printf "%.3f" $Mem2)

echo "Memory requirement:" `printf "%.3f" $Memory` "GB"
echo "Time taken by inferencing task:" $Time "ms"
echo "Elapsed Time: $(($end-$start)) seconds"

cd $scripts_path
