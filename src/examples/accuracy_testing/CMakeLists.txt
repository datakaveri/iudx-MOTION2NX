add_executable(tensor_gt_relu tensor_gt_relu.cpp)
add_executable(argmax argmax.cpp)
add_executable(tensor_gt_mul_test tensor_gt_mul_test.cpp)
add_executable(Image_Share_Reciever Image_Share_Reciever.cpp)
add_executable(Weights_Share_Reciever Weights_Share_Reciever.cpp)
add_executable(final_output_provider final_output_provider.cpp)


find_package(Boost COMPONENTS json log program_options REQUIRED)

target_compile_features(tensor_gt_relu PRIVATE cxx_std_20)
target_compile_features(argmax PRIVATE cxx_std_20)
target_compile_features(tensor_gt_mul_test PRIVATE cxx_std_20)
target_compile_features(Image_Share_Reciever PRIVATE cxx_std_20)
target_compile_features(Weights_Share_Reciever PRIVATE cxx_std_20)
target_compile_features(final_output_provider PRIVATE cxx_std_20)

target_link_libraries(tensor_gt_relu
    MOTION::motion
    Boost::json
    Boost::log
    Boost::program_options
)

target_link_libraries(argmax
    MOTION::motion
    Boost::json
    Boost::log
    Boost::program_options
)

target_link_libraries(tensor_gt_mul_test
    MOTION::motion
    Boost::json
    Boost::log
    Boost::program_options
)

target_link_libraries(Image_Share_Reciever
    MOTION::motion
    Boost::json
    Boost::log
    Boost::program_options
)

target_link_libraries(Weights_Share_Reciever
    MOTION::motion
    Boost::json
    Boost::log
    Boost::program_options
)

target_link_libraries(final_output_provider
    MOTION::motion
    Boost::json
    Boost::log
    Boost::program_options
)
