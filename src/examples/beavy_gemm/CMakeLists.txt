add_executable(beavy_gemm beavy_gemm.cpp)

find_package(Boost COMPONENTS json log program_options REQUIRED)

target_compile_features(beavy_gemm PRIVATE cxx_std_20)

target_link_libraries(beavy_gemm
    MOTION::motion
    Boost::json
    Boost::log
    Boost::program_options
)