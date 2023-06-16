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
#include <optional>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <random>
// #include <regex>
#include <stdexcept>
#include <thread>

// #include <boost/algorithm/string.hpp>
#include <boost/json/serialize.hpp>
// #include <boost/lexical_cast.hpp>
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
  Matrix weights[2];
  Matrix biases[2];
  std::size_t my_id;
  std::string filenames;
  std::vector<std::string> data;
  std::vector<std::string> filepaths;
  std::string currentpath;
  int port;
};

void read_filenames(Options* options) {
  // read file_config.txt
  // auto p = std::filesystem::current_path();

  // model
  auto p = options->filenames;
  std::cout << p << std::endl;

  // model
  std::ifstream indata;
  indata.open(p);
  if (std::ifstream(p)) {
    std::cout << "File found\n";
  } else {
    std::cout << "File not found\n";
  }
  assert(indata);

  // model
  std::string line;
  while (std::getline(indata, line)) {
    std::stringstream lineStream(line);
    std::string cell;

    while (std::getline(lineStream, cell, ' ')) {
      options->data.push_back(cell);
    }
  }
  std::cout << "\n";
}

void generate_filepaths(Options* options) {
  // creation of directory eg. server0
  std::error_code err;
  std::string dirname = "server" + std::to_string(options->my_id);
  std::string dirname1 = options->currentpath + "/" + dirname;
  std::filesystem::create_directories(dirname1, err);

  //.../server0/
  // std::string temp = std::filesystem::current_path();
  // std::cout << "g:" << temp << "\n";
  std::string filename = options->currentpath + "/" + dirname + "/";
  std::ofstream file;
  std::string fullfilename;

  // creation of file_config_0.txt and file_config_1.txt
  std::string model_later =
      options->currentpath + "/file_config_model" + std::to_string(options->my_id);
  std::ofstream file_later1, file_later2;
  file_later2.open(model_later, std::ios_base::out);

  //../server0/filename
  for (int i = 0; i < options->data.size(); ++i) {
    std::string n = options->data[i];
    fullfilename = filename + n;
    file_later2 << fullfilename << "\n";
    options->filepaths.push_back(fullfilename);
  }
}

void retrieve_shares(Options* options) {
  std::ofstream file;

  ////////////////////////////////////////////////////////////

  for (auto i = 0; i < 10; i++) {
    std::cout << "Reading shares from weights provider \n";
    auto temp = options->filepaths[i];
    std::cout << temp << "\n";
    
    auto pair2 = COMPUTE_SERVER::get_provider_mat_mul_data(options->port);
    // auto [q1,q2,q3] = COMPUTE_SERVER::get_provider_mat_mul_data_new(port_number);
    std::vector<COMPUTE_SERVER::Shares> input_values_dp1 = pair2.second.first;
    // std::vector<COMPUTE_SERVER::Shares> input_values_dp1 = q3.first;
    
    if ((i + 2) % 2 == 0) {
      std::cout << "Weights \n";
      file.open(temp, std::ios_base::out);

      options->weights[i / 2].row = pair2.second.second[0];
      options->weights[i / 2].col = pair2.second.second[1];
      // options->weights[i / 2].row = q3.second[0];
      // options->weights[i / 2].col = q3.second[1];
      file << options->weights[i / 2].row << " " << options->weights[i / 2].col << "\n";

      for (int j = 0; j < input_values_dp1.size(); j++) {
        options->weights[i / 2].Delta.push_back(input_values_dp1[j].Delta);
        options->weights[(i) / 2].delta.push_back(input_values_dp1[j].delta);
        file << input_values_dp1[j].Delta << " " << input_values_dp1[j].delta << "\n";
      }
      std::cout << std::endl;
      file.close();
    } else {
      std::cout << "Bias \n";
      file.open(temp, std::ios_base::out);

      options->biases[(i - 1) / 2].row = pair2.second.second[0];
      options->biases[(i - 1) / 2].col = pair2.second.second[1];
      // options->biases[i / 2].row = q3.second[0];
      // options->biases[i / 2].col = q3.second[1];
      file << options->biases[(i - 1) / 2].row << " " << options->biases[(i - 1) / 2].col << "\n";
      // std::cout << "Size:" << input_values_dp1.size() << "rows:" << options->biases[(i - 1) / 2].row
                // << "columns:" << options->biases[(i - 2) / 2].col << "\n";
      
      for (int j = 0; j < input_values_dp1.size(); j++) {
        options->biases[(i - 1) / 2].Delta.push_back(input_values_dp1[j].Delta);
        options->biases[(i - 1) / 2].delta.push_back(input_values_dp1[j].delta);
        file << input_values_dp1[j].Delta << " " << input_values_dp1[j].delta << "\n";
      }
      std::cout << std::endl;
      file.close();
    }
  }
}

std::optional<Options> parse_program_options(int argc, char* argv[]) {
  Options options;
  boost::program_options::options_description desc("Allowed options");
  // clang-format off
  desc.add_options()
    ("help,h", po::bool_switch()->default_value(false),"produce help message")
    ("config-file", po::value<std::string>(), "config file containing options")
    ("my-id", po::value<std::size_t>()->required(), "my party id")
    ("port" , po::value<int>()->required(), "Port number on which to listen")
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
  if (vm.count("config-file")) {
    std::ifstream ifs(vm["config-file"].as<std::string>().c_str());
    po::store(po::parse_config_file(ifs, desc), vm);
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
  options.filenames = vm["file-names"].as<std::string>();
  options.currentpath = vm["current-path"].as<std::string>();
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
  read_filenames(&options);
  generate_filepaths(&options);
  retrieve_shares(&options);

  return options;
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
    std::cout << "Error accepting connection: " << error.what() << std::endl;
    socket_.close();
    return;
  }
  // Read and write the number of fractional bits
  boost::system::error_code read_error;
  int validation_bit;
  read(socket_, boost::asio::buffer(&validation_bit, sizeof(validation_bit)), read_error);
  if (read_error) {
    std::cout << "Error while receiving validation bit: "<<read_error << "\n";
    socket_.close();
    return;
  } 
  std::cout << "Received validation bit\n";
  socket_.close();

}

int main(int argc, char* argv[]) {
  auto options = parse_program_options(argc, argv);

  if(options->my_id == 0) {
  	get_confirmation(*options);
  }

  if (!options.has_value()) {
    return EXIT_FAILURE;
  }
  try {
    auto logger = std::make_shared<MOTION::Logger>(options->my_id,
                                                   boost::log::trivial::severity_level::trace);

  } catch (std::runtime_error& e) {
    std::cerr << "ERROR OCCURRED: " << e.what() << "\n";
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
