
add_executable(3PC_Relu0 3PC_Relu0.cpp)

add_executable(3PC_Relu1 3PC_Relu1.cpp)

add_executable(3PC_Relu2 3PC_Relu2.cpp)

find_package(Boost COMPONENTS json log program_options REQUIRED)

target_compile_features(3PC_Relu0 PRIVATE cxx_std_20)

target_compile_features(3PC_Relu1 PRIVATE cxx_std_20)

target_compile_features(3PC_Relu2 PRIVATE cxx_std_20)

target_link_libraries(3PC_Relu0
    MOTION::motion
    Boost::json
    Boost::log
    Boost::program_options
)

target_link_libraries(3PC_Relu1
    MOTION::motion
    Boost::json
    Boost::log
    Boost::program_options
)

target_link_libraries(3PC_Relu2
    MOTION::motion
    Boost::json
    Boost::log
    Boost::program_options
)
