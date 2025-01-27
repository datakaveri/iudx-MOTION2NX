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
#include "./fixed-point.h"

#define MAX_CONNECT_RETRIES 50
using namespace boost::asio;
namespace po = boost::program_options;
using ip::tcp;
/******************************************************************************************/
//This file Generates the random permute using 1 .. 64 numbers with given seed 
//GenerateRandomPermutation(std::vector<int>& nums, std::uint64_t seed)
//To generate random shares at helper node we need 
//RandomNumOverPrime(E& engine, int l = 0, int pr = 67)
// RandomNumOverOddRing
/*****************************************************************************************/
template <typename E>
std::uint64_t RandomNumDistribution(E& engine) {
  std::uniform_int_distribution<unsigned long long> distribution(
      std::numeric_limits<std::uint64_t>::min(), std::numeric_limits<std::uint64_t>::max());
  return distribution(engine);
}

template <typename E>
std::uint64_t RandomNumOverPrime(E& engine, int l = 0, int pr = 67) {
  std::uniform_int_distribution<unsigned long long> distribution(
      l, pr-1);
  return distribution(engine);
}

template <typename E>
std::uint64_t RandomNumOverOddRing(E& engine, int l = 0, std::uint64_t p = std::numeric_limits<std::uint64_t>::max()-1) {
  std::uniform_int_distribution<unsigned long long> distribution(
      l, p);
  return distribution(engine);
}

struct Options {
  std::string permutefile;
  std::size_t fractional_bits;
};

std::optional<Options> parse_program_options(int argc, char* argv[]) {
  Options options;
  po::options_description desc("Allowed options");
  // clang-format off
  desc.add_options()
    ("help,h", po::bool_switch()->default_value(false),"produce help message")
    ("fractional-bits", po::value<size_t>()->default_value(13), "Number of fractional bits")
    ;
  // clang-format on

  po::variables_map vm;
  po::store(po::parse_command_line(argc, argv, desc), vm);
  bool help = vm["help"].as<bool>();
  if (help) {
    std::cerr << desc << "\n";
    return std::nullopt;
  }
  try {
    po::notify(vm);
  } catch (std::exception& e) {
    std::cerr << "Input parse error:" << e.what() << std::endl;
    return std::nullopt;
  }
  
  
  options.permutefile =  "/home/iudx/Desktop/Haritha/iudx-MOTION2NX-1/build_debwithrelinfo_gcc/3PC_Relu/permute";
  options.fractional_bits = vm["fractional-bits"].as<size_t>();
  return options;
}


void GenerateRandomPermutation(std::vector<int>& nums, std::size_t start = 0, std::size_t end = 0, std::uint64_t seed = 0)  {
    std::mt19937 gen(seed); // Initialize  with the given seed
    std::shuffle(nums.begin()+start, nums.begin()+end+1, gen); // Shuffle the vector with the seeded generator

    std::cout << "A random permutation of the given set is: ";
    for (int num : nums) {
        std::cout << num << " ";
    }
    std::cout << std::endl;
}
int main(int argc, char* argv[]) {
  auto options = parse_program_options(argc, argv);

  if (!options.has_value()) {
    std::cerr << "Error while parsing the given input options.\n";
    return EXIT_FAILURE;
  }

std::vector<int> nums(10);
for(int i = 0; i < nums.size(); i++)
   {
    nums[i] = i;
   }
GenerateRandomPermutation(nums, 2, 6, 12);
// std::random_device rd;
// std::mt19937 gen(rd());
// for (int i = 0; i < 100; i++) {
//     std::uint64_t t = RandomNumOverPrime(gen);
   
//     std:: cout << t << "\n";
//      t = RandomNumOverOddRing(gen);
//      std :: cout << t << "\n";
//     }
return EXIT_SUCCESS;
}