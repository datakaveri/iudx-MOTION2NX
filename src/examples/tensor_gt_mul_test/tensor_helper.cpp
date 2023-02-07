/*
inputpath changed to imageprovider
imageprovider is file name inside server 0 and server 1 respectively
eg. (ip1_0 , ip1_1) , (ip2_0,ip2_1) and so on..
from bash file we are giving ip1 and in this file it is appended to ip1_0 and ip1_1

At the argument "--filepath " give the path of the file containing shares from build_deb.... folder
Server-0
./bin/tensor_gt_mul --my-id 0 --party 0,::1,7002 --party 1,::1,7000 --arithmetic-protocol beavy
--boolean-protocol yao --fractional-bits 13 --config-file-model file_config_model0 --layer-id 1

Server-1
./bin/tensor_gt_mul --my-id 1 --party 0,::1,7002 --party 1,::1,7000 --arithmetic-protocol beavy
--boolean-protocol yao --fractional-bits 13 --config-file-model file_config_model1 --layer-id 1

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

#include <bits/stdc++.h>
#include <filesystem>
#include <fstream>
#include <utility>
#include "communication/communication_layer.h"
#include "communication/message_handler.h"
#include "communication/tcp_transport.h"
#include "utility/logger.h"

#include <boost/algorithm/string.hpp>
#include <boost/json/serialize.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/log/trivial.hpp>
#include <boost/program_options.hpp>

#include <iostream>
#include <iterator>
#include <parallel/algorithm>
#include <vector>

namespace po = boost::program_options;
int j = 0;

struct Matrix {
  std::vector<uint64_t> Delta;
  std::vector<uint64_t> delta;
  int row;
  int col;
};

struct Options {
  //////////////////////////changes////////////////////////////
  int num_elements;
  //////////////////////////////////////////////////////////////
  std::string imageprovider;
  std::string modelpath;
  std::size_t layer_id;
  std::size_t my_id;
  Matrix image_file;
  Matrix W_file;
  Matrix B_file;
  Matrix row;
  Matrix col;
};

//////////////////New functions////////////////////////////////////////
/// In read_file also include file not there error and file empty alerts
std::uint64_t read_file(std::ifstream& pro) {
  std::string str;
  char num;
  while (pro >> std::noskipws >> num) {
    if (num != ' ' && num != '\n') {
      str.push_back(num);
    } else {
      break;
    }
  }

  std::string::size_type sz = 0;
  std::uint64_t ret = (uint64_t)std::stoull(str, &sz, 0);
  return ret;
}

// this function reads all lines but takes into consideration only the required input
// if j==count , the program will not make str empty and return it once j>count
std::string read_filepath(std::ifstream& indata, int count) {
  std::string str;

  while (j <= count) {
    char num;
    while (indata) {
      std::getline(indata, str);

      if (j < count) {
        str = " ";
      }
      j++;
      break;
    }
  }
  return str;
}

void image_shares(Options* options, std::string p, std::string q) {
  std::ifstream temps;
  temps.open(p);

  std::ofstream file1;
  file1.open(q);
  // std::cout << "p:" << p << "\n";
  if (temps) {
    std::cout << "File found\n";
  } else {
    std::cout << "File not found\n";
  }
  assert(temps);

  std::uint64_t rows = read_file(temps);
  options->image_file.row = rows;
  file1 << rows << " ";
  std::cout << "r " << rows << " ";
  std::uint64_t cols = read_file(temps);
  file1 << cols << "\n";
  options->image_file.col = cols;
  std::cout << "c " << cols << "\n";

  for (int i = 0; i < rows * cols; ++i) {
    uint64_t m1 = read_file(temps);

    // std::cout << m1 << " ";
    options->image_file.Delta.push_back(m1);
    uint64_t m2 = read_file(temps);
    file1 << m2 << "\n";
    // std::cout << m2 << "\n";
    options->image_file.delta.push_back(m2);
  }
  temps.close();
}

void W_shares(Options* options, std::string p) {
  std::ifstream indata;
  indata.open(p);
  if (std::ifstream(p)) {
    std::cout << "File found\n";
  } else {
    std::cout << "File not found\n";
  }
  assert(indata);

  int rows = read_file(indata);
  options->W_file.row = rows;
  std::cout << "r " << rows << " ";
  int cols = read_file(indata);
  options->W_file.col = cols;
  std::cout << "c " << cols << "\n";

  for (int i = 0; i < rows * cols; ++i) {
    uint64_t m1 = read_file(indata);
    options->W_file.Delta.push_back(m1);
    uint64_t m2 = read_file(indata);
    options->W_file.delta.push_back(m2);
  }
  indata.close();
}

void B_shares(Options* options, std::string p) {
  std::ifstream indata;
  indata.open(p);

  if (std::ifstream(p)) {
    std::cout << "File found\n";
  } else {
    std::cout << "File not found\n";
  }
  assert(indata);

  int rows = read_file(indata);
  options->B_file.row = rows;
  std::cout << "r " << rows << " ";
  int cols = read_file(indata);
  options->B_file.col = cols;
  std::cout << "c " << cols << "\n";

  for (int i = 0; i < rows * cols; ++i) {
    uint64_t m1 = read_file(indata);
    options->B_file.Delta.push_back(m1);
    uint64_t m2 = read_file(indata);
    options->B_file.delta.push_back(m2);
  }
  indata.close();
}

// changes made in this function
void file_read(Options* options) {
  std::string path = std::filesystem::current_path();
  std::string t1, i, t3;
  // if layer_id=1 then read filename inside server
  // else read file_config_input (id is appended)
  if (options->layer_id == 1) {
    std::cout << "hello\n";
    t1 = path + "/server" + std::to_string(options->my_id) + "/" + options->imageprovider;
    t3 = path + "/server" + std::to_string(options->my_id) + "/" + options->imageprovider + "_" +
         std::to_string(options->my_id);
  } else if (options->layer_id > 1) {
    // outputshare_0/1 inside server 1/0
    t1 = path + "/server" + std::to_string(options->my_id) + "/" + options->imageprovider + "_" +
         std::to_string(options->my_id);
  }

  image_shares(options, t1, t3);
  std::cout << "i:" << t1 << "\n";
  // model path

  std::string t2 = path + "/" + options->modelpath;
  std::cout << "t2:" << t2 << "\n";

  std::ifstream file2;
  file2.open(t2);
  if (file2) {
    std::cout << "File found\n";
  } else {
    std::cout << "File not found\n";
  }

  // file1.close();

  j = 0;

  //  skip 2(layer_id-1) and read first 2 lines
  int c = 2 * (options->layer_id - 1);
  std::string w = read_filepath(file2, c);

  std::cout << "w:" << w << "\n";
  std::cout << "j:" << j << "\n";
  std::string b = read_filepath(file2, c + 1);
  std::cout << "b:" << b << "\n";
  file2.close();

  W_shares(options, w);
  B_shares(options, b);
}

//////////////////////////////////////////////////////////////////////
std::optional<Options> parse_program_options(int argc, char* argv[]) {
  Options options;
  boost::program_options::options_description desc("Allowed options");
  // clang-format off
  desc.add_options()
    ("help,h", po::bool_switch()->default_value(false),"produce help message")
    ("config-file-input", po::value<std::string>()->required(), "config file containing options")
    ("config-file-model", po::value<std::string>()->required(), "config file containing options")
    ("my-id", po::value<std::size_t>()->required(), "my party id")
    ("layer-id", po::value<std::size_t>()->required(), "layer id")
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
  //////////////////////////////////////////////////////////////////
  options.imageprovider = vm["config-file-input"].as<std::string>();
  options.modelpath = vm["config-file-model"].as<std::string>();
  options.layer_id = vm["layer-id"].as<std::size_t>();

  /////////////////////////////////////////////////////////////////
  if (options.my_id > 1) {
    std::cerr << "my-id must be one of 0 and 1\n";
    return std::nullopt;
  }

  //////////////////////////////////////////////////////////////////
  file_read(&options);
  ////////////////////////////////////////////////////////////////////

  return options;
}

std::uint64_t getuint64(std::vector<std::uint8_t>& message, int index) {
  std::uint64_t num = 0;
  for (auto i = 0; i < 8; i++) {
    num = num << 8;
    num = num | message[(index + 1) * 8 - 1 - i];
  }
  return num;
}

class TestMessageHandler : public MOTION::Communication::MessageHandler {
  void received_message(std::size_t party_id, std::vector<std::uint8_t>&& message) {
    std::cout << "Message received from party " << party_id << ":";
    std::ofstream file("out1");
    std::uint64_t rows, col;
    rows = getuint64(message, 0);
    col = getuint64(message, 1);
    file << rows << " " << col << std::endl;
    for (auto i = 2; i < rows * col; ++i) {
      auto num1 = getuint64(message, i);
      // auto num2 = getuint64(message, 2 + i + 1);
      std::cout << num1 << std::endl;
      file << num1 << "\n";
    }
    std::cout << std::endl;
  }
};

// auto create_composite_circuit(const Options& options, MOTION::TwoPartyTensorBackend& backend) {
//  retrieve the gate factories for the chosen protocols
//    auto& arithmetic_tof = backend.get_tensor_op_factory(options.arithmetic_protocol);
//    auto& boolean_tof = backend.get_tensor_op_factory(MOTION::MPCProtocol::Yao);

//   ////////////////Using below to set size of input tensor/////////////////////
//   ////////Looks clumsy but could not fix it
//   /*const MOTION::tensor::GemmOp gemm_op1 = {
//       .input_A_shape_ = {256, 784}, .input_B_shape_ = {784, 1}, .output_shape_ = {256, 1}};
// */

//   std::cout << options.W_file.row << " " << options.W_file.col << "\n";
//   std::cout << options.image_file.row << " " << options.image_file.col << "\n";

//   const MOTION::tensor::GemmOp gemm_op1 = {
//       .input_A_shape_ = {options.W_file.row, options.W_file.col},
//       .input_B_shape_ = {options.image_file.row, options.image_file.col},
//       .output_shape_ = {options.W_file.row, options.image_file.col}};

//   const auto W1_dims = gemm_op1.get_input_A_tensor_dims();

//   const auto X_dims = gemm_op1.get_input_B_tensor_dims();
//   const auto B1_dims = gemm_op1.get_output_tensor_dims();

//   /////////////////////////////////////////////////////////////////////////
//   MOTION::tensor::TensorCP tensor_X, tensor_W1, tensor_B1;
//   MOTION::tensor::TensorCP gemm_output1, add_output1;

//   auto pairX = arithmetic_tof.make_arithmetic_64_tensor_input_shares(X_dims);
//   std::vector<ENCRYPTO::ReusableFiberPromise<MOTION::IntegerValues<uint64_t>>> input_promises_X
//   =
//       std::move(pairX.first);
//   tensor_X = pairX.second;

//   auto pairW1 = arithmetic_tof.make_arithmetic_64_tensor_input_shares(W1_dims);
//   std::vector<ENCRYPTO::ReusableFiberPromise<MOTION::IntegerValues<uint64_t>>>
//   input_promises_W1 =
//       std::move(pairW1.first);
//   tensor_W1 = pairW1.second;

//   auto pairB1 = arithmetic_tof.make_arithmetic_64_tensor_input_shares(B1_dims);
//   std::vector<ENCRYPTO::ReusableFiberPromise<MOTION::IntegerValues<uint64_t>>>
//   input_promises_B1 =
//       std::move(pairB1.first);
//   tensor_B1 = pairB1.second;

//   ///////////////////////////////////////////////////////////////
//   input_promises_X[0].set_value(options.image_file.Delta);
//   input_promises_X[1].set_value(options.image_file.delta);

//   input_promises_W1[0].set_value(options.W_file.Delta);
//   input_promises_W1[1].set_value(options.W_file.delta);

//   input_promises_B1[0].set_value(options.B_file.Delta);
//   input_promises_B1[1].set_value(options.B_file.delta);
//   ///////////////////////////////////////////////////////////////////

//   gemm_output1 =
//       arithmetic_tof.make_tensor_gemm_op(gemm_op1, tensor_W1, tensor_X,
//       options.fractional_bits);
//   add_output1 = arithmetic_tof.make_tensor_add_op(gemm_output1, tensor_B1);

//   ENCRYPTO::ReusableFiberFuture<std::vector<std::uint64_t>> output_future, main_output_future,
//       main_output;

//   if (options.my_id == 0) {
//     arithmetic_tof.make_arithmetic_tensor_output_other(add_output1);
//   } else {
//     main_output_future = arithmetic_tof.make_arithmetic_64_tensor_output_my(add_output1);
//   }

//   return std::move(main_output_future);
//}

// void run_composite_circuit(const Options& options, MOTION::TwoPartyTensorBackend& backend) {
//  auto output_future = create_composite_circuit(options, backend);
//  backend.run();
//  if (options.my_id == 1) {
//    auto main = output_future.get();

//   for (int i = 0; i < main.size(); ++i) {
//     long double temp =
//         MOTION::new_fixed_point::decode<uint64_t, long double>(main[i],
//         options.fractional_bits);

//     std::cout << temp << " , ";
//   }
// }
//}

int main(int argc, char* argv[]) {
  auto options = parse_program_options(argc, argv);

  const auto localhost = "127.0.0.1";
  const auto num_parties = 2;
  int my_id = options->my_id;

  // MOTION::Communication::tcp_parties_config config;
  // config.reserve(num_parties);
  // for (std::size_t party_id = 0; party_id < num_parties; ++party_id) {
  //   config.push_back({localhost, 10000 + party_id});
  // };

  // MOTION::Communication::TCPSetupHelper helper(my_id, config);
  // auto comm_layer = std::make_unique<MOTION::Communication::CommunicationLayer>(
  //     my_id, helper.setup_connections());

  // auto logger = std::make_shared<MOTION::Logger>(my_id,
  // boost::log::trivial::severity_level::trace); comm_layer->set_logger(logger);
  // comm_layer->register_fallback_message_handler(
  //     [](auto party_id) { return std::make_shared<TestMessageHandler>(); });
  // comm_layer->start();
}
