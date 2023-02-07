add_executable(tensor_gt_relu tensor_gt_relu.cpp)

find_package(Boost COMPONENTS json log program_options REQUIRED)

target_compile_features(tensor_gt_relu PRIVATE cxx_std_20)

target_link_libraries(tensor_gt_relu
    MOTION::motion
    Boost::json
    Boost::log
    Boost::program_options
)
