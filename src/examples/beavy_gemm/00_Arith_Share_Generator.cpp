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
//This file takes clear values as input and generate Arithmatic shares writes to files
//For now hardecoded these path into options,  
//Input File : /home/iudx/Desktop/Haritha/iudx-MOTION2NX-1/build_debwithrelinfo_gcc/3PC_Relu/Arith_Input
//Output Files : "/home/iudx/Desktop/Haritha/iudx-MOTION2NX-1/build_debwithrelinfo_gcc/3PC_Relu/OutputShare_0"
//               "/home/iudx/Desktop/Haritha/iudx-MOTION2NX-1/build_debwithrelinfo_gcc/3PC_Relu/OutputShare_1"
/***********************************************************************************************************/
struct Options {
  std::string NameofImageFile;
  std::string NameofOutputFile;
  std::string fullfilepath;
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
  
  options.NameofImageFile = "Arith_Input";
  options.fullfilepath ="/home/iudx/Desktop/Haritha/iudx-MOTION2NX-1/build_debwithrelinfo_gcc/3PC_Relu/";
  options.fractional_bits = vm["fractional-bits"].as<size_t>();
  options.fullfilepath += options.NameofImageFile;
  options.NameofOutputFile = "/home/iudx/Desktop/Haritha/iudx-MOTION2NX-1/build_debwithrelinfo_gcc/3PC_Relu/OutputShare";
  std::cout << "Image Path: " << options.fullfilepath << "\n";
  // --------------------------------- Input Validation ---------------------------------------//
  if (std::ifstream(options.fullfilepath)) {
    std::cout << "Image file found at : "<< options.fullfilepath << "\n";
  } else {
    std::cout << "No image file found at " << options.fullfilepath << std::endl;
    return std::nullopt;
  }

  return options;
}

template <typename E>
std::uint64_t RandomNumDistribution(E& engine) {
  std::uniform_int_distribution<unsigned long long> distribution(
      std::numeric_limits<std::uint64_t>::min(), std::numeric_limits<std::uint64_t>::max());
  return distribution(engine);
}

int Arith_Share_Generation(std::string file_path, std::string output_file, std::uint64_t fractional_bits ) {
std::ifstream input_file;

std::cout << "Inside Arith_share_generation \n";

  try {
    input_file.open(file_path);
    if (!input_file) {
      std::cerr << "Unable to open the image file.\n";
      throw std::ifstream::failure("Error opening the image file.");
    }
  }
  catch(std::exception& e){
      std::cerr<<"Error during Arithmatic share generation: "<<e.what()<<std::endl;
      return EXIT_FAILURE;
    }
    std::string line;
    int num_r = -1; 
    int num_c = -1;
    int k = 0;
    float temp;
  // reading the first line and getting the number of rows and columns
  try{
    input_file >> num_r >> num_c;
    }
    catch (std::ifstream::failure e) {
      std::cerr << "Error while reading rows and columns from input shares file.\n";
      exit(1);
    }
    if (input_file.eof()) {
      std::cerr << "Input shares file doesn't contain rows and columns" << std::endl;
      exit(1);
    }
  std::cout << "Total num elements : " << num_r*num_c << "\n";
  int num_vals = num_r*num_c;
  std::vector<std::uint64_t> cs0_data(num_vals, 0);
  std::vector<std::uint64_t> cs1_data(num_vals, 0);
  std::vector<float> data(num_vals, 0.0);
  while (k < num_r*num_c) {
      try{    
        input_file >> data[k];
      }
      catch (std::ifstream::failure e) {
      std::cerr << "Error while reading the weight shares.\n";
      exit(1);
      }
      if (input_file.eof()) {
        std::cerr << "Weight shares file contains less number of elements" << std::endl;
        exit(1);
      }
      k++;
    }

    input_file.close();
    if (k != num_r*num_c) 
       std::cout << "The number of elements expeted are : " << num_r*num_c << ", but present in file are : " << k << "\n";
  
  std::cout << "Generating Arith shares and writing into file. \n";
  std::ofstream outdata_0, outdata_1;
  outdata_0.open(output_file+"_0", std::ios_base::out);
  outdata_1.open(output_file+"_1", std::ios_base::out);
  assert(outdata_0);
  assert(outdata_1);
  outdata_0 << num_r << " " << num_c << "\n";
  outdata_1 << num_r << " " << num_c << "\n";
  // Now that we read the data, generating Arithmatic shares
  try {
    for (int i = 0; i < num_r*num_c; i++) {
      std::random_device rd;
      std::mt19937 gen(rd());
      std::uint64_t X0 = RandomNumDistribution(gen);
      std::uint64_t X1 = MOTION::new_fixed_point::encode<uint64_t, long double>(data[i], fractional_bits);
      std::cout << X1 << "\n";
      X1 = X1 - X0;
      cs0_data[i] = X0;
      cs1_data[i] = X1;
      outdata_0 << cs0_data[i] << "\n";
      outdata_1 << cs1_data[i] << "\n";
      std::cout << cs0_data[i] << " " <<  cs1_data[i] << " :" << MOTION::new_fixed_point::decode<uint64_t, float>(X0+X1, fractional_bits) << "\n";
    }
  } catch (std::exception& e) {
    std::cerr << "Error during image share generation: " << e.what() << std::endl;
    return EXIT_FAILURE;
  }
  return 1;
}

int main(int argc, char* argv[]) {
  auto options = parse_program_options(argc, argv);

  if (!options.has_value()) {
    std::cerr << "Error while parsing the given input options.\n";
    return EXIT_FAILURE;
  }
std::cout<< options->fullfilepath << "\n";

Arith_Share_Generation(options->fullfilepath, options->NameofOutputFile, options->fractional_bits);

return EXIT_SUCCESS;
}