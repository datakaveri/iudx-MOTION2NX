#include <boost/archive/text_oarchive.hpp>
#include <boost/asio.hpp>
#include <boost/chrono.hpp>
#include <boost/program_options.hpp>
#include <boost/serialization/string.hpp>
#include <boost/thread.hpp>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <random>
#include <stdexcept>
#include <type_traits>
#include <vector>
#include "my_functions.h"
int main()
{
    std::cout << " In main \n";
    std::cout << f1(0, 4) << "\n";
    std::cout << f1(1, 2) << "\n";
    printValue(10);
}