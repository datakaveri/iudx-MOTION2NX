
add_executable(weights_provider weights_provider.cpp)

set(REQUIRED_BOOST_VERSION "1.75.0")

find_package(Boost ${REQUIRED_BOOST_VERSION}
COMPONENTS 
json 
log 
program_options 
REQUIRED)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/newB1.csv
    ${CMAKE_CURRENT_BINARY_DIR} COPYONLY)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/newB2.csv
    ${CMAKE_CURRENT_BINARY_DIR} COPYONLY)


configure_file(${CMAKE_CURRENT_SOURCE_DIR}/newW1.csv
    ${CMAKE_CURRENT_BINARY_DIR} COPYONLY)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/newW2.csv
    ${CMAKE_CURRENT_BINARY_DIR} COPYONLY)
    
    
target_compile_features(weights_provider PRIVATE cxx_std_20)


target_link_libraries(weights_provider
    Boost::json
    Boost::log
    Boost::program_options
)
