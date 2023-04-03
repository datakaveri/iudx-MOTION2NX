#! /bin/bash
image_config=${BASE_DIR}/config_files/file_config_input
model_config=${BASE_DIR}/config_files/file_config_model
build_path=${BASE_DIR}/build_debwithrelinfo_gcc
build_path_model=${BASE_DIR}/Dataprovider/weights_provider
image_path=${BASE_DIR}/Dataprovider/image_provider
debug_0=$build_path/server0/debug_files
debug_1=$build_path/server1/debug_files



cs1_ip=192.168.1.127
cs1_port=3390

cs0_ip=192.168.1.141
cs0_port=4567

if [ ! -d "$debug_0" ];
then
	# Recursively create the required directories
	mkdir -p $debug_0
fi
if [ ! -d "$debug_1" ];
then
	mkdir -p $debug_1
fi

fractional_bits=13

$build_path/bin/Weights_Share_Receiver --my-id 0 --port $cs0_port --file-names $model_config --current-path $build_path >> $debug_0/Weights_Share_Receiver0.txt &
pid2=$i
wait $pid2



image_ids=(15)

for i in "${image_ids[@]}"
# for((i=1;i<=1;i++))
do
$build_path/bin/Image_Share_Receiver --my-id 0 --port $cs0_port --fractional-bits $fractional_bits --file-names $image_config --index $i --current-path $build_path >> $debug_0/Image_Share_Receiver0.txt &
pid1=$!
# $build_path/bin/Image_Share_Receiver --my-id 1 --fractional-bits $fractional_bits --file-names $image_config --index $i --current-path $build_path >> $debug_1/Image_Share_Receiver1.txt &
# pid2=$!

sleep 2
$build_path/bin/image_provider_iudx --compute-server0-ip $cs0_ip --compute-server0-port $cs0_port --compute-server1-ip $cs1_ip --compute-server1-port $cs1_port --fractional-bits $fractional_bits --NameofImageFile X$i --filepath $image_path >> $debug_1/image_provider.txt &
pid3=$!

wait $pid3
# wait $pid1 $pid2 
wait $pid1
done
$SHELL
