//./bin/03_PrimeShareGenerator

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
/***********************************************************************************************************/
//This file reads clear values (input) and generate prime shares of each bit in the 64 bit integer in
//For now hardecoded these path into options,  
//Input File : /home/iudx/Desktop/Haritha/iudx-MOTION2NX-1/build_debwithrelinfo_gcc/3PC_Relu/XClear
//             
//Output Files : "/home/iudx/Desktop/Haritha/iudx-MOTION2NX-1/build_debwithrelinfo_gcc/3PC_Relu/XPrimeShare_0"
//               "/home/iudx/Desktop/Haritha/iudx-MOTION2NX-1/build_debwithrelinfo_gcc/3PC_Relu/XPrimeShare_1"
/***********************************************************************************************************/
struct Options {
  std::string X0_PrimeShare;
  std::string X1_PrimeShare;
  std::string X_Clear;
  std::size_t fractional_bits;
};

std::optional<Options> parse_program_options(int argc, char* argv[]) {
  Options options;
  po::options_description desc("Allowed options");
  // clang-format off
  desc.add_options()
    ("help,h", po::bool_switch()->default_value(false),"produce help message")
    
    ("fractional-bits", po::value<size_t>()->default_value(13), "Number of fractional bits")
    ("filepath", po::value<std::string>()->default_value("Arith_Input"), "Name of the image file for which shares should be created")
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
  
  options.X0_PrimeShare = "/home/iudx/Desktop/Haritha/iudx-MOTION2NX-1/build_debwithrelinfo_gcc/3PC_Relu/XPrimeShare_0";
  options.X1_PrimeShare = "/home/iudx/Desktop/Haritha/iudx-MOTION2NX-1/build_debwithrelinfo_gcc/3PC_Relu/XPrimeShare_1";
  options.X_Clear = "/home/iudx/Desktop/Haritha/iudx-MOTION2NX-1/build_debwithrelinfo_gcc/3PC_Relu/XClear";
  options.fractional_bits = vm["fractional-bits"].as<size_t>();

  // --------------------------------- Input Validation ---------------------------------------//
  return options;
}

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


int PrimeShareGenerator(std::string p0, std::string p1, std::string p2, std::uint64_t p = 67)
{
    std::cout << "Inside PrimeShareGenerator .. \n";
    std::ifstream x_file;
    std::ofstream x0_prime, x1_prime;

  try {
    x_file.open(p0);
    if (!x_file) {
      std::cerr << "Unable to open XClear file.\n";
      throw std::ifstream::failure("Error opening the X_clear file.");
    }
  }
  catch(std::exception& e){
      std::cerr<<"Error during PrimeShareGenerator: "<<e.what()<<std::endl;
      return EXIT_FAILURE;
    }

  try {
    x0_prime.open(p1);
    if (!x_file) {
      std::cerr << "Unable to open X0_prime file.\n";
      throw std::ifstream::failure("Error opening X0_prime file.");
    }
  }
  catch(std::exception& e){
      std::cerr<<"Error during  PrimeShareGenerator: "<<e.what()<<std::endl;
      return EXIT_FAILURE;
    }
  try {
    x1_prime.open(p2);
    if (!x_file) {
      std::cerr << "Unable to open X1_prime file.\n";
      throw std::ifstream::failure("Error opening X1_prime file.");
    }
  }
  catch(std::exception& e){
      std::cerr<<"Error during  PrimeShareGenerator: "<<e.what()<<std::endl;
      return EXIT_FAILURE;
    }
  

    std::string line;
    int num_r = -1; 
    int num_c = -1;
    int k = 0;
  // reading the first line and getting the number of rows and columns
  try{
    x_file >> num_r >> num_c;
    }
    catch (std::ifstream::failure e) {
      std::cerr << "Error while reading rows and columns from input shares file.\n";
      exit(1);
    }
  x0_prime << num_r << " " << num_c << "\n";
  x1_prime << num_r << " " << num_c << "\n";
  std::cout << "Total num elements : " << num_r*num_c << "\n";
  int num_vals = num_r*num_c;
  int num_bits = 64;
  std::vector<std::uint64_t> data_0(num_vals*num_bits, 0), data_1(num_vals*64, 0);
  std::uint64_t temp;
  std::random_device rd;
  std::mt19937 gen(rd());
  
  while (k < num_r*num_c) {
      try{    
        x_file >> temp;
        //std::cout << " temp : " << temp << "\n";
      }
      catch (std::ifstream::failure e) {
      std::cerr << "Error while reading Clear values.\n";
      exit(1);
      }
      
      if (x_file.eof() ) {
        std::cerr << "Clear vale file contains less number of elements" << std::endl;
        exit(1);
      }
      for(int i = 0; i < num_bits; i++)
      {
        int bit = (temp>>i) & 1;
        auto t = RandomNumOverPrime(gen);
        data_0[(k*num_bits) + i] = t;
        data_1[(k*num_bits) + i] = p - data_0[(k*num_bits) + i] + bit;
        x0_prime << data_0[(k*num_bits) + i] << "\n";
        x1_prime << data_1[(k*num_bits) + i] << "\n";
        std::cout << i << " : " << data_0[(k*num_bits) + i] << " , " << data_1[(k*num_bits) + i] << ", " << bit << "\n";
      }
      // std::cout << " Prime Shares for value : " << temp << "\n";
      // for(int i = 0; i < num_bits; i++)
      // {
      //   int bit = (temp>>i) & 1;
      //   std::cout << i << " : " << (data_0[(k*num_bits) + i] + data_1[(k*num_bits) + i]) % p <<" , " << bit << "\n";
      // }
      k++;
    }

    x_file.close();
 return 0;
}
int main(int argc, char* argv[]) {
  auto options = parse_program_options(argc, argv);

  if (!options.has_value()) {
    std::cerr << "Error while parsing the given input options.\n";
    return EXIT_FAILURE;
  }

PrimeShareGenerator(options->X_Clear, options->X0_PrimeShare, options->X1_PrimeShare);

return EXIT_SUCCESS;
}