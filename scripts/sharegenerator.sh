#! /bin/bash
abs_path=${BASE_DIR}/config_files/file_config_input

abs_path2=${BASE_DIR}/config_files/file_config_model

build_path=${BASE_DIR}/build_debwithrelinfo_gcc

build_path_model=${BASE_DIR}/Dataprovider/weights_provider

image_path=${BASE_DIR}/Dataprovider/image_provider

pwd
# DEFAULT_TABS_TITLE1="T1"
#  DEFAULT_TABS_TITLE2="T2"

j=13

DEFAULT_TABS_CMD1="$build_path/bin/Weights_Share_Generator --my-id 0 --party 0,::1,7003 --party 1,::1,7000 --arithmetic-protocol beavy --boolean-protocol yao --fractional-bits $j --file-names $abs_path2 --current-path $build_path"
DEFAULT_TABS_CMD2="$build_path/bin/Weights_Share_Generator --my-id 1 --party 0,::1,7003 --party 1,::1,7000 --arithmetic-protocol beavy --boolean-protocol yao --fractional-bits $j --file-names $abs_path2 --current-path $build_path"
gnome-terminal --tab -- bash -ic "export TITLE_DEFAULT='$DEFAULT_TABS_TITLE1'; $DEFAULT_TABS_CMD1; exec bash;"
gnome-terminal --tab -- bash -ic "export TITLE_DEFAULT='$DEFAULT_TABS_TITLE2'; $DEFAULT_TABS_CMD2; exec bash;"


DEFAULT_TABS_TITLE3="T3"

cd $build_path


DEFAULT_TABS_CMD3="$build_path/bin/weights_provider --compute-server0-port 1234 --compute-server1-port 1235 --dp-id 0 --fractional-bits $j --filepath $build_path_model"
 gnome-terminal --tab -- bash -ic "export TITLE_DEFAULT='$DEFAULT_TABS_TITLE3'; $DEFAULT_TABS_CMD3; exec bash;"

cd $build_path

# # --------------------------------------------------------------------------------------------------------------------------------------------------------------
sleep 60
# for i in {1..2}

# list=(241 257 290 305 380 381 412 435 478 479 522 543 674 689 760 834 844 874 877 885 939 1000)
list=(1 2)

for i in "${list[@]}"

do


DEFAULT_TABS_CMD1="$build_path/bin/Image_Share_Generator --my-id 0 --party 0,::1,7003 --party 1,::1,7000 --arithmetic-protocol beavy --boolean-protocol yao --fractional-bits $j --file-names $abs_path --index $i --current-path $build_path"
DEFAULT_TABS_CMD2="$build_path/bin/Image_Share_Generator --my-id 1 --party 0,::1,7003 --party 1,::1,7000 --arithmetic-protocol beavy --boolean-protocol yao --fractional-bits $j --file-names $abs_path --index $i --current-path $build_path"

 gnome-terminal --tab -- bash -ic "export TITLE_DEFAULT='$DEFAULT_TABS_TITLE1'; $DEFAULT_TABS_CMD1; exec bash;"
 gnome-terminal --tab -- bash -ic "export TITLE_DEFAULT='$DEFAULT_TABS_TITLE2'; $DEFAULT_TABS_CMD2; exec bash;"

DEFAULT_TABS_TITLE3="T3"

cd $build_path

DEFAULT_TABS_CMD3="$build_path/bin/image_provider_iudx --compute-server0-port 1234 --compute-server1-port 1235 --fractional-bits $j --NameofImageFile X$i --filepath $image_path "
# DEFAULT_TABS_CMD3="./image_provider --compute-server0-port 1234 --compute-server1-port 1235 --fractional-bits 13 "
gnome-terminal --tab -- bash -ic "export TITLE_DEFAULT='$DEFAULT_TABS_TITLE3'; $DEFAULT_TABS_CMD3; exec bash;"

cd $build_path

sleep 10

done
$SHELL
