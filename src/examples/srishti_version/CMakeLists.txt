add_executable(srishti_version srishti_version.cpp)

find_package(Boost COMPONENTS json log program_options REQUIRED)

target_compile_features(srishti_version PRIVATE cxx_std_20)

target_link_libraries(srishti_version
    MOTION::motion
    Boost::json
    Boost::log
    Boost::program_options
)
