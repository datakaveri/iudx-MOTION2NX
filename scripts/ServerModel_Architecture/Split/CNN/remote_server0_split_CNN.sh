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
image_config=${BASE_DIR}/config_files/file_config_input_remote
build_path=${BASE_DIR}/build_debwithrelinfo_gcc
image_path=${BASE_DIR}/data/ImageProvider
image_provider_path=${BASE_DIR}/data/ImageProvider/Final_Output_Shares
debug_0=${BASE_DIR}/logs/server0
scripts_path=${BASE_DIR}/scripts
smpc_config_path=${BASE_DIR}/config_files/smpc-cnn-config.json
smpc_config=`cat $smpc_config_path`
#####################Inputs#########################################################################################################
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

# Ports on which weights,image provider  receiver listens/talks
cs0_port_model_receiver=`echo $smpc_config | jq -r .cs0_port_model_receiver`
cs0_port_image_receiver=`echo $smpc_config | jq -r .cs0_port_image_receiver`

# Ports on which Image provider listens for final inference output
cs0_port_cs0_output_receiver=`echo $smpc_config | jq -r .cs0_port_cs0_output_receiver`
cs0_port_cs1_output_receiver=`echo $smpc_config | jq -r .cs0_port_cs1_output_receiver`

# Ports on which server0 and server1 of the inferencing tasks talk to each other
cs0_port_inference=`echo $smpc_config | jq -r .cs0_port_inference`
cs1_port_inference=`echo $smpc_config | jq -r .cs1_port_inference`
relu0_port_inference=`echo $smpc_config | jq -r .relu0_port_inference`
relu1_port_inference=`echo $smpc_config | jq -r .relu1_port_inference`
fractional_bits=`echo $smpc_config | jq -r .fractional_bits`

# Index of the image for which inferencing task is run
image_id=`echo $smpc_config | jq -r .image_id`
image_share="remote_image_shares"

# #number of splits
# splits=`echo "$smpc_config" | jq -r .splits`

if [ ! -d "$debug_0" ];
then
	# Recursively create the required directories
	mkdir -p $debug_0
fi


cd $build_path

if [ -f finaloutput_0 ]; then
   rm finaloutput_0
   # echo "final output 0 is removed"
fi

if [ -f MemoryDetails0 ]; then
   rm MemoryDetails0
   # echo "Memory Details0 are removed"
fi

if [ -f AverageMemoryDetails0 ]; then
   rm AverageMemoryDetails0
   # echo "Average Memory Details0 are removed"
fi

if [ -f AverageMemory0 ]; then
   rm AverageMemory0
   # echo "Average Memory Details0 are removed"
fi

if [ -f AverageTimeDetails0 ]; then
   rm AverageTimeDetails0
   # echo "AverageTimeDetails0 is removed"
fi

if [ -f AverageTime0 ]; then
   rm AverageTime0
   # echo "AverageTime0 is removed"
fi

######################### Weights Share Receiver ############################################################################################
echo "Weight shares receiver starts"
$build_path/bin/Weights_Share_Receiver_CNN --my-id 0 --port $cs0_port_model_receiver --current-path $build_path > $debug_0/Weights_Share_Receiver0.txt &
pid1=$!

#########################Image Share Receiver ############################################################################################
echo "Image shares receiver starts"

$build_path/bin/Image_Share_Receiver_CNN --my-id 0 --port $cs0_port_image_receiver --fractional-bits $fractional_bits --file-names $image_config --current-path $build_path > $debug_0/Image_Share_Receiver0.txt &
pid2=$!

wait $pid1
check_exit_statuses $? 
wait $pid2
check_exit_statuses $?

echo "Weight shares received"
echo "Image shares received"
######################### Share receivers end ############################################################################################

########################Inferencing task starts ###############################################################################################
echo "Inferencing task of the image shared starts"
start=$(date +%s)

cp server0/Image_shares/remote_image_shares server0/outputshare_0
cp server0/Image_shares/remote_image_shares server0/cnn_outputshare_0
sed -i "1s/^[^ ]* //" server0/outputshare_0

layer_types=($(cat layer_types0))
number_of_layers=${layer_types[0]}

split_info=$(echo "$smpc_config" | jq -r '.split_layers_genr[]')
split_info_index=0
split_info_layers=($(echo $split_info | jq -r '.layer_id'))
split_info_length=${#split_info_layers[@]}

for ((layer_id=1; layer_id<$number_of_layers; layer_id++)); do
   num_splits=1

   # Check for information in split info
   if [[ $split_info_index -lt $split_info_length ]] && [[ $layer_id -eq ${split_info_layers[split_info_index]} ]];
   then
      split=$(jq -r ".split_layers_genr[$split_info_index]" <<< "$smpc_config");
      num_splits=$(jq -r '.splits' <<< "$split");
      ((split_info_index++))
   fi

   if [ ${layer_types[layer_id]} -eq 0 ] && [ $num_splits -eq 1 ];
   then
      input_config="outputshare"

      $build_path/bin/tensor_gt_mul_test --my-id 0 --party 0,$cs0_host,$cs0_port_inference --party 1,$cs1_host,$cs1_port_inference --arithmetic-protocol beavy --boolean-protocol yao --fractional-bits $fractional_bits --config-file-input $input_config --config-file-model file_config_model0 --layer-id $layer_id --current-path $build_path > $debug_0/tensor_gt_mul0_layer${layer_id}.txt &
      pid1=$!
      wait $pid1 
      check_exit_statuses $?
      echo "Layer $layer_id: Matrix multiplication and addition is done"
      
      $build_path/bin/tensor_gt_relu --my-id 0 --party 0,$cs0_host,$relu0_port_inference --party 1,$cs1_host,$relu1_port_inference --arithmetic-protocol beavy --boolean-protocol yao --fractional-bits $fractional_bits --filepath file_config_input0 --current-path $build_path > $debug_0/tensor_gt_relu0_layer${layer_id}.txt &
      pid1=$!
      wait $pid1
      check_exit_statuses $?
      echo "Layer $layer_id: ReLU is done"
   
   elif [ ${layer_types[layer_id]} -eq 0 ] && [ $num_splits -gt 1 ];
   then
      cp $build_path/server0/outputshare_0  $build_path/server0/split_input_0
      input_config="split_input"

      num_rows=$(jq -r '.rows' <<< "$split");
      echo "Number of splits for layer $layer_id matrix multiplication: $num_rows::$num_splits"
      x=$(($num_rows/$num_splits))
      for(( m = 1; m <= $num_splits; m++ )); do 
         let l=$((m-1)) 
         let a=$((l*x+1)) 
         let b=$((m*x)) 
         let r=$((l*x))
         
         $build_path/bin/tensor_gt_mul_split --my-id 0 --party 0,$cs0_host,$cs0_port_inference --party 1,$cs1_host,$cs1_port_inference --arithmetic-protocol beavy --boolean-protocol yao --fractional-bits $fractional_bits --config-file-input $input_config --config-file-model file_config_model0 --layer-id $layer_id --row_start $a --row_end $b --split $num_splits --current-path $build_path > $debug_0/tensor_gt_mul0_layer${layer_id}_split.txt &
         pid1=$!
         wait $pid1
         check_exit_statuses $? 
         echo "Layer $layer_id, split $m: Matrix multiplication and addition is done."
         if [ $m -eq 1 ]; then
            touch finaloutput_0
            printf "$r 1\n" > finaloutput_0
            $build_path/bin/appendfile 0
            pid1=$!
            wait $pid1 
            check_exit_statuses $?
         else 
            $build_path/bin/appendfile 0
            pid1=$!
            wait $pid1 
            check_exit_statuses $?
         fi

         sed -i "1s/${r} 1/${b} 1/" finaloutput_0
      done

      cp finaloutput_0  $build_path/server0/outputshare_0 
      if [ -f finaloutput_0 ]; then
         rm finaloutput_0
      fi
      if [ -f server0/split_input_0 ]; then
         rm server0/split_input_0
      fi
      check_exit_statuses $?
      
      $build_path/bin/tensor_gt_relu --my-id 0 --party 0,$cs0_host,$relu0_port_inference --party 1,$cs1_host,$relu1_port_inference --arithmetic-protocol beavy --boolean-protocol yao --fractional-bits $fractional_bits --filepath file_config_input0 --current-path $build_path > $debug_0/tensor_gt_relu0_layer${layer_id}.txt &
      pid1=$!
      wait $pid1
      check_exit_statuses $?
      echo "Layer $layer_id: ReLU is done"
   
   elif [ ${layer_types[layer_id]} -eq 1 ] && [ $num_splits -eq 1 ];
   then
      input_config="cnn_outputshare"

      $build_path/bin/conv --my-id 0 --party 0,$cs0_host,$cs0_port_inference --party 1,$cs1_host,$cs1_port_inference --arithmetic-protocol beavy --boolean-protocol yao --fractional-bits $fractional_bits --config-file-input $input_config --config-file-model file_config_model0 --layer-id $layer_id --current-path $build_path > $debug_0/cnn0_layer${layer_id}.txt &
      pid1=$!
      wait $pid1
      check_exit_statuses $?
      echo "Layer $layer_id: Convolution is done"

      $build_path/bin/tensor_gt_relu --my-id 0 --party 0,$cs0_host,$relu0_port_inference --party 1,$cs1_host,$relu1_port_inference --arithmetic-protocol beavy --boolean-protocol yao --fractional-bits $fractional_bits --filepath file_config_input0 --current-path $build_path > $debug_0/tensor_gt_relu0_layer${layer_id}.txt &
      pid1=$!
      wait $pid1
      check_exit_statuses $?
      echo "Layer $layer_id: ReLU is done"
      tail -n +2 server0/outputshare_0 >> server0/cnn_outputshare_0
   
   elif [ ${layer_types[layer_id]} -eq 1 ] && [ $num_splits -gt 1 ];
   then
      cp $build_path/server0/cnn_outputshare_0  $build_path/server0/split_input_0
      input_config="split_input"

      echo "Number of splits for layer $layer_id convolution: $num_splits"
      num_kernels=$(jq -r '.kernels' <<< "$split");

      x=$(($num_kernels/$num_splits))
      for(( m = 1; m <= $num_splits; m++ )); do 
         let l=$((m-1)) 
         let a=$((l*x+1))
         let b=$((m*x))
         
         $build_path/bin/conv_split --my-id 0 --party 0,$cs0_host,$cs0_port_inference --party 1,$cs1_host,$cs1_port_inference --arithmetic-protocol beavy --boolean-protocol yao --fractional-bits $fractional_bits --config-file-input $input_config --config-file-model file_config_model0 --layer-id $layer_id --kernel_start $a --kernel_end $b --current-path $build_path > $debug_0/cnn0_layer${layer_id}_split.txt &
         pid1=$!
         wait $pid1
         check_exit_statuses $? 
         echo "Layer $layer_id, split $m: Convolution is done."

         tail -n +2 server0/outputshare_0 >> server0/final_outputshare_0
      done

      cp server0/final_outputshare_0  server0/outputshare_0 
      if [ -f server0/final_outputshare_0 ]; then
         rm server0/final_outputshare_0
      fi
      if [ -f server0/split_input_0 ]; then
         rm server0/split_input_0
      fi
      check_exit_statuses $?

      $build_path/bin/tensor_gt_relu --my-id 0 --party 0,$cs0_host,$relu0_port_inference --party 1,$cs1_host,$relu1_port_inference --arithmetic-protocol beavy --boolean-protocol yao --fractional-bits $fractional_bits --filepath file_config_input0 --current-path $build_path > $debug_0/tensor_gt_relu0_layer${layer_id}.txt &
      pid1=$!
      wait $pid1
      check_exit_statuses $?
      echo "Layer $layer_id: ReLU is done"
      tail -n +2 server0/outputshare_0 >> server0/cnn_outputshare_0
   fi
done

###################################### Last Layer #########################################################################

input_config="outputshare"

$build_path/bin/tensor_gt_mul_test --my-id 0 --party 0,$cs0_host,$cs0_port_inference --party 1,$cs1_host,$cs1_port_inference --arithmetic-protocol beavy --boolean-protocol yao --fractional-bits $fractional_bits --config-file-input $input_config --config-file-model file_config_model0 --layer-id $layer_id --current-path $build_path > $debug_0/tensor_gt_mul0_layer${layer_id}.txt &
pid1=$!
wait $pid1
check_exit_statuses $?
echo "Layer $layer_id: Matrix multiplication and addition is done"

####################################### Argmax  ###########################################################################
$build_path/bin/argmax --my-id 0 --threads 1 --party 0,$cs0_host,$cs0_port_inference --party 1,$cs1_host,$cs1_port_inference --arithmetic-protocol beavy --boolean-protocol beavy --config-filename file_config_input0 --config-input $image_share --current-path $build_path > $debug_0/argmax0_layer${layer_id}.txt &
pid1=$!
wait $pid1
check_exit_statuses $?
echo "Layer $layer_id: Argmax is done"

end=$(date +%s)
####################################### Final output provider  ###########################################################################

$build_path/bin/final_output_provider --my-id 0 --connection-ip $reverse_ssh_host --connection-port $cs0_port_cs0_output_receiver --config-input $image_share --current-path $build_path > $debug_0/final_output_provider.txt &
pid3=$!
wait $pid3
check_exit_statuses $?
echo "Output shares of server 0 sent to the image provider"


wait $pid5 $pid6 

awk '{ sum += $1 } END { print sum }' AverageTimeDetails0 >> AverageTime0
#  > AverageTimeDetails0 #clearing the contents of the file

sort -r -g AverageMemoryDetails0 | head  -1 >> AverageMemory0
#  > AverageMemoryDetails0 #clearing the contents of the file

echo -e "\nInferencing Finished"

Mem=`cat AverageMemory0`
Time=`cat AverageTime0`

Mem=$(printf "%.2f" $Mem) 
Convert_KB_to_GB=$(printf "%.14f" 9.5367431640625E-7)
Mem2=$(echo "$Convert_KB_to_GB * $Mem" | bc -l)

Memory=$(printf "%.3f" $Mem2)

echo "Memory requirement:" `printf "%.3f" $Memory` "GB"
echo "Time taken by inferencing task:" $Time "ms"
echo "Elapsed Time: $(($end-$start)) seconds"

cd $scripts_path 
