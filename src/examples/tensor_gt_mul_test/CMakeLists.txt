add_executable(tensor_gt_mul_test tensor_gt_mul_test.cpp)
add_executable(tensor_helper tensor_helper.cpp)

find_package(Boost COMPONENTS json log program_options REQUIRED)

target_compile_features(tensor_gt_mul_test PRIVATE cxx_std_20)
target_compile_features(tensor_helper PRIVATE cxx_std_20)

target_link_libraries(tensor_gt_mul_test
    MOTION::motion
    Boost::json
    Boost::log
    Boost::program_options
)

target_link_libraries(tensor_helper
    MOTION::motion
    Boost::json
    Boost::log
    Boost::program_options
)
