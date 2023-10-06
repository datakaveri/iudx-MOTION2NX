/*

./bin/weight_share_receiver_genr --my-id 0 --port 1234 --current-path ${BASE_DIR}/build_debwithrelinfo_gcc

./bin/weight_share_receiver_genr --my-id 1 --port 1235 --current-path ${BASE_DIR}/build_debwithrelinfo_gcc
*/
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
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
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
#include <regex>
#include <stdexcept>
#include <thread>

#include <boost/algorithm/string.hpp>
#include <boost/json/serialize.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/log/trivial.hpp>
#include <boost/program_options.hpp>

#include "compute_server/compute_server_CNN.h"
#include "utility/logger.h"
#include "utility/fixed_point.h"

namespace po = boost::program_options;

struct Options {
  std::size_t my_id;
  std::vector<std::string> data;
  std::vector<std::string> filepaths;
  std::string currentpath;
  int port;
  int number_of_layers;
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
  // options.filenames = vm["file-names"].as<std::string>();
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

void generate_filepaths(Options& options) {
  std::error_code file_error;
  std::string dir_name = "server" + std::to_string(options.my_id);
  std::string dir_path = options.currentpath + "/" + dir_name;
  std::filesystem::create_directories(dir_path, file_error);
  if(file_error){
      std::cerr<<"Error while creating directory "<<dir_path<<std::endl;
      throw std::runtime_error("Error while creating directory "+dir_path+"\n");
      // return EXIT_FAILURE;
  }

  std::string share_data_config_file_name = "file_config_model" + std::to_string(options.my_id);
  std::string share_data_config_file_path = options.currentpath + "/" + share_data_config_file_name;
  std::ofstream config_file;
  config_file.open(share_data_config_file_path, std::ios_base::out);
  if(!config_file.is_open()){
      config_file.close();
      throw std::runtime_error("Unable to open config file "+ share_data_config_file_path + "\n");
  }
  try{
    for (int i = 1; i <= options.number_of_layers; ++i) {
      std::string weight_share_file_name, bias_share_file_name;
      string curr_layer = std::to_string(i);

      weight_share_file_name = "W" + curr_layer;
      std::string weight_share_file_path = dir_path + "/" + weight_share_file_name;
      config_file << weight_share_file_path << "\n";
      
      bias_share_file_name = "B" + curr_layer;
      std::string bias_share_file_path = dir_path + "/" + bias_share_file_name;
      config_file << bias_share_file_path << "\n";

      options.filepaths.push_back(weight_share_file_path);
      options.filepaths.push_back(bias_share_file_path);
    }
  }
  catch(std::exception& e){
      config_file.close();
      throw std::runtime_error("Error while reading the weight and bias share file paths from the config file (" + share_data_config_file_path + "). \nError: " + e.what() + "\n"); 
    }
}

void store_types(Options& options, std::vector<int> types) {
  std::string types_file_path = options.currentpath + "/layer_types" + std::to_string(options.my_id);
  std::ofstream types_file;
  types_file.open(types_file_path, std::ios_base::out);
  if(!types_file.is_open()){
    types_file.close();
    throw std::runtime_error("Unable to open file "+ types_file_path + "\n");
  }

  types_file << options.number_of_layers << "\n";
  for (int i=0; i<options.number_of_layers; i++) {
    types_file << types[2*i] << "\n";
    try {
      if (types[2*i+1] != 0)
        throw(i);
    }
    catch (int layer_id) {
      std::cout << "Invalid Type for Bias in Layer " << layer_id << std::endl;
    }
  }

  types_file.close();
  return;
}

void retrieve_shares(Options& options) {
  std::ofstream file;
  ////////////////////////////////////////////////////////////
  std::vector<int> types;
  std::vector<std::vector<int>> dims;
  std::vector<std::vector<COMPUTE_SERVER_CNN::Shares>> data;

  auto [numberOfVectors, frac, data_and_dims] =
      COMPUTE_SERVER_CNN::get_provider_total_data(options.port);

  tie(types, dims, data) = data_and_dims;

  options.number_of_layers = numberOfVectors/2;
  if(options.number_of_layers<1){
    throw std::runtime_error("Number of layers cannot be lesser than one.\n");
  }

  generate_filepaths(options);
  store_types(options, types);                                // writing into layer_types0/layer_types1 file

  for (auto i = 0; i < numberOfVectors; i++) {
    auto data_share_file_path = options.filepaths[i];
    std::cout << "Writing shares to file "<< data_share_file_path << "\n";
    std::vector<int> currentdims = dims[i];
    std::vector<COMPUTE_SERVER_CNN::Shares> input_values_dp1 = data[i];
    file.open(data_share_file_path, std::ios_base::out);
    if(!file.is_open()){
        file.close();
        throw std::runtime_error("Unable to open file "+data_share_file_path+"\n");
    }
    for (auto iter:currentdims)
      std::cout << iter << " ";
    std::cout << std::endl;
    
    for (int j=0; j<currentdims.size(); ++j) {
      file << currentdims[j] ;
      if (j == currentdims.size()-1)
        file << '\n';
      else
        file << ' ';
    }
    
    for (int j = 0; j < input_values_dp1.size(); j++) {
      file << input_values_dp1[j].Delta << " " << input_values_dp1[j].delta << "\n";
    }
    file.close();
  }
}


void get_confirmation(const Options& options) {
  boost::asio::io_service io_service;

  // listen for new connection
  tcp::acceptor acceptor_(io_service, tcp::endpoint(tcp::v4(), options.port));

  // socket creation
  tcp::socket socket_(io_service);

  // waiting for the connection
  try {
  acceptor_.accept(socket_);
  std::cout<<"Accepted a connection at port "<<options.port<<"\n";
  }
  catch (const boost::system::system_error& error) {
    throw; // It rethrows using the same details.
  }

  // Read and write the number of fractional bits
  boost::system::error_code read_error;
  int validation_bit;
  read(socket_, boost::asio::buffer(&validation_bit, sizeof(validation_bit)), read_error);
  if (read_error) {
    throw std::runtime_error("Error while receiving the validation bit: "+read_error.message()+"\n");
  }
  std::cout<<"Received the validation bit successfully.\n";
}

int main(int argc, char* argv[]) {
  auto options = parse_program_options(argc, argv);
  if (!options.has_value()) {
    std::cerr<<"Error while parsing the given input options.\n";
    return EXIT_FAILURE;
  }
  try{
      retrieve_shares(*options);
      // if (options->my_id == 0) {
      //   get_confirmation(*options);
      // }
  }
  catch (const boost::system::system_error& error) {
    std::cerr << error.what();
    return EXIT_FAILURE;
  }

  catch(std::exception& e){
    std::cerr <<e.what()<< std::endl;
    return EXIT_FAILURE;
  }
  std::cout<<"Weight share receiver finished its operations successfully.\n";
  return EXIT_SUCCESS;
}
