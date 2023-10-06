/*
This code reads shares from build_debwithrelinfo/ShareFiles/A.txt and
build_debwithrelinfo/ShareFiles/B.txt and performs a ReLU operation on them.
If this code is run with a function to write output shares (in tensor_op.cpp)
output shares of this will be written. The following instructions run this code.

At the argument "--filepath " give the path of the file containing shares from build_deb.... folder
Server-0
./bin/tensor_gt_mul --my-id 0 --party 0,::1,7002 --party 1,::1,7000 --arithmetic-protocol beavy --boolean-protocol yao --fractional-bits 13 --config-file-input file_config_input0 --config-file-model file_config_model0 --layer-id 1

Server-1
./bin/tensor_gt_mul --my-id 1 --party 0,::1,7002 --party 1,::1,7000 --arithmetic-protocol beavy --boolean-protocol yao --fractional-bits 13  --config-file-input file_config_input1 --config-file-model file_config_model1 --layer-id 1

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
#include "utility/new_fixed_point.h"

namespace po = boost::program_options;
namespace fp = MOTION::new_fixed_point;
int j = 0;

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
  //////////////////////////changes////////////////////////////
  int num_elements;
  //////////////////////////////////////////////////////////////
  std::size_t fractional_bits;
  std::string inputpath;
  std::string modelpath;
  std::size_t layer_id;

  std::size_t my_id;
  // std::string filepath_frombuild;
  MOTION::Communication::tcp_parties_config tcp_config;
  bool no_run = false;
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

void image_shares(Options* options, std::string p) {
  std::ifstream temps;
  temps.open(p);
  // std::cout << "p:" << p << "\n";
  if (temps) {
    std::cout << "image shares File found\n";
  } else {
    std::cout << "image shares File not found\n";
  }
  assert(temps);

  std::uint64_t rows = read_file(temps);
  options->image_file.row = rows;
  std::cout << "r " << rows << " ";
  std::uint64_t cols = read_file(temps);
  options->image_file.col = cols;
  std::cout << "c " << cols << "\n";

  for (int i = 0; i < rows * cols; ++i) {
    uint64_t m1 = read_file(temps);
    
    options->image_file.Delta.push_back(m1);
    uint64_t m2 = read_file(temps);
    
    std::cout << m1 << ' ' << m2 << std::endl;
    options->image_file.delta.push_back(m2);
  }
  temps.close();
}

void W_shares(Options* options, std::string p) {
  std::ifstream indata;
  indata.open(p);
  std :: cout << "\n W_shares path :" << p;
  if (std::ifstream(p)) {
    std::cout << "\n W_shares File found\n";
  } else {
    std::cout << "\n W_shares File NOT found\n";
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
    std::cout << m1 << ' ' << m2 << std::endl;
  }
  indata.close();
}

void B_shares(Options* options, std::string p) {
  std::ifstream indata;
  indata.open(p);

  if (std::ifstream(p)) {
    std::cout << "\n B_shares File found\n";
  } else {
    std::cout << "\n B_shares File not found\n";
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

void file_read(Options* options) {
  std::string path = std::filesystem::current_path();
  std::string t1 = path + "/" + options->inputpath;
  std::string t2 = path + "/" + options->modelpath;
   std::cout << "\n t1 :" << t1 << "\n";
   std::cout << "\n t2 :" << t2 << "\n";
  std::ifstream file1, file2;
  file1.open(t1);
  file2.open(t2);
  if (file1)
  {
    std:: cout << "\n File Read, t1 :" << "File found\n";
  }
  else
  {
    std:: cout << "\n File Read, t1 :" << "File NOT found\n";
  }

  if (file2)
  {
    std:: cout << "\n File Read, t2 :" << "File found\n";
  }
  else
  {
    std:: cout << "\n File Read, t2 :" << "File NOT found\n";
  }
  std :: string i;
  std :: cout << "*****My ID is : " << options->my_id << "\n";
  if (options->my_id == 0){
    std :: cout << "*****My ID is : " << options->my_id << "\n";
  i = "/home/iudx/motion2nx-LATEST/iudx-MOTION2NX/build_debwithrelinfo_gcc/server0/outputshare_0";
  }
  else{
    i = "/home/iudx/motion2nx-LATEST/iudx-MOTION2NX/build_debwithrelinfo_gcc/server1/outputshare_1";
  }
  //std::string i = read_filepath(file1, 0);
  std::cout << "image shares path i :" << i << "\n";

  file1.close();

  j = 0;
  image_shares(options, i);

  //  skip 2(layer_id-1) and read first 2 lines
  int c = 2 * (options->layer_id - 1);
  //std::string w = read_filepath(file2, c);
  std::string w;
  if (options->my_id == 0)
  {
   w = "/home/iudx/motion2nx-LATEST/iudx-MOTION2NX/build_debwithrelinfo_gcc/server0/W2";
  }
  if (options->my_id == 1)
  {
   w = "/home/iudx/motion2nx-LATEST/iudx-MOTION2NX/build_debwithrelinfo_gcc/server1/W2";
  }

  std::cout << "w:" << w << "\n";
  //std::cout << "j:" << j << "\n";
  // std::string b = read_filepath(file2, c + 1);
  // std::cout << "b:" << b << "\n";
  file2.close();

  W_shares(options, w);
  //B_shares(options, b);
}

//////////////////////////////////////////////////////////////////////
std::optional<Options> parse_program_options(int argc, char* argv[]) {
  Options options;
  boost::program_options::options_description desc("Allowed options");
  // clang-format off
  desc.add_options()
    ("help,h", po::bool_switch()->default_value(false),"produce help message")
    ("config-file-input", po::value<std::string>()->required(), "config file containing options")
    ("config-file-model", po::value<std::string>()->default_value(""), "config file containing options")
    ("my-id", po::value<std::size_t>()->required(), "my party id")
    ("layer-id", po::value<std::size_t>()->required(), "layer id")
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
  //////////////////////////////////////////////////////////////////
  options.inputpath = vm["config-file-input"].as<std::string>();
  options.modelpath = vm["config-file-model"].as<std::string>();
  options.layer_id = vm["layer-id"].as<std::size_t>();
  /////////////////////////////////////////////////////////////////
  options.fractional_bits = vm["fractional-bits"].as<std::size_t>();
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

  //////////////////////////////////////////////////////////////////
  file_read(&options);
  ////////////////////////////////////////////////////////////////////

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

void print_stats(const Options& options,
                 const MOTION::Statistics::AccumulatedRunTimeStats& run_time_stats,
                 const MOTION::Statistics::AccumulatedCommunicationStats& comm_stats) {
  if (options.json) {
    auto obj = MOTION::Statistics::to_json("tensor_gt_mul", run_time_stats, comm_stats);
    obj.emplace("party_id", options.my_id);
    obj.emplace("arithmetic_protocol", MOTION::ToString(options.arithmetic_protocol));
    obj.emplace("boolean_protocol", MOTION::ToString(options.boolean_protocol));
    obj.emplace("simd", options.num_simd);
    obj.emplace("threads", options.threads);
    obj.emplace("sync_between_setup_and_online", options.sync_between_setup_and_online);
    std::cout << obj << "\n";
  } else {
    std::cout << MOTION::Statistics::print_stats("tensor_gt_mul", run_time_stats, comm_stats);
  }
}

auto create_composite_circuit(const Options& options, MOTION::TwoPartyTensorBackend& backend) {
  // retrieve the gate factories for the chosen protocols
  auto& arithmetic_tof = backend.get_tensor_op_factory(options.arithmetic_protocol);
  auto& boolean_tof = backend.get_tensor_op_factory(MOTION::MPCProtocol::Yao);

  const MOTION::tensor::GemmOp gemm_op1 = {
      .input_A_shape_ = {2, 2},
      .input_B_shape_ = {2, 2},
      .output_shape_ = {2,2},
      .transA_ = {false}};

  const auto W1_dims = gemm_op1.get_input_A_tensor_dims();
  const auto X_dims = gemm_op1.get_input_B_tensor_dims();
  // const auto B1_dims = gemm_op1.get_output_tensor_dims();

  /////////////////////////////////////////////////////////////////////////
  MOTION::tensor::TensorCP tensor_X, tensor_W1;//, tensor_B1;
  MOTION::tensor::TensorCP cmm_output1;//, add_output1;

  auto pairX = arithmetic_tof.make_arithmetic_64_tensor_input_shares(X_dims);
  std::vector<ENCRYPTO::ReusableFiberPromise<MOTION::IntegerValues<uint64_t>>> input_promises_X =
      std::move(pairX.first);
  tensor_X = pairX.second;
  ///////////////////////////////////////////////////////////////
  input_promises_X[0].set_value(options.image_file.Delta);
  input_promises_X[1].set_value(options.image_file.delta);

  std :: cout << "CONST_model_data_shares \n";
  std :: vector<float> W_const;
  std :: vector<uint64_t> W_encoded(4, 0);
  W_const.push_back(-1);
  W_const.push_back(-2);
  W_const.push_back(-3);
  W_const.push_back(-4);
  auto frac_bits = options.fractional_bits; 
  std::transform(std::begin(W_const), std::end(W_const), std::begin(W_encoded), [frac_bits](auto j) { return fp::encode<std::uint64_t, float>(j, 13); });

  // std ::cout << "in main cpp :" << gemm_op1.compute_output_size() << "\n";
  ///////////////////////////////////////////////////////////////////
std:: cout << "\n 494: calling make_tensor_constMatrix_Mul_op \n";
  //  gemm_output1 =
  //      arithmetic_tof.make_tensor_gemm_op(gemm_op1, tensor_W1, tensor_X, options.fractional_bits);
  cmm_output1 = arithmetic_tof.make_tensor_constMatrix_Mul_op(gemm_op1,  W_encoded, tensor_X, options.fractional_bits);
  ENCRYPTO::ReusableFiberFuture<std::vector<std::uint64_t>> output_future, main_output_future,
      main_output;

  if (options.my_id == 0) {
    arithmetic_tof.make_arithmetic_tensor_output_other(cmm_output1);
  } else {
    main_output_future = arithmetic_tof.make_arithmetic_64_tensor_output_my(cmm_output1);
  }

  return std::move(main_output_future);
}

void run_composite_circuit(const Options& options, MOTION::TwoPartyTensorBackend& backend) {
  auto output_future = create_composite_circuit(options, backend);
  backend.run();
  if (options.my_id == 1) {
    auto main = output_future.get();
    std::cout << main.size() << std::endl;
    for (int i = 0; i < main.size(); ++i) {
      long double temp =
          MOTION::new_fixed_point::decode<uint64_t, long double>(main[i], options.fractional_bits);

      std::cout << temp << " , ";
    }
  }
}

int main(int argc, char* argv[]) {
  std::cout << "Inside main";
  auto options = parse_program_options(argc, argv);
  if (!options.has_value()) {
    return EXIT_FAILURE;
  }

  try {
    auto comm_layer = setup_communication(*options);
    auto logger = std::make_shared<MOTION::Logger>(options->my_id,
                                                   boost::log::trivial::severity_level::trace);
    comm_layer->set_logger(logger);
    MOTION::Statistics::AccumulatedRunTimeStats run_time_stats;
    MOTION::Statistics::AccumulatedCommunicationStats comm_stats;
    MOTION::TwoPartyTensorBackend backend(*comm_layer, options->threads,
                                          options->sync_between_setup_and_online, logger);
    run_composite_circuit(*options, backend);
    comm_layer->sync();
    comm_stats.add(comm_layer->get_transport_statistics());
    comm_layer->reset_transport_statistics();
    run_time_stats.add(backend.get_run_time_stats());
    comm_layer->shutdown();
    print_stats(*options, run_time_stats, comm_stats);
  } catch (std::runtime_error& e) {
    std::cerr << "ERROR OCCURRED: " << e.what() << "\n";
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}