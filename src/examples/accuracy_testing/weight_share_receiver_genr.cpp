/*

./bin/weight_share_receiver_genr --my-id 0 --port 1234 --current-path ${BASE_DIR}/build_debwithrelinfo_gcc

./bin/weight_share_receiver_genr --my-id 1 --port 1235 --current-path ${BASE_DIR}/build_debwithrelinfo_gcc

*/
/* This code generates shares files for w1,b1,w2,b2.

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
#include <algorithm>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <random>
#include <regex>
#include <stdexcept>
#include <thread>

#include <boost/algorithm/string.hpp>
#include <boost/json/serialize.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/log/trivial.hpp>
#include <boost/program_options.hpp>

#include "algorithm/circuit_loader.h"
#include "base/gate_factory.h"
#include "base/two_party_backend.h"
#include "communication/communication_layer.h"
#include "communication/tcp_transport.h"
#include "compute_server/compute_server.h"
#include "statistics/analysis.h"
#include "utility/logger.h"

#include "base/two_party_tensor_backend.h"
#include "protocols/beavy/tensor.h"
#include "tensor/tensor.h"
#include "tensor/tensor_op.h"
#include "tensor/tensor_op_factory.h"
#include "utility/fixed_point.h"

namespace po = boost::program_options;

static std::vector<uint64_t> generate_inputs(const MOTION::tensor::TensorDimensions dims) {
  return MOTION::Helpers::RandomVector<uint64_t>(dims.get_data_size());
}

struct Options {
  std::size_t threads;
  bool json;
  std::size_t num_simd;
  bool sync_between_setup_and_online;
  std::size_t my_id;
  bool no_run = false;
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
    ("threads", po::value<std::size_t>()->default_value(0), "number of threads to use for gate evaluation")
    ("json", po::bool_switch()->default_value(false), "output data in JSON format")
    ("num-simd", po::value<std::size_t>()->default_value(1), "number of SIMD values")
    ("current-path",po::value<std::string>()->required(), "current path build_debwithrelinfo")
    ("sync-between-setup-and-online", po::bool_switch()->default_value(false),
     "run a synchronization protocol before the online phase starts")
    ("no-run", po::bool_switch()->default_value(false), "just build the circuit, but not execute it")
    ;
  // clang-format on

  po::variables_map vm;
  po::store(po::parse_command_line(argc, argv, desc), vm);
  bool help = vm["help"].as<bool>();
  if (help) {
    std::cerr << desc << "\n";
    return std::nullopt;
  }

  options.my_id = vm["my-id"].as<std::size_t>();
  options.port = vm["port"].as<int>();
  options.threads = vm["threads"].as<std::size_t>();
  options.json = vm["json"].as<bool>();
  options.num_simd = vm["num-simd"].as<std::size_t>();
  options.sync_between_setup_and_online = vm["sync-between-setup-and-online"].as<bool>();
  options.no_run = vm["no-run"].as<bool>();
  // options.filenames = vm["file-names"].as<std::string>();
  options.currentpath = vm["current-path"].as<std::string>();
  if (options.my_id > 1) {
    std::cerr << "my-id must be one of 0 and 1\n";
    return std::nullopt;
  }

  return options;
}

void generate_filepaths(Options& options) {
  std::error_code err;
  std::string dir_name = "server" + std::to_string(options.my_id);
  std::string dir_path = options.currentpath + "/" + dir_name;
  std::filesystem::create_directories(dir_path, err);

  std::string share_data_config_file_name = "file_config_model" + std::to_string(options.my_id);
  std::string share_data_config_file_path = options.currentpath + "/" + share_data_config_file_name;
  std::ofstream config_file;
  config_file.open(share_data_config_file_path, std::ios_base::out);

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

void retrieve_shares(Options& options) {
  std::ofstream file;

  ////////////////////////////////////////////////////////////

  auto [numberOfLayers, frac, data_and_dims] =
      COMPUTE_SERVER::get_provider_total_data_genr(options.port);
  options.number_of_layers = numberOfLayers;

  generate_filepaths(options);

  for (auto i = 0; i < 2*numberOfLayers; i++) {
    std::cout << "Writing shares to file\n";
    auto data_share_file_path = options.filepaths[i];
    std::cout << data_share_file_path << "\n";
    std::vector<COMPUTE_SERVER::Shares> input_values_dp1 = data_and_dims.first[i];
    auto currentdims = data_and_dims.second[i];
    file.open(data_share_file_path, std::ios_base::out);
    file << currentdims.first << " " << currentdims.second << "\n";
    std::cout << "rows:" << currentdims.first << " columns:" << currentdims.second << std::endl;
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
  acceptor_.accept(socket_);

  // Read and write the number of fractional bits
  boost::system::error_code ec;
  int validation_bit;
  read(socket_, boost::asio::buffer(&validation_bit, sizeof(validation_bit)), ec);
  if (ec) {
    std::cout << ec << "\n";
  } else {
    std::cout << "No Error, have received validation bit\n";
  }
}

int main(int argc, char* argv[]) {
  auto options = parse_program_options(argc, argv);
  if (!options.has_value()) {
    return EXIT_FAILURE;
  }

  retrieve_shares(*options);

  // if (options->my_id == 0) {
  //   get_confirmation(*options);
  // }

  try {
    auto logger = std::make_shared<MOTION::Logger>(options->my_id,
                                                   boost::log::trivial::severity_level::trace);

  } catch (std::runtime_error& e) {
    std::cerr << "ERROR OCCURRED: " << e.what() << "\n";
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
