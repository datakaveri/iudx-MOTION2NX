add_executable(argmax argmax.cpp)

find_package(Boost COMPONENTS json log program_options REQUIRED)

target_compile_features(argmax PRIVATE cxx_std_20)

target_link_libraries(argmax
    MOTION::motion
    Boost::json
    Boost::log
    Boost::program_options
)
