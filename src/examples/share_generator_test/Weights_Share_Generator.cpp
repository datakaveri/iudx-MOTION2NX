/*
./bin/Weights_Share_Generator --my-id 0 --party 0,::1,7003 --party 1,::1,7000 --arithmetic-protocol
beavy --boolean-protocol yao --fractional-bits 13 --file-names file_config_model
--file-names-image file_config_input


./bin/Weights_Share_Generator --my-id 1 --party 0,::1,7003 --party 1,::1,7000 --arithmetic-protocol
beavy --boolean-protocol yao --fractional-bits 13 --file-names file_config_model
--file-names-image file_config_input

./weights_provider --compute-server0-port 1234 --compute-server1-port 1235 --dp-id 1
--fractional-bits 13
*/
/* This code generates shares files for w1,b1,w2,b2.
This function needs a config file as input sample file_config_weights_1.txt is as follows

W1
B1
W2
B2

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

struct Matrix {
  std::vector<uint64_t> Delta;
  std::vector<uint64_t> delta;
  int row;
  int col;
};

struct Options {
  std::size_t threads;
  bool json;
  std::size_t num_repetitions;
  std::size_t num_simd;
  bool sync_between_setup_and_online;
  MOTION::MPCProtocol arithmetic_protocol;
  MOTION::MPCProtocol boolean_protocol;
  Matrix image;
  Matrix weights[2];
  Matrix biases[2];
  std::size_t fractional_bits;
  std::size_t my_id;
  MOTION::Communication::tcp_parties_config tcp_config;
  bool no_run = false;
  std::string filenames;
  std::string filenamesimage;
  std::vector<std::string> data;
  int actual_answer;
  std::vector<std::string> filepaths;
};

void read_filenames(Options* options) {
  // read file_config.txt
  auto p = std::filesystem::current_path();

  // model
  p += "/" + options->filenames;
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
  std::filesystem::create_directories(dirname, err);

  //.../server0/
  std::string temp = std::filesystem::current_path();
  std::string filename = temp + "/" + dirname + "/";
  std::ofstream file;
  std::string fullfilename;

  // creation of file_config_0.txt and file_config_1.txt
  // std::string image_later = temp + "/" + "file_config_input" + std::to_string(options->my_id);
  std::string model_later = temp + "/" + "file_config_model" + std::to_string(options->my_id);
  std::ofstream file_later1, file_later2;
  // file_later1.open(image_later, std::ios_base::out);
  file_later2.open(model_later, std::ios_base::out);

  //../server0/filename
  for (int i = 0; i < options->data.size(); ++i) {
    std::string n = options->data[i];
    fullfilename = filename + n;
    file_later2 << fullfilename << "\n";
    options->filepaths.push_back(fullfilename);
  }
}

void retrieve_shares(int port_number, Options* options) {
  std::ofstream file;

  ////////////////////////////////////////////////////////////

  for (auto i = 0; i < 4; i++) {
    std::cout << "Reading shares from weights provider \n";
    auto pair2 = COMPUTE_SERVER::get_provider_mat_mul_data(port_number);
    // auto [q1,q2,q3] = COMPUTE_SERVER::get_provider_mat_mul_data_new(port_number);
    std::vector<COMPUTE_SERVER::Shares> input_values_dp1 = pair2.second.first;
    // std::vector<COMPUTE_SERVER::Shares> input_values_dp1 = q3.first;
    auto temp = options->filepaths[i];
    std::cout << temp << "\n";
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
      file.close();
    } else {
      std::cout << "Bias \n";
      file.open(temp, std::ios_base::out);

      options->biases[(i - 1) / 2].row = pair2.second.second[0];
      options->biases[(i - 1) / 2].col = pair2.second.second[1];
      // options->biases[i / 2].row = q3.second[0];
      // options->biases[i / 2].col = q3.second[1];
      file << options->biases[(i - 1) / 2].row << " " << options->biases[(i - 1) / 2].col << "\n";
      std::cout << "Size:" << input_values_dp1.size() << "rows:" << options->biases[(i - 1) / 2].row
                << "columns:" << options->biases[(i - 2) / 2].col << "\n";
      for (int j = 0; j < input_values_dp1.size(); j++) {
        options->biases[(i - 1) / 2].Delta.push_back(input_values_dp1[j].Delta);
        options->biases[(i - 1) / 2].delta.push_back(input_values_dp1[j].delta);
        file << input_values_dp1[j].Delta << " " << input_values_dp1[j].delta << "\n";
      }
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
    ("party", po::value<std::vector<std::string>>()->multitoken(),
     "(party id, IP, port), e.g., --party 1,127.0.0.1,7777")
    ("threads", po::value<std::size_t>()->default_value(0), "number of threads to use for gate evaluation")
    ("json", po::bool_switch()->default_value(false), "output data in JSON format")
    ("fractional-bits", po::value<std::size_t>()->default_value(16),
     "number of fractional bits for fixed-point arithmetic")
    ("arithmetic-protocol", po::value<std::string>()->required(), "2PC protocol (GMW or BEAVY)")
    ("boolean-protocol", po::value<std::string>()->required(), "2PC protocol (Yao, GMW or BEAVY)")
    ("repetitions", po::value<std::size_t>()->default_value(1), "number of repetitions")
    ("num-simd", po::value<std::size_t>()->default_value(1), "number of SIMD values")
    ("file-names",po::value<std::string>()->required(), "filename")
    ("file-names-image",po::value<std::string>()->required(), "filename of image")
    
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
  if (vm.count("config-file")) {
    std::ifstream ifs(vm["config-file"].as<std::string>().c_str());
    po::store(po::parse_config_file(ifs, desc), vm);
  }
  try {
    po::notify(vm);
  } catch (std::exception& e) {
    std::cerr << "error:" << e.what() << "\n\n";
    std::cerr << desc << "\n";
    return std::nullopt;
  }

  options.my_id = vm["my-id"].as<std::size_t>();
  options.threads = vm["threads"].as<std::size_t>();
  options.json = vm["json"].as<bool>();
  options.num_repetitions = vm["repetitions"].as<std::size_t>();
  options.num_simd = vm["num-simd"].as<std::size_t>();
  options.sync_between_setup_and_online = vm["sync-between-setup-and-online"].as<bool>();
  options.no_run = vm["no-run"].as<bool>();
  options.fractional_bits = vm["fractional-bits"].as<std::size_t>();
  options.filenames = vm["file-names"].as<std::string>();
  options.filenamesimage = vm["file-names-image"].as<std::string>();

  if (options.my_id > 1) {
    std::cerr << "my-id must be one of 0 and 1\n";
    return std::nullopt;
  }

  auto arithmetic_protocol = vm["arithmetic-protocol"].as<std::string>();
  boost::algorithm::to_lower(arithmetic_protocol);
  if (arithmetic_protocol == "gmw") {
    options.arithmetic_protocol = MOTION::MPCProtocol::ArithmeticGMW;
  } else if (arithmetic_protocol == "beavy") {
    options.arithmetic_protocol = MOTION::MPCProtocol::ArithmeticBEAVY;
  } else {
    std::cerr << "invalid protocol: " << arithmetic_protocol << "\n";
    return std::nullopt;
  }
  auto boolean_protocol = vm["boolean-protocol"].as<std::string>();
  boost::algorithm::to_lower(boolean_protocol);
  if (boolean_protocol == "yao") {
    options.boolean_protocol = MOTION::MPCProtocol::Yao;
  } else if (boolean_protocol == "gmw") {
    options.boolean_protocol = MOTION::MPCProtocol::BooleanGMW;
  } else if (boolean_protocol == "beavy") {
    options.boolean_protocol = MOTION::MPCProtocol::BooleanBEAVY;
  } else {
    std::cerr << "invalid protocol: " << boolean_protocol << "\n";
    return std::nullopt;
  }

  read_filenames(&options);

  if (options.my_id == 0) {
    generate_filepaths(&options);
  } else if (options.my_id == 1) {
    generate_filepaths(&options);
  }

  if (options.my_id == 0) {
    retrieve_shares(1234, &options);
  } else {
    retrieve_shares(1235, &options);
  }

  if (options.weights[0].col != options.image.row) {
    std::cout << options.weights[0].col << "image :" << options.image.row << "\n";
    std::cerr
        << "Invalid inputs, number of columns for dp0 must be equal to number of rows for dp1 \n";
    return std::nullopt;
  }

  const auto parse_party_argument =
      [](const auto& s) -> std::pair<std::size_t, MOTION::Communication::tcp_connection_config> {
    const static std::regex party_argument_re("([01]),([^,]+),(\\d{1,5})");
    std::smatch match;
    if (!std::regex_match(s, match, party_argument_re)) {
      throw std::invalid_argument("invalid party argument");
    }
    auto id = boost::lexical_cast<std::size_t>(match[1]);
    auto host = match[2];
    auto port = boost::lexical_cast<std::uint16_t>(match[3]);
    return {id, {host, port}};
  };

  const std::vector<std::string> party_infos = vm["party"].as<std::vector<std::string>>();
  if (party_infos.size() != 2) {
    std::cerr << "expecting two --party options\n";
    return std::nullopt;
  }

  options.tcp_config.resize(2);
  std::size_t other_id = 2;

  const auto [id0, conn_info0] = parse_party_argument(party_infos[0]);
  const auto [id1, conn_info1] = parse_party_argument(party_infos[1]);
  if (id0 == id1) {
    std::cerr << "need party arguments for party 0 and 1\n";
    return std::nullopt;
  }
  options.tcp_config[id0] = conn_info0;
  options.tcp_config[id1] = conn_info1;

  return options;
}

std::unique_ptr<MOTION::Communication::CommunicationLayer> setup_communication(
    const Options& options) {
  MOTION::Communication::TCPSetupHelper helper(options.my_id, options.tcp_config);
  return std::make_unique<MOTION::Communication::CommunicationLayer>(options.my_id,
                                                                     helper.setup_connections());
}

int main(int argc, char* argv[]) {
 
  auto options = parse_program_options(argc, argv);
  if (!options.has_value()) {
    return EXIT_FAILURE;
  }

  try {
    auto comm_layer = setup_communication(*options);
    auto logger = std::make_shared<MOTION::Logger>(options->my_id,
                                                   boost::log::trivial::severity_level::trace);
    comm_layer->set_logger(logger);
    MOTION::TwoPartyTensorBackend backend(*comm_layer, options->threads,
                                          options->sync_between_setup_and_online, logger);
    // run_composite_circuit(*options, backend);
    comm_layer->shutdown();
  } catch (std::runtime_error& e) {
    std::cerr << "ERROR OCCURRED: " << e.what() << "\n";
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}