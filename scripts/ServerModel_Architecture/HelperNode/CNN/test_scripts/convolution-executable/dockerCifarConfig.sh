#!/bin/bash

build_path=${BASE_DIR}/build_debwithrelinfo_gcc

# Remove files starting with 'W' in server0
if ls ${build_path}/server0/W* 1> /dev/null 2>&1; then
    rm ${build_path}/server0/W*
fi

# Remove files starting with 'W' in server1
if ls ${build_path}/server1/W* 1> /dev/null 2>&1; then
    rm ${build_path}/server1/W*
fi

# Remove files starting with 'B' in server0
if ls ${build_path}/server0/B[1-6]* 1> /dev/null 2>&1; then
    rm ${build_path}/server0/B[1-6]*
fi

# Remove files starting with 'B' in server1
if ls ${build_path}/server1/B[1-6]* 1> /dev/null 2>&1; then
    rm ${build_path}/server1/B[1-6]*
fi

# Update smpc-helpernode-config.json
echo '{
    "cs0_host": "20.197.27.117",
    "cs1_host": "13.200.145.195",
    "helpernode_host": "13.234.59.235",
    "reverse_ssh_host": "52.172.238.250”,
    "cs0_dns_resolve": false,
    "cs1_dns_resolve": false,
    "helpernode_dns_resolve": false,
    "reverse_ssh_dns_resolve": false,
    "cs0_port_model_receiver": 4016,
    "cs1_port_model_receiver": 4017,
    "cs0_port_cs0_output_receiver": 4007,
    "cs0_port_cs1_output_receiver": 4008,
    "cs0_port_inference": 4020,
    "cs1_port_inference":4021,
    "helpernode_port_inference":4022,
    "relu0_port_inference": 4023,
    "relu1_port_inference": 4024,
    "cs0_port_image_receiver": 4025,
    "cs1_port_image_receiver": 4026,
    "number_of_layers": 6,
    "fractional_bits": 13,
    "image_id": 1,
    "image_rows": 32,
    "channels": 3,
    "layer_types": [1,1,1,1,0,0]
}' > ${BASE_DIR}/config_files/example-smpc-helpernode-config.json

# Update image_config.json
echo '{
    "cs0_host": "20.197.27.117",
    "cs1_host": "13.234.59.235",
    "cs0_dns_resolve": false,
    "cs1_dns_resolve": false,
    "cs0_port_cs0_output_receiver": 4007,
    "cs0_port_cs1_output_receiver": 4008,
    "cs0_port_image_receiver": 4025,
    "cs1_port_image_receiver": 4026,
    "fractional_bits": 13,
    "image_id": 1,
    "image_rows": 32,
    "image_cols": 32,
    "channels": 3
}' > ${BASE_DIR}/config_files/example-image_config.json

# Update model_helpernode_config.json
echo '{
    "no_of_layers" : 6,
    "Layers" : {
        "1" : { 
            "Weights" : {"type": "CN", "kernels": 32, "channels": 3, "rows" : 3, "columns" : 3, "pads": [1,1,1,1], "strides": [1,1], "file_name" : "CIFAR10/W1.csv"}, 
            "Bias" : {"rows" : 32, "columns" : 1, "file_name" : "CIFAR10/B1.csv"}},
        "2" : {
            "Weights" : {"type": "CN", "kernels": 32, "channels": 32, "rows" : 5, "columns" : 5, "pads": [1,1,1,1], "strides": [2,2], "file_name" : "CIFAR10/W2.csv"}, 
            "Bias" : {"rows" : 32, "columns" : 1, "file_name" : "CIFAR10/B2.csv"}},
        "3" : {
            "Weights" : {"type": "CN", "kernels": 64, "channels": 32, "rows" : 5, "columns" : 5, "pads": [2,2,2,2], "strides": [2,2], "file_name" : "CIFAR10/W3.csv"}, 
            "Bias" : {"rows" : 64, "columns" : 1, "file_name" : "CIFAR10/B3.csv"}},
        "4" : {
            "Weights" : {"type": "CN", "kernels": 64, "channels": 64, "rows" : 3, "columns" : 3, "pads": [0,0,0,0], "strides": [1,1], "file_name" : "CIFAR10/W4.csv"}, 
            "Bias" : {"rows" : 64, "columns" : 1, "file_name" : "CIFAR10/B4.csv"}},
        "5" : {
            "Weights" : {"rows" : 512, "columns" : 2304, "file_name" : "CIFAR10/W5.csv"}, 
            "Bias" : {"rows" : 512, "columns" : 1, "file_name" : "CIFAR10/B5.csv"}},
        "6" : {
            "Weights" : {"rows" : 10, "columns" : 512, "file_name" : "CIFAR10/W6.csv"}, 
            "Bias" : {"rows" : 10, "columns" : 1, "file_name" : "CIFAR10/B6.csv"}}
    }
}' > ${BASE_DIR}/config_files/example-model_helpernode_config.json
