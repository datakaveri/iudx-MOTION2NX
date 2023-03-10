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


for((i=1;i<=20;i++))
# image_ids=(0) 
# for i in "${image_ids[@]}"
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

# layer_id =2 and k=file_config_input ... 0 and 1 is appended in tensor_gt_mul_test
DEFAULT_TABS_CMD5="$build_path/bin/tensor_gt_mul_test --my-id 0 --party 0,::1,7002 --party 1,::1,7000 --arithmetic-protocol beavy --boolean-protocol yao --fractional-bits $fractional_bits --config-file-input $input_config --config-file-model file_config_model0 --layer-id $layer_id --current-path $build_path"
DEFAULT_TABS_CMD6="$build_path/bin/tensor_gt_mul_test --my-id 1 --party 0,::1,7002 --party 1,::1,7000 --arithmetic-protocol beavy --boolean-protocol yao --fractional-bits $fractional_bits --config-file-input $input_config --config-file-model file_config_model1 --layer-id $layer_id --current-path $build_path"
DEFAULT_TABS_CMD7="$build_path/bin/argmax --my-id 0 --party 0,::1,7000 --party 1,::1,7001 --arithmetic-protocol beavy --boolean-protocol beavy --repetitions 1 --config-filename file_config_input0 --config-input X$i --current-path $build_path"
DEFAULT_TABS_CMD8="$build_path/bin/argmax --my-id 1 --party 0,::1,7000 --party 1,::1,7001 --arithmetic-protocol beavy --boolean-protocol beavy --repetitions 1 --config-filename file_config_input1 --config-input X$i --current-path $build_path"
DEFAULT_TABS_CMD9="$build_path/bin/output_shares_receiver --my-id 0 --listening-port 1234"
DEFAULT_TABS_CMD10="$build_path/bin/output_shares_receiver --my-id 1 --listening-port 1235"
DEFAULT_TABS_CMD11="$build_path/bin/final_output_provider --my-id 0 --connection-port 1234 --config-input X$i"
DEFAULT_TABS_CMD12="$build_path/bin/final_output_provider --my-id 1 --connection-port 1235 --config-input X$i"

gnome-terminal --tab -- bash -ic "export TITLE_DEFAULT='$DEFAULT_TABS_TITLE1';$DEFAULT_TABS_CMD9;exec bash;exit;"
gnome-terminal --tab -- bash -ic "export TITLE_DEFAULT='$DEFAULT_TABS_TITLE2';$DEFAULT_TABS_CMD10;exec bash;exit;"
gnome-terminal --tab -- bash -ic "export TITLE_DEFAULT='$DEFAULT_TABS_TITLE3';$DEFAULT_TABS_CMD1;$DEFAULT_TABS_CMD3;$DEFAULT_TABS_CMD5;$DEFAULT_TABS_CMD7;$DEFAULT_TABS_CMD11;exec bash;exit;"
gnome-terminal --tab -- bash -ic "export TITLE_DEFAULT='$DEFAULT_TABS_TITLE4';$DEFAULT_TABS_CMD2;$DEFAULT_TABS_CMD4;$DEFAULT_TABS_CMD6;$DEFAULT_TABS_CMD8;$DEFAULT_TABS_CMD12;exec bash;exit;"

sleep 150
done
