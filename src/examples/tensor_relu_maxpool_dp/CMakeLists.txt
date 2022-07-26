add_executable(tensor_relu_maxpool_dp tensor_relu_maxpool_dp.cpp)

find_package(Boost COMPONENTS json log program_options REQUIRED)

target_compile_features(tensor_relu_maxpool_dp PRIVATE cxx_std_20)

target_link_libraries(tensor_relu_maxpool_dp
    MOTION::motion
    Boost::json
    Boost::log
    Boost::program_options
)