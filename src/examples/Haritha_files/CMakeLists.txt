add_executable(test_bed test_bed.cpp)
add_executable(cnn cnn.cpp)
add_executable(cnn_nn cnn_nn.cpp)
add_executable(2_cnn 2_cnn.cpp)
add_executable(my_share_generator my_share_generator.cpp)
add_executable(my_const_mat_mult my_const_mat_mult.cpp)
#add_executable(CONST_model_data_shares CONST_model_data_shares.cpp)
add_executable(Gemm_understanding Gemm_understanding.cpp)
add_executable(Const_Matrix_mul Const_Matrix_mul.cpp)
find_package(Boost COMPONENTS json log program_options REQUIRED)

target_compile_features(test_bed PRIVATE cxx_std_20)

target_compile_features(cnn PRIVATE cxx_std_20)

target_compile_features(my_share_generator PRIVATE cxx_std_20)

target_compile_features(cnn_nn PRIVATE cxx_std_20)

target_compile_features(2_cnn PRIVATE cxx_std_20)

target_compile_features(my_const_mat_mult PRIVATE cxx_std_20)

#target_compile_features(CONST_model_data_shares PRIVATE cxx_std_20)

target_compile_features(Gemm_understanding PRIVATE cxx_std_20)

target_compile_features(Const_Matrix_mul PRIVATE cxx_std_20)

target_link_libraries(test_bed
    MOTION::motion
    Boost::json
    Boost::log
    Boost::program_options
)

target_link_libraries(cnn
    MOTION::motion
    Boost::json
    Boost::log
    Boost::program_options
)

target_link_libraries(my_share_generator
    MOTION::motion
    Boost::json
    Boost::log
    Boost::program_options
)

target_link_libraries(cnn_nn
    MOTION::motion
    Boost::json
    Boost::log
    Boost::program_options
)

target_link_libraries(2_cnn
    MOTION::motion
    Boost::json
    Boost::log
    Boost::program_options
)

target_link_libraries(my_const_mat_mult
    MOTION::motion
    Boost::json
    Boost::log
    Boost::program_options
)

#target_link_libraries(CONST_model_data_shares
#MOTION::motion
#Boost::json
#Boost::log
#Boost::program_options
#)



target_link_libraries(Gemm_understanding
MOTION::motion
Boost::json
Boost::log
Boost::program_options
)

target_link_libraries(Const_Matrix_mul
MOTION::motion
Boost::json
Boost::log
Boost::program_options
)