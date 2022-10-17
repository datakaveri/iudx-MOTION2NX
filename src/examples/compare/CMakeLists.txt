add_executable(compare compare.cpp)

find_package(Boost COMPONENTS json log program_options REQUIRED)

target_compile_features(compare PRIVATE cxx_std_20)

target_link_libraries(compare
    MOTION::motion
    Boost::json
    Boost::log
    Boost::program_options
)
