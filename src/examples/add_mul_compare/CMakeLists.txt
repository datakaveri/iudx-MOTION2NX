add_executable(add_mul_compare add_mul_compare.cpp)

find_package(Boost COMPONENTS json log program_options REQUIRED)

target_compile_features(add_mul_compare PRIVATE cxx_std_20)

target_link_libraries(add_mul_compare
    MOTION::motion
    Boost::json
    Boost::log
    Boost::program_options
)
