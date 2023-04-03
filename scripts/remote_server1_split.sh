#! /bin/bash
image_config=${BASE_DIR}/config_files/file_config_input
model_config=${BASE_DIR}/config_files/file_config_model
build_path=${BASE_DIR}/build_debwithrelinfo_gcc
build_path_model=${BASE_DIR}/Dataprovider/weights_provider
image_provider_path=${BASE_DIR}/Dataprovider/image_provider/Final_Output_Shares
image_path=${BASE_DIR}/Dataprovider/image_provider
debug_0=$build_path/server0/debug_files
debug_1=$build_path/server1/debug_files

# #####################Inputs##########################################################################################################

# cs0_ip is the ip of server0, cs1_ip is the ip of server1
cs0_ip=127.0.0.1
cs1_ip=127.0.0.1

# Ports on which weights recceiver talk
cs0_port=3390
cs1_port=4567

# Ports on which image provider talks
cs0_port_image=3390
cs1_port_image=4567


# Ports on which server0 and server1 of the inferencing tasks talk to each other
port0_inference=3390
port1_inference=4567

fractional_bits=13

# Index of the image for which inferencing task is run
image_ids=(22)

##########################################################################################################################################
if [ ! -d "$debug_0" ];
then
	# Recursively create the required directories
	mkdir -p $debug_0
fi
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
echo "Weight Shares Receiver starts"
$build_path/bin/Weights_Share_Receiver --my-id 1 --port $cs1_port --file-names $model_config --current-path $build_path >> $debug_1/Weights_Share_Receiver.txt &
pid2=$!


#########################Weights Provider ############################################################################################
echo "Weight Provider starts"
$build_path/bin/weights_provider --compute-server0-ip $cs0_ip --compute-server0-port $cs0_port --compute-server1-ip $cs1_ip --compute-server1-port $cs1_port --dp-id 0 --fractional-bits $fractional_bits --filepath $build_path_model >> $debug_1/weights_provider.txt &
pid3=$!

wait $pid3
wait $pid2 
echo "Weight Shares received"

#########################Image Share Receiver ############################################################################################
echo "Image Shares Receiver starts"
for i in "${image_ids[@]}"
do

$build_path/bin/Image_Share_Receiver --my-id 1 --port $cs1_port_image --fractional-bits $fractional_bits --file-names $image_config --current-path $build_path >> $debug_1/Image_Share_Receiver.txt &
pid2=$!

wait $pid2

done

echo "Image shares received"
#########################Share generators end ############################################################################################




########################Inferencing task starts ###############################################################################################
for i in "${image_ids[@]}"
# for((image_ids=1;image_ids<=5;image_ids++))
do
 echo 

 echo "image_ids X"$i >> MemoryDetails1 
 echo "NN Inference for Image ID: $i"
 #number of splits
 splits=4
 echo "Number of splits for layer 1 matrix multiplication - $splits"
 x=$((256/splits))
 
############################Inputs for inferencing tasks #######################################################################################
 for  (( m = 1; m <= $splits; m++ )) 
  do 
layer_id=1
input_config=" "
if [ $layer_id -eq 1 ];
then
    input_config="ip$i"
fi
let l=$((m-1)) 
	let a=$(((m-1)*x+1)) 
	let b=$((m*x)) 
	let r=$((l*x)) 

#######################################Matrix multiplication layer 1 ###########################################################################

   #Layer 1   
   $build_path/bin/tensor_gt_mul_split --my-id 1 --party 0,$cs0_ip,$port0_inference --party 1,$cs1_ip,$port1_inference --arithmetic-protocol beavy --boolean-protocol yao --fractional-bits 13 --config-file-input $input_config --config-file-model file_config_model1 --layer-id $layer_id --row_start $a --row_end $b --split $splits --current-path $build_path  > $build_path/server1/debug_files/tensor_gt_mul1_layer1_split.txt &
   pid1=$!
   wait $pid1  
   echo "Layer 1, split $m - multiplication is done"
   
   if [ $m -eq 1 ];then

      touch finaloutput_1

      printf "$x 1\n" >> finaloutput_1

      $build_path/bin/appendfile 1
      pid1=$!
      wait $pid1 
      
      else 
      
      $build_path/bin/appendfile 1
      pid1=$!
      wait $pid1 
    fi

		sed -i "1s/${r} 1/${b} 1/" finaloutput_1
 done

 cp finaloutput_1  $build_path/server1/outputshare_1


#######################################ReLu layer 1 ####################################################################################
$build_path/bin/tensor_gt_relu --my-id 1 --party 0,$cs0_ip,$port0_inference --party 1,$cs1_ip,$port1_inference --arithmetic-protocol beavy --boolean-protocol yao --fractional-bits $fractional_bits --filepath file_config_input1 --current-path $build_path > $build_path/server1/debug_files/tensor_gt_relu1_layer1.txt &
pid1=$!

wait $pid1 
echo "layer 1 - ReLu is done"

#######################Next layer, layer 2, inputs for layer 2 ###################################################################################################
((layer_id++))

# #Updating the config file for layers 2 and above. 
if [ $layer_id -gt 1 ];
then
    input_config="outputshare"
fi

#######################################Matrix multiplication layer 2 ###########################################################################
$build_path/bin/tensor_gt_mul_test --my-id 1 --party 0,$cs0_ip,$port0_inference --party 1,$cs1_ip,$port1_inference --arithmetic-protocol beavy --boolean-protocol yao --fractional-bits $fractional_bits --config-file-input $input_config --config-file-model file_config_model1 --layer-id $layer_id --current-path $build_path > $build_path/server1/debug_files/tensor_gt_mul1_layer2.txt &
pid1=$!

wait $pid1 
echo "layer 2 - matrix multiplicationa and addition  is done"

####################################### Argmax  ###########################################################################

$build_path/bin/argmax --my-id 1 --party 0,$cs0_ip,$port0_inference --party 1,$cs1_ip,$port1_inference --arithmetic-protocol beavy --boolean-protocol beavy --repetitions 1 --config-filename file_config_input1 --config-input X$i --current-path $build_path  > $build_path/server1/debug_files/argmax1_layer2.txt &
pid1=$!


wait $pid1 
echo "layer 2 - argmax is done"

####################################### Final output provider  ###########################################################################

$build_path/bin/final_output_provider --my-id 1 --connection-port 2233 --connection-ip $cs0_ip --config-input X$i --current-path $build_path > $build_path/server1/debug_files/final_output_provider1.txt &
pid4=$!

wait $pid4 
echo "Output shares of server 1 sent to the Image provider"

wait 
#kill $pid5 $pid6


done
