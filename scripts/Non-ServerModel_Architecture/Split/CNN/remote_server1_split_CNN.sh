#! /bin/bash
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
model_config=${BASE_DIR}/config_files/model_cnn_config.json
build_path=${BASE_DIR}/build_debwithrelinfo_gcc
model_provider_path=${BASE_DIR}/data/ModelProvider
debug_1=${BASE_DIR}/logs/server1/
smpc_config_path=${BASE_DIR}/config_files/smpc-cnn-config.json
smpc_config=`cat $smpc_config_path`
# #####################Inputs##########################################################################################################
# Do dns reolution or not 
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

fractional_bits=`echo $smpc_config | jq -r .fractional_bits`

image_share="remote_image_shares"

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
$build_path/bin/Weights_Share_Receiver_CNN --my-id 1 --port $cs1_port_model_receiver --current-path $build_path > $debug_1/Weights_Share_Receiver.txt &
pid2=$!


#########################Weights Provider ############################################################################################
echo "Weight Provider starts"
$build_path/bin/CNN_weights_provider_genr --compute-server0-ip $cs0_host --compute-server0-port $cs0_port_model_receiver --compute-server1-ip $cs1_host --compute-server1-port $cs1_port_model_receiver --fractional-bits $fractional_bits --filepath $model_provider_path --config-file-path $model_config > $debug_1/weights_provider.txt &
pid3=$!

wait $pid3
wait $pid2 
echo "Weight shares received"

#########################Image Share Receiver ############################################################################################
echo "Image shares receiver starts"

$build_path/bin/Image_Share_Receiver_CNN --my-id 1 --port $cs1_port_image_receiver --fractional-bits $fractional_bits --file-names $image_config --current-path $build_path > $debug_1/Image_Share_Receiver.txt &
pid2=$!

wait $pid2

echo "Image shares received"
#########################Share generators end ############################################################################################


########################Inferencing task starts ###############################################################################################

echo "Inferencing task of the image shared starts"
start=$(date +%s)

cp server1/Image_shares/remote_image_shares server1/outputshare_1
cp server1/Image_shares/remote_image_shares server1/cnn_outputshare_1
sed -i "1s/^[^ ]* //" server1/outputshare_1

layer_types=($(cat layer_types1))
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

      $build_path/bin/tensor_gt_mul_test --my-id 1 --party 0,$cs0_host,$cs0_port_inference --party 1,$cs1_host,$cs1_port_inference --arithmetic-protocol beavy --boolean-protocol yao --fractional-bits $fractional_bits --config-file-input $input_config --config-file-model file_config_model1 --layer-id $layer_id --current-path $build_path > $debug_1/tensor_gt_mul1_layer${layer_id}.txt &
      pid1=$!
      wait $pid1 
      check_exit_statuses $?
      echo "Layer $layer_id: Matrix multiplication and addition is done"
      
      $build_path/bin/tensor_gt_relu --my-id 1 --party 0,$cs0_host,$relu0_port_inference --party 1,$cs1_host,$relu1_port_inference --arithmetic-protocol beavy --boolean-protocol yao --fractional-bits $fractional_bits --filepath file_config_input1 --current-path $build_path > $debug_1/tensor_gt_relu1_layer${layer_id}.txt &
      pid1=$!
      wait $pid1
      check_exit_statuses $?
      echo "Layer $layer_id: ReLU is done"
   
   elif [ ${layer_types[layer_id]} -eq 0 ] && [ $num_splits -gt 1 ];
   then
      cp $build_path/server1/outputshare_1  $build_path/server1/split_input_1
      input_config="split_input"

      num_rows=$(jq -r '.rows' <<< "$split");
      echo "Number of splits for layer $layer_id matrix multiplication: $num_rows::$num_splits"
      x=$(($num_rows/$num_splits))
      for(( m = 1; m <= $num_splits; m++ )); do 
         let l=$((m-1))
         let a=$((l*x+1))
         let b=$((m*x))
         let r=$((l*x))
         
         $build_path/bin/tensor_gt_mul_split --my-id 1 --party 0,$cs0_host,$cs0_port_inference --party 1,$cs1_host,$cs1_port_inference --arithmetic-protocol beavy --boolean-protocol yao --fractional-bits $fractional_bits --config-file-input $input_config --config-file-model file_config_model1 --layer-id $layer_id --row_start $a --row_end $b --split $num_splits --current-path $build_path > $debug_1/tensor_gt_mul1_layer${layer_id}_split.txt &
         pid1=$!
         wait $pid1
         check_exit_statuses $? 
         echo "Layer $layer_id, split $m: Matrix multiplication and addition is done."
         if [ $m -eq 1 ]; then
            touch finaloutput_1
            printf "$r 1\n" > finaloutput_1
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
      if [ -f finaloutput_1 ]; then
         rm finaloutput_1
      fi
      if [ -f server1/split_input_1 ]; then
         rm server1/split_input_1
      fi
      check_exit_statuses $?

      $build_path/bin/tensor_gt_relu --my-id 1 --party 0,$cs0_host,$relu0_port_inference --party 1,$cs1_host,$relu1_port_inference --arithmetic-protocol beavy --boolean-protocol yao --fractional-bits $fractional_bits --filepath file_config_input1 --current-path $build_path > $debug_1/tensor_gt_relu1_layer${layer_id}.txt &
      pid1=$!
      wait $pid1
      check_exit_statuses $?
      echo "Layer $layer_id: ReLU is done"
   
   elif [ ${layer_types[layer_id]} -eq 1 ] && [ $num_splits -eq 1 ];
   then
      input_config="cnn_outputshare"
      
      $build_path/bin/conv --my-id 1 --party 0,$cs0_host,$cs0_port_inference --party 1,$cs1_host,$cs1_port_inference --arithmetic-protocol beavy --boolean-protocol yao --fractional-bits $fractional_bits --config-file-input $input_config --config-file-model file_config_model1 --layer-id $layer_id --current-path $build_path > $debug_1/cnn1_layer${layer_id}.txt &
      pid1=$!
      wait $pid1 
      check_exit_statuses $?
      echo "Layer $layer_id: Convolution is done"

      $build_path/bin/tensor_gt_relu --my-id 1 --party 0,$cs0_host,$relu0_port_inference --party 1,$cs1_host,$relu1_port_inference --arithmetic-protocol beavy --boolean-protocol yao --fractional-bits $fractional_bits --filepath file_config_input1 --current-path $build_path > $debug_1/tensor_gt_relu1_layer${layer_id}.txt &
      pid1=$!
      wait $pid1
      check_exit_statuses $?
      echo "Layer $layer_id: ReLU is done"
      tail -n +2 server1/outputshare_1 >> server1/cnn_outputshare_1
   
   elif [ ${layer_types[layer_id]} -eq 1 ] && [ $num_splits -gt 1 ];
   then
      cp $build_path/server1/cnn_outputshare_1  $build_path/server1/split_input_1
      input_config="split_input"

      echo "Number of splits for layer $layer_id convolution: $num_splits"
      num_kernels=$(jq -r '.kernels' <<< "$split");
      
      x=$(($num_kernels/$num_splits))
      for(( m = 1; m <= $num_splits; m++ )); do 
         let l=$((m-1)) 
         let a=$((l*x+1))
         let b=$((m*x))
         $build_path/bin/conv_split --my-id 1 --party 0,$cs0_host,$cs0_port_inference --party 1,$cs1_host,$cs1_port_inference --arithmetic-protocol beavy --boolean-protocol yao --fractional-bits $fractional_bits --config-file-input $input_config --config-file-model file_config_model1 --layer-id $layer_id --kernel_start $a --kernel_end $b --current-path $build_path > $debug_1/cnn1_layer${layer_id}_split.txt &
         pid1=$!
         wait $pid1
         check_exit_statuses $? 
         echo "Layer $layer_id, split $m: Convolution is done."

         tail -n +2 server1/outputshare_1 >> server1/final_outputshare_1
      done

      cp server1/final_outputshare_1  server1/outputshare_1 
      if [ -f server1/final_outputshare_1 ]; then
         rm server1/final_outputshare_1
      fi
      if [ -f server1/split_input_1 ]; then
         rm server1/split_input_1
      fi
      check_exit_statuses $?

      $build_path/bin/tensor_gt_relu --my-id 1 --party 0,$cs0_host,$relu0_port_inference --party 1,$cs1_host,$relu1_port_inference --arithmetic-protocol beavy --boolean-protocol yao --fractional-bits $fractional_bits --filepath file_config_input1 --current-path $build_path > $debug_1/tensor_gt_relu1_layer${layer_id}.txt &
      pid1=$!
      wait $pid1
      check_exit_statuses $?
      echo "Layer $layer_id: ReLU is done"
      tail -n +2 server1/outputshare_1 >> server1/cnn_outputshare_1
   fi
done

input_config="outputshare"

$build_path/bin/tensor_gt_mul_test --my-id 1 --party 0,$cs0_host,$cs0_port_inference --party 1,$cs1_host,$cs1_port_inference --arithmetic-protocol beavy --boolean-protocol yao --fractional-bits $fractional_bits --config-file-input $input_config --config-file-model file_config_model1 --layer-id $layer_id --current-path $build_path > $debug_1/tensor_gt_mul1_layer${layer_id}.txt &
pid1=$!
wait $pid1 
check_exit_statuses $?
echo "Layer $layer_id: Matrix multiplication and addition is done"

####################################### Argmax  ###########################################################################

$build_path/bin/argmax --my-id 1 --threads 1 --party 0,$cs0_host,$cs0_port_inference --party 1,$cs1_host,$cs1_port_inference --arithmetic-protocol beavy --boolean-protocol beavy --config-filename file_config_input1 --config-input $image_share --current-path $build_path > $debug_1/argmax1_layer${layer_id}.txt &
pid1=$!
wait $pid1
check_exit_statuses $?
echo "Layer $layer_id: Argmax is done"

end=$(date +%s)
####################################### Final output provider  ###########################################################################

$build_path/bin/final_output_provider --my-id 1 --connection-port $cs0_port_cs1_output_receiver --connection-ip $cs0_host --config-input $image_share --current-path $build_path > $debug_1/final_output_provider.txt &
pid4=$!
wait $pid4 
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
