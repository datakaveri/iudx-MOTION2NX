#! /bin/bash

build_path=${BASE_DIR}/build_debwithrelinfo_gcc
image_provider_path=${BASE_DIR}/Dataprovider/image_provider/Final_Output_Shares
fractional_bits=13

#/home/srishti04/IUDX/iudx-MOTION2NX/build_debwithrelinfo_gcc


debug_0="$build_path/server0/debug_files"
debug_1="$build_path/server1/debug_files"

if [ ! -d "$debug_0" ]; then
   echo "$debug_0 does not exist."
   mkdir $debug_0
fi
if [ ! -d "$debug_1" ]; then
   echo "$debug_1 does not exist."
   mkdir $debug_1
fi

image_ids=(9)
for i in "${image_ids[@]}"
# for((i=1;i<=;i++))
do 
echo 
echo "Inferencing task of example X$i"
layer_id=1
input_config=" "
if [ $layer_id -eq 1 ];
then
    input_config="ip$i"
fi

  $build_path/bin/server2_old > $build_path/server0/debug_files/server2.txt &
  pid1=$!
  $build_path/bin/server1_old --fn1 file_config_model1 --fn2 remote_image_shares --current-path $build_path > $build_path/server0/debug_files/server1.txt &
  pid2=$!
  $build_path/bin/server0_old --fn1 file_config_model0 --fn2 remote_image_shares --current-path $build_path > $build_path/server0/debug_files/server0.txt &
  pid3=$!

wait $pid1 $pid2 $pid3
echo "layer 1 - gt mul is done"

$build_path/bin/output_shares_receiver --my-id 0 --listening-port 1234 --current-path $image_provider_path> $build_path/server0/debug_files/output_shares_receiver0.txt &
pid5=$!
#echo "$pid5:output shares receiver0, Party 0"


$build_path/bin/output_shares_receiver --my-id 1 --listening-port 1235 --current-path $image_provider_path> $build_path/server1/debug_files/output_shares_receiver1.txt &
pid6=$!
#echo "$pid6:output shares receiver0, Party 1"
echo "Image Provider listening for the inferencing result"


$build_path/bin/tensor_gt_relu --my-id 0 --party 0,::1,7002 --party 1,::1,7000 --arithmetic-protocol beavy --boolean-protocol yao --fractional-bits $fractional_bits --filepath file_config_input0 --current-path $build_path > $build_path/server0/debug_files/tensor_gt_relu0_layer1.txt &
pid1=$!
#echo "$pid1:Layer 1 ReLU, Party 0"

$build_path/bin/tensor_gt_relu --my-id 1 --party 0,::1,7002 --party 1,::1,7000 --arithmetic-protocol beavy --boolean-protocol yao --fractional-bits $fractional_bits --filepath file_config_input1 --current-path $build_path > $build_path/server1/debug_files/tensor_gt_relu1_layer1.txt &

pid2=$!
#echo "$pid2:Layer 1 ReLU, Party 1"
wait $pid1 $pid2 
echo "layer 1 - relu is done"

((layer_id++))

# #Updating the config file for layers 2 and above. 
if [ $layer_id -gt 1 ];
then
    input_config="outputshare"
fi

# # layer_id =2 and k=file_config_input ... 0 and 1 is appended in tensor_gt_mul_test
$build_path/bin/tensor_gt_mul_test --my-id 0 --party 0,::1,7002 --party 1,::1,7000 --arithmetic-protocol beavy --boolean-protocol yao --fractional-bits $fractional_bits --config-file-input $input_config --config-file-model file_config_model0 --layer-id $layer_id --current-path $build_path > $build_path/server0/debug_files/tensor_gt_mul0_layer2.txt &
pid1=$!
#echo "$pid1:Layer 2, Party 0"

$build_path/bin/tensor_gt_mul_test --my-id 1 --party 0,::1,7002 --party 1,::1,7000 --arithmetic-protocol beavy --boolean-protocol yao --fractional-bits $fractional_bits --config-file-input $input_config --config-file-model file_config_model1 --layer-id $layer_id --current-path $build_path > $build_path/server1/debug_files/tensor_gt_mul1_layer2.txt &

pid2=$!
#echo "$pid2:Layer 2, Party 1"
wait $pid1 $pid2
echo "layer 2 - gt mul is done"

#wait



$build_path/bin/argmax --my-id 0 --party 0,::1,7000 --party 1,::1,7001 --arithmetic-protocol beavy --boolean-protocol beavy --repetitions 1 --config-filename file_config_input0 --config-input $i --current-path $build_path  > $build_path/server0/debug_files/argmax0_layer2.txt &
pid1=$!
#echo "$pid1:Layer 2 argmax, Party 0"

$build_path/bin/argmax --my-id 1 --party 0,::1,7000 --party 1,::1,7001 --arithmetic-protocol beavy --boolean-protocol beavy --repetitions 1 --config-filename file_config_input1 --config-input $i --current-path $build_path  > $build_path/server1/debug_files/argmax1_layer2.txt &
pid2=$!
#echo "$pid2:Layer 2 argmax, Party 1"

wait $pid1 $pid2
echo "layer 2 - argmax is done"


$build_path/bin/final_output_provider --my-id 0 --connection-port 1234 --config-input $i --current-path $build_path > $build_path/server0/debug_files/final_output_provider0.txt &
pid3=$!
#echo "$pid3:final output provider0, Party 0"
# wait $pid1 $pid3 
echo "Output shares of server 0 sent to the Image provider"

$build_path/bin/final_output_provider --my-id 1 --connection-port 1235 --config-input $i --current-path $build_path > $build_path/server1/debug_files/final_output_provider1.txt &
pid4=$!
#echo "$pid4:final output provider0, Party 1"
wait $pid5 $pid3 $pid6 $pid4 
echo "Output shares of server 1 sent to the Image provider"
echo "Reconstruction Starts"
$build_path/bin/Reconstruct --current-path $image_provider_path 


#kill $pid5 $pid6

wait 

done
