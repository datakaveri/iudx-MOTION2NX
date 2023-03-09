#! /bin/bash
# chmod +x test.sh
DEFAULT_TABS_TITLE1="T1"
DEFAULT_TABS_TITLE2="T2"

build_path=${BASE_DIR}/build_debwithrelinfo_gcc

fractional_bits=13

#Delete the old count file if any. Write a new count file with 0 as its data
if [ -f count ]; then
   rm count
   echo "count is removed"
fi



image_ids=(0 1) 
for i in "${image_ids[@]}"
do 
layer_id=1
input_config=" "
if [ $layer_id -eq 1 ];
then
    input_config="ip$i"
fi

#Layer 1
DEFAULT_TABS_CMD1="$build_path/bin/tensor_gt_mul_test --my-id 0 --party 0,::1,7002 --party 1,::1,7000 --arithmetic-protocol beavy --boolean-protocol yao --fractional-bits $fractional_bits --config-file-input $input_config --config-file-model file_config_model0 --layer-id $layer_id --current-path $build_path"
DEFAULT_TABS_CMD2="$build_path/bin/tensor_gt_mul_test --my-id 1 --party 0,::1,7002 --party 1,::1,7000 --arithmetic-protocol beavy --boolean-protocol yao --fractional-bits $fractional_bits  --config-file-input $input_config --config-file-model file_config_model1 --layer-id $layer_id --current-path $build_path"
DEFAULT_TABS_CMD3="$build_path/bin/tensor_gt_relu --my-id 0 --party 0,::1,7002 --party 1,::1,7000 --arithmetic-protocol beavy --boolean-protocol yao --fractional-bits $fractional_bits --filepath file_config_input0 --current-path $build_path"
DEFAULT_TABS_CMD4="$build_path/bin/tensor_gt_relu --my-id 1 --party 0,::1,7002 --party 1,::1,7000 --arithmetic-protocol beavy --boolean-protocol yao --fractional-bits $fractional_bits --filepath file_config_input1 --current-path $build_path"

((layer_id++))

#Updating the config file for layers 2 and above. 
if [ $layer_id -gt 1 ];
then
    input_config="outputshare"
fi

# Layer 2
DEFAULT_TABS_CMD5="$build_path/bin/tensor_gt_mul_test --my-id 0 --party 0,::1,7002 --party 1,::1,7000 --arithmetic-protocol beavy --boolean-protocol yao --fractional-bits $fractional_bits --config-file-input $input_config --config-file-model file_config_model0 --layer-id $layer_id --current-path $build_path"
DEFAULT_TABS_CMD6="$build_path/bin/tensor_gt_mul_test --my-id 1 --party 0,::1,7002 --party 1,::1,7000 --arithmetic-protocol beavy --boolean-protocol yao --fractional-bits $fractional_bits --config-file-input $input_config --config-file-model file_config_model1 --layer-id $layer_id --current-path $build_path"
DEFAULT_TABS_CMD7="$build_path/bin/argmax --my-id 0 --party 0,::1,7000 --party 1,::1,7001 --arithmetic-protocol beavy --boolean-protocol yao --repetitions 1 --config-filename file_config_input0 --config-input X$i --current-path $build_path"
DEFAULT_TABS_CMD8="$build_path/bin/argmax --my-id 1 --party 0,::1,7000 --party 1,::1,7001 --arithmetic-protocol beavy --boolean-protocol yao --repetitions 1 --config-filename file_config_input1 --config-input X$i --current-path $build_path"

gnome-terminal --tab -- bash -ic "export TITLE_DEFAULT='$DEFAULT_TABS_TITLE1';$DEFAULT_TABS_CMD1;$DEFAULT_TABS_CMD3;$DEFAULT_TABS_CMD5;$DEFAULT_TABS_CMD7;exec bash;exit;"
gnome-terminal --tab -- bash -ic "export TITLE_DEFAULT='$DEFAULT_TABS_TITLE2';$DEFAULT_TABS_CMD2;$DEFAULT_TABS_CMD4;$DEFAULT_TABS_CMD6;$DEFAULT_TABS_CMD8;exec bash;exit;"

sleep 80
done
