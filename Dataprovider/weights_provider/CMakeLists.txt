
add_executable(weights_provider weights_provider.cpp)
add_executable(weights_provider_remote weights_provider_remote.cpp)

set(REQUIRED_BOOST_VERSION "1.75.0")

find_package(Boost ${REQUIRED_BOOST_VERSION}
COMPONENTS 
json 
log 
program_options 
REQUIRED)
target_compile_features(weights_provider PRIVATE cxx_std_20)
target_compile_features(weights_provider_remote PRIVATE cxx_std_20)


target_link_libraries(weights_provider
    Boost::json
    Boost::log
    Boost::program_options
)

target_link_libraries(weights_provider_remote
    Boost::json
    Boost::log
    Boost::program_options
)
