# add_executable(image_provider image_provider.cpp)
add_executable(image_provider_iudx image_provider_iudx.cpp)
add_executable(image_provider_CNN image_provider_CNN.cpp)
add_executable(output_shares_receiver output_shares_receiver.cpp)
add_executable(Reconstruct Reconstruct.cpp)
add_executable(layerno_provider layerno_provider.cpp)

set(REQUIRED_BOOST_VERSION "1.75.0")

find_package(Boost ${REQUIRED_BOOST_VERSION}
COMPONENTS 
json 
log 
program_options 
REQUIRED)

target_compile_features(image_provider_iudx PRIVATE cxx_std_20)
target_link_libraries(image_provider_iudx
    Boost::json
    Boost::log
    Boost::program_options
)

target_compile_features(image_provider_CNN PRIVATE cxx_std_20)
target_link_libraries(image_provider_CNN
    Boost::json
    Boost::log
    Boost::program_options
)

# target_compile_features(image_provider PRIVATE cxx_std_20)

# target_link_libraries(image_provider
#     Boost::json
#     Boost::log
#     Boost::program_options
# )


target_compile_features(output_shares_receiver PRIVATE cxx_std_20)

target_link_libraries(output_shares_receiver
    Boost::json
    Boost::log
    Boost::program_options
)

target_compile_features(Reconstruct PRIVATE cxx_std_20)

target_link_libraries(Reconstruct
    Boost::json
    Boost::log
    Boost::program_options
)

target_compile_features(layerno_provider PRIVATE cxx_std_20)

target_link_libraries(layerno_provider
    Boost::json
    Boost::log
    Boost::program_options
)

