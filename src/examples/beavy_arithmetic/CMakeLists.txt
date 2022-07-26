add_executable(beavy_arithmetic beavy_arithmetic.cpp)

find_package(Boost COMPONENTS json log program_options REQUIRED)

target_compile_features(beavy_arithmetic PRIVATE cxx_std_20)

target_link_libraries(beavy_arithmetic
    MOTION::motion
    Boost::json
    Boost::log
    Boost::program_options
)