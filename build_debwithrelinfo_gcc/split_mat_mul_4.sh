#! /bin/bash

pwd
DEFAULT_TABS_TITLE1="T1"
DEFAULT_TABS_TITLE2="T2"

#Delete the old count file if any. Write a new count file with 0 as its data
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



for((example=1;example<=9;example++))



do

sleep 30

echo "Example X"$example >> MemoryDetails0
echo "Example X"$example >> MemoryDetails1 

# for((p=1;p<=8;p*=2))

# do 
p=1

x=$((256/p))
echo $p
echo $x



for  (( m = 1; m <= 1; m++ )) 

do 

	layer_id=1

	k=" "

#ip(i)-->ip1 , ip2 ,ip3 .. and so on 
	if [ $layer_id -eq 1 ];
	then
    		k="ip$example"

	fi

#prints k 
	# echo $k
	let l=$((m-1))
	let a=$(((m-1)*x+1))
	echo "a:"
	echo $a
	let b=$((m*x))
	echo "b:"
	echo $b
	let r=$((l*x))


#layer_id =1 and k=ip1/ip2
	DEFAULT_TABS_CMD1="./bin/tensor_gt_mul_split --my-id 0 --party 0,::1,7002 --party 1,::1,7000 --arithmetic-protocol beavy --boolean-protocol yao --fractional-bits 13 --config-file-input $k --config-file-model file_config_model0 --layer-id $layer_id --row_start $a --row_end $b --split $p"
	DEFAULT_TABS_CMD2="./bin/tensor_gt_mul_split --my-id 1 --party 0,::1,7002 --party 1,::1,7000 --arithmetic-protocol beavy --boolean-protocol yao --fractional-bits 13 --config-file-input $k --config-file-model file_config_model1 --layer-id $layer_id --row_start $a --row_end $b --split $p" 

	gnome-terminal --tab -- bash -ic "export TITLE_DEFAULT='$DEFAULT_TABS_TITLE1';$DEFAULT_TABS_CMD1;exec bash;exit;"
    gnome-terminal --tab -- bash -ic "export TITLE_DEFAULT='$DEFAULT_TABS_TITLE2';$DEFAULT_TABS_CMD2;exec bash;exit;"

   sleep 50
	if [ $m -eq 1 ]; then
      
      touch finaloutput_0
      touch finaloutput_1
		# cp ~/Desktop/IUDX/iudx-MOTION2NX/build_debwithrelinfo_gcc/server0/outputshare_0  finaloutput_0
		# cp ~/Desktop/IUDX/iudx-MOTION2NX/build_debwithrelinfo_gcc/server1/outputshare_1  finaloutput_1
      printf "$x 1\n" >> finaloutput_0
      printf "$x 1\n" >> finaloutput_1
      DEFAULT_TABS_CMD42="./bin/appendfile"
      gnome-terminal --tab -- bash -ic "export TITLE_DEFAULT='$DEFAULT_TABS_TITLE1';$DEFAULT_TABS_CMD42;exec bash;exit;"


      sleep 40
	else 

		# tail -n $x /home/srishti04/Desktop/IUDX/iudx-MOTION2NX/build_debwithrelinfo_gcc/server0/outputshare_0 >> finaloutput_0

		# tail -n $x /home/srishti04/Desktop/IUDX/iudx-MOTION2NX/build_debwithrelinfo_gcc/server1/outputshare_1 >> finaloutput_1
    
      DEFAULT_TABS_CMD42="./bin/appendfile"
      gnome-terminal --tab -- bash -ic "export TITLE_DEFAULT='$DEFAULT_TABS_TITLE1';$DEFAULT_TABS_CMD42;exec bash;exit;"
      
      sleep 40
      fi

		sed -i "1s/${r} 1/${b} 1/" finaloutput_0
		sed -i "1s/${r} 1/${b} 1/" finaloutput_1


   # cat finaloutput_0
   # cat finaloutput_1

   # grep -vn "^[[:alnum:]]*$" ~/Desktop/IUDX/iudx-MOTION2NX/build_debwithrelinfo_gcc/server0/outputshare_0 
   # grep -vn "^[[:alnum:]]*$" ~/Desktop/IUDX/iudx-MOTION2NX/build_debwithrelinfo_gcc/server1/outputshare_1

   sleep 10

done

cp finaloutput_0  ~/Desktop/IUDX/iudx-MOTION2NX/build_debwithrelinfo_gcc/server0/outputshare_0 
cp finaloutput_1  ~/Desktop/IUDX/iudx-MOTION2NX/build_debwithrelinfo_gcc/server1/outputshare_1

# cat ~/Desktop/IUDX/iudx-MOTION2NX/build_debwithrelinfo_gcc/server0/outputshare_0 
# cat ~/Desktop/IUDX/iudx-MOTION2NX/build_debwithrelinfo_gcc/server1/outputshare_1
 

# cat /home/srishti04/Desktop/IUDX/iudx-MOTION2NX/build_debwithrelinfo_gcc/server0/outputshare_0 
# cat /home/srishti04/Desktop/IUDX/iudx-MOTION2NX/build_debwithrelinfo_gcc/server1/outputshare_1

# sleep 30

# DEFAULT_TABS_CMD22="./bin/readfile"
# gnome-terminal --tab -- bash -ic "export TITLE_DEFAULT='$DEFAULT_TABS_TITLE1';$DEFAULT_TABS_CMD22;exec bash;exit;"

sleep 40

DEFAULT_TABS_CMD3="./bin/tensor_gt_relu --my-id 0 --party 0,::1,7002 --party 1,::1,7000 --arithmetic-protocol beavy --boolean-protocol yao --fractional-bits 13 --filepath file_config_input0"
DEFAULT_TABS_CMD4="./bin/tensor_gt_relu --my-id 1 --party 0,::1,7002 --party 1,::1,7001 --arithmetic-protocol beavy --boolean-protocol yao --fractional-bits 13 --filepath file_config_input1"



((layer_id++))


#k=file_config_input for layer 2 
if [ $layer_id -gt 1 ];
then
    k="outputshare"

fi

sleep 20

# #layer_id =2 and k=file_config_input ... 0 and 1 is appended in tensor_gt_mul_test
DEFAULT_TABS_CMD5="./bin/tensor_gt_mul_test --my-id 0 --party 0,::1,7002 --party 1,::1,7000 --arithmetic-protocol beavy --boolean-protocol yao --fractional-bits 13 --config-file-input $k --config-file-model file_config_model0 --layer-id $layer_id"
DEFAULT_TABS_CMD6="./bin/tensor_gt_mul_test --my-id 1 --party 0,::1,7002 --party 1,::1,7000 --arithmetic-protocol beavy --boolean-protocol yao --fractional-bits 13 --config-file-input $k --config-file-model file_config_model1 --layer-id $layer_id"


DEFAULT_TABS_CMD7="./bin/argmax --my-id 0 --party 0,::1,7000 --party 1,::1,7001 --arithmetic-protocol beavy --boolean-protocol yao --repetitions 1 --config-filename file_config_input0 --config-input X$example"
DEFAULT_TABS_CMD8="./bin/argmax --my-id 1 --party 0,::1,7000 --party 1,::1,7001 --arithmetic-protocol beavy --boolean-protocol yao --repetitions 1 --config-filename file_config_input1 --config-input X$example"


gnome-terminal --tab -- bash -ic "export TITLE_DEFAULT='$DEFAULT_TABS_TITLE1';$DEFAULT_TABS_CMD3;$DEFAULT_TABS_CMD5;$DEFAULT_TABS_CMD7; exec bash;exit;"
gnome-terminal --tab -- bash -ic "export TITLE_DEFAULT='$DEFAULT_TABS_TITLE2';$DEFAULT_TABS_CMD4;$DEFAULT_TABS_CMD6;$DEFAULT_TABS_CMD8; exec bash;exit;"

sleep 60

   if [ -f finaloutput_0 ]; then

   rm finaloutput_0

   echo "final output 0 is removed"

fi

if [ -f finaloutput_1 ]; then

   rm finaloutput_1

   echo "final output 1 is removed"

fi





awk '{ sum += $1 } END { print sum }' AverageTimeDetails0 >> AverageTime0
> AverageTimeDetails0


awk '{ sum += $1 } END { print sum }' AverageTimeDetails1 >> AverageTime1
> AverageTimeDetails1


sort -r -n AverageMemoryDetails0 | head  -1 >> AverageMemory0
> AverageTimeDetails1
sort -r -n AverageMemoryDetails1 | head  -1 >> AverageMemory1
> AverageTimeDetails1

done 
sleep 10



awk '{ sum += $1 } END { print sum/9}' AverageTime0 >> AverageTime0
tail -1 AverageTime0 | tee AverageTime0
awk '{ sum += $1 } END { print sum/9}' AverageTime1 >> AverageTime1
tail -1 AverageTime1 | tee AverageTime1


awk '{ sum += $1 } END { print sum/9}' AverageMemory0 >> AverageMemory0
tail -1 AverageMemory0 | tee AverageMemory0
awk '{ sum += $1 } END { print sum/9}' AverageMemory1 >> AverageMemory1
tail -1 AverageMemory1 | tee AverageMemory1     

$SHELL