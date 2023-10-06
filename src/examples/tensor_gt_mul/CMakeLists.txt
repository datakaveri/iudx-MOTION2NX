add_executable(tensor_gt_mul tensor_gt_mul.cpp)
add_executable(tensor_gt_test_my tensor_gt_test_my.cpp)
add_executable(constAdd constAdd.cpp)
add_executable(constMul constMul.cpp)
add_executable(Shares_input Shares_input.cpp)
add_executable(indicator indicator.cpp)


find_package(Boost COMPONENTS json log program_options REQUIRED)

target_compile_features(tensor_gt_mul PRIVATE cxx_std_20)

target_compile_features(tensor_gt_test_my PRIVATE cxx_std_20)

target_compile_features(constAdd PRIVATE cxx_std_20)

target_compile_features(constMul PRIVATE cxx_std_20)

target_compile_features(Shares_input PRIVATE cxx_std_20)

target_compile_features(indicator PRIVATE cxx_std_20)



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

target_link_libraries(constAdd
    MOTION::motion
    Boost::json
    Boost::log
    Boost::program_options
)

target_link_libraries(constMul
    MOTION::motion
    Boost::json
    Boost::log
    Boost::program_options
)

target_link_libraries(Shares_input
     MOTION::motion
    Boost::json
    Boost::log
    Boost::program_options
)

target_link_libraries(indicator
     MOTION::motion
    Boost::json
    Boost::log
    Boost::program_options
)
