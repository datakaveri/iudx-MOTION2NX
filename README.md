# IUDX-MOTION2NX 

This software is an extension of the [Encrypto group’s MOTION2NX framework for Generic Hybrid Two-Party Computation](https://github.com/encryptogroup/MOTION2NX).

Additions  
1. Four-way communication model where we have two servers and two data providers.
2. New tensor operations - dot product, constant multiplication, matrix multiplication, ReLU, ArgMax   
3. Modular Neural Network Inference (MNIST)
4. Beavy operations
5. Accuracy and Loss testing for Neural Network Inference


This code is provided as a experimental implementation for testing purposes and should not be used in a production environment. 

## Prerequisites
Make sure you have installed all of the following prerequisites on your development machine:
* Git - [Download & Install Git](https://git-scm.com/downloads). OSX and Linux machines typically have this already installed. You can install it using 
```sudo apt install git```

* g++ compiler - OSX and Linux machines typically have this already installed. The latest version should ideally be above 9 for gcc or g++. To install g++ ,   install using ```sudo apt install g++```
* Boost libraries - 

    1. The following command is to uninstall the existing boost libraries present on the system. It deletes boosts except the dependencies. 
    	
       ```sudo apt-get update ```
       ```sudo apt-get -y --purge remove libboost-all-dev libboost-doc libboost-dev ```
       ```sudo rm -f /usr/lib/libboost_* ```
 
       
    2. For the latest versions of boost, we might need to install additional dependencies, which can be done using :    
    
       ``` sudo apt-get -y install build-essential g++ python-dev autotools-dev libicu-dev libbz2-dev```
    
    3. Once the set up is ready , [ download boost](https://www.boost.org/users/history/version_1_79_0.html) . Make sure to install the .tar.gz extension as it        is easier to install with.
    
    4. Go to the place where it got downloaded and to extract the folder do , ```tar -zxvf boost_1_79_0.tar.gz```
    
    5. To check the number of cores which are free.It makes the installation faster.

		```cpuCores=`cat /proc/cpuinfo | grep "cpu cores" | uniq | awk '{print $NF}'```

		```echo "Available CPU cores: "$cpuCores```
   
    6. Change directory to boost folder and run the following commands. 

	``` ./bootstrap.sh --with-libraries=atomic,date_time,exception,filesystem,iostreams,locale,program_options,regex,signals,system,test,thread,timer,log```

	``` sudo ./b2 --with=all -j $cpuCores install ```
	
	This might take an hour to setup the full boost library.
    
    7. To check if it is installed successfully, in the command line interface type :
    
       	```cat /usr/local/include/boost/version.hpp | grep "BOOST_LIB_VERSION" ``` 
    
       Good to go if boost version is greater than or equal to 1.75.
    
* Eigen - [Download Eigen](https://gitlab.com/libeigen/eigen/-/archive/3.4.0/eigen-3.4.0.tar.bz2)

  1. Go to downloads using terminal and then extract the above folder using ```tar -xvf eigen-3.4.0```
		```
		cd eigen-3.4.0
		mkdir build_dir
		cd build_dir
		cmake ~/Downloads/eigen-3.4.0
		make install or sudo make install
		```
		
  2. Eigen is installed, check the version using the following instruction 

 	```grep "#define EIGEN_[^_]*_VERSION" /usr/local/include/eigen3/Eigen/src/Core/util/Macros.h```

 

## Build Instructions


This software was developed and tested in the following environment (it might
also work with older versions):

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
It also tries to download and build Boost, fmt, and flatbuffers if it cannot find these libraries in the system.



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
 
  Explanation of the flags:\
          `CC=gcc CXX=g++`: select GCC as compiler\
          `-B build_debwithrelinfo_gcc`: create a build directory\
          `-DCMAKE_BUILD_TYPE=DebWithRelInfo`: compile with optimization and also add debug symbols -- makes tests run faster and debugging easier\
          `-DMOTION_BUILD_EXE=On`: build example executables and benchmarks\
          `-DMOTION_BUILD_TESTS=On`: build tests\
          `-DMOTION_USE_AVX=AVX2`: compile with AVX2 instructions (choose one of `AVX`/`AVX2`/`AVX512`)


- Once that is done, execute the command to install the executables and their dependencies. This process takes another hour worst case:\
  ```cmake --build build_debwithrelinfo_gcc```
  
- cd into the repository folder, and run the following ONCE\
  ```sudo -s source setup.sh ```\
  This sets the required environment variables in ~/.bashrc, ~/.profile, and /etc/environment files.\
  Execute this command only once.
  
  
  ## Steps for running the Modular Neural Network Inference code
  
- cd into the [path_to_repository_folder]/scripts folder.
- Open a new terminal and run the script.
  ```
  bash sharegenerator.sh
  ```
  
- This script opens multiple gnome tabs and generates MNIST image shares and neural network shares (weights and bias shares). The image and neural network shares are saved inside [path_to_repository_folder]/build_debwithrelinfo_gcc/server0 in server0, and [Repository_folder_path]/build_debwithrelinfo_gcc/server1 in server1. Once the program finishes execution, close all the opened tabs.
- To classify a new MNIST image, follow the steps given below.
	- Convert the image to a pixel vector (784 rows, 1 column).
	- Save it in [path_to_repository_folder]/image_provider/images folder with the filename of the format ‘X[image_number].csv.
	- Open [path_to_repository_folder]/scripts/sharegenerator.sh and assign the image number to the list. 
     	```
      	list=([your_image_number])
        ```
    - Run the script 
      ```
      bash sharegenerator.sh
      ```

- Open a new terminal, and run the following script.
  ```
  bash test.sh
  ```
  This runs the neural network inference using the generated image and neural network shares. It runs it layer by layer and saves the intermediate results.\ 
  The final MNIST classification result is displayed on the terminal.  


