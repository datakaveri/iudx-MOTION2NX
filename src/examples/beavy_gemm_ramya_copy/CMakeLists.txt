add_executable(beavy_gemm_ramya_copy beavy_gemm_ramya_copy.cpp)

find_package(Boost COMPONENTS json log program_options REQUIRED)

target_compile_features(beavy_gemm_ramya_copy PRIVATE cxx_std_20)

target_link_libraries(beavy_gemm_ramya_copy
    MOTION::motion
    Boost::json
    Boost::log
    Boost::program_options
)
