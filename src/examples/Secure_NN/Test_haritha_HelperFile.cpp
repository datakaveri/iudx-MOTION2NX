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
// This file writes arithmetic shares a0 and a1 in L world to their respective files 
// and acts as a helper file to test three-party communication between Party 0,
// Party 1 and Party 2 in order to compute ReLU shares in L world at Party 0 and Party 1.
// Data is initialized here and split into arithmetic shares in L world and then passed
// to 3PC_Relu_0, 3PC_Relu_1 and 3PC_Relu_2 to write ReLU shares to the respective files
// at Party 0 and Party 1 and then the output is tested using Test_haritha.cpp by
// reconstructing the shares and verifying the output.
/*****************************************************************************/

//******** START : RandomNumberGenerators *********************** 
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
//******** END : RandomNumberGenerators ***********************

//*********************** START PRIME operations*****************
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

// GenerateSharesOverL: Generates arithmetic shares of vector a in L world and stores them in vectors a0 and a1
// Input  : Empty vectors of type std::uint64_t a0 and a1, vector of type std::uint64_t a to generate shares of, and length of the vector len
// Output : Updates vectors a0 and a1 containing arithmetic shares of a at party 0 and party 1 respectively
void GenerateSharesOverL(std::vector<uint64_t>& a0, std::vector<uint64_t>& a1, std::vector<uint64_t>& a, std::size_t len)
{
std::cout << "Entered GenerateSharesOverL \n";
std::uint64_t t;
std::random_device rd;
std::mt19937 gen(rd());
for(int i = 0; i < len; i++)
  {
    a0[i] = RandomNumDistribution(gen);
    a1[i] = a[i] - a0[i];
  }
std::cout << "Exiting GenerateSharesOverL \n";
}

// WriteArithToFile: Write arithmetic shares into a file specified as second argument
// Input  : Arith shares and file path
// Output : After writing to file succefully returns zero
int WriteArithToFile(std::vector<uint64_t>&pub, std::string file_path)
{
std::ofstream output_file;
std::cout << "##### Entered  WriteArithToFile ...##### \n";
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
  output_file << pub[0] << " " << pub[1] << std::endl;
  for(int i = 2; i < pub.size(); i++)
     {
      output_file << pub[i] << std::endl;
     }
  std::cout << "##### Exiting  WriteArithToFile ##### \n\n";
  return 0;
}

struct Options {
  std::string permutefile;
  std::size_t fractional_bits;
  std::string a_L_shares_0;
  std::string a_L_shares_1;
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
  
  const std::string baseDirectory = (std::string)std::getenv("BASE_DIR");

  options.permutefile = baseDirectory + "/build_debwithrelinfo_gcc/3PC_Relu/permute";
  options.fractional_bits = vm["fractional-bits"].as<size_t>();

  options.a_L_shares_0 = baseDirectory + "/build_debwithrelinfo_gcc/3PC_Relu/X_OutputShare_0";
  options.a_L_shares_1 = baseDirectory + "/build_debwithrelinfo_gcc/3PC_Relu/X_OutputShare_1";

  return options;
}

int main(int argc, char* argv[]) {
  std::default_random_engine engine;
  std::random_device rd;
  std::mt19937 generator(rd());
  InitializeModuloPrimeOps();
  auto options = parse_program_options(argc, argv);
  std::size_t rows = 2;
  std::size_t cols = 20;
  std::size_t len = rows * cols;
  int setLen = 10;

  // Distribution of small random values ranging from -100 to 100
  std::uniform_real_distribution<float> distribution((float)(-100.0), (float)(100.0));
  // Distribution of large integer positive numbers (0 to pow(2, 32) - 1)
  std::uniform_real_distribution<float> distributionRealRositive((float)(generator.min()), (float)(generator.max()));
  // Distribution of integer negative numbers 
  std::uniform_real_distribution<float> distributionRealNegative((float)(std::numeric_limits<short>::min()), (float)(0.0));
  // Distribution of small random values ranging from -2.0 to 2.0
  std::uniform_real_distribution<float> distributionRealSmallValues((float)(-2.0), (float)(2.0));


  std::vector<float> data(len, 0.0);
  std::vector<std::uint64_t> a(len, 0),  a_L_0(len, 0), a_L_1(len, 0);
  // data[0] = 7;
  // data[1] = LMINUS_ONE -1;
  // data[2] = 0.5;
  // data[3] = 0.1;
  // data[4] = -1;
  // data[5] = -7.56;
  // data[6] = 0; 
  // data[7] = 4.58;

  // for (int i = 0; i < len; i++) {
  //   a[i] = MOTION::new_fixed_point::encode<uint64_t, long double>(data[i], 13);
  // }

  for(int i = 0; i < setLen; i++)
  {
    data[i] = distribution(engine);
    a[i] = MOTION::new_fixed_point::encode<uint64_t, long double>(data[i], FIXED_POINT);
    std::cout << "Index " << i << ": " << a[i] << ", data[i]: " << data[i] << std::endl;
  }

  for (int i = setLen; i < 2 * setLen; i++) {
    data[i] = distributionRealRositive(engine);
    a[i] = MOTION::new_fixed_point::encode<uint64_t, long double>(data[i], FIXED_POINT);
    std::cout << "Index " << i << ": " << a[i] << ", data[i]: " << data[i] << std::endl;
  }

  for (int i = 2 * setLen; i < 3 * setLen; i++) {
    data[i] = distributionRealNegative(engine);
    std::cout << "Index " << i << ": " << data[i] << std::endl;
    a[i] = MOTION::new_fixed_point::encode<uint64_t, long double>(data[i], FIXED_POINT);
    std::cout << "Index " << i << ": " << a[i] << ", data[i]: " << data[i] << std::endl;
  }

  for (int i = 3 * setLen; i < len; i++) {
    data[i] = distributionRealSmallValues(engine);
    a[i] = MOTION::new_fixed_point::encode<uint64_t, long double>(data[i], FIXED_POINT);    
    std::cout << "Index " << i << ": " << a[i] << ", data[i]: " << data[i] << std::endl;
  }

  GenerateSharesOverL(a_L_0, a_L_1, a, len);
  a_L_0.insert(a_L_0.begin(), cols);
  a_L_0.insert(a_L_0.begin(), rows);
  a_L_1.insert(a_L_1.begin(), cols);
  a_L_1.insert(a_L_1.begin(), rows);

  WriteArithToFile(a_L_0, options->a_L_shares_0);
  WriteArithToFile(a_L_1, options->a_L_shares_1);
}