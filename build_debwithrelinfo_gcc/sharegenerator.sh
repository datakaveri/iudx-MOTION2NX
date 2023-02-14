#! /bin/bash

DIR_MOTION2NX=$HOME/iudx-MOTION2NX-new
DIR_SMPC=$HOME/smpc-compute-server-comms-new
pwd
DEFAULT_TABS_TITLE1="T1"
DEFAULT_TABS_TITLE2="T2"

DEFAULT_TABS_CMD1="./bin/Weights_Share_Generator --my-id 0 --party 0,::1,7003 --party 1,::1,7000 --arithmetic-protocol beavy --boolean-protocol yao --fractional-bits 13 --file-names file_config_model --file-names-image file_config_input"
DEFAULT_TABS_CMD2="./bin/Weights_Share_Generator --my-id 1 --party 0,::1,7003 --party 1,::1,7000 --arithmetic-protocol beavy --boolean-protocol yao --fractional-bits 13 --file-names file_config_model --file-names-image file_config_input"
 gnome-terminal --tab -- bash -ic "export TITLE_DEFAULT='$DEFAULT_TABS_TITLE1'; $DEFAULT_TABS_CMD1; exec bash;"
 gnome-terminal --tab -- bash -ic "export TITLE_DEFAULT='$DEFAULT_TABS_TITLE2'; $DEFAULT_TABS_CMD2; exec bash;"


DEFAULT_TABS_TITLE3="T3"

cd $DIR_SMPC/build_

DEFAULT_TABS_CMD3="./weights_provider --compute-server0-port 1234 --compute-server1-port 1235 --dp-id 0 --fractional-bits 13"
 gnome-terminal --tab -- bash -ic "export TITLE_DEFAULT='$DEFAULT_TABS_TITLE3'; $DEFAULT_TABS_CMD3; exec bash;"

cd $DIR_MOTION2NX/build_debwithrelinfo_gcc

# # --------------------------------------------------------------------------------------------------------------------------------------------------------------
sleep 60
for i in {1..10}

do

# i=35

DEFAULT_TABS_CMD1="./bin/Image_Share_Generator --my-id 0 --party 0,::1,7003 --party 1,::1,7000 --arithmetic-protocol beavy --boolean-protocol yao --fractional-bits 13 --file-names file_config_input --index $i"
DEFAULT_TABS_CMD2="./bin/Image_Share_Generator --my-id 1 --party 0,::1,7003 --party 1,::1,7000 --arithmetic-protocol beavy --boolean-protocol yao --fractional-bits 13 --file-names file_config_input --index $i"

 gnome-terminal --tab -- bash -ic "export TITLE_DEFAULT='$DEFAULT_TABS_TITLE1'; $DEFAULT_TABS_CMD1; exec bash;"
 gnome-terminal --tab -- bash -ic "export TITLE_DEFAULT='$DEFAULT_TABS_TITLE2'; $DEFAULT_TABS_CMD2; exec bash;"

DEFAULT_TABS_TITLE3="T3"

cd $DIR_SMPC/build_

DEFAULT_TABS_CMD3="./image_provider_iudx --compute-server0-port 1234 --compute-server1-port 1235 --fractional-bits 13 --NameofImageFile X$i"
# DEFAULT_TABS_CMD3="./image_provider --compute-server0-port 1234 --compute-server1-port 1235 --fractional-bits 13 "
gnome-terminal --tab -- bash -ic "export TITLE_DEFAULT='$DEFAULT_TABS_TITLE3'; $DEFAULT_TABS_CMD3; exec bash;"

cd $DIR_MOTION2NX/build_debwithrelinfo_gcc

sleep 10

done
$SHELL
