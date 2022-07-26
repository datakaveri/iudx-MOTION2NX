add_executable(beavy_dot_product beavy_dot_product.cpp)

find_package(Boost COMPONENTS json log program_options REQUIRED)

target_compile_features(beavy_dot_product PRIVATE cxx_std_20)

target_link_libraries(beavy_dot_product
    MOTION::motion
    Boost::json
    Boost::log
    Boost::program_options
)