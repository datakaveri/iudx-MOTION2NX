Non-ServerModel_Architecture folder
-----------------------------------
The Non-Server model considers that each data provider operates a compute server. Hence, the dataprovider scripts and compute server scripts are combined in this architecture.

Run docker containers in cloud for each of the server/provider, using the corresponding docker compose files. The service name for server0/image provider is smpc-server0, and the service name for server1/model provider is smpc-server1. Service name for the helpernode is smpc-server2.
For Eg. 
docker compose -f docker-compose.remote_split.yaml -f ../../docker-compose.remote-registry.yaml up -d smpc-server0

1) Split folder: Contains scripts for the split version for NN Inferencing.
2) HelperNode folder: Contains scripts for the HelperNode version for NN Inferencing.
