add_executable(data_provider_dot_product_1d data_provider_dot_product_1d.cpp)

find_package(Boost COMPONENTS json log program_options REQUIRED)

target_compile_features(data_provider_dot_product_1d PRIVATE cxx_std_20)

target_link_libraries(data_provider_dot_product_1d
    MOTION::motion
    Boost::json
    Boost::log
    Boost::program_options
)