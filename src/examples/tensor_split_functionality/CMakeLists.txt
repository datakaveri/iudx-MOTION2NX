add_executable(tensor_split_functionality tensor_split_functionality.cpp)

find_package(Boost COMPONENTS json log program_options REQUIRED)

target_compile_features(tensor_split_functionality PRIVATE cxx_std_20)

target_link_libraries(tensor_split_functionality
    MOTION::motion
    Boost::json
    Boost::log
    Boost::program_options
)