#include <boost/program_options.hpp>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include "compute_server/compute_server.h"

namespace po = boost::program_options;

struct Options {
  int port;
  int my_id;
  std::string currentpath;
};

std::optional<Options> parse_program_options(int argc, char* argv[]) {
  Options options;
  boost::program_options::options_description desc("Allowed options");
  // clang-format off
  desc.add_options()
    ("help,h", po::bool_switch()->default_value(false),"produce help message")
    ("my-id", po::value<std::size_t>()->required(), "my party id")
    ("port" , po::value<int>()->required(), "Port number on which to listen")
    ("current-path",po::value<std::string>()->required(), "current path build_debwithrelinfo")
    ;

  po::variables_map vm;
  po::store(po::parse_command_line(argc, argv, desc), vm);
  bool help = vm["help"].as<bool>();
  if (help) {
    std::cerr << desc << "\n";
    return std::nullopt;
  }
  try {
    po::notify(vm);
  } 
  catch (std::exception& e) {
    std::cerr << "Error in the input arguments:" << e.what() << "\n\n";
    std::cerr << desc << "\n";
    return std::nullopt;
  }

  options.my_id = vm["my-id"].as<std::size_t>();
  options.port = vm["port"].as<int>();
  options.currentpath = vm["current-path"].as<std::string>();

  // ----------------- Input Validation ----------------------------------------//
  if (options.my_id > 1) {
    std::cerr << "my-id must be 0 or 1\n";
    return std::nullopt;
  }
  // Check if the port numbers are within the valid range (1-65535)
  if ((options.port < 1) || (options.port > std::numeric_limits<unsigned short>::max())) {
      std::cerr<<"Invalid port "<<options.port<<".\n";
      return std::nullopt;  // Out of range
  }
  //Check if currentpath directory exists
  if(!std::filesystem::is_directory(options.currentpath))
    {
      std::cerr<<"Directory ("<<options.currentpath<<") does not exist.\n";
      return std::nullopt;
    }
//---------------------------------------------------------------------------------//

return options;
}

int retrive_layer_number(Options& options)
{
  int number_of_layers=COMPUTE_SERVER::read_number_of_layers_constmodel(options.port);
  
  std::cout<<"number_of_layers:"<<number_of_layers<<"\n";

  std::string path= options.currentpath+"/no_of_layers.txt";
  std::cout<<path<<"\n";
  std::ofstream file;
  try{
  file.open(path,std::ios_base::out);
      if (file) {
      std::cout << "output (no_of_layers) file found\n";
    } else {
      std::cout << "outputshare (no_of_layers) file not found\n";
    }
  } catch (std::ifstream::failure e) {
    std::cerr << "Error while opening the no_of_layers.txt file.\n";
    return EXIT_FAILURE;
  }

  file<<number_of_layers<<"\n";
  file.close();
}

int main(int argc, char* argv[])
{
    auto options=parse_program_options(argc,argv);
      if (!options.has_value()) {
    std::cerr<<"Error while parsing the given input options.\n";
    return EXIT_FAILURE;
  }

  try{
      retrive_layer_number(*options);
  }
  catch(std::exception& e){
      std::cerr<<"Error:"<<e.what();
      return EXIT_FAILURE;
  }


  return EXIT_SUCCESS;
}
