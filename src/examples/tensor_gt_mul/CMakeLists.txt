add_executable(tensor_gt_mul tensor_gt_mul.cpp)
add_executable(tensor_gt_test_my tensor_gt_test_my.cpp)

find_package(Boost COMPONENTS json log program_options REQUIRED)

target_compile_features(tensor_gt_mul PRIVATE cxx_std_20)

target_compile_features(tensor_gt_test_my PRIVATE cxx_std_20)


target_link_libraries(tensor_gt_mul
    MOTION::motion
    Boost::json
    Boost::log
    Boost::program_options
)

target_link_libraries(tensor_gt_test_my
    MOTION::motion
    Boost::json
    Boost::log
    Boost::program_options
)
