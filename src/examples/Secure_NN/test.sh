#!/bin/bash

logfile0=${BASE_DIR}/src/examples/Secure_NN/log/logfile0
logfile1=${BASE_DIR}/src/examples/Secure_NN/log/logfile1
logfile2=${BASE_DIR}/src/examples/Secure_NN/log/logfile2
start=`date +%s.%N`

~/3Party_Relu/build_debwithrelinfo_gcc/bin/3PC_Relu2 --party 0,127.0.0.1,4009 --party 1,127.0.0.1,4010 --helper_node 127.0.0.1,4011 --current-path ${BASE_DIR}/build_debwithrelinfo_gcc > $logfile2 & ~/3Party_Relu/build_debwithrelinfo_gcc/bin/3PC_Relu0 --party 0,127.0.0.1,4009 --party 1,127.0.0.1,4010 --helper_node 127.0.0.1,4011 --current-path ${BASE_DIR}/build_debwithrelinfo_gcc > $logfile0 & ~/3Party_Relu/build_debwithrelinfo_gcc/bin/3PC_Relu1 --party 0,127.0.0.1,4009 --party 1,127.0.0.1,4010 --helper_node 127.0.0.1,4011 --current-path ${BASE_DIR}/build_debwithrelinfo_gcc > $logfile1

wait

end=`date +%s.%N`
runtime=$(echo "$end - $start" | bc -l)


echo "This is the elapsed time: " $runtime