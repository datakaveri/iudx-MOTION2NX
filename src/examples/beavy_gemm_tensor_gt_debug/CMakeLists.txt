add_executable(beavy_gemm_tensor_gt_debug beavy_gemm_tensor_gt_debug.cpp)

find_package(Boost COMPONENTS json log program_options REQUIRED)

target_compile_features(beavy_gemm_tensor_gt_debug PRIVATE cxx_std_20)

target_link_libraries(beavy_gemm_tensor_gt_debug
    MOTION::motion
    Boost::json
    Boost::log
    Boost::program_options
)
