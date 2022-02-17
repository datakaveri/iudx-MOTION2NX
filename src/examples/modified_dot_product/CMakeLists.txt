add_executable(modified_dot_product modified_dot_product.cpp)

find_package(Boost COMPONENTS json log program_options REQUIRED)

target_compile_features(modified_dot_product PRIVATE cxx_std_20)

target_link_libraries(modified_dot_product
    MOTION::motion
    Boost::json
    Boost::log
    Boost::program_options
)