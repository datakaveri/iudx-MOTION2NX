// /*
// ./bin/Image_Share_Generator --my-id 0 --party 0,::1,7003 --party 1,::1,7000 --arithmetic-protocol
// beavy --boolean-protocol yao --fractional-bits 13 --file-names file_config_input --index 1

// ./bin/Image_Share_Generator --my-id 1 --party 0,::1,7003 --party 1,::1,7000 --arithmetic-protocol
// beavy --boolean-protocol yao --fractional-bits 13 --file-names file_config_input --index 1

// ./image_provider_version2 --compute-server0-port 1234 --compute-server1-port 1235
// --fractional-bits 13 --NameofImageFile X6

// */

// /* This code generates shares files for image,w1,b1,w2,b2. Aditionally it generates the
// ActualAnswer of image in both the folders. The title of this file is read from the first line in
// the file_config.txt This function needs a config file as input sample file_config_image_1.txt is
// as follows
// X
// ip1

// I have modified the image provider code to send actual answer as the first number via socket.
// Also the imagefile eg.,X.csv should contain the first number as the actual answer.
// I also modified compute server.cpp to send the extra number to the main function.
// */
// // MIT License
// //
// // Copyright (c) 2021 Lennart Braun
// //
// // Permission is hereby granted, free of charge, to any person obtaining a copy
// // of this software and associated documentation files (the "Software"), to deal
// // in the Software without restriction, including without limitation the rights
// // to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// // copies of the Software, and to permit persons to whom the Software is
// // furnished to do so, subject to the following conditions:
// //
// // The above copyright notice and this permission notice shall be included in all
// // copies or substantial portions of the Software.
// //
// // THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// // IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// // FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// // AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// // LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// // OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// // SOFTWARE.

// #include <dirent.h>
// #include <algorithm>
// #include <chrono>
// #include <cmath>
// #include <filesystem>
// #include <fstream>
// #include <iostream>
// #include <random>
// #include <regex>
// #include <stdexcept>
// #include <thread>

// #include <boost/algorithm/string.hpp>
// #include <boost/json/serialize.hpp>
// #include <boost/lexical_cast.hpp>
// #include <boost/log/trivial.hpp>
// #include <boost/program_options.hpp>

// #include "algorithm/circuit_loader.h"
// #include "base/gate_factory.h"
// #include "base/two_party_backend.h"
// #include "communication/communication_layer.h"
// #include "communication/tcp_transport.h"
// #include "compute_server/compute_server.h"
// #include "statistics/analysis.h"
// #include "utility/logger.h"

// #include "base/two_party_tensor_backend.h"
// #include "protocols/beavy/tensor.h"
// #include "tensor/tensor.h"
// #include "tensor/tensor_op.h"
// #include "tensor/tensor_op_factory.h"
// #include "utility/fixed_point.h"

// namespace po = boost::program_options;

// static std::vector<uint64_t> generate_inputs(const MOTION::tensor::TensorDimensions dims) {
//   return MOTION::Helpers::RandomVector<uint64_t>(dims.get_data_size());
// }

// struct Matrix {
//   std::vector<uint64_t> Delta;
//   std::vector<uint64_t> delta;
//   int row;
//   int col;
// };

// struct Options {
//   std::size_t threads;
//   bool json;
//   std::size_t num_repetitions;
//   std::size_t num_simd;
//   bool sync_between_setup_and_online;
//   MOTION::MPCProtocol arithmetic_protocol;
//   MOTION::MPCProtocol boolean_protocol;
//   Matrix image;
//   Matrix weights[2];
//   Matrix biases[2];
//   std::size_t fractional_bits;
//   std::size_t my_id;
//   MOTION::Communication::tcp_parties_config tcp_config;
//   bool no_run = false;
//   std::string filenames;
//   std::vector<std::string> data;
//   int actual_answer;
//   std::vector<std::string> filepaths;
//   std::string index;
// };

// void read_filenames(Options* options) {
//   // read file_config.txt
//   auto p = std::filesystem::current_path();
//   p += "/" + options->filenames;
//   std::cout << p << std::endl;

//   std::ifstream indata;
//   indata.open(p);
//   if (std::ifstream(p)) {
//     std::cout << "File found\n";
//   } else {
//     std::cout << "File not found\n";
//   }
//   assert(indata);

//   std::string line;
//   while (std::getline(indata, line)) {
//     std::stringstream lineStream(line);
//     std::string cell;

//     while (std::getline(lineStream, cell, ' ')) {
//       options->data.push_back(cell);
//     }
//   }
//   std::cout << "\n";
// }

// void generate_filepaths(Options* options) {
//   // creation of directory eg. server0
//   std::error_code err;
//   std::string dirname = "server" + std::to_string(options->my_id);
//   std::filesystem::create_directories(dirname, err);

//   //.../server0/
//   std::string temp = std::filesystem::current_path();
//   std::string filename = temp + "/" + dirname + "/";
//   std::ofstream file;
//   std::string fullfilename;

//   //../server0/filenameshare_id0 / 1 only for image shares, for actual answer it is only X
//   std::cout << "Size of data: " << options->data.size() << "\n";
//   std::string n0 = options->data[0] + options->index;
//   fullfilename = filename + n0;
//   options->filepaths.push_back(fullfilename);
//   // std::string n1 = options->data[1] + "_" + std::to_string(options->my_id);
//   std::string n1 = options->data[1] + options->index;
//   fullfilename = filename + n1;
//   options->filepaths.push_back(fullfilename);
// }

// void retrieve_shares(int port_number, Options* options) {
//   std::ofstream file;
//   // options->actual_answer=COMPUTE_SERVER::get_actual_answer(port_number);

//   auto [p1, p2, p3] = COMPUTE_SERVER::get_provider_mat_mul_data_new(port_number);
//   options->actual_answer = p1;
//   std::cout << "actual answer in main:" << options->actual_answer << "\n";
//   auto temp0 = options->filepaths[0];
//   file.open(temp0, std::ios_base::out);
//   if (file) {
//     std::cout << "File found\n";
//   } else {
//     std::cout << "File not found\n";
//   }
//   file << options->actual_answer;
//   file.close();
//   // auto pair1 = COMPUTE_SERVER::get_provider_mat_mul_data(port_number);
//   // std::vector<COMPUTE_SERVER::Shares> input_values_dp0 = pair1.second.first;
//   std::vector<COMPUTE_SERVER::Shares> input_values_dp0 = p3.first;

//   // std::ofstream file;
//   auto temp = options->filepaths[1];

//   file.open(temp, std::ios_base::out);
//   if (file) {
//     std::cout << "File found\n";
//   } else {
//     std::cout << "File not found\n";
//   }

//   // options->image.row = pair1.second.second[0];
//   // options->image.col = pair1.second.second[1];
//   // options->fractional_bits = pair1.first;
//   options->image.row = p3.second[0];
//   options->image.col = p3.second[1];
//   options->fractional_bits = p2;

//   file << options->image.row << " " << options->image.col << "\n";

//   for (int i = 0; i < input_values_dp0.size(); i++) {
//     options->image.Delta.push_back(input_values_dp0[i].Delta);
//     options->image.delta.push_back(input_values_dp0[i].delta);
//     std::cout << input_values_dp0[i].Delta << " " << input_values_dp0[i].delta << "\n";
//     file << input_values_dp0[i].Delta << " " << input_values_dp0[i].delta << "\n";
//   }

//   file.close();
// }

// std::optional<Options> parse_program_options(int argc, char* argv[]) {
//   Options options;
//   boost::program_options::options_description desc("Allowed options");
//   // clang-format off
//   desc.add_options()
//     ("help,h", po::bool_switch()->default_value(false),"produce help message")
//     ("config-file", po::value<std::string>(), "config file containing options")
//     ("my-id", po::value<std::size_t>()->required(), "my party id")
//     ("party", po::value<std::vector<std::string>>()->multitoken(),
//      "(party id, IP, port), e.g., --party 1,127.0.0.1,7777")
//     ("threads", po::value<std::size_t>()->default_value(0), "number of threads to use for gate
//     evaluation")
//     ("json", po::bool_switch()->default_value(false), "output data in JSON format")
//     ("fractional-bits", po::value<std::size_t>()->default_value(16),
//      "number of fractional bits for fixed-point arithmetic")
//     ("arithmetic-protocol", po::value<std::string>()->required(), "2PC protocol (GMW or BEAVY)")
//     ("boolean-protocol", po::value<std::string>()->required(), "2PC protocol (Yao, GMW or
//     BEAVY)")
//     ("repetitions", po::value<std::size_t>()->default_value(1), "number of repetitions")
//     ("num-simd", po::value<std::size_t>()->default_value(1), "number of SIMD values")
//     ("file-names",po::value<std::string>()->required(), "filename")
//     ("index",po::value<std::string>()->default_value("0"), "index")
//     ("sync-between-setup-and-online", po::bool_switch()->default_value(false),
//      "run a synchronization protocol before the online phase starts")
//     ("no-run", po::bool_switch()->default_value(false), "just build the circuit, but not execute
//     it")
//     ;
//   // clang-format on

//   po::variables_map vm;
//   po::store(po::parse_command_line(argc, argv, desc), vm);
//   bool help = vm["help"].as<bool>();
//   if (help) {
//     std::cerr << desc << "\n";
//     return std::nullopt;
//   }
//   if (vm.count("config-file")) {
//     std::ifstream ifs(vm["config-file"].as<std::string>().c_str());
//     po::store(po::parse_config_file(ifs, desc), vm);
//   }
//   try {
//     po::notify(vm);
//   } catch (std::exception& e) {
//     std::cerr << "error:" << e.what() << "\n\n";
//     std::cerr << desc << "\n";
//     return std::nullopt;
//   }

//   options.my_id = vm["my-id"].as<std::size_t>();
//   options.threads = vm["threads"].as<std::size_t>();
//   options.json = vm["json"].as<bool>();
//   options.num_repetitions = vm["repetitions"].as<std::size_t>();
//   options.num_simd = vm["num-simd"].as<std::size_t>();
//   options.sync_between_setup_and_online = vm["sync-between-setup-and-online"].as<bool>();
//   options.no_run = vm["no-run"].as<bool>();
//   options.fractional_bits = vm["fractional-bits"].as<std::size_t>();
//   options.filenames = vm["file-names"].as<std::string>();
//   options.index = vm["index"].as<std::string>();
//   std::cout << "index" << options.index << "\n";
//   if (options.my_id > 1) {
//     std::cerr << "my-id must be one of 0 and 1\n";
//     return std::nullopt;
//   }

//   auto arithmetic_protocol = vm["arithmetic-protocol"].as<std::string>();
//   boost::algorithm::to_lower(arithmetic_protocol);
//   if (arithmetic_protocol == "gmw") {
//     options.arithmetic_protocol = MOTION::MPCProtocol::ArithmeticGMW;
//   } else if (arithmetic_protocol == "beavy") {
//     options.arithmetic_protocol = MOTION::MPCProtocol::ArithmeticBEAVY;
//   } else {
//     std::cerr << "invalid protocol: " << arithmetic_protocol << "\n";
//     return std::nullopt;
//   }
//   auto boolean_protocol = vm["boolean-protocol"].as<std::string>();
//   boost::algorithm::to_lower(boolean_protocol);
//   if (boolean_protocol == "yao") {
//     options.boolean_protocol = MOTION::MPCProtocol::Yao;
//   } else if (boolean_protocol == "gmw") {
//     options.boolean_protocol = MOTION::MPCProtocol::BooleanGMW;
//   } else if (boolean_protocol == "beavy") {
//     options.boolean_protocol = MOTION::MPCProtocol::BooleanBEAVY;
//   } else {
//     std::cerr << "invalid protocol: " << boolean_protocol << "\n";
//     return std::nullopt;
//   }

//   read_filenames(&options);

//   if (options.my_id == 0) {
//     generate_filepaths(&options);
//   } else if (options.my_id == 1) {
//     generate_filepaths(&options);
//   }

//   if (options.my_id == 0) {
//     retrieve_shares(1234, &options);
//   } else {
//     retrieve_shares(1235, &options);
//   }

//   // if (options.weights[0].col != options.image.row) {
//   //   std::cout << options.weights[0].col << "image :"<<options.image.row<< "\n";
//   //   std::cerr
//   //       << "Invalid inputs, number of columns for dp0 must be equal to number of rows for dp1
//   //       \n";
//   //   return std::nullopt;
//   // }

//   const auto parse_party_argument =
//       [](const auto& s) -> std::pair<std::size_t, MOTION::Communication::tcp_connection_config> {
//     const static std::regex party_argument_re("([01]),([^,]+),(\\d{1,5})");
//     std::smatch match;
//     if (!std::regex_match(s, match, party_argument_re)) {
//       throw std::invalid_argument("invalid party argument");
//     }
//     auto id = boost::lexical_cast<std::size_t>(match[1]);
//     auto host = match[2];
//     auto port = boost::lexical_cast<std::uint16_t>(match[3]);
//     return {id, {host, port}};
//   };

//   const std::vector<std::string> party_infos = vm["party"].as<std::vector<std::string>>();
//   if (party_infos.size() != 2) {
//     std::cerr << "expecting two --party options\n";
//     return std::nullopt;
//   }

//   options.tcp_config.resize(2);
//   std::size_t other_id = 2;

//   const auto [id0, conn_info0] = parse_party_argument(party_infos[0]);
//   const auto [id1, conn_info1] = parse_party_argument(party_infos[1]);
//   if (id0 == id1) {
//     std::cerr << "need party arguments for party 0 and 1\n";
//     return std::nullopt;
//   }
//   options.tcp_config[id0] = conn_info0;
//   options.tcp_config[id1] = conn_info1;

//   return options;
// }

// std::unique_ptr<MOTION::Communication::CommunicationLayer> setup_communication(
//     const Options& options) {
//   MOTION::Communication::TCPSetupHelper helper(options.my_id, options.tcp_config);
//   return std::make_unique<MOTION::Communication::CommunicationLayer>(options.my_id,
//                                                                      helper.setup_connections());
// }

// int main(int argc, char* argv[]) {
//   auto options = parse_program_options(argc, argv);
//   if (!options.has_value()) {
//     return EXIT_FAILURE;
//   }

//   try {
//     auto comm_layer = setup_communication(*options);
//     auto logger = std::make_shared<MOTION::Logger>(options->my_id,
//                                                    boost::log::trivial::severity_level::trace);
//     comm_layer->set_logger(logger);
//     MOTION::TwoPartyTensorBackend backend(*comm_layer, options->threads,
//                                           options->sync_between_setup_and_online, logger);
//     // run_composite_circuit(*options, backend);
//     comm_layer->shutdown();
//   } catch (std::runtime_error& e) {
//     std::cerr << "ERROR OCCURRED: " << e.what() << "\n";
//     return EXIT_FAILURE;
//   }
//   return EXIT_SUCCESS;
// }

/*
./bin/Image_Share_Generator --my-id 0 --party 0,::1,7003 --party 1,::1,7000 --arithmetic-protocol
beavy
--boolean-protocol yao --fractional-bits 13 --file-names file_config_image.txt --index 1

./bin/Image_Share_Generator --my-id 1 --party 0,::1,7003 --party 1,::1,7000 --arithmetic-protocol
beavy
--boolean-protocol yao --fractional-bits 13 --file-names file_config_image.txt --index 1

./image_provider_version2 --compute-server0-port 1234 --compute-server1-port 1235 --fractional-bits
13 --NameofImageFile X6_n.csv

*/

/* This code generates shares files for image,w1,b1,w2,b2. Aditionally it generates the ActualAnswer
of image in both the folders. The title of this file is read from the first line in the
file_config.txt This function needs a config file as input sample file_config_image_1.txt is as
follows
X
ip1


I have modified the image provider code to send actual answer as the first number via socket.
Also the imagefile eg.,X.csv should contain the first number as the actual answer.
I also modified compute server.cpp to send the extra number to the main function.
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
  std::vector<std::string> data;
  int actual_answer;
  std::vector<std::string> filepaths;
  std::string index;
};

void read_filenames(Options* options) {
  // read file_config.txt
  auto p = std::filesystem::current_path();
  p += "/" + options->filenames;
  std::cout << p << std::endl;

  std::ifstream indata;
  indata.open(p);
  if (std::ifstream(p)) {
    std::cout << "File found\n";
  } else {
    std::cout << "File not found\n";
  }
  assert(indata);

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

  //../server0/filenameshare_id0 / 1 only for image shares, for actual answer it is only X
  std::cout << "Size of data: " << options->data.size() << "\n";
  std::string n0 = options->data[0] + options->index;
  fullfilename = filename + n0;
  options->filepaths.push_back(fullfilename);
  // std::string n1 = options->data[1] + "_" + std::to_string(options->my_id);
  std::string n1 = options->data[1] + options->index;
  fullfilename = filename + n1;
  options->filepaths.push_back(fullfilename);
}

void retrieve_shares(int port_number, Options* options) {
  std::ofstream file;
  // options->actual_answer=COMPUTE_SERVER::get_actual_answer(port_number);

  auto [p1, p2, p3] = COMPUTE_SERVER::get_provider_mat_mul_data_new(port_number);
  options->actual_answer = p1;
  std::cout << "actual answer in main:" << options->actual_answer << "\n";
  auto temp0 = options->filepaths[0];
  file.open(temp0, std::ios_base::out);
  if (file) {
    std::cout << "File found\n";
  } else {
    std::cout << "File not found\n";
  }
  file << options->actual_answer;
  file.close();
  // auto pair1 = COMPUTE_SERVER::get_provider_mat_mul_data(port_number);
  // std::vector<COMPUTE_SERVER::Shares> input_values_dp0 = pair1.second.first;
  std::vector<COMPUTE_SERVER::Shares> input_values_dp0 = p3.first;

  // std::ofstream file;
  auto temp = options->filepaths[1];

  file.open(temp, std::ios_base::out);
  if (file) {
    std::cout << "File found\n";
  } else {
    std::cout << "File not found\n";
  }

  // options->image.row = pair1.second.second[0];
  // options->image.col = pair1.second.second[1];
  // options->fractional_bits = pair1.first;
  options->image.row = p3.second[0];
  options->image.col = p3.second[1];
  options->fractional_bits = p2;

  file << options->image.row << " " << options->image.col << "\n";
  std::cout << "Size:" << input_values_dp0.size() << "\n";
  for (int i = 0; i < input_values_dp0.size(); i++) {
    options->image.Delta.push_back(input_values_dp0[i].Delta);
    options->image.delta.push_back(input_values_dp0[i].delta);
    std::cout << "Share number:" << i << "\n";
    file << input_values_dp0[i].Delta << " " << input_values_dp0[i].delta << "\n";
  }

  file.close();
  int count = 0;
  string ch;
  string s1 = " ";
  string s2 = "\n";
  std::cout << "File path:" << temp << "\n";
  std::ifstream mFile;
  mFile.open(temp);
  if (mFile.is_open()) {
    while (mFile >> std::noskipws >> ch) {
      if (ch != s1) {
        std::cout << ch << "\n";
        count++;
      } else {
        break;
      }
    }

    mFile.close();
    std::cout << "Number of lines in the file are: " << count << std::endl;
  } else
    std::cout << "Couldn't open the file\n";
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
    ("index",po::value<std::string>()->default_value("0"), "index")
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
  options.index = vm["index"].as<std::string>();
  std::cout << "index" << options.index << "\n";
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

  // if (options.weights[0].col != options.image.row) {
  //   std::cout << options.weights[0].col << "image :"<<options.image.row<< "\n";
  //   std::cerr
  //       << "Invalid inputs, number of columns for dp0 must be equal to number of rows for dp1
  //       \n";
  //   return std::nullopt;
  // }

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
