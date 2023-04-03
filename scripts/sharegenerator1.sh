#! /bin/bash
image_config=${BASE_DIR}/config_files/file_config_input
model_config=${BASE_DIR}/config_files/file_config_model
build_path=${BASE_DIR}/build_debwithrelinfo_gcc
build_path_model=${BASE_DIR}/Dataprovider/weights_provider
image_path=${BASE_DIR}/Dataprovider/image_provider
debug_0=$build_path/server0/debug_files
debug_1=$build_path/server1/debug_files

# ip0=192.168.1.141

# ip1=192.168.1.141

cs0_ip=192.168.1.125
cs0_port=3390

cs1_ip=192.168.1.141
cs1_port=4567

if [ ! -d "$debug_0" ];
then
	# Recursively create the required directories
	mkdir -p $debug_0
fi
if [ ! -d "$debug_1" ];
then
	mkdir -p $debug_1
fi

echo "weights start"
fractional_bits=13
# pwd
# $build_path/bin/Weights_Share_Receiver --my-id 0 --file-names $model_config --current-path $build_path >> $debug_0/Weights_Share_Receiver0.txt &
# pid1=$!
$build_path/bin/Weights_Share_Receiver --my-id 1 --port $cs1_port --file-names $model_config --current-path $build_path >> $debug_1/Weights_Share_Receiver1.txt &
pid2=$!

$build_path/bin/weights_provider --compute-server0-ip $cs0_ip --compute-server0-port $cs0_port --compute-server1-ip $cs1_ip --compute-server1-port $cs1_port --dp-id 0 --fractional-bits $fractional_bits --filepath $build_path_model >> $debug_1/weights_provider.txt &
pid3=$!

wait $pid3
wait $pid2 

echo "weights send"

image_ids=(21)

echo "image start"
for i in "${image_ids[@]}"

# for((i=2;i<=6;i+=2))
do

$build_path/bin/Image_Share_Receiver --my-id 1 --port $cs1_port --fractional-bits $fractional_bits --file-names $image_config --index $i --current-path $build_path >> $debug_1/Image_Share_Receiver1.txt &
pid2=$!

# sleep 2
# $build_path/bin/image_provider_iudx --compute-server0-port 1234 --compute-server1-port 1235 --fractional-bits $fractional_bits --NameofImageFile X$i --filepath $image_path >> $debug_1/image_provider.txt &
# pid3=$!

# wait $pid3
# wait $pid1 $pid2 
wait $pid2

echo "image received"
done

$SHELL