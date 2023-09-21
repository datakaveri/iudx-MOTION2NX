add_executable(lg_training training.cpp)
add_executable(lg10_training training_all_labels.cpp)

find_package(Boost COMPONENTS json log program_options REQUIRED)

target_compile_features(lg_training PRIVATE cxx_std_20)
target_compile_features(lg10_training PRIVATE cxx_std_20)

target_link_libraries(lg_training
    MOTION::motion
    Boost::json
    Boost::log
    Boost::program_options
)

target_link_libraries(lg10_training
    MOTION::motion
    Boost::json
    Boost::log
    Boost::program_options
)