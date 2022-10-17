add_executable(test_fixed_point test_fixed_point.cpp)

find_package(Boost COMPONENTS json log program_options REQUIRED)

target_compile_features(test_fixed_point PRIVATE cxx_std_20)

target_link_libraries(test_fixed_point
    MOTION::motion
    Boost::json
    Boost::log
    Boost::program_options
)
