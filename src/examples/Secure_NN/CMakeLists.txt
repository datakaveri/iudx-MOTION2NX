add_subdirectory(debug)

add_subdirectory(debug_2)

add_executable(3PC_Relu0 3PC_Relu0.cpp)

add_executable(3PC_Relu1 3PC_Relu1.cpp)

add_executable(3PC_Relu2 3PC_Relu2.cpp)

add_executable(Test_haritha Test_haritha.cpp)

add_executable(Test_haritha_HelperFile Test_haritha_HelperFile.cpp)

add_executable(3PC_ArithToABY 3PC_ArithToABY.cpp)

add_executable(3PC_ABYToArithShareGenerator 3PC_ABYToArithShareGenerator.cpp)

add_executable(3PC_Conv_Relu_0 3PC_Conv_Relu_0.cpp)

add_executable(3PC_Conv_Relu_1 3PC_Conv_Relu_1.cpp)

add_executable(3PC_Conv_Relu_2 3PC_Conv_Relu_2.cpp)

add_executable(3PC_MatMul_Relu_0 3PC_MatMul_Relu_0.cpp)

add_executable(3PC_MatMul_Relu_1 3PC_MatMul_Relu_1.cpp)

add_executable(3PC_MatMul_Relu_2 3PC_MatMul_Relu_2.cpp)

add_executable(3PC_CNN_NN_0 3PC_CNN_NN_0.cpp)

add_executable(3PC_CNN_NN_1 3PC_CNN_NN_1.cpp)

add_executable(3PC_CNN_NN_2 3PC_CNN_NN_2.cpp)

find_package(Boost COMPONENTS json log program_options REQUIRED)

target_compile_features(3PC_Relu0 PRIVATE cxx_std_20)

target_compile_features(3PC_Relu1 PRIVATE cxx_std_20)

target_compile_features(3PC_Relu2 PRIVATE cxx_std_20)

target_compile_features(Test_haritha PRIVATE cxx_std_20)

target_compile_features(Test_haritha_HelperFile PRIVATE cxx_std_20)

target_compile_features(3PC_ArithToABY PRIVATE cxx_std_20)

target_compile_features(3PC_ABYToArithShareGenerator PRIVATE cxx_std_20)

target_compile_features(3PC_Conv_Relu_0 PRIVATE cxx_std_20)

target_compile_features(3PC_Conv_Relu_1 PRIVATE cxx_std_20)

target_compile_features(3PC_Conv_Relu_2 PRIVATE cxx_std_20)

target_compile_features(3PC_MatMul_Relu_0 PRIVATE cxx_std_20)

target_compile_features(3PC_MatMul_Relu_1 PRIVATE cxx_std_20)

target_compile_features(3PC_MatMul_Relu_2 PRIVATE cxx_std_20)

target_compile_features(3PC_CNN_NN_0 PRIVATE cxx_std_20)

target_compile_features(3PC_CNN_NN_1 PRIVATE cxx_std_20)

target_compile_features(3PC_CNN_NN_2 PRIVATE cxx_std_20)

target_link_libraries(3PC_Relu0
    MOTION::motion
    Boost::json
    Boost::log
    Boost::program_options
)

target_link_libraries(3PC_Relu1
    MOTION::motion
    Boost::json
    Boost::log
    Boost::program_options
)

target_link_libraries(3PC_Relu2
    MOTION::motion
    Boost::json
    Boost::log
    Boost::program_options
)

target_link_libraries(Test_haritha
    MOTION::motion
    Boost::json
    Boost::log
    Boost::program_options
)

target_link_libraries(Test_haritha_HelperFile
    MOTION::motion
    Boost::json
    Boost::log
    Boost::program_options
)

target_link_libraries(3PC_ArithToABY
    MOTION::motion
    Boost::json
    Boost::log
    Boost::program_options
)

target_link_libraries(3PC_ABYToArithShareGenerator
    MOTION::motion
    Boost::json
    Boost::log
    Boost::program_options
)


target_link_libraries(3PC_Conv_Relu_0
    MOTION::motion
    Boost::json
    Boost::log
    Boost::program_options
)

target_link_libraries(3PC_Conv_Relu_1
    MOTION::motion
    Boost::json
    Boost::log
    Boost::program_options
)

target_link_libraries(3PC_Conv_Relu_2
    MOTION::motion
    Boost::json
    Boost::log
    Boost::program_options
)

target_link_libraries(3PC_MatMul_Relu_0
    MOTION::motion
    Boost::json
    Boost::log
    Boost::program_options
)

target_link_libraries(3PC_MatMul_Relu_1
    MOTION::motion
    Boost::json
    Boost::log
    Boost::program_options
)

target_link_libraries(3PC_MatMul_Relu_2
    MOTION::motion
    Boost::json
    Boost::log
    Boost::program_options
)

target_link_libraries(3PC_CNN_NN_0
    MOTION::motion
    Boost::json
    Boost::log
    Boost::program_options
)

target_link_libraries(3PC_CNN_NN_1
    MOTION::motion
    Boost::json
    Boost::log
    Boost::program_options
)

target_link_libraries(3PC_CNN_NN_2
    MOTION::motion
    Boost::json
    Boost::log
    Boost::program_options
)