add_executable(tensor_gt tensor_gt.cpp)

find_package(Boost COMPONENTS json log program_options REQUIRED)

target_compile_features(tensor_gt PRIVATE cxx_std_20)

target_link_libraries(tensor_gt
    MOTION::motion
    Boost::json
    Boost::log
    Boost::program_options
)