add_executable(share_generator_test share_generator_test.cpp)
add_executable(Image_Share_Generator Image_Share_Generator.cpp)
add_executable(Weights_Share_Generator Weights_Share_Generator.cpp)


find_package(Boost COMPONENTS json log program_options REQUIRED)

target_compile_features(share_generator_test PRIVATE cxx_std_20)

target_link_libraries(share_generator_test
    MOTION::motion
    Boost::json
    Boost::log
    Boost::program_options
)


target_compile_features(Image_Share_Generator PRIVATE cxx_std_20)

target_link_libraries(Image_Share_Generator
    MOTION::motion
    Boost::json
    Boost::log
    Boost::program_options
)



target_compile_features(Weights_Share_Generator PRIVATE cxx_std_20)

target_link_libraries(Weights_Share_Generator
    MOTION::motion
    Boost::json
    Boost::log
    Boost::program_options
)
