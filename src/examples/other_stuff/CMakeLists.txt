add_executable(other_stuff other_stuff.cpp)

find_package(Boost COMPONENTS json log program_options REQUIRED)

target_compile_features(other_stuff PRIVATE cxx_std_20)

target_link_libraries(other_stuff
    MOTION::motion
    Boost::json
    Boost::log
    Boost::program_options
)
