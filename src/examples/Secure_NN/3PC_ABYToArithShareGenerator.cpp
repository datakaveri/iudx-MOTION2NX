// At Server 0:
// ./bin/3PC_ABYToArithShareGenerator --my_id 0 --filepath ${BASE_DIR}/build_debwithrelinfo_gcc/3PC_Relu/X_OutputShare_0 --current-path ${BASE_DIR}/build_debwithrelinfo_gcc

// At Server 1:
// ./bin/3PC_ABYToArithShareGenerator --my_id 1 --filepath ${BASE_DIR}/build_debwithrelinfo_gcc/3PC_Relu/X_OutputShare_1 --current-path ${BASE_DIR}/build_debwithrelinfo_gcc

#include <boost/archive/text_oarchive.hpp>
#include <boost/asio.hpp>
#include <boost/chrono.hpp>
#include <boost/program_options.hpp>
#include <boost/serialization/string.hpp>
#include <boost/thread.hpp>
#include <cstdint>
#include <chrono>
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
using namespace std::chrono;
//%%%%%%%%%%%%%%%%% END GLOBAL VARIABLES %%%%%%%%%%%%%%%%%%%%%%%%%

/*******************************************************************************/
// This file converts ABY2.0 shares (public shares, private shares @ Party 0 and private shares @ Party 1)
// to arithmetic shares (Arith_Shares_0 and Arith_Shares_1) and writes these vectors to their respective files 
// and acts as a helper file to test three-party communication between Party 0,
// Party 1 and Party 2 in order to compute ReLU shares in L world at Party 0 and Party 1.
// ABY2.0 shares of x, stored in clear in 10_ABYToArithShareGenerator are read from their respective filepaths are
// converted to arithmetic shares using RandomABYGenerator and written to the respective filepaths to be
// utilized by Party 0 and Party 1 to execute three-party secure computation for ReLU.
/*****************************************************************************/

//******** START : RandomNumberGenerators*********************** 
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
//******** END : RandomNumberGenerators***********************
//*********************** START PRIME operations*****************
// Initializes the addition, subtraction and multiplication tables over modulo prime
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

// ReadABYSharesIntoVec: Reads ABY2.0 shares (public and private shares) from file_path into a vector.
// Input  : Filepath containing ABY2.0 shares, two empty vectors of type std::uint64_t publicShares and privateShares and dimensions of the matrix (rows and cols).
// Output : Neglects the first two values that is the dimensions and reads the shares into the vector.
int ReadABYSharesIntoVec(std::string file_path, std::vector<uint64_t>&publicShares, std::vector<std::uint64_t>& privateShares, int& rows, int& cols)
{
std::ifstream input_file;
std::cout << "##### Entered ReadABYSharesIntoVec ...##### \n";

  try {
    input_file.open(file_path);
    if (!input_file) {
      std::cerr << "Unable to open ABY share file.\n";
      throw std::ifstream::failure("Error opening ABY share file.");
    }
  }
  catch(std::exception& e){
      std::cerr<<"Error during opening ABY share file: "<<e.what()<<std::endl;
      return EXIT_FAILURE;
  }

  int num_r = 0; 
  int num_c = 0;
  
  // Reading the first line and updating the number of rows and columns
  try {
    input_file >> num_r >> num_c;
    rows = num_r;
    cols = num_c;
  }
  catch (std::ifstream::failure e) {
    std::cerr << "Error while reading dimensions: rows and columns from input shares file.\n";
    exit(1);
  }
  if (input_file.eof()) {
    std::cerr << "Input shares file doesn't contain rows and columns." << std::endl;
    exit(1);
  }

  std::cout << "Total number of elements: " << num_r * num_c << "\n";
  int num_vals = num_r * num_c;
  publicShares.resize(num_vals);
  privateShares.resize(num_vals);

  std::uint64_t tempPublic, tempPrivate;
  int k = 0;

  while (k < num_r * num_c) {
      try {    
        input_file >> tempPublic >> tempPrivate;
        publicShares[k] = tempPublic;
        privateShares[k] = tempPrivate;
      }
      catch (std::ifstream::failure e) {
      std::cerr << "Error while reading the ABY shares.\n";
      exit(1);
      }
      if (input_file.eof()) {
        std::cerr << "ABY shares file contains less number of elements" << std::endl;
        exit(1);
      }
      k++;
    }
  std::cout << "Number of elements read into public shares vector: " << publicShares.size() << "\n";
  std::cout << "Number of elements read into private shares vector: " << privateShares.size() << "\n";
  input_file.close();
  if (k != num_r * num_c) 
    std::cout << "The number of elements expeted are: " << num_r * num_c << ", but the elements present in the file are: " << k << "\n";
  std :: cout << "##### Exiting ReadABYSharesIntoVec ##### \n\n";
  return 1;
}

// WriteArithToFile : Write shares into a file specified as second argument
// Input  : Arith shares and file path, (rows and columns) dimensions of the matrix
// Output : After writing to file succefully returns zero
int WriteArithToFile(std::vector<uint64_t>&pub, std::string file_path, std::size_t rows, std::size_t cols)
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
  output_file << rows << " " << cols << std::endl;
  for(int i = 0; i < pub.size(); i++)
     {
      output_file << pub[i] << std::endl;
     }
  std::cout << "##### Exiting  WriteArithToFile ##### \n\n";
  return 0;
}

// ABYToArithShareGenerator: Converts ABY2.0 shares to arithmetic shares
// Arithmetic share @ P0: (Public share / 2) - Private Share @ P0
// Arithmetic share @ P1: (Public share / 2) - Private Share @ P1
// Input  : Three vectors of type std::uint64_t Public_shares and private_shares representing ABY2.0 shares and an empty vector ArithShares of type std::uint64_t
// Output : Updates ArithShares to store arithmetic shares
void ABYToArithShareGenerator(std::vector<std::uint64_t>& Public_shares, std::vector<std::uint64_t>& private_shares, std::vector<std::uint64_t>& ArithShares) {
  std::cout << "Entering ABY2.0ToArithShareGenerator." << std::endl;

  std::size_t len = Public_shares.size();
  ArithShares.resize(len);

  for (int i = 0; i < len; i++) {
    ArithShares[i] = (Public_shares[i] >> 1) - private_shares[i];
  }

  std::cout << "Exiting ABY2.0ToArithShareGenerator.\n" << std::endl;
  return;
}

void testMemoryOccupied(int WriteToFiles, int my_id, std::string path) {
  int tSize = 0, resident = 0, share = 0;
  std::ifstream buffer("/proc/self/statm");
  buffer >> tSize >> resident >> share;
  buffer.close();

  long page_size_kb =
      sysconf(_SC_PAGE_SIZE) / 1024;  // in case x86-64 is configured to use 2MB pages
  double rss = resident * page_size_kb;
  std::cout << "RSS - " << rss << " kB\n";
  double shared_mem = share * page_size_kb;
  std::cout << "Shared Memory - " << shared_mem << " kB\n";
  std::cout << "Private Memory - " << rss - shared_mem << "kB\n";
  std::cout << std::endl;
  if (WriteToFiles == 1) {
    // Generate path for the AverageMemoryDetails file and MemoryDetails file
    std::string t1 = path + "/" + "AverageMemoryDetails" + std::to_string(my_id);
    std::string t2 = path + "/" + "MemoryDetails" + std::to_string(my_id);

    // Write to the AverageMemoryDetails files
    std::ofstream file1;
    file1.open(t1, std::ios_base::app);
    file1 << rss;
    file1 << "\n";
    file1.close();

    std::ofstream file2;
    file2.open(t2, std::ios_base::app);
    file2 << "10_ABYToArithShareGenerator: \n";
    file2 << "RSS - " << rss << " kB\n";
    file2 << "Shared Memory - " << shared_mem << " kB\n";
    file2 << "Private Memory - " << rss - shared_mem << "kB\n";
    file2.close();
  }
}

struct Options {
  int my_id;
  std::string permutefile;
  std::string current_path;
  std::size_t fractional_bits;
  std::string X_ABY_Shares;
  std::string X_Arith_Shares;
};

std::optional<Options> parse_program_options(int argc, char* argv[]) {
  Options options;
  po::options_description desc("Allowed options");
  // clang-format off
  desc.add_options()
    ("help,h", po::bool_switch()->default_value(false),"produce help message")
    ("fractional-bits", po::value<size_t>()->default_value(13), "Number of fractional bits")
    ("my_id", po::value<std::size_t>()->required(), "My id")
    ("filepath", po::value<std::string>()->required(), "File containing ABY Shares")
    ("current-path", po::value<std::string>()->required(), "Current build path")
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

  options.my_id = vm["my_id"].as<std::size_t>();
  options.current_path = vm["current-path"].as<std::string>();
  options.X_ABY_Shares = vm["filepath"].as<std::string>();
  options.X_Arith_Shares = baseDirectory + "/build_debwithrelinfo_gcc/server" + std::to_string(options.my_id) + "/outputshare_" + std::to_string(options.my_id);
  options.permutefile = baseDirectory + "/build_debwithrelinfo_gcc/3PC_Relu/permute";
  options.fractional_bits = vm["fractional-bits"].as<size_t>();
  return options;
}

int main(int argc, char* argv[]) {
  auto start = high_resolution_clock::now();
  auto options = parse_program_options(argc, argv);
  if (!options.has_value()) {
    std::cerr << "No options given.\n";
    return EXIT_FAILURE;  
  }
  int rows, cols;
  std::vector<std::uint64_t> X_public, X_private;

  ReadABYSharesIntoVec(options->X_ABY_Shares, X_public, X_private, rows, cols);
  std::size_t len = rows * cols;

  std::vector<std::uint64_t> X_Arith_Shares;
  InitializeModuloPrimeOps();

  // Convert ABY2.0 shares to arithmetic shares at each party and write the shares to their respective filepaths.
  ABYToArithShareGenerator(X_public, X_private, X_Arith_Shares);
  WriteArithToFile(X_Arith_Shares, options->X_Arith_Shares, rows, cols);

  std::cout << "\n";
  std::cout << "**************************************************************\n"; 
  testMemoryOccupied(1, options->my_id, options->current_path);
  auto stop = high_resolution_clock::now();
  auto duration = duration_cast<milliseconds>(stop - start);

  std::string timeDetails = options->current_path + "/AverageTimeDetails" + std::to_string(options->my_id);
  std::string memoryDetails = options->current_path + "/MemoryDetails" + std::to_string(options->my_id);

  std::cout << "Duration: " << duration.count() << std::endl;

  std::ofstream timeDetailsFile;
  timeDetailsFile.open(timeDetails, std::ios_base::app);
  if (!timeDetailsFile.is_open()) {
    std::cerr << "Error: Unable to open TimeDetails file." << std::endl;
  } else {
    timeDetailsFile << duration.count() << std::endl;
  }
  timeDetailsFile.close();

  return EXIT_SUCCESS;
}