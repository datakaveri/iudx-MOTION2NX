add_executable(tensor_mat_mul tensor_matrix_multiplication.cpp)

find_package(Boost COMPONENTS json log program_options REQUIRED)

target_compile_features(tensor_mat_mul PRIVATE cxx_std_20)

target_link_libraries(tensor_mat_mul
    MOTION::motion
    Boost::json
    Boost::log
    Boost::program_options
)