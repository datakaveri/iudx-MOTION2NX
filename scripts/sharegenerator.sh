#! /bin/bash
image_config=${BASE_DIR}/config_files/file_config_input
model_config=${BASE_DIR}/config_files/file_config_model
build_path=${BASE_DIR}/build_debwithrelinfo_gcc
build_path_model=${BASE_DIR}/Dataprovider/weights_provider
image_path=${BASE_DIR}/Dataprovider/image_provider

fractional_bits=13
pwd
DEFAULT_TABS_CMD1="$build_path/bin/Weights_Share_Reciever --my-id 0 --file-names $model_config --current-path $build_path"
DEFAULT_TABS_CMD2="$build_path/bin/Weights_Share_Reciever --my-id 1 --file-names $model_config --current-path $build_path"
gnome-terminal --tab -- bash -ic "export TITLE_DEFAULT='$DEFAULT_TABS_TITLE1'; $DEFAULT_TABS_CMD1; exec bash;"
gnome-terminal --tab -- bash -ic "export TITLE_DEFAULT='$DEFAULT_TABS_TITLE2'; $DEFAULT_TABS_CMD2; exec bash;"

DEFAULT_TABS_TITLE3="T3"

DEFAULT_TABS_CMD3="$build_path/bin/weights_provider --compute-server0-port 1234 --compute-server1-port 1235 --dp-id 0 --fractional-bits $j --filepath $build_path_model"
gnome-terminal --tab -- bash -ic "export TITLE_DEFAULT='$DEFAULT_TABS_TITLE3'; $DEFAULT_TABS_CMD3; exec bash;"

sleep 60

image_ids=(0 1)

for i in "${image_ids[@]}"
do
DEFAULT_TABS_CMD1="$build_path/bin/Image_Share_Reciever --my-id 0 --fractional-bits $fractional_bits --file-names $image_config --index $i --current-path $build_path"
DEFAULT_TABS_CMD2="$build_path/bin/Image_Share_Reciever --my-id 1 --fractional-bits $fractional_bits --file-names $image_config --index $i --current-path $build_path"
gnome-terminal --tab -- bash -ic "export TITLE_DEFAULT='$DEFAULT_TABS_TITLE1'; $DEFAULT_TABS_CMD1; exec bash;"
gnome-terminal --tab -- bash -ic "export TITLE_DEFAULT='$DEFAULT_TABS_TITLE2'; $DEFAULT_TABS_CMD2; exec bash;"

DEFAULT_TABS_TITLE3="T3"
DEFAULT_TABS_CMD3="$build_path/bin/image_provider_iudx --compute-server0-port 1234 --compute-server1-port 1235 --fractional-bits $j --NameofImageFile X$i --filepath $image_path "
# DEFAULT_TABS_CMD3="./image_provider --compute-server0-port 1234 --compute-server1-port 1235 --fractional-bits 13 "
gnome-terminal --tab -- bash -ic "export TITLE_DEFAULT='$DEFAULT_TABS_TITLE3'; $DEFAULT_TABS_CMD3; exec bash;"

sleep 10
done
$SHELL