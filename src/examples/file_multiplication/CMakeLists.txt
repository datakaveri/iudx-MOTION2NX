add_executable(file_multiplication file_multiplication.cpp)

find_package(Boost COMPONENTS json log program_options REQUIRED)

target_compile_features(file_multiplication PRIVATE cxx_std_20)

target_link_libraries(file_multiplication
    MOTION::motion
    Boost::json
    Boost::log
    Boost::program_options
)