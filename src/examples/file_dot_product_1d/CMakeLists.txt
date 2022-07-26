add_executable(file_dot_product_1d file_dot_product_1d.cpp)

find_package(Boost COMPONENTS json log program_options REQUIRED)

target_compile_features(file_dot_product_1d PRIVATE cxx_std_20)

target_link_libraries(file_dot_product_1d
    MOTION::motion
    Boost::json
    Boost::log
    Boost::program_options
)