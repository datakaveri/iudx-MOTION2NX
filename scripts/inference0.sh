#! /bin/bash

build_path=${BASE_DIR}/build_debwithrelinfo_gcc
image_provider_path=${BASE_DIR}/Dataprovider/image_provider/Final_Output_Shares
fractional_bits=13

# ip0=192.168.1.213

# ip1=192.168.1.141

ip0=127.0.0.1

ip1=127.0.0.1

port0=3390

port1=4567

debug_0="$build_path/server0/debug_files"

if [ ! -d "$debug_0" ]; then
   echo "$debug_0 does not exist."
   mkdir $debug_0
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

$build_path/bin/tensor_gt_mul_test --my-id 0 --party 0,$ip0,$port0  --party 1,$ip1,$port1 --arithmetic-protocol beavy --boolean-protocol yao --fractional-bits $fractional_bits --config-file-input $input_config --config-file-model file_config_model0 --layer-id $layer_id --current-path $build_path > $build_path/server0/debug_files/tensor_gt_mul0_layer1.txt &
pid1=$!

wait $pid1
echo "layer 1 - gt mul is done"

$build_path/bin/output_shares_receiver --my-id 0 --listening-port 8899 --current-path $image_provider_path --index $i> $build_path/server0/debug_files/output_shares_receiver0.txt &
#$build_path/bin/output_shares_receiver0 --my-id 0 --listening-port 1234 > $build_path/server0/debug_files/output_shares_receiver0.txt &
pid5=$!



$build_path/bin/output_shares_receiver --my-id 1 --listening-port 2233 --current-path $image_provider_path --index $i> $build_path/server1/debug_files/output_shares_receiver1.txt &
#$build_path/bin/output_shares_receiver0 --my-id 1 --listening-port 1235 > $build_path/server1/debug_files/output_shares_receiver1.txt &
pid6=$!

echo "Image Provider listening for the inferencing result"


$build_path/bin/tensor_gt_relu --my-id 0 --party 0,$ip0,$port0 --party 1,$ip1,$port1 --arithmetic-protocol beavy --boolean-protocol yao --fractional-bits $fractional_bits --filepath file_config_input0 --current-path $build_path > $build_path/server0/debug_files/tensor_gt_relu1_layer0.txt &
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
$build_path/bin/tensor_gt_mul_test --my-id 0 --party 0,$ip0,$port0 --party 1,$ip1,$port1 --arithmetic-protocol beavy --boolean-protocol yao --fractional-bits $fractional_bits --config-file-input $input_config --config-file-model file_config_model0 --layer-id $layer_id --current-path $build_path > $build_path/server0/debug_files/tensor_gt_mul0_layer2.txt &
pid1=$!

wait $pid1 

echo "layer 2 - gt mul is done"

#wait

$build_path/bin/argmax --my-id 0 --party 0,$ip0,$port0 --party 1,$ip1,$port1 --arithmetic-protocol beavy --boolean-protocol beavy --repetitions 1 --config-filename file_config_input0 --config-input X$i --current-path $build_path  > $build_path/server0/debug_files/argmax0_layer2.txt &
pid1=$!

wait $pid1 

echo "layer 2 - argmax is done"


$build_path/bin/final_output_provider --my-id 0 --connection-port 8899 --config-input X$i --current-path $build_path > $build_path/server0/debug_files/final_output_provider0.txt &
pid3=$!

echo "Output shares of server 0 sent to the Image provider"


wait $pid5 $pid6 $pid3

echo "Output shares of server 0 received to the Image provider"
echo "Output shares of server 1 received to the Image provider"
echo "Reconstruction Starts"
$build_path/bin/Reconstruct --current-path $image_provider_path --index $i


#kill $pid5 $pid6

wait 

done