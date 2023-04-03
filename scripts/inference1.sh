#! /bin/bash

build_path=${BASE_DIR}/build_debwithrelinfo_gcc
image_provider_path=${BASE_DIR}/Dataprovider/image_provider/Final_Output_Shares
fractional_bits=13

# ip0=192.168.1.186

# ip1=192.168.1.141

ip0=127.0.0.1

ip1=127.0.0.1

port0=3390

port1=4567


debug_1="$build_path/server1/debug_files"


if [ ! -d "$debug_1" ]; then
   echo "$debug_1 does not exist."
   mkdir $debug_1
fi

image_ids=(11 12 13)
for i in "${image_ids[@]}"
# for((i=1;i<=5;i++))
do 
echo 
echo "Inferencing task of example X$i"
layer_id=1
input_config=" "
if [ $layer_id -eq 1 ];
then
    input_config="ip$i"
fi

$build_path/bin/tensor_gt_mul_test --my-id 1 --party 0,$ip0,$port0  --party 1,$ip1,$port1 --arithmetic-protocol beavy --boolean-protocol yao --fractional-bits $fractional_bits --config-file-input $input_config --config-file-model file_config_model1 --layer-id $layer_id --current-path $build_path > $build_path/server1/debug_files/tensor_gt_mul1_layer1.txt &
pid1=$!
 
wait $pid1
echo "layer 1 - gt mul is done"


$build_path/bin/tensor_gt_relu --my-id 1 --party 0,$ip0,$port0 --party 1,$ip1,$port1 --arithmetic-protocol beavy --boolean-protocol yao --fractional-bits $fractional_bits --filepath file_config_input1 --current-path $build_path > $build_path/server1/debug_files/tensor_gt_relu1_layer1.txt &
pid1=$!

wait $pid1 
echo "layer 1 - relu is done"

((layer_id++))

# #Updating the config file for layers 2 and above. 
if [ $layer_id -gt 1 ];
then
    input_config="outputshare"
fi

# # layer_id =2 and k=file_config_input ... 0 and 1 is appended in tensor_gt_mul_test
$build_path/bin/tensor_gt_mul_test --my-id 1 --party 0,$ip0,$port0 --party 1,$ip1,$port1 --arithmetic-protocol beavy --boolean-protocol yao --fractional-bits $fractional_bits --config-file-input $input_config --config-file-model file_config_model1 --layer-id $layer_id --current-path $build_path > $build_path/server1/debug_files/tensor_gt_mul1_layer2.txt &
pid1=$!

wait $pid1 
echo "layer 2 - gt mul is done"

#wait

$build_path/bin/argmax --my-id 1 --party 0,$ip0,$port0 --party 1,$ip1,$port1 --arithmetic-protocol beavy --boolean-protocol beavy --repetitions 1 --config-filename file_config_input1 --config-input X$i --current-path $build_path  > $build_path/server1/debug_files/argmax1_layer2.txt &
pid1=$!


wait $pid1 
echo "layer 2 - argmax is done"

$build_path/bin/final_output_provider --my-id 1 --connection-port 2233 --connection-ip $ip1 --config-input X$i --current-path $build_path > $build_path/server1/debug_files/final_output_provider1.txt &
pid4=$!

sleep 5
wait $pid4 
echo "Output shares of server 1 sent to the Image provider"

wait 
#kill $pid5 $pid6


done