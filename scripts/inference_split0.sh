#! /bin/bash

build_path=${BASE_DIR}/build_debwithrelinfo_gcc
image_provider_path=${BASE_DIR}/Dataprovider/image_provider/Final_Output_Shares
scripts_path=${BASE_DIR}/scripts
fractional_bits=13

# ip0=192.168.1.127

# ip1=192.168.1.141

ip0=172.30.9.43

ip1=192.168.1.141

port0=3390

port1=4567

debug_0=$build_path/server0/debug_files

if [ ! -d "$debug_0" ];
then
	mkdir $debug_0
fi


cd $build_path

if [ -f finaloutput_0 ]; then
   rm finaloutput_0
   # echo "final output 0 is removed"
fi

if [ -f MemoryDetails0 ]; then
   rm MemoryDetails0
   # echo "Memory Details0 are removed"
fi

if [ -f AverageMemoryDetails0 ]; then
   rm AverageMemoryDetails0
   # echo "Average Memory Details0 are removed"
fi

if [ -f AverageMemory0 ]; then
   rm AverageMemory0
   # echo "Average Memory Details0 are removed"
fi

if [ -f AverageTimeDetails0 ]; then
   rm AverageTimeDetails0
   # echo "AverageTimeDetails0 is removed"
fi

if [ -f AverageTime0 ]; then
   rm AverageTime0
   # echo "AverageTime0 is removed"
fi

image_no=(15)
for image_ids in "${image_no[@]}"
# for((image_ids=1;image_ids<=5;image_ids++))
do
 echo 
 echo "image_ids X"$image_ids >> MemoryDetails0

 echo "NN Inference for Image ID: $image_ids"
 #number of splits
 splits=4
 echo "Number of splits for layer 1 matrix multiplication - $splits"
 x=$((256/splits))
 
 for  (( m = 1; m <= $splits; m++ )) 
  do 
	layer_id=1
	input_config=" "
	if [ $layer_id -eq 1 ];
	then
    	input_config="ip$image_ids"
	fi
	let l=$((m-1)) 
	let a=$(((m-1)*x+1)) 
	let b=$((m*x)) 
	let r=$((l*x)) 

   #Layer 1
	$build_path/bin/tensor_gt_mul_split --my-id 0 --party 0,$ip0,$port0 --party 1,$ip1,$port1 --arithmetic-protocol beavy --boolean-protocol yao --fractional-bits 13 --config-file-input $input_config --config-file-model file_config_model0 --layer-id $layer_id --row_start $a --row_end $b --split $splits --current-path $build_path > $build_path/server0/debug_files/tensor_gt_mul0_layer1_split.txt &
   pid1=$!
   
   wait $pid1 
   echo "Layer 1, split $m - multiplication is done"
   
   if [ $m -eq 1 ];then
      touch finaloutput_0

      printf "$x 1\n" >> finaloutput_0

      $build_path/bin/appendfile 0
      pid1=$!
      wait $pid1 
      
      else 
      
      $build_path/bin/appendfile 0
      pid1=$!
      wait $pid1 
    fi

		sed -i "1s/${r} 1/${b} 1/" finaloutput_0

 done

 cp finaloutput_0  $build_path/server0/outputshare_0 

$build_path/bin/output_shares_receiver --my-id 0 --listening-port 8899 --current-path $image_provider_path --index $image_ids > $build_path/server0/debug_files/output_shares_receiver0_split.txt &
pid5=$!

$build_path/bin/output_shares_receiver --my-id 1 --listening-port 2233  --current-path $image_provider_path --index $image_ids > $build_path/server1/debug_files/output_shares_receiver1_split.txt &
pid6=$!

$build_path/bin/tensor_gt_relu --my-id 0 --party 0,$ip0,$port0 --party 1,$ip1,$port1 --arithmetic-protocol beavy --boolean-protocol yao --fractional-bits 13 --filepath file_config_input0 --current-path $build_path > $build_path/server0/debug_files/tensor_relu0_layer1_split.txt &
pid1=$!


wait $pid1 
echo "Layer 1 - ReLU is done"

((layer_id++))

#Layer 2
if [ $layer_id -gt 1 ];then
   input_config="outputshare"
fi

$build_path/bin/tensor_gt_mul_test --my-id 0 --party 0,$ip0,$port0 --party 1,$ip1,$port1 --arithmetic-protocol beavy --boolean-protocol yao --fractional-bits $fractional_bits --config-file-input $input_config --config-file-model file_config_model0 --layer-id $layer_id --current-path $build_path > $build_path/server0/debug_files/tensor_gt_mul0_layer2_split.txt & 
pid1=$!

wait $pid1 
echo "Layer 2 - multiplication is done"


$build_path/bin/argmax --my-id 0 --party 0,$ip0,$port0 --party 1,$ip1,$port1 --arithmetic-protocol beavy --boolean-protocol beavy --repetitions 1 --config-filename file_config_input0 --config-input X$image_ids --current-path $build_path  > $build_path/server0/debug_files/argmax0_layer2_split.txt &
pid1=$!


wait $pid1 
echo "Layer 2 - ArgMax is done"

$build_path/bin/final_output_provider --my-id 0 --connection-port 8899 --config-input X$image_ids --current-path $build_path --connection-ip $ip0 > $build_path/server0/debug_files/final_output_provider0_split.txt 
pid3=$!

wait $pid5 $pid3 $pid6 

echo "Output shares of server 0 received to the Image provider"
echo "Output shares of server 1 received to the Image provider"
echo "Reconstruction Starts"

$build_path/bin/Reconstruct --current-path $image_provider_path --index $image_ids
echo "Reconstruction Finished"

wait

 if [ -f finaloutput_0 ];then
   rm finaloutput_0
   # echo "final output 0 is removed"
 fi


 awk '{ sum += $1 } END { print sum }' AverageTimeDetails0 >> AverageTime0
 > AverageTimeDetails0 #clearing the contents of the file

 sort -r -n AverageMemoryDetails0 | head  -1 >> AverageMemory0
 > AverageMemoryDetails0 #clearing the contents of the file


done 

echo -e "\nInferencing Finished"

awk '{ sum += $1 } END { print sum}' AverageTime0 >> AverageTime0


awk '{ sum += $1 } END { print sum}' AverageMemory0 >> AverageMemory0


cd $scripts_path

$SHELL