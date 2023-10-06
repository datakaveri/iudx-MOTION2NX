add_executable(constAdd constAdd.cpp)
add_executable(constMul constMul.cpp)
add_executable(indicator indicator.cpp)
add_executable(Shares_input Shares_input.cpp)
add_executable(hamm hamm.cpp)
add_executable(constMatMul constMatMul.cpp)
add_executable(sigmoid sigmoid.cpp)

find_package(Boost COMPONENTS json log program_options REQUIRED)

target_compile_features(constAdd PRIVATE cxx_std_20)

target_compile_features(constMul PRIVATE cxx_std_20)

target_compile_features(indicator PRIVATE cxx_std_20)

target_compile_features(Shares_input PRIVATE cxx_std_20)

target_compile_features(hamm PRIVATE cxx_std_20)

target_compile_features(constMatMul PRIVATE cxx_std_20)

target_compile_features(sigmoid PRIVATE cxx_std_20)

target_link_libraries(constAdd
    MOTION::motion
    Boost::json
    Boost::log
    Boost::program_options
)

target_link_libraries(constMul
    MOTION::motion
    Boost::json
    Boost::log
    Boost::program_options
)

target_link_libraries(Shares_input
     MOTION::motion
    Boost::json
    Boost::log
    Boost::program_options
)

target_link_libraries(indicator
     MOTION::motion
    Boost::json
    Boost::log
    Boost::program_options
)

target_link_libraries(hamm
     MOTION::motion
    Boost::json
    Boost::log
    Boost::program_options
)

target_link_libraries(constMatMul
     MOTION::motion
    Boost::json
    Boost::log
    Boost::program_options
)

target_link_libraries(sigmoid
     MOTION::motion
    Boost::json
    Boost::log
    Boost::program_options
)
