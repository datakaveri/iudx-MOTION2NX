#! /bin/bash

build_path=${BASE_DIR}/build_debwithrelinfo_gcc
image_provider_path=${BASE_DIR}/Dataprovider/image_provider/Final_Output_Shares
scripts_path=${BASE_DIR}/scripts
fractional_bits=13

ip0=192.168.1.125

ip1=192.168.1.141

# ip0=127.0.0.1

# ip1=127.0.0.1

port0=3390

port1=4567

debug_1=$build_path/server1/debug_files

if [ ! -d "$debug_1" ];
then
	mkdir $debug_1
fi

cd $build_path


if [ -f finaloutput_1 ]; then
   rm finaloutput_1
   # echo "final output 1 is removed"
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


image_no=(21)
for image_ids in "${image_no[@]}"
# for((image_ids=1;image_ids<=5;image_ids++))
do
 echo 

 echo "image_ids X"$image_ids >> MemoryDetails1 
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
   $build_path/bin/tensor_gt_mul_split --my-id 1 --party 0,$ip0,$port0 --party 1,$ip1,$port1 --arithmetic-protocol beavy --boolean-protocol yao --fractional-bits 13 --config-file-input $input_config --config-file-model file_config_model1 --layer-id $layer_id --row_start $a --row_end $b --split $splits --current-path $build_path  > $build_path/server1/debug_files/tensor_gt_mul1_layer1_split.txt &
   pid1=$!
   wait $pid1  
   echo "Layer 1, split $m - multiplication is done"
   
   if [ $m -eq 1 ];then

      touch finaloutput_1

      printf "$x 1\n" >> finaloutput_1

      $build_path/bin/appendfile 1
      pid1=$!
      wait $pid1 
      
      else 
      
      $build_path/bin/appendfile 1
      pid1=$!
      wait $pid1 
    fi

		sed -i "1s/${r} 1/${b} 1/" finaloutput_1
 done

 cp finaloutput_1  $build_path/server1/outputshare_1

$build_path/bin/tensor_gt_relu --my-id 1 --party 0,$ip0,$port0 --party 1,$ip1,$port1 --arithmetic-protocol beavy --boolean-protocol yao --fractional-bits 13 --filepath file_config_input1 --current-path $build_path > $build_path/server1/debug_files/tensor_relu1_layer1_split.txt &
pid1=$!

wait $pid1 
echo "Layer 1 - ReLU is done"

((layer_id++))

#Layer 2
if [ $layer_id -gt 1 ];then
   input_config="outputshare"
fi

$build_path/bin/tensor_gt_mul_test --my-id 1 --party 0,$ip0,$port0 --party 1,$ip1,$port1 --arithmetic-protocol beavy --boolean-protocol yao --fractional-bits $fractional_bits --config-file-input $input_config --config-file-model file_config_model1 --layer-id $layer_id --current-path $build_path > $build_path/server1/debug_files/tensor_gt_mul1_layer2_split.txt &  
pid1=$!

wait $pid1 
echo "Layer 2 - multiplication is done"


$build_path/bin/argmax --my-id 1 --party 0,$ip0,$port0 --party 1,$ip1,$port1 --arithmetic-protocol beavy --boolean-protocol beavy --repetitions 1 --config-filename file_config_input1 --config-input X$image_ids --current-path $build_path  > $build_path/server1/debug_files/argmax1_layer2_split.txt &
pid2=$!

wait $pid1 $pid2 
echo "Layer 2 - ArgMax is done"

$build_path/bin/final_output_provider --my-id 1 --connection-port 2233 --connection-ip $ip0 --config-input X$image_ids --current-path $build_path > $build_path/server1/debug_files/final_output_provider1_split.txt 
pid1=$!

wait $pid1

 if [ -f finaloutput_1 ]; then
   rm finaloutput_1
   # echo "final output 1 is removed"
 fi

 awk '{ sum += $1 } END { print sum }' AverageTimeDetails1 >> AverageTime1
 > AverageTimeDetails1 #clearing the contents of the file
 sort -r -n AverageMemoryDetails1 | head  -1 >> AverageMemory1
 > AverageMemoryDetails1 #clearing the contents of the file

done 

echo -e "\nInferencing Finished"

awk '{ sum += $1 } END { print sum}' AverageTime1 >> AverageTime1

awk '{ sum += $1 } END { print sum}' AverageMemory1 >> AverageMemory1
cd $scripts_path
$SHELL