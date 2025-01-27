add_executable(beavy_gemm beavy_gemm.cpp)

add_executable(3PC_Relu_0 3PC_Relu_0.cpp)

add_executable(3PC_Relu_1 3PC_Relu_1.cpp)

add_executable(3PC_Relu_2 3PC_Relu_2.cpp)

add_executable(00_Arith_Share_Generator 00_Arith_Share_Generator.cpp)
add_executable(01_Msg_Generation_with_Id 01_Msg_Generation_with_Id.cpp)
add_executable(02_DecodeArithShares 02_DecodeArithShares.cpp)
add_executable(03_PrimeShareGenerator 03_PrimeShareGenerator.cpp)
add_executable(04_Random_Permute 04_Random_Permute.cpp)
add_executable(05_PrivateCompare 05_PrivateCompare.cpp)
add_executable(06_ShareConvert 06_ShareConvert.cpp)
add_executable(07_ComputeMSB 07_ComputeMSB.cpp)

add_executable(main
main.cpp
my_functions.cpp
)

find_package(Boost COMPONENTS json log program_options REQUIRED)

target_compile_features(beavy_gemm PRIVATE cxx_std_20)

target_compile_features(3PC_Relu_0 PRIVATE cxx_std_20)

target_compile_features(3PC_Relu_1 PRIVATE cxx_std_20)

target_compile_features(3PC_Relu_2 PRIVATE cxx_std_20)

target_compile_features(00_Arith_Share_Generator PRIVATE cxx_std_20)
target_compile_features(01_Msg_Generation_with_Id PRIVATE cxx_std_20)
target_compile_features(02_DecodeArithShares PRIVATE cxx_std_20)
target_compile_features(03_PrimeShareGenerator PRIVATE cxx_std_20)
target_compile_features(04_Random_Permute PRIVATE cxx_std_20)
target_compile_features(05_PrivateCompare PRIVATE cxx_std_20)
target_compile_features(06_ShareConvert PRIVATE cxx_std_20)
target_compile_features(07_ComputeMSB PRIVATE cxx_std_20)

target_compile_features(main PRIVATE cxx_std_20)

target_link_libraries(beavy_gemm
    MOTION::motion
    Boost::json
    Boost::log
    Boost::program_options
)

target_link_libraries(3PC_Relu_0
    MOTION::motion
    Boost::json
    Boost::log
    Boost::program_options
)

target_link_libraries(3PC_Relu_1
    MOTION::motion
    Boost::json
    Boost::log
    Boost::program_options
)

target_link_libraries(3PC_Relu_2
    MOTION::motion
    Boost::json
    Boost::log
    Boost::program_options
)

target_link_libraries(00_Arith_Share_Generator
    MOTION::motion
    Boost::json
    Boost::log
    Boost::program_options
)


target_link_libraries(01_Msg_Generation_with_Id
    MOTION::motion
    Boost::json
    Boost::log
    Boost::program_options
)

target_link_libraries(02_DecodeArithShares
    MOTION::motion
    Boost::json
    Boost::log
    Boost::program_options
)

target_link_libraries(main
    MOTION::motion
    Boost::json
    Boost::log
    Boost::program_options
)

target_link_libraries(03_PrimeShareGenerator
MOTION::motion
Boost::json
Boost::log
Boost::program_options
)
target_link_libraries(04_Random_Permute
MOTION::motion
Boost::json
Boost::log
Boost::program_options
)
target_link_libraries(05_PrivateCompare
MOTION::motion
Boost::json
Boost::log
Boost::program_options
)
target_link_libraries(06_ShareConvert
MOTION::motion
Boost::json
Boost::log
Boost::program_options
)
target_link_libraries(07_ComputeMSB
MOTION::motion
Boost::json
Boost::log
Boost::program_options
)
