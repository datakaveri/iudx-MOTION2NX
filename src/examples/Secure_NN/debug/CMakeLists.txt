add_executable(3PC_CNN_NN_debug_0 3PC_CNN_NN_debug_0.cpp)

add_executable(3PC_CNN_NN_debug_1 3PC_CNN_NN_debug_1.cpp)

add_executable(3PC_CNN_NN_debug_2 3PC_CNN_NN_debug_2.cpp)

find_package(Boost COMPONENTS json log program_options REQUIRED)

target_compile_features(3PC_CNN_NN_debug_0 PRIVATE cxx_std_20)

target_compile_features(3PC_CNN_NN_debug_1 PRIVATE cxx_std_20)

target_compile_features(3PC_CNN_NN_debug_2 PRIVATE cxx_std_20)

target_link_libraries(3PC_CNN_NN_debug_0
  MOTION::motion
  Boost::json
  Boost::log
  Boost::program_options
)

target_link_libraries(3PC_CNN_NN_debug_1
  MOTION::motion
  Boost::json
  Boost::log
  Boost::program_options
)

target_link_libraries(3PC_CNN_NN_debug_2
  MOTION::motion
  Boost::json
  Boost::log
  Boost::program_options
)