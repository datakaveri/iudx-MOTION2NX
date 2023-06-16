// MIT License
//
// Copyright (c) 2021 Lennart Braun
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR I N CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include <dirent.h>
// #include <algorithm>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <random>
// #include <regex>
#include <stdexcept>
#include <thread>

#include <boost/log/trivial.hpp>
#include <boost/program_options.hpp>

#include "compute_server/compute_server.h"
#include "utility/logger.h"
#include "utility/fixed_point.h"

namespace po = boost::program_options;

struct Matrix {
  std::vector<uint64_t> Delta;
  std::vector<uint64_t> delta;
  int row;
  int col;
};

struct Options {
  // Matrix image;
  std::size_t fractional_bits;
  std::size_t my_id;
  std::string filenames;
  std::vector<std::string> data;
  std::vector<std::string> filepaths;
  std::string currentpath;
  int port;
};

void read_filenames(Options& options) {
  // read file_config 
  auto config_path = options.filenames;
  std::cout << "Config path for retrieving filenames: "<< config_path << std::endl;
  std::ifstream image_config_file;
  image_config_file.open(config_path);
  if (!image_config_file) {
    std::cerr << "Config file not found.\n";
    throw std::runtime_error(" Image config file not found.\n");
    // return EXIT_FAILURE;
  }
  // assert(image_config);
  try{
    std::string line;
    while (std::getline(image_config_file, line)) {
      std::stringstream lineStream(line);
      std::string cell;
      while (std::getline(lineStream, cell, ' ')) {
        options.data.push_back(cell);
        }
      }
    }
  catch(std::exception& e)
    {
      std::cerr<<"Error while reading the config file. Error: "<<e.what()<<std::endl;
      // return EXIT_FAILURE;
    }
  // return EXIT_SUCCESS;
}

void generate_filepaths(Options& options) {
  std::error_code err;
  std::string dirname = "server" + std::to_string(options.my_id);
  std::string dirname1 = options.currentpath + "/" + dirname + "/Actual_label/";
  std::filesystem::create_directories(dirname1, err);
  if(err){
      std::cerr<<"Error while creating directory "<<dirname1<<std::endl;
      throw std::runtime_error("Error while creating directory "+dirname1+"\n");
      // return EXIT_FAILURE;
    }
  std::string dirname2 = options.currentpath + "/" + dirname + "/Image_shares/";
  std::filesystem::create_directories(dirname2, err);
  if(err){
      std::cerr<<"Error while creating directory "<<dirname2<<std::endl;
      throw std::runtime_error("Error while creating directory "+dirname2+"\n");
      // return EXIT_FAILURE;
    }
  std::ofstream file;
  std::string fullfilename;
  // std::string n0 = options.data[0];
  fullfilename = dirname1 + options.data[0];
  options.filepaths.push_back(fullfilename);
  // std::string n1 = options.data[1];
  fullfilename = dirname2 + options.data[1];
  options.filepaths.push_back(fullfilename);
  // return EXIT_SUCCESS;
}


void retrieve_shares(Options& options) {
  std::ofstream shares_file;
  auto [frac_bits, shares_and_sizes] = COMPUTE_SERVER::get_provider_mat_mul_data(options.port);
  std::vector<COMPUTE_SERVER::Shares> image_shares_all = shares_and_sizes.first;
  // options.image.row = shares_and_sizes.second[0];
  // options.image.col = shares_and_sizes.second[1];
  options.fractional_bits = frac_bits;

  auto temp = options.filepaths[1];
  shares_file.open(temp, std::ios_base::out);
  if (!shares_file) {
    std::cerr << "Unable to create/open file "<<temp<<"\n";
    throw std::runtime_error("Unable to create file " + temp+"\n");
    // return EXIT_FAILURE;
  }
  try{
    shares_file << shares_and_sizes.second[0] << " " << shares_and_sizes.second[1] << "\n";
    std::cout << "Number of image share pairs received:" << image_shares_all.size() << "\n";
    for (int i = 0; i < image_shares_all.size(); i++) {
      // options.image.Delta.push_back(image_shares_all[i].Delta);
      // options.image.delta.push_back(image_shares_all[i].delta);
      shares_file << image_shares_all[i].Delta << " " << image_shares_all[i].delta << "\n";
    }
  }
  catch(std::exception& e)
    {
      std::cerr<<"Error while writing shares to file. Error: "<<e.what()<<std::endl;
      // return EXIT_FAILURE;
    }
  shares_file.close();

  // int count = 0;
  // std::string line;
  // std::ifstream mFile;
  // mFile.open(temp);
  // if(!mFile.is_open())
  //   {
  //     std::cerr<<"Unable to read the image shares file\n";
  //     return;
  //   }
  // while(std::getline(mFile,line)) {
  //     // std::cout << line << "\n";
  //     count++; 
  //   }
  // mFile.close();
  // std::cout << "Number of lines in the image shares file is " << count << std::endl;  
}


std::optional<Options> parse_program_options(int argc, char* argv[]) {
  Options options;
  boost::program_options::options_description desc("Allowed options");
  // clang-format off
  desc.add_options()
    ("help,h", po::bool_switch()->default_value(false),"produce help message")
    ("my-id", po::value<std::size_t>()->required(), "my party id")
    ("port" , po::value<int>()->required(), "Port number on which to listen")
    ("fractional-bits", po::value<std::size_t>()->default_value(16), "number of fractional bits for fixed-point arithmetic")
    ("file-names",po::value<std::string>()->required(), "filename")
    ("current-path",po::value<std::string>()->required(), "current path build_debwithrelinfo")
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
  } 
  catch (std::exception& e) {
    std::cerr << "Error in the input arguments:" << e.what() << "\n\n";
    std::cerr << desc << "\n";
    return std::nullopt;
  }

  options.my_id = vm["my-id"].as<std::size_t>();
  options.port = vm["port"].as<int>();
  options.fractional_bits = vm["fractional-bits"].as<std::size_t>();
  options.filenames = vm["file-names"].as<std::string>();
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
  // ----------------------------------------------------------------------------------//

  return options;
}


int main(int argc, char* argv[]) {
  auto options = parse_program_options(argc, argv);
  if (!options.has_value()) {
    std::cerr<<"Error while parsing the given input options.\n";
    return EXIT_FAILURE;
  }
  try{
      read_filenames(*options);
      generate_filepaths(*options);
      retrieve_shares(*options);
  }
  catch(std::exception& e){
      std::cerr<<"Error:"<<e.what();
      return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
