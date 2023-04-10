#! /bin/bash

build_path=${BASE_DIR}/build_debwithrelinfo_gcc
image_provider_path=${BASE_DIR}/Dataprovider/image_provider/Final_Output_Shares
fractional_bits=13

debug_0="$build_path/server0/debug_files"
# debug_1="$build_path/server1/debug_files"

if [ ! -d "$debug_0" ]; then
   echo "$debug_0 does not exist."
   mkdir $debug_0
fi
# if [ ! -d "$debug_1" ]; then
#    echo "$debug_1 does not exist."
#    mkdir $debug_1
# fi

image_ids=(1)
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

$build_path/bin/tensor_gt_mul_test --my-id 0 --party 0,192.168.1.141,7002 --party 1,192.168.1.141,7000 --arithmetic-protocol beavy --boolean-protocol yao --fractional-bits $fractional_bits --config-file-input $input_config --config-file-model file_config_model0 --layer-id $layer_id --current-path $build_path > $build_path/server0/debug_files/tensor_gt_mul0_layer1.txt &
pid1=$!
#echo "$pid1: layer 1, id = 0"

$build_path/bin/tensor_gt_mul_test --my-id 1 --party 0,192.168.1.141,7002 --party 1,192.168.1.141,7000 --arithmetic-protocol beavy --boolean-protocol yao --fractional-bits $fractional_bits  --config-file-input $input_config --config-file-model file_config_model1 --layer-id $layer_id --current-path $build_path >  $build_path/server1/debug_files/tensor_gt_mul1_layer1.txt &
pid2=$!
#echo "$pid2: layer 1, id = 1"
# wait $pid1 $pid2 
wait $pid1 $pid2
echo "layer 1 - gt mul is done"

wait 
done