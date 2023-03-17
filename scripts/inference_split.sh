#! /bin/bash

build_path=${BASE_DIR}/build_debwithrelinfo_gcc
image_provider_path=${BASE_DIR}/Dataprovider/image_provider/Final_Output_Shares
scripts_path=${BASE_DIR}/scripts
fractional_bits=13
debug_0=$build_path/server0/debug_files
debug_1=$build_path/server1/debug_files
if [ ! -d "$debug_0" ];
then
	mkdir $debug_0
fi
if [ ! -d "$debug_1" ];
then
	mkdir $debug_1
fi

cd $build_path
if [ -f finaloutput_0 ]; then
   rm finaloutput_0
   # echo "final output 0 is removed"
fi

if [ -f finaloutput_1 ]; then
   rm finaloutput_1
   # echo "final output 1 is removed"
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

if [ -f MemoryDetails1 ]; then
   rm MemoryDetails1
   # echo "Memory Details1 are removed"
fi

if [ -f AverageMemoryDetails1 ]; then
   rm AverageMemoryDetails1
   # echo "Average Memory Details1 are removed"
fi
if [ -f AverageMemory1 ]; then
   rm AverageMemory1
   # echo "Average Memory Details1 are removed"
fi

if [ -f AverageTimeDetails1 ]; then
   rm AverageTimeDetails1
   # echo "AverageTimeDetails1 is removed"
fi

if [ -f AverageTime1 ]; then
   rm AverageTime1
   # echo "AverageTime1 is removed"
fi

for((image_ids=1;image_ids<=1;image_ids++))
 do
 echo 
 echo "image_ids X"$image_ids >> MemoryDetails0
 echo "image_ids X"$image_ids >> MemoryDetails1 
 echo "NN Inference for Image ID: $image_ids"
 #number of splits
 splits=8
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
	$build_path/bin/tensor_gt_mul_split --my-id 0 --party 0,::1,7002 --party 1,::1,7000 --arithmetic-protocol beavy --boolean-protocol yao --fractional-bits 13 --config-file-input $input_config --config-file-model file_config_model0 --layer-id $layer_id --row_start $a --row_end $b --split $splits --current-path $build_path > $build_path/server0/debug_files/tensor_gt_mul0_layer1_split.txt &
   pid1=$!
   
   $build_path/bin/tensor_gt_mul_split --my-id 1 --party 0,::1,7002 --party 1,::1,7000 --arithmetic-protocol beavy --boolean-protocol yao --fractional-bits 13 --config-file-input $input_config --config-file-model file_config_model1 --layer-id $layer_id --row_start $a --row_end $b --split $splits --current-path $build_path  > $build_path/server1/debug_files/tensor_gt_mul1_layer1_split.txt &
   pid2=$!
   wait $pid1 $pid2 
   echo "Layer 1, split $m - multiplication is done"
   
   if [ $m -eq 1 ];then
      touch finaloutput_0
      touch finaloutput_1

      printf "$x 1\n" >> finaloutput_0
      printf "$x 1\n" >> finaloutput_1

      $build_path/bin/appendfile 
      pid1=$!
      wait $pid1 
      
      else 
      
      $build_path/bin/appendfile 
      pid1=$!
      wait $pid1 
    fi

		sed -i "1s/${r} 1/${b} 1/" finaloutput_0
		sed -i "1s/${r} 1/${b} 1/" finaloutput_1
 done

 cp finaloutput_0  $build_path/server0/outputshare_0 
 cp finaloutput_1  $build_path/server1/outputshare_1

$build_path/bin/output_shares_receiver --my-id 0 --listening-port 1234 --current-path $image_provider_path --index $image_ids > $build_path/server0/debug_files/output_shares_receiver0_split.txt &
pid5=$!

$build_path/bin/output_shares_receiver --my-id 1 --listening-port 1235  --current-path $image_provider_path --index $image_ids > $build_path/server1/debug_files/output_shares_receiver1_split.txt &
pid6=$!

$build_path/bin/tensor_gt_relu --my-id 0 --party 0,::1,7002 --party 1,::1,7000 --arithmetic-protocol beavy --boolean-protocol yao --fractional-bits 13 --filepath file_config_input0 --current-path $build_path > $build_path/server0/debug_files/tensor_relu0_layer1_split.txt &
pid1=$!

$build_path/bin/tensor_gt_relu --my-id 1 --party 0,::1,7002 --party 1,::1,7001 --arithmetic-protocol beavy --boolean-protocol yao --fractional-bits 13 --filepath file_config_input1 --current-path $build_path > $build_path/server1/debug_files/tensor_relu1_layer1_split.txt &
pid2=$!

wait $pid1 $pid2 
echo "Layer 1 - ReLU is done"

((layer_id++))

#Layer 2
if [ $layer_id -gt 1 ];then
   input_config="outputshare"
fi

$build_path/bin/tensor_gt_mul_test --my-id 0 --party 0,::1,7002 --party 1,::1,7000 --arithmetic-protocol beavy --boolean-protocol yao --fractional-bits $fractional_bits --config-file-input $input_config --config-file-model file_config_model0 --layer-id $layer_id --current-path $build_path > $build_path/server0/debug_files/tensor_gt_mul0_layer2_split.txt & 
pid1=$!

$build_path/bin/tensor_gt_mul_test --my-id 1 --party 0,::1,7002 --party 1,::1,7000 --arithmetic-protocol beavy --boolean-protocol yao --fractional-bits $fractional_bits --config-file-input $input_config --config-file-model file_config_model1 --layer-id $layer_id --current-path $build_path > $build_path/server1/debug_files/tensor_gt_mul1_layer2_split.txt &  
pid2=$!

wait $pid1 $pid2 
echo "Layer 2 - multiplication is done"


$build_path/bin/argmax --my-id 0 --party 0,::1,7000 --party 1,::1,7001 --arithmetic-protocol beavy --boolean-protocol beavy --repetitions 1 --config-filename file_config_input0 --config-input X$image_ids --current-path $build_path  > $build_path/server0/debug_files/argmax0_layer2_split.txt &
pid1=$!

$build_path/bin/argmax --my-id 1 --party 0,::1,7000 --party 1,::1,7001 --arithmetic-protocol beavy --boolean-protocol beavy --repetitions 1 --config-filename file_config_input1 --config-input X$image_ids --current-path $build_path  > $build_path/server1/debug_files/argmax1_layer2_split.txt &
pid2=$!

wait $pid1 $pid2 
echo "Layer 2 - ArgMax is done"

$build_path/bin/final_output_provider --my-id 0 --connection-port 1234 --config-input X$image_ids --current-path $build_path > $build_path/server0/debug_files/final_output_provider0_split.txt 
pid3=$!

$build_path/bin/final_output_provider --my-id 1 --connection-port 1235 --config-input X$image_ids --current-path $build_path > $build_path/server1/debug_files/final_output_provider1_split.txt 
pid4=$!
wait $pid5 $pid3 $pid6 $pid4
echo "Output shares of Server 0 sent to the Image provider"
echo "Output shares of Server 1 sent to the Image provider"
echo "Reconstruction Starts"
$build_path/bin/Reconstruct --current-path $image_provider_path --index $image_ids
echo "Reconstruction Finished"
wait

 if [ -f finaloutput_0 ];then
   rm finaloutput_0
   # echo "final output 0 is removed"
 fi
 if [ -f finaloutput_1 ]; then
   rm finaloutput_1
   # echo "final output 1 is removed"
 fi

 awk '{ sum += $1 } END { print sum }' AverageTimeDetails0 >> AverageTime0
 > AverageTimeDetails0 #clearing the contents of the file
 awk '{ sum += $1 } END { print sum }' AverageTimeDetails1 >> AverageTime1
 > AverageTimeDetails1 #clearing the contents of the file
 sort -r -n AverageMemoryDetails0 | head  -1 >> AverageMemory0
 > AverageMemoryDetails0 #clearing the contents of the file
 sort -r -n AverageMemoryDetails1 | head  -1 >> AverageMemory1
 > AverageMemoryDetails1 #clearing the contents of the file

done 

echo -e "\nInferencing Finished"

awk '{ sum += $1 } END { print sum}' AverageTime0 >> AverageTime0
awk '{ sum += $1 } END { print sum}' AverageTime1 >> AverageTime1

awk '{ sum += $1 } END { print sum}' AverageMemory0 >> AverageMemory0
awk '{ sum += $1 } END { print sum}' AverageMemory1 >> AverageMemory1
cd $scripts_path
$SHELL