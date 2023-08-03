## ServerModel_Architecture
---------------------------------
Server model considers the data providers and compute servers as separate entities.
Run the ImageProvider.sh in one terminal/machine, the ModelProvider.sh in another terminal/machine. Run the docker containers in cloud using the corresponding docker compose files. Use smpc-server0 service for server0, smpc-server1 for server 1 and smpc-server2 for helpernode server.


1) Split folder: Contains scripts for the Server model split version for NN Inferencing.

2) HelperNode folder: Contains scripts for the Server model HelperNode version for NN Inferencing.

(For Eg.) To run the Server Model Split version, navigate to the Split folder and run the following to start the docker container.

    docker compose -f docker-compose.remote_split_servermodel.yaml -f ../../../docker-compose.remote-registry.yaml up -d smpc-server0

The servers should start first, and then the image and model shares are sent to it by the data providers. The reconstructed output is sent back to the image provider.

