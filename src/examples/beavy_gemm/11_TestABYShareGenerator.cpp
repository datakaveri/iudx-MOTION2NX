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
#include <parallel/algorithm>
#include <random>
#include <stdexcept>
#include <type_traits>
#include <vector>
#include "./fixed-point.h"

using namespace boost::asio;
namespace po = boost::program_options;
using ip::tcp;
typedef std::uint64_t myType;
const myType minus_one = (myType) - 1;
#define MAX_CONNECT_RETRIES 50

//%%%%%%%%%%%%%%%%% START GLOBAL VARIABLES %%%%%%%%%%%%%%%%%%%%%%%
const auto LMINUS_ONE = std::numeric_limits<std::uint64_t>::max();
const uint64_t PRIME_NUM = 67;
uint64_t addModPrime[PRIME_NUM][PRIME_NUM];
uint64_t multModPrime[PRIME_NUM][PRIME_NUM];
uint64_t subModPrime[PRIME_NUM][PRIME_NUM];
const auto BIT_SIZE = 64;
const auto FIXED_POINT = 13;
//%%%%%%%%%%%%%%%%% END GLOBAL VARIABLES %%%%%%%%%%%%%%%%%%%%%%%%%

/*******************************************************************************/
// This file writes ABY2.0 shares PublicShares, PrivateShares_0 and PrivateShares_1 to their respective files 
// and acts as a helper file to test three-party communication between Party 0,
// Party 1 and Party 2 in order to compute ReLU shares in L world at Party 0 and Party 1.
// Data (x and y) is stored in clear, converted to ABY2.0 shares using RandomABYGenerator and written
// to the respective filepaths to be added to Party 0 and Party 1 and then passed to 
// 10_ABYToArithShareGenerator to convert these shares to arithmetic shares to execute three-party secure computation.
/*****************************************************************************/

// ******** START : RandomNumberGenerators*********************** 
template <typename E>
std::uint64_t RandomNumDistribution(E& engine) {
  std::uniform_int_distribution<unsigned long long> distribution(
      std::numeric_limits<std::uint64_t>::min(), std::numeric_limits<std::uint64_t>::max());
  return distribution(engine);
}

template <typename E>
std::uint64_t RandomNumOverPrime(E& engine, int l = 0, int pr = PRIME_NUM) {
  std::uniform_int_distribution<unsigned long long> distribution(
      l, pr-1);
  return distribution(engine);
}
template <typename E>
std::uint64_t RandomNonZeroNumOverPrime(E& engine, int l = 1, int pr = PRIME_NUM) {
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
// ******** END : RandomNumberGenerators***********************

// *********************** START PRIME operations*****************
// InitializeModuloPrimeOps: Initializes the addition, subtraction and multiplication tables over modulo prime
// a + b, a - b, a * b ; a,b {0,1, ... ,PRIME_NUM -1 }
void InitializeModuloPrimeOps()
{
  for (int i = 0; i< PRIME_NUM;  i++)
  {
    for (int j = 0; j< PRIME_NUM;  j++)
    {
     addModPrime[i][j] = (i+j) % PRIME_NUM;
     multModPrime[i][j] = (i*j) % PRIME_NUM;
    }
  }
  for (int i = 0; i< PRIME_NUM;  i++)
  {
    for (int j = 0; j< PRIME_NUM;  j++)
    {
     if (j == 0) subModPrime[i][j] = i; 
     else 
     {
      auto temp = PRIME_NUM - j;
      subModPrime[i][j] = addModPrime[i][temp]; 
     }
    }
  }

}
inline std::uint64_t AddModuloPrime(std::uint64_t a, std::uint64_t b)
{return addModPrime[a][b];}
inline std::uint64_t MultiplyModuloPrime(std::uint64_t a, std::uint64_t b)
{return multModPrime[a][b];}
inline std::uint64_t SubtractModuloPrime(std::uint64_t a, std::uint64_t b)
{return subModPrime[a][b];}
//*********************** END PRIME operations*****************

// WriteABYToFile: Write shares into a file specified as second argument
// Input  : Public and private shares shares and file path, rows and columns (dimensions) of the matrix
// Output : After writing to file, successfully returns zero
int WriteABYToFile(std::vector<uint64_t>&publicShares, std::vector<std::uint64_t>& privateShares, std::string file_path, std::size_t rows, std::size_t cols)
{
std::ofstream output_file;
std::cout << "##### Entered WriteABYToFile ...##### \n";
  try {
    output_file.open(file_path);
    if (!output_file) {
      std::cerr << "Unable to open file to write ABY.\n";
      throw std::ifstream::failure("Error opening ABY share file to write.");
    }
  }
  catch(std::exception& e){
      std::cerr<<"Error during opening ABY share file: "<<e.what()<<std::endl;
      return EXIT_FAILURE;
    }
  output_file << rows << " " << cols << std::endl;
  for(int i = 0; i < publicShares.size(); i++)
     {
      output_file << publicShares[i] << " " << privateShares[i] << std::endl;
     }
  std::cout << "##### Exiting WriteABYToFile ##### \n\n";
  return 0;
}

// RandomABYGenerator: Generates ABY2.0 shares (public shares, private shares at P0 and private shares at P1 )from values in clear
// Input  : Vector of type std::uint64_t a storing values in clear, and empty vectors PublicShares, PrivateShares_0 and PrivateShares_1 of type std::uint64_t
// Output : Updates PublicShares, PrivateShares_0 and PrivateShares_1 to store ABY2.0 shares
void RandomABYGenerator(std::vector<std::uint64_t>& a, std::vector<std::uint64_t>& PublicShares, std::vector<std::uint64_t>& PrivateShares_0, std::vector<std::uint64_t>& PrivateShares_1) {
  std::cout << "Entering RandomABYGenerator." << std::endl;
  std::size_t len = a.size();
  std::random_device rd;
  std::mt19937 gen(rd());
  for (int i = 0; i < len; i++) {
    PrivateShares_0[i] = RandomNumDistribution(gen);  // Shares generated in L world, L = pow(2, 64)
    PrivateShares_1[i] = RandomNumDistribution(gen);
    PublicShares[i] = a[i] + PrivateShares_0[i] + PrivateShares_1[i];
  }
  std::cout << "Exiting RandomABYGenerator.\n" << std::endl;
  return;
}

struct Options {
  std::string permutefile;
  std::size_t fractional_bits;
  std::string ABY_Shares_0;
  std::string ABY_Shares_1;
  std::string ClearValues;
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

  const std::string baseDirectory = (std::string)std::getenv("BASE_DIR");
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

  options.ABY_Shares_0 = baseDirectory + "/build_debwithrelinfo_gcc/3PC_Relu/X_OutputShare_0";
  options.ABY_Shares_1 = baseDirectory + "/build_debwithrelinfo_gcc/3PC_Relu/X_OutputShare_1";
  options.ClearValues = baseDirectory + "/build_debwithrelinfo_gcc/3PC_Relu/ClearValues";
  
  options.permutefile =  baseDirectory + "/build_debwithrelinfo_gcc/3PC_Relu/permute";
  options.fractional_bits = vm["fractional-bits"].as<size_t>();
  return options;
}

int main(int argc, char* argv[]) {
  auto options = parse_program_options(argc, argv);
  if (!options.has_value()) {
    std::cerr<<"No options given.\n";
    return EXIT_FAILURE;
  }
  std::random_device rd;
  std::mt19937 gen(rd());

  // Initialize dimensions of test data here
  int rows = 32768;
  int cols = 1;
  std::size_t len = rows * cols;

  std::vector<float> data(len, 0.0);
  std::vector<std::uint64_t> a(len, 0);
  InitializeModuloPrimeOps();

  for (int i = 0; i < len; i++) {
    data[i] = (i - len * 0.5) / 2;
  }

  for (int i = 0; i < len; i++) {
    a[i] = MOTION::new_fixed_point::encode<uint64_t, long double>(data[i], FIXED_POINT);
  }

  std::vector<std::uint64_t> PublicShares(len, 0), PrivateShares_0(len, 0), PrivateShares_1(len, 0);
  RandomABYGenerator(a, PublicShares, PrivateShares_0, PrivateShares_1);
  WriteABYToFile(PublicShares, PrivateShares_0, options->ABY_Shares_0, rows, cols);
  WriteABYToFile(PublicShares, PrivateShares_1, options->ABY_Shares_1, rows, cols);

  std::ofstream ClearValuesFile;
  try {
    ClearValuesFile.open(options->ClearValues);
    if (!ClearValuesFile) {
      std::cerr << "Error: Unable to write values to clear values file." << std::endl;
    }
  }
  catch (std::exception& e) {
    std::cerr << "Error: Unable to write values to clear values file." << std::endl; 
  }

  ClearValuesFile << len << std::endl;

  for (int i = 0; i < len; i++) {
    // PublicShares[i] = x + PrivateShares_0[i] + PrivateShares_1[i] 
    ClearValuesFile << data[i] << std::endl;

    auto ReconstructedData = PublicShares[i] - PrivateShares_0[i] - PrivateShares_1[i];
    auto tempReconstructedData = MOTION::new_fixed_point::decode<uint64_t, long double>(ReconstructedData, FIXED_POINT);

    // Verifying output
    std::cout << "Data at index " << i << ": " << data[i] << ", After reconstruction: " << tempReconstructedData << std::endl;
    std::cout << "Public shares: " << PublicShares[i] << ", Private shares at P0: " << PrivateShares_0[i] << ", Private shares at Party 1: " << PrivateShares_1[i] << std::endl;

    std::cout << "\n";
  }

  ClearValuesFile.close();

  return EXIT_SUCCESS;
}