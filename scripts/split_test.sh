#! /bin/bash

pwd
DEFAULT_TABS_TITLE1="T1"
DEFAULT_TABS_TITLE2="T2"

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


DEFAULT_TABS_CMD0="$build_path/bin/appendfile"
fractional_bits=13

for((image_ids=1;image_ids<=20;image_ids++))

# image_ids=(0)

 do

 sleep 30

 echo "image_ids X"$image_ids >> MemoryDetails0
 echo "image_ids X"$image_ids >> MemoryDetails1 


 #number of splits
 splits=1

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
	DEFAULT_TABS_CMD1="./bin/tensor_gt_mul_split --my-id 0 --party 0,::1,7002 --party 1,::1,7000 --arithmetic-protocol beavy --boolean-protocol yao --fractional-bits 13 --config-file-input $input_config --config-file-model file_config_model0 --layer-id $layer_id --row_start $a --row_end $b --split $splits --current-path $build_path"
	DEFAULT_TABS_CMD2="./bin/tensor_gt_mul_split --my-id 1 --party 0,::1,7002 --party 1,::1,7000 --arithmetic-protocol beavy --boolean-protocol yao --fractional-bits 13 --config-file-input $input_config --config-file-model file_config_model1 --layer-id $layer_id --row_start $a --row_end $b --split $splits --current-path $build_path" 

	 gnome-terminal --tab -- bash -ic "export TITLE_DEFAULT='$DEFAULT_TABS_TITLE1';$DEFAULT_TABS_CMD1;exec bash;exit;"
    gnome-terminal --tab -- bash -ic "export TITLE_DEFAULT='$DEFAULT_TABS_TITLE2';$DEFAULT_TABS_CMD2;exec bash;exit;"
    sleep 90

    if [ $m -eq 1 ];then
      
      # cd $build_path

      touch finaloutput_0
      touch finaloutput_1

      printf "$x 1\n" >> finaloutput_0
      printf "$x 1\n" >> finaloutput_1

      gnome-terminal --tab -- bash -ic "export TITLE_DEFAULT='$DEFAULT_TABS_TITLE1';$DEFAULT_TABS_CMD0;exec bash;exit;"
      sleep 40
      
      else 

      gnome-terminal --tab -- bash -ic "export TITLE_DEFAULT='$DEFAULT_TABS_TITLE1';$DEFAULT_TABS_CMD0;exec bash;exit;"
      sleep 40
    fi

		sed -i "1s/${r} 1/${b} 1/" finaloutput_0
		sed -i "1s/${r} 1/${b} 1/" finaloutput_1
      sleep 10

 done

 cp finaloutput_0  $build_path/server0/outputshare_0 
 cp finaloutput_1  $build_path/server1/outputshare_1
 sleep 40

 DEFAULT_TABS_CMD3="./bin/tensor_gt_relu --my-id 0 --party 0,::1,7002 --party 1,::1,7000 --arithmetic-protocol beavy --boolean-protocol yao --fractional-bits 13 --filepath file_config_input0 --current-path $build_path"
 DEFAULT_TABS_CMD4="./bin/tensor_gt_relu --my-id 1 --party 0,::1,7002 --party 1,::1,7001 --arithmetic-protocol beavy --boolean-protocol yao --fractional-bits 13 --filepath file_config_input1 --current-path $build_path"



 ((layer_id++))


 #Layer 2
 if [ $layer_id -gt 1 ];then
    input_config="outputshare"

 fi

 sleep 20

 DEFAULT_TABS_CMD5="./bin/tensor_gt_mul_test --my-id 0 --party 0,::1,7002 --party 1,::1,7000 --arithmetic-protocol beavy --boolean-protocol yao --fractional-bits $fractional_bits --config-file-input $input_config --config-file-model file_config_model0 --layer-id $layer_id --current-path $build_path"
 DEFAULT_TABS_CMD6="./bin/tensor_gt_mul_test --my-id 1 --party 0,::1,7002 --party 1,::1,7000 --arithmetic-protocol beavy --boolean-protocol yao --fractional-bits $fractional_bits --config-file-input $input_config --config-file-model file_config_model1 --layer-id $layer_id --current-path $build_path"
 DEFAULT_TABS_CMD7="./bin/argmax --my-id 0 --party 0,::1,7000 --party 1,::1,7001 --arithmetic-protocol beavy --boolean-protocol beavy --repetitions 1 --config-filename file_config_input0 --config-input X$image_ids --current-path $build_path"
 DEFAULT_TABS_CMD8="./bin/argmax --my-id 1 --party 0,::1,7000 --party 1,::1,7001 --arithmetic-protocol beavy --boolean-protocol beavy --repetitions 1 --config-filename file_config_input1 --config-input X$image_ids --current-path $build_path"

 gnome-terminal --tab -- bash -ic "export TITLE_DEFAULT='$DEFAULT_TABS_TITLE1';$DEFAULT_TABS_CMD3;$DEFAULT_TABS_CMD5;$DEFAULT_TABS_CMD7; exec bash;exit;"
 gnome-terminal --tab -- bash -ic "export TITLE_DEFAULT='$DEFAULT_TABS_TITLE2';$DEFAULT_TABS_CMD4;$DEFAULT_TABS_CMD6;$DEFAULT_TABS_CMD8; exec bash;exit;"

 sleep 60


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
sleep 10

awk '{ sum += $1 } END { print sum}' AverageTime0 >> AverageTime0
tail -1 AverageTime0 | tee AverageTime0
awk '{ sum += $1 } END { print sum}' AverageTime1 >> AverageTime1
tail -1 AverageTime1 | tee AverageTime1


awk '{ sum += $1 } END { print sum}' AverageMemory0 >> AverageMemory0
tail -1 AverageMemory0 | tee AverageMemory0
awk '{ sum += $1 } END { print sum}' AverageMemory1 >> AverageMemory1
tail -1 AverageMemory1 | tee AverageMemory1  

 $SHELL