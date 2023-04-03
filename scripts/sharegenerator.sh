#! /bin/bash
image_config=${BASE_DIR}/config_files/file_config_input
model_config=${BASE_DIR}/config_files/file_config_model
build_path=${BASE_DIR}/build_debwithrelinfo_gcc
build_path_model=${BASE_DIR}/Dataprovider/weights_provider
image_path=${BASE_DIR}/Dataprovider/image_provider
debug_0=$build_path/server0/debug_files
debug_1=$build_path/server1/debug_files
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
# pwd
$build_path/bin/Weights_Share_Receiver --my-id 0 --file-names $model_config --current-path $build_path >> $debug_0/Weights_Share_Receiver0.txt &
pid1=$!
$build_path/bin/Weights_Share_Receiver --my-id 1 --file-names $model_config --current-path $build_path >> $debug_1/Weights_Share_Receiver1.txt &
pid2=$i

sleep 2
$build_path/bin/weights_provider --compute-server0-port 1234 --compute-server1-port 1235 --dp-id 0 --fractional-bits $fractional_bits --filepath $build_path_model >> $debug_1/weights_provider.txt &
pid3=$!

wait $pid3
wait $pid1 $pid2 


image_ids=(1 2 7 8 9 )
for i in "${image_ids[@]}"
# for((i=1;i<=5;i++))
do
$build_path/bin/Image_Share_Receiver --my-id 0 --fractional-bits $fractional_bits --file-names $image_config --index $i --current-path $build_path >> $debug_0/Image_Share_Receiver0.txt &
pid1=$!
$build_path/bin/Image_Share_Receiver --my-id 1 --fractional-bits $fractional_bits --file-names $image_config --index $i --current-path $build_path >> $debug_1/Image_Share_Receiver1.txt &
pid2=$!

sleep 2
$build_path/bin/image_provider_iudx --compute-server0-port 1234 --compute-server1-port 1235 --fractional-bits $fractional_bits --NameofImageFile X$i --filepath $image_path >> $debug_1/image_provider.txt &
pid3=$!

wait $pid3
wait $pid1 $pid2 


done
$SHELL
