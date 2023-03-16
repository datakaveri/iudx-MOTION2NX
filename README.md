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

## Prerequisites
Make sure you have installed all of the following prerequisites on your development machine:
* **Git** - [Download & Install Git](https://git-scm.com/downloads). OSX and Linux machines typically have this already installed. You can install it using 
```sudo apt install git```

* **g++ compiler** - OSX and Linux machines typically have this already installed. The latest version should ideally be above 9 for gcc or g++. To install g++ ,   install using ```sudo apt install g++```
* **Boost libraries** - 

    1. First uninstall the existing boost libraries present on the system. The following commands delete boost except its dependencies. 
    	
       ```sudo apt-get update ```  \
       ```sudo apt-get -y --purge remove libboost-all-dev libboost-doc libboost-dev ``` \
       ```sudo rm -f /usr/lib/libboost_* ``` 
 
       
    2. Now install the latest version of boost and some additional dependencies, which can be done using:    
    
       ``` sudo apt-get -y install build-essential g++ python-dev autotools-dev libicu-dev libbz2-dev```
    
    3. Once the set up is ready, [ download boost](https://www.boost.org/users/history/version_1_79_0.html). Make sure to install the .tar.gz extension as it is easier to install.
   
    4. Go to the place where it is downloaded and use the following command to extract boost. \
    ```tar -zxvf boost_1_79_0.tar.gz```
    
    5. Run the following commands before proceeding to the next step.

		```cpuCores=`cat /proc/cpuinfo | grep "cpu cores" | uniq | awk '{print $NF}'```

		```echo "Available CPU cores: "$cpuCores```
   
    6. Change directory to the folder where boost was extracted and run the following commands. 

		``` ./bootstrap.sh --with-libraries=atomic,date_time,exception,filesystem,iostreams,locale,program_options,regex,signals,system,test,thread,timer,log```

		``` sudo ./b2 --with=all -j $cpuCores install ```
	
		This might take upto an hour to setup the full boost library.
    
    7. To check if it is installed successfully, run the following commands.
    
       	```cat /usr/local/include/boost/version.hpp | grep "BOOST_LIB_VERSION" ``` 
    
       Good to go if boost version is greater than or equal to 1.75.
    
* **Eigen** - [Download Eigen](https://gitlab.com/libeigen/eigen/-/archive/3.4.0/eigen-3.4.0.tar.bz2)

  1. After downloading, extract the above folder using ```tar -xvf eigen-3.4.0```. 
  2. Run the following commands to install Eigen.
		```
		cd eigen-3.4.0
		mkdir build_dir
		cd build_dir
		cmake ../../eigen-3.4.0
		make install (or) sudo make install
		```
		
  3. Check the Eigen version using the following instruction. 

 		```grep "#define EIGEN_[^_]*_VERSION" /usr/local/include/eigen3/Eigen/src/Core/util/Macros.h```

 

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

The build system downloads and builds GoogleTest and Benchmark if required.
It also tries to download and build fmt, and flatbuffers if it cannot find these libraries in the system.



## Steps for Installing the Repository

 - Check whether gnome is installed using \
   ```gnome-shell --version```\
   If if is not installed, install it using\
   ```sudo apt install ubuntu-gnome-desktop```
 - Clone the Repository\
   ```git clone https://github.com/datakaveri/iudx-MOTION2NX.git ```
   
   (or)
	
   If you want to clone the modulartensor branch:\
   ```git clone -b modulartensor https://github.com/datakaveri/iudx-MOTION2NX.git ```
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
  ```cmake --build build_debwithrelinfo_gcc```
  
- cd into the repository folder, and run the following ONCE\
  ```source setup.sh ```\
  This sets the required environment variables in ~/.bashrc, ~/.profile, and /etc/environment files.\
  Execute this command only once.
  
  
  ## Steps for running the Modular Neural Network Inference code
  
- To classify the given sample MNIST images, do the following.
	- cd into the "[ path to repository folder ]/scripts" folder.
	- Open a new terminal and run the script.
    	
	  ```
	  bash sharegenerator.sh
	  ```
 	- This script opens multiple gnome tabs and generates MNIST image shares and neural network model shares (weights and bias shares). The image and neural 	network shares are saved inside "[ path to repository folder ]/build_debwithrelinfo_gcc/server0" in  Server-0, and "[ path to repository folder ]/build_debwithrelinfo_gcc/server1" in Server-1. Once the program finishes execution, close  all the opened tabs.
  
  <p align="center">	(or)	</p>
 
 - To classify a new MNIST image, follow the steps given below.
	- Flatten the image to a normalised (between 0 to 1) pixel vector (784 rows, 1 column).
	- Save it in "[ path to repository folder ]/image_provider/images" folder with the filename of the format ‘X[image_number].csv.
	- Open "[ path to repository folder ]/scripts/sharegenerator.sh" and assign the image number to the list.
 
     	```
      	image_ids=([your_image_number])
        ```
    - Run the script 
    
      ```
      bash sharegenerator.sh
      ```

- Open a new terminal, and run the following script.
  ```
  bash inference.sh
  ```
  This runs the neural network inference using the generated image and neural network shares. It runs it layer by layer and saves the intermediate results. \
  The final MNIST classification result is displayed on the terminal.  

