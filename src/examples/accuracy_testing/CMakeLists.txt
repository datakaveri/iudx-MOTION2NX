add_executable(tensor_gt_relu tensor_gt_relu.cpp)
add_executable(argmax argmax.cpp)
add_executable(tensor_gt_mul_test tensor_gt_mul_test.cpp)
add_executable(Image_Share_Receiver Image_Share_Receiver.cpp)
add_executable(Weights_Share_Receiver Weights_Share_Receiver.cpp)
add_executable(Weights_Share_Receiver_remote Weights_Share_Receiver_remote.cpp)
add_executable(final_output_provider final_output_provider.cpp)
add_executable(tensor_gt_mul_split tensor_gt_mul_split.cpp)
add_executable(weight_share_receiver_genr weight_share_receiver_genr.cpp)
add_executable(conv conv.cpp)
add_executable(conv_split conv_split.cpp)
add_executable(training training.cpp)
add_executable(Image_Share_Receiver_CNN Image_Share_Receiver_CNN.cpp)
add_executable(Weights_Share_Receiver_CNN Weights_Share_Receiver_CNN.cpp)

find_package(Boost COMPONENTS json log program_options REQUIRED)

target_compile_features(tensor_gt_relu PRIVATE cxx_std_20)
target_compile_features(argmax PRIVATE cxx_std_20)
target_compile_features(tensor_gt_mul_test PRIVATE cxx_std_20)
target_compile_features(Image_Share_Receiver PRIVATE cxx_std_20)
target_compile_features(Weights_Share_Receiver PRIVATE cxx_std_20)
target_compile_features(Weights_Share_Receiver_remote PRIVATE cxx_std_20)
target_compile_features(Weights_Share_Receiver_CNN PRIVATE cxx_std_20)
target_compile_features(final_output_provider PRIVATE cxx_std_20)
target_compile_features(tensor_gt_mul_split PRIVATE cxx_std_20)
target_compile_features(weight_share_receiver_genr PRIVATE cxx_std_20)
target_compile_features(Image_Share_Receiver_CNN PRIVATE cxx_std_20)
target_compile_features(training PRIVATE cxx_std_20)
target_compile_features(conv PRIVATE cxx_std_20)
target_compile_features(conv_split PRIVATE cxx_std_20)



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

target_link_libraries(Image_Share_Receiver
    MOTION::motion
    Boost::json
    Boost::log
    Boost::program_options
)

target_link_libraries(Image_Share_Receiver_CNN
    MOTION::motion
    Boost::json
    Boost::log
    Boost::program_options
)

target_link_libraries(Weights_Share_Receiver
    MOTION::motion
    Boost::json
    Boost::log
    Boost::program_options
)

target_link_libraries(Weights_Share_Receiver_CNN
    MOTION::motion
    Boost::json
    Boost::log
    Boost::program_options
)

target_link_libraries(Weights_Share_Receiver_remote
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

target_link_libraries(tensor_gt_mul_split
    MOTION::motion
    Boost::json
    Boost::log
    Boost::program_options
)

target_link_libraries(weight_share_receiver_genr
    MOTION::motion
    Boost::json
    Boost::log
    Boost::program_options
)

target_link_libraries(conv
    MOTION::motion
    Boost::json
    Boost::log
    Boost::program_options
)

target_link_libraries(conv_split
    MOTION::motion
    Boost::json
    Boost::log
    Boost::program_options
)

target_link_libraries(training
    MOTION::motion
    Boost::json
    Boost::log
    Boost::program_options
)
