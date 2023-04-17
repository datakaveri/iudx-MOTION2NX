# IUDX-MOTION2NX 

This software is an extension of the [Encrypto group’s MOTION2NX framework for Generic Hybrid Two-Party
Computation](https://github.com/encryptogroup/MOTION2NX). The goal of this extension is to implement a neural network
inferencing task on a framework where the data is supplied to the “secure compute servers” by the “data providers”. More
specifically, we added two data providers to supply private and public shares to the compute servers. These data providers
support only ArithmeticBEAVY secret sharing protocol. We can extend this setting to multiple (>2) data providers as well.

## New additions
1. **Data providers:** Implemented data providers that provide shares of their respective private data to the secure compute
servers for performing privacy preserving operations. 

2. **New gates to support constant multiplication:** This operation is multiplying a constant (which is common knowledge to
both the parties) with private data. This new gate does not require creation of shares for the constant, thereby making the
operation faster. 

3. **Modular approach to Neural Network Inferencing on MNIST data:** Two compute servers are involved in the inferencing
task. There are two data providers, one of them provides the neural network model (weights and biases) to the compute
servers and the other data provider (Image provider) provides image that has to be inferred. We do not reconstruct the final
output “in clear” at the compute servers in order to maintain privacy. The compute servers send their respective output shares
to the image provider. The image provider then reconstructs the output “in clear”.
In our modular approach, we split the deep (multi-layer) network model into smaller modules that are executed sequentially.
We ensure that each module still preserves the privacy of its respective input and output (thereby preserving the privacy of
the intermediate results in the deep neural network). \
The modular approach helps us to accomplish the following: \
	(a)    Reduce the memory usage for secure computation of the neural network inferencing task. \
	(b)    Provides us with the ability to easily execute a deep neural network with large number of layers (>2) because the memory usage does not scale up with the number of layers in our modular approach. 

4. Accuracy and Cross Entropy Loss testing for MNIST Neural Network Inference 


This code is provided as a experimental implementation for testing purposes and should not be used in a production environment. 

## Working Environment


This software was developed and tested in the following environment (it might not work with older versions):

- [Ubuntu 22.04.2 LTS](https://ubuntu.com/download/desktop)
- [GCC 11.3.0](https://gcc.gnu.org/) 
- [CMake 3.22.1](https://cmake.org/)
- [Boost 1.79.0](https://www.boost.org/users/history/version_1_79_0.html)
- [OpenSSL 3.0.2](https://openssl.org/)
- [Eigen 3.4.0](https://eigen.tuxfamily.org/)
- [fmt 8.0.1](https://github.com/fmtlib/fmt)
- [flatbuffers 2.0.0](https://github.com/google/flatbuffers)
- [GoogleTest 1.11.0 (optional, for tests, build automatically)](https://github.com/google/googletest)
- [Google Benchmark 1.6.0 (optional, for some benchmarks, build automatically)](https://github.com/google/benchmark)
- [HyCC (optional, for the HyCCAdapter)](https://gitlab.com/securityengineering/HyCC)
- [ONNX 1.10.2 (optional, for the ONNXAdapter)](https://github.com/onnx/onnx)


Make sure you have installed all of the following prerequisites on your development machine (a machine with Ubuntu 22.04) to create above environment:
* **Git** - [Download & Install Git](https://git-scm.com/downloads). OSX and Linux machines typically have this already installed. You can install it using 
``sudo apt update && sudo apt install git``

* **g++ compiler** - OSX and Linux machines typically have this already installed. The latest version should ideally be above 9 for gcc or g++. To install g++ ,   install using ``sudo apt update &&sudo apt install g++``
* **Openssl** : Install openssl package and libraries need for motion2nx using 
`` sudo apt update && sudo apt-get -y install --no-install-recommends openssl libssl-dev ``
* **CMAKE and build tools** - Install cmake and other buiuld tools using ``sudo apt update &&   sudo apt-get -y install --no-install-recommends build-essential g++ python3-dev autotools-dev libicu-dev libbz2-dev cmake `` 

* **Boost libraries** - 

    1. First uninstall the existing boost libraries present on the system. The following commands delete boost except its dependencies. 
    	
       ```
       sudo apt-get update && sudo apt-get -y --purge remove libboost-all-dev libboost-doc libboost-dev && sudo rm -f /usr/lib/libboost_* 
       ```
    
    3. [Download boost](https://www.boost.org/users/history/version_1_79_0.html). Make sure to install the .tar.gz extension as it is easier to install.
   
    4. Go to the place where it is downloaded and use the following command to extract boost. \
    ``tar -zxvf boost_1_79_0.tar.gz``
    
    5. Run the following commands before proceeding to the next step. `` NUM_JOBS=`nproc` ``
       ```
       echo "Number of parallel jobs that can be done on this machine: $NUM_JOBS"
       ```
   
    6. Change directory to the folder where boost was extracted and run the following commands. 

       ``` 
       ./bootstrap.sh --with-libraries=atomic,date_time,exception,filesystem,iostreams,locale,program_options,regex,system,test,thread,timer,log,json,context,fiber
       ```

       ``` 
       sudo ./b2 --with=all -j $NUM_JOBS install 
       ```
	
       This might take upto an hour to setup the full boost library.
    
    7. To check if it is installed successfully, run the following commands.
    
       ```
       cat /usr/local/include/boost/version.hpp | grep "BOOST_LIB_VERSION" 
       ``` 
    
    Good to go if boost version is greater than or equal to 1.75.
    
* **Eigen** - 
  1. [Download Eigen](https://gitlab.com/libeigen/eigen/-/archive/3.4.0/eigen-3.4.0.tar.bz2)
  2. After downloading, extract the above folder using ``tar -xvf eigen-3.4.0``. 
  2. Run the following commands to install Eigen.
     ```
     cd eigen-3.4.0
     mkdir build_dir
     cd build_dir
     cmake ../../eigen-3.4.0
     make install (or) sudo make install
     ```
		
  3. Check the Eigen version using the following instruction. 

     ```
     grep "#define EIGEN_[^_]*_VERSION" /usr/local/include/eigen3/Eigen/src/Core/util/Macros.h
     ```


* **Other Packages/Libraries** :The build system downloads and builds GoogleTest and Benchmark if required.
It also tries to download and build fmt, and flatbuffers if it cannot find these libraries in the system.



## Steps for Building Motion2nx

 - Check whether gnome is installed using \
   ``gnome-shell --version``\
   If if is not installed, install it using\
   ```
   sudo apt install ubuntu-gnome-desktop
   ```
 - Clone the Repository\
   ```
   git clone https://github.com/datakaveri/iudx-MOTION2NX.git
   ```
   
   (or)
	
   If you want to clone the modulartensor branch:\
   ```
   git clone -b modulartensor https://github.com/datakaveri/iudx-MOTION2NX.git 
   ```
- Now cd into the repository and build it using the following command

  ```
  CC=gcc CXX=g++ cmake \
  -B build_debwithrelinfo_gcc \
  -DCMAKE_BUILD_TYPE=DebWithRelInfo \
  -DMOTION_BUILD_EXE=On \
  -DMOTION_BUILD_TESTS=On \
  -DMOTION_USE_AVX=AVX2
  ```
- The above code builds the necessary folder, and each of the flags indicate the requirements to be fulfilled for the build.
 
  **Explanation of the flags:** \
          `CC=gcc CXX=g++`: Selects GCC as compiler\
          `-B build_debwithrelinfo_gcc`: Creates a build directory named "build_debwithrelinfo_gcc"\
          `-DCMAKE_BUILD_TYPE=DebWithRelInfo`: Compiles with optimization and debug symbols -- makes tests run faster and debugging easier\
          `-DMOTION_BUILD_EXE=On`: Builds example executables and benchmarks\
          `-DMOTION_BUILD_TESTS=On`: Builds tests\
          `-DMOTION_USE_AVX=AVX2`: Compiles with AVX2 instructions (choose one of `AVX`/`AVX2`/`AVX512`)


- Once that is done, execute the command to install the executables and their dependencies. This process can take upto an hour:\
  ```
  cmake --build build_debwithrelinfo_gcc
  ```
  
- cd into the repository folder, and run the following ONCE\
  ``source setup.sh``\
  
  This sets the required environment variables in ~/.bashrc, ~/.profile.\
  Execute this command only once.
  
  ## Steps for running the Modular Neural Network Inference code
  
    There are sample MNIST images in "[path to repository folder]/iudx-MOTION2NX/Dataprovider/image_provider/images". We use the   following convention $Xi, i\in \{0,1,2,...\}$. We provide images $X0$ to $X98$ in the images folder. Each image is a column     matrix of size $785 \times 1$. The first number of every column matrix is the actual label of the image. All the remaining     numbers are normalised pixel values ranging from 0 to 1.
    
  The sharegenerator.sh script creates secret shares of the image and sends it to the compute servers. Set image_ids in the       script to create shares of the specified images. For example, to create shares of image X23 , put ``image_ids=(23)`` 
  
- To create shares for a given sample MNIST image, do the following.
	- cd into the "[ path to repository folder ]/scripts" folder.
	- Open a new terminal and run the script.
    	
	  ```
	  bash sharegenerator.sh
	  ```
 	- This script generates MNIST image shares and neural network model shares (weights and bias shares). The image and neural 	network shares are saved inside "[ path to repository folder ]/build_debwithrelinfo_gcc/server0" in  Server-0, and "[ path to repository folder ]/build_debwithrelinfo_gcc/server1" in Server-1.
  
  <p align="center">	(or)	</p>
 
 - To create a new MNIST image matrix , follow the steps given below.
	- Flatten the image to a normalised (between 0 to 1) pixel vector (784 rows, 1 column). Use the "flatten_image.py"               python code given in "[path to repository folder ]/Dataprovider/image_provider" to flatten the image matrix.
	-  To run flatten_image.py, run the following command.
	
	   ``` 
	   python3 flatten_image.py --input_image_path [path to image] --output_image_ID [image number] 
	   ```
	
	- Open "[ path to repository folder ]/scripts/sharegenerator.sh" and assign the image number to the image_ids list.               For example, to create shares of image X23 , put ``image_ids=(23)``
 
     	```
      	image_ids=([your_image_number])
        ```
    - Run the script 
    
      ```
      bash sharegenerator.sh
      ```
The script inference.sh performs the inferencing task on the image index given by $i$ in the script.
- Open a new terminal, and run the following script.
  
  ```
  bash inference.sh
  ```
  
  This runs the neural network inference using the generated image and neural network shares. It runs it layer by layer. \
  The final MNIST classification result is displayed on the terminal.  
  
  <p align="center">	(or)	</p>
  
  To run the neural network inference with reduced memory requirement, run the following script.
   
  ```
  bash inference_split.sh
  ```
 The script inference_split.sh performs the inferencing task with splits on layer 1 matrix multiplication on the image index given by image_ids in the script.
  This script reduces the average memory requirement by the number of splits specified in the inference_split script. To change the     number of splits, update the split variable in the script. For example, ``split=2``. User can change this number to be         1, 2, 4, 8, 16.  

## Docker based deployment
### Pre-requisite 
- Install docker-ce, docker compose through following script for Ubuntu:
  ```
  curl -sL https://raw.githubusercontent.com/datakaveri/iudx-deployment/master/Docker-Swarm-deployment/single-node/infrastructure/files/packages-docker-install.sh | sudo bash
  ```
  <p align="center">	(or)	</p>
  
  refer official docs https://docs.docker.com/engine/install/  

- For convenience, you can manage docker as non root user, ref: https://docs.docker.com/engine/install/linux-postinstall/#manage-docker-as-a-non-root-user
  
  * Note : Please do not manage docker as non-root user in production envrionment

- Copy the config files 
  ```
  cp -r config_files/example-smpc-remote-config.json config_files/smpc-remote-config.json
  ```
- Copy the example data to use for inferencing
  ```
  cp -r example-data/data .
  ```
- To create a new MNIST image matrix for inferencing through SMPC , follow the steps mentioned above
### Build docker image
- Build docker image locally, this can be used for test/dev deployment
  ```
  ./docker/build.sh
  ```
- To build and push docker image to docker registry
  ```
  ./docker/build-push.sh
  ```
- To update Build cache in registry, 
  ```
  ./docker/build-cache.sh
  ```
### Local SMPC Deployment
- Deploy all smpc servers in one container using local image
  ```
  docker compose -f docker-compose.local.yaml up -d
  ```

### 2 Party SMPC deployment
- Deploy the two servers locally using local image

  1. SMPC server 0 - SMPC compute server0, image provide
  2. SMPC server 1 - SMPC compute server1, model provider

  - without splits
    ```
    docker compose -f docker-compose.remote.yaml up -d
    ```
  - with splits
    ```
    docker compose -f docker-compose.remote_split.yaml up -d
    ```
- To run the two server locally using docker image from container registry, 
copy example-docker-compose.remote.yaml to docker-compose.remote.yaml file and 
replace with appropriate docker image tag you want to deploy

  ```
  cp example-docker-compose.remote-registry.yaml docker-compose.remote-registry.yaml
  ```
  - without splits

    ```
    docker compose -f docker-compose.remote.yaml -f docker-compose.remote-registry.yaml up -d
    ```
  - with splits 
    ```
    docker compose -f docker-compose.remote_split.yaml -f docker-compose.remote-registry.yaml up -d
    ```
- To run each server on different machine, git clone this repo on each of the machines and run following, : 
  ```
  # after copy, replace with appropriate image tag
  cp example-docker-compose.remote-registry.yaml docker-compose.remote-registry.yaml
  ```
  - without splits

    ```
    # On SMPC server 0
    docker compose -f docker-compose.remote.yaml -f docker-compose.remote-registry.yaml up -d smpc-server0
    ```
    ```
    # On SMPC server 1
    docker compose -f docker-compose.remote.yaml -f docker-compose.remote-registry.yaml up -d smpc-server1
    ```
  - with splits
    ```
    # On SMPC server 0
    docker compose -f docker-compose.remote_split.yaml -f docker-compose.remote-registry.yaml up -d smpc-server0
    ```
    ```
    # On SMPC server 1
    docker compose -f docker-compose.remote_split.yaml -f docker-compose.remote-registry.yaml up -d smpc-server1
    ```
### Miscellaneous Docker commands
- To get logs of container, use following commands
  ```
  # Get compose name
  docker compose ls
  ```
  ```
  # using compose name get logs
  docker compose --project-name <compose-name> logs -f
  ```
- To bring down container use following command:

  ```
  # using compose name bring down
  docker compose --project-name <compose-name> down
  ```