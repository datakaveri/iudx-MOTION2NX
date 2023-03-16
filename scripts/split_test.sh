#! /bin/bash

build_path=${BASE_DIR}/build_debwithrelinfo_gcc
cd $build_path
if [ -f finaloutput_0 ]; then
   rm finaloutput_0
   echo "final output 0 is removed"
fi

if [ -f finaloutput_1 ]; then
   rm finaloutput_1
   echo "final output 1 is removed"
fi

if [ -f MemoryDetails0 ]; then
   rm MemoryDetails0
   echo "Memory Details0 are removed"
fi

if [ -f AverageMemoryDetails0 ]; then
   rm AverageMemoryDetails0
   echo "Average Memory Details0 are removed"
fi

if [ -f AverageMemory0 ]; then
   rm AverageMemory0
   echo "Average Memory Details0 are removed"
fi

if [ -f AverageTimeDetails0 ]; then
   rm AverageTimeDetails0
   echo "AverageTimeDetails0 is removed"
fi

if [ -f AverageTime0 ]; then
   rm AverageTime0
   echo "AverageTime0 is removed"
fi

if [ -f MemoryDetails1 ]; then
   rm MemoryDetails1
   echo "Memory Details1 are removed"
fi

if [ -f AverageMemoryDetails1 ]; then
   rm AverageMemoryDetails1
   echo "Average Memory Details1 are removed"
fi
if [ -f AverageMemory1 ]; then
   rm AverageMemory1
   echo "Average Memory Details1 are removed"
fi

if [ -f AverageTimeDetails1 ]; then
   rm AverageTimeDetails1
   echo "AverageTimeDetails1 is removed"
fi

if [ -f AverageTime1 ]; then
   rm AverageTime1
   echo "AverageTime1 is removed"
fi

fractional_bits=13

for((image_ids=2;image_ids<=4;image_ids++))
 do
#  sleep 30
 echo "image_ids X"$image_ids >> MemoryDetails0
 echo "image_ids X"$image_ids >> MemoryDetails1 

 #number of splits
 splits=2
 x=$((256/splits))

 for  (( m = 1; m <= $splits; m++ )) 
  do 
	layer_id=1
	input_config=" "
   #ip(i)-->ip1 , ip2 ,ip3 .. and so on 
	if [ $layer_id -eq 1 ];
	then
    	input_config="ip$image_ids"
	fi
	let l=$((m-1)) 
	let a=$(((m-1)*x+1)) 
	echo "a:$a" 
	let b=$((m*x)) 
	echo "b:$b"
	let r=$((l*x)) 

   #Layer 1
	$build_path/bin/tensor_gt_mul_split --my-id 0 --party 0,::1,7002 --party 1,::1,7000 --arithmetic-protocol beavy --boolean-protocol yao --fractional-bits 13 --config-file-input $input_config --config-file-model file_config_model0 --layer-id $layer_id --row_start $a --row_end $b --split $splits --current-path $build_path > $build_path/server0/debug_files/tensor_gt_mul0_layer1_split.txt &
   pid1=$!
   echo "$pid1: layer 1, id = 0"
   
   $build_path/bin/tensor_gt_mul_split --my-id 1 --party 0,::1,7002 --party 1,::1,7000 --arithmetic-protocol beavy --boolean-protocol yao --fractional-bits 13 --config-file-input $input_config --config-file-model file_config_model1 --layer-id $layer_id --row_start $a --row_end $b --split $splits --current-path $build_path  > $build_path/server1/debug_files/tensor_gt_mul1_layer1_split.txt &
   pid2=$!
   echo "$pid2:layer 1 id = 1"
   wait $pid1 $pid2 
   echo "layer 1 - gt mul is done"
   
    if [ $m -eq 1 ];then
      touch finaloutput_0
      touch finaloutput_1

      printf "$x 1\n" >> finaloutput_0
      printf "$x 1\n" >> finaloutput_1

      $build_path/bin/appendfile 
      pid1=$!
      echo "$pid1:append file"
      wait $pid1 
      echo "layer 1 - append file is done"
      
      else 
      
      $build_path/bin/appendfile 
      pid1=$!
      echo "$pid1:append file"
      wait $pid1 
      echo "layer 1 - append file is done"
    fi

		sed -i "1s/${r} 1/${b} 1/" finaloutput_0
		sed -i "1s/${r} 1/${b} 1/" finaloutput_1
      # sleep 10

 done

 cp finaloutput_0  $build_path/server0/outputshare_0 
 cp finaloutput_1  $build_path/server1/outputshare_1
#  sleep 40

$build_path/bin/tensor_gt_relu --my-id 0 --party 0,::1,7002 --party 1,::1,7000 --arithmetic-protocol beavy --boolean-protocol yao --fractional-bits 13 --filepath file_config_input0 --current-path $build_path > $build_path/server0/debug_files/tensor_relu0_layer1_split.txt &

pid1=$!
echo "$pid1:Layer 1 ReLU, Party 0"
$build_path/bin/tensor_gt_relu --my-id 1 --party 0,::1,7002 --party 1,::1,7001 --arithmetic-protocol beavy --boolean-protocol yao --fractional-bits 13 --filepath file_config_input1 --current-path $build_path > $build_path/server1/debug_files/tensor_relu1_layer1_split.txt &

pid2=$!
echo "$pid2:Layer 1 ReLU, Party 1"
wait $pid1 $pid2 
echo "layer 1 - relu is done"

 ((layer_id++))

 #Layer 2
 if [ $layer_id -gt 1 ];then
    input_config="outputshare"

 fi

$build_path/bin/tensor_gt_mul_test --my-id 0 --party 0,::1,7002 --party 1,::1,7000 --arithmetic-protocol beavy --boolean-protocol yao --fractional-bits $fractional_bits --config-file-input $input_config --config-file-model file_config_model0 --layer-id $layer_id --current-path $build_path > $build_path/server0/debug_files/tensor_gt_mul0_layer2_split.txt & 
pid1=$!
echo "$pid1:Layer 2, Party 0"
$build_path/bin/tensor_gt_mul_test --my-id 1 --party 0,::1,7002 --party 1,::1,7000 --arithmetic-protocol beavy --boolean-protocol yao --fractional-bits $fractional_bits --config-file-input $input_config --config-file-model file_config_model1 --layer-id $layer_id --current-path $build_path > $build_path/server1/debug_files/tensor_gt_mul1_layer2_split.txt &  
pid2=$!
echo "$pid2:Layer 2, Party 1"
wait $pid1 $pid2 
echo "layer 2 - gt mul is done"

$build_path/bin/argmax --my-id 0 --party 0,::1,7000 --party 1,::1,7001 --arithmetic-protocol beavy --boolean-protocol beavy --repetitions 1 --config-filename file_config_input0 --config-input X$image_ids --current-path $build_path  > $build_path/server0/debug_files/argmax0_layer2_split.txt &
pid1=$!
echo "$pid1:Layer 2 argmax, Party 0"
$build_path/bin/argmax --my-id 1 --party 0,::1,7000 --party 1,::1,7001 --arithmetic-protocol beavy --boolean-protocol beavy --repetitions 1 --config-filename file_config_input1 --config-input X$image_ids --current-path $build_path  > $build_path/server1/debug_files/argmax1_layer2_split.txt &

pid2=$!
echo "$pid2:Layer 2 argmax, Party 1"
wait $pid1 $pid2 
echo "layer 2 - argmax is done"

$build_path/bin/output_shares_receiver --my-id 0 --listening-port 1234 > $build_path/server0/debug_files/output_shares_receiver0_split.txt &
pid1=$!
echo "$pid1:output shares receiver, Party 0"
$build_path/bin/output_shares_receiver --my-id 1 --listening-port 1235 > $build_path/server1/debug_files/output_shares_receiver1_split.txt &
pid2=$!
echo "$pid2:output shares receiver, Party 1"

sleep 1

$build_path/bin/final_output_provider --my-id 0 --connection-port 1234 --config-input X$image_ids > $build_path/server0/debug_files/final_output_provider0_split.txt &
pid3=$!
echo "$pid3:final output provider, Party 0"
$build_path/bin/final_output_provider --my-id 1 --connection-port 1235 --config-input X$image_ids > $build_path/server1/debug_files/final_output_provider1_split.txt &
pid4=$!
echo "$pid4:final output provider, Party 1"
wait $pid1 $pid2 $pid3 $pid4
wait

 if [ -f finaloutput_0 ];then
   rm finaloutput_0
   echo "final output 0 is removed"
 fi
 if [ -f finaloutput_1 ]; then
   rm finaloutput_1
   echo "final output 1 is removed"
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

awk '{ sum += $1 } END { print sum}' AverageTime0 >> AverageTime0
tail -1 AverageTime0 | tee AverageTime0
awk '{ sum += $1 } END { print sum}' AverageTime1 >> AverageTime1
tail -1 AverageTime1 | tee AverageTime1


awk '{ sum += $1 } END { print sum}' AverageMemory0 >> AverageMemory0
tail -1 AverageMemory0 | tee AverageMemory0
awk '{ sum += $1 } END { print sum}' AverageMemory1 >> AverageMemory1
tail -1 AverageMemory1 | tee AverageMemory1  
echo "Finished"

 $SHELL


