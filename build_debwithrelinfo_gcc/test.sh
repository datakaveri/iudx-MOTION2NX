#! /bin/bash
chmod +x test.sh
pwd
DEFAULT_TABS_TITLE1="T1"
DEFAULT_TABS_TITLE2="T2"

#Delete the old count file if any. Write a new count file with 0 as its data
if [ -f count ]; then

   rm count

   echo "count is removed"

fi

touch count
touch nomatch
echo 0 >> count

for i in {1..10}

do 
# i=151

layer_id=1

k=" "

#ip(i)-->ip1 , ip2 ,ip3 .. and so on 
if [ $layer_id -eq 1 ];
then
    k="ip$i"

fi

#prints k 
echo $k



#layer_id =1 and k=ip1/ip2
DEFAULT_TABS_CMD1="./bin/tensor_gt_mul_test --my-id 0 --party 0,::1,7002 --party 1,::1,7000 --arithmetic-protocol beavy --boolean-protocol yao --fractional-bits 13 --config-file-input $k --config-file-model file_config_model0 --layer-id $layer_id"
DEFAULT_TABS_CMD2="./bin/tensor_gt_mul_test --my-id 1 --party 0,::1,7002 --party 1,::1,7000 --arithmetic-protocol beavy --boolean-protocol yao --fractional-bits 13  --config-file-input $k --config-file-model file_config_model1 --layer-id $layer_id"
DEFAULT_TABS_CMD3="./bin/tensor_gt_relu --my-id 0 --party 0,::1,7002 --party 1,::1,7000 --arithmetic-protocol beavy --boolean-protocol yao --fractional-bits 13 --filepath file_config_input0"
DEFAULT_TABS_CMD4="./bin/tensor_gt_relu --my-id 1 --party 0,::1,7002 --party 1,::1,7000 --arithmetic-protocol beavy --boolean-protocol yao --fractional-bits 13 --filepath file_config_input1"


((layer_id++))


#k=file_config_input for layer 2 
if [ $layer_id -gt 1 ];
then
    k="outputshare"

fi

# layer_id =2 and k=file_config_input ... 0 and 1 is appended in tensor_gt_mul_test
DEFAULT_TABS_CMD5="./bin/tensor_gt_mul_test --my-id 0 --party 0,::1,7002 --party 1,::1,7000 --arithmetic-protocol beavy --boolean-protocol yao --fractional-bits 13 --config-file-input $k --config-file-model file_config_model0 --layer-id $layer_id"
DEFAULT_TABS_CMD6="./bin/tensor_gt_mul_test --my-id 1 --party 0,::1,7002 --party 1,::1,7000 --arithmetic-protocol beavy --boolean-protocol yao --fractional-bits 13 --config-file-input $k --config-file-model file_config_model1 --layer-id $layer_id"
DEFAULT_TABS_CMD7="./bin/argmax --my-id 0 --party 0,::1,7000 --party 1,::1,7001 --arithmetic-protocol beavy --boolean-protocol yao --repetitions 1 --config-filename file_config_input0 --config-input X$i"
DEFAULT_TABS_CMD8="./bin/argmax --my-id 1 --party 0,::1,7000 --party 1,::1,7001 --arithmetic-protocol beavy --boolean-protocol yao --repetitions 1 --config-filename file_config_input1 --config-input X$i"

gnome-terminal --tab -- bash -ic "export TITLE_DEFAULT='$DEFAULT_TABS_TITLE1';$DEFAULT_TABS_CMD1;$DEFAULT_TABS_CMD3;$DEFAULT_TABS_CMD5;$DEFAULT_TABS_CMD7;exec bash;exit;"
gnome-terminal --tab -- bash -ic "export TITLE_DEFAULT='$DEFAULT_TABS_TITLE2';$DEFAULT_TABS_CMD2;$DEFAULT_TABS_CMD4;$DEFAULT_TABS_CMD6;$DEFAULT_TABS_CMD8;exec bash;exit;"

sleep 60

# echo hello

done

  
