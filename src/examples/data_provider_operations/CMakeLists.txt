add_executable(data_provider_operations data_provider_operations.cpp)

find_package(Boost COMPONENTS json log program_options REQUIRED)

target_compile_features(data_provider_operations PRIVATE cxx_std_20)

target_link_libraries(data_provider_operations
    MOTION::motion
    Boost::json
    Boost::log
    Boost::program_options
)