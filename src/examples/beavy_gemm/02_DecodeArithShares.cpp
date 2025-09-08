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
//This file reads Arithmetic values as input and generate clear
//For now hardecoded these path into options,  
//Input File : /home/iudx/Desktop/Haritha/iudx-MOTION2NX-1/build_debwithrelinfo_gcc/3PC_Relu/Z_Arith_shares_0
//             /home/iudx/Desktop/Haritha/iudx-MOTION2NX-1/build_debwithrelinfo_gcc/3PC_Relu/Z_Arith_shares_1
//Output Files : "/home/iudx/Desktop/Haritha/iudx-MOTION2NX-1/build_debwithrelinfo_gcc/3PC_Relu/OutputShare_0"
//               "/home/iudx/Desktop/Haritha/iudx-MOTION2NX-1/build_debwithrelinfo_gcc/3PC_Relu/OutputShare_1"
/***********************************************************************************************************/
struct Options {
  std::string X0;
  std::string X1;
  
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
  
  options.X0 ="/home/iudx/Desktop/Haritha/iudx-MOTION2NX-1/build_debwithrelinfo_gcc/3PC_Relu/Z_Arith_shares_0";
  options.fractional_bits = vm["fractional-bits"].as<size_t>();
  
  options.X1 = "/home/iudx/Desktop/Haritha/iudx-MOTION2NX-1/build_debwithrelinfo_gcc/3PC_Relu/Z_Arith_shares_1";
  // --------------------------------- Input Validation ---------------------------------------//
  return options;
}

template <typename E>
std::uint64_t RandomNumDistribution(E& engine) {
  std::uniform_int_distribution<unsigned long long> distribution(
      std::numeric_limits<std::uint64_t>::min(), std::numeric_limits<std::uint64_t>::max());
  return distribution(engine);
}


int ArithToClearCompute(std::string p0, std::string p1)
{
    std::cout << "Inside ArithToClearCompute \n";
    std::ifstream x0_file, x1_file;

  try {
    x0_file.open(p0);
    if (!x0_file) {
      std::cerr << "Unable to open X0 arith share file.\n";
      throw std::ifstream::failure("Error opening the image file.");
    }
  }
  catch(std::exception& e){
      std::cerr<<"Error during ArithToClearCompute: "<<e.what()<<std::endl;
      return EXIT_FAILURE;
    }

    try {
    x1_file.open(p1);
    if (!x1_file) {
      std::cerr << "Unable to open X1 arith share file.\n";
      throw std::ifstream::failure("Error opening the image file.");
    }
  }
  catch(std::exception& e){
      std::cerr<<"Error during ArithToClearCompute: "<<e.what()<<std::endl;
      return EXIT_FAILURE;
    }
    std::string line;
    int num_r = -1; 
    int num_c = -1;
    int k = 0;
    float temp;
  // reading the first line and getting the number of rows and columns
  try{
    x0_file >> num_r >> num_c;
    x1_file >> num_r >> num_c;
    }
    catch (std::ifstream::failure e) {
      std::cerr << "Error while reading rows and columns from input shares file.\n";
      exit(1);
    }
  std::cout << "Total num elements : " << num_r*num_c << "\n";
  int num_vals = num_r*num_c;
  std::vector<std::uint64_t> data_0(num_vals, 0.0), data_1(num_vals, 0.0);
  while (k < num_r*num_c) {
      try{    
        x0_file >> data_0[k];
        x1_file >> data_1[k];
        //std::cout << " x0 : " << data_0[k] << ", x1 : " << data_1[k] << "\n";
      }
      catch (std::ifstream::failure e) {
      std::cerr << "Error while reading Arith shares.\n";
      exit(1);
      }
      auto y = MOTION::new_fixed_point::decode<std::uint64_t, float>(data_0[k] + data_1[k], 13);
      std :: cout << k << " : " << y << "\n";
      if (x0_file.eof() || x1_file.eof()) {
        std::cerr << "Arith shares file contains less number of elements" << std::endl;
        exit(1);
      }
      k++;
    }

    x0_file.close();
    x1_file.close();
    
 return 0;

}

int main(int argc, char* argv[]) {
  auto options = parse_program_options(argc, argv);

  if (!options.has_value()) {
    std::cerr << "Error while parsing the given input options.\n";
    return EXIT_FAILURE;
  }

ArithToClearCompute(options->X0, options->X1);


return EXIT_SUCCESS;
}