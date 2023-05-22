add_executable(tensor_dot_product tensor_dot_product.cpp)
add_executable(dp dp.cpp)

find_package(Boost COMPONENTS json log program_options REQUIRED)

target_compile_features(tensor_dot_product PRIVATE cxx_std_20)
target_link_libraries(tensor_dot_product
    MOTION::motion
    Boost::json
    Boost::log
    Boost::program_options
)

target_compile_features(dp PRIVATE cxx_std_20)
target_link_libraries(dp
    Boost::json
    Boost::log
    Boost::program_options
)
