/*
This code reads shares from build_debwithrelinfo/ShareFiles/A.txt and
build_debwithrelinfo/ShareFiles/B.txt and performs a ReLU operation on them.
If this code is run with a function to write output shares (in tensor_op.cpp)
output shares of this will be written. The following instructions run this code.

At the argument "--filepath " give the path of the file containing shares from build_deb.... folder
Server-0
./bin/tensor_argmax --my-id 0 --party 0,::1,7002 --party 1,::1,7001 --arithmetic-protocol beavy
--boolean-protocol yao --fractional-bits 13 --filepath file_config_input0 --current-path
${BASE_DIR}/build_debwithrelinfo_gcc

Server-1
./bin/tensor_argmax --my-id 1 --party 0,::1,7002 --party 1,::1,7001 --arithmetic-protocol beavy
--boolean-protocol yao --repetitions 1 --fractional-bits 13 --filepath file_config_input1
--current-path ${BASE_DIR}/build_debwithrelinfo_gcc
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
#include <algorithm>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <numeric>
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

void testMemoryOccupied(bool WriteToFiles, int my_id, std::string path) {
  int tSize = 0, resident = 0, share = 0;
  std::ifstream buffer("/proc/self/statm");
  buffer >> tSize >> resident >> share;
  buffer.close();

  long page_size_kb =
      sysconf(_SC_PAGE_SIZE) / 1024;  // in case x86-64 is configured to use 2MB pages
  double rss = resident * page_size_kb;
  std::cout << "RSS - " << rss << " kB\n";
  double shared_mem = share * page_size_kb;
  std::cout << "Shared Memory - " << shared_mem << " kB\n";
  std::cout << "Private Memory - " << rss - shared_mem << "kB\n";
  std::cout << std::endl;
  if (WriteToFiles == 1) {
    /////// Generate path for the AverageMemoryDetails file and MemoryDetails file
    std::string t1 = path + "/" + "AverageMemoryDetails" + std::to_string(my_id);
    std::string t2 = path + "/" + "MemoryDetails" + std::to_string(my_id);

    ///// Write to the AverageMemoryDetails files
    std::ofstream file1;
    file1.open(t1, std::ios_base::app);
    file1 << rss;
    file1 << "\n";
    file1.close();

    std::ofstream file2;
    file2.open(t2, std::ios_base::app);
    file2 << "Argmax: \n";
    file2 << "RSS - " << rss << " kB\n";
    file2 << "Shared Memory - " << shared_mem << " kB\n";
    file2 << "Private Memory - " << rss - shared_mem << "kB\n";
    file2.close();
  }
}

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
  //////////////////////////changes////////////////////////////
  Matrix input;
  Matrix index;
  std::uint64_t num_elements;
  std::uint64_t column_size;
  std::string currentpath;
  //////////////////////////////////////////////////////////////
  std::size_t fractional_bits;
  std::size_t my_id;
  std::string filepath_frombuild;
  MOTION::Communication::tcp_parties_config tcp_config;
  bool no_run = false;
};

bool is_empty(std::ifstream& file) { return file.peek() == std::ifstream::traits_type::eof(); }

//////////////////New functions////////////////////////////////////////
/// In read_file also include file not there error and file empty alerts
std::uint64_t read_file(std::ifstream& indata) {
  std::string str;
  char num;
  while (indata >> std::noskipws >> num) {
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

int input_shares(Options* options, std::string p) {
  std::ifstream indata1, indata2;

  try {
    indata1.open(p);
    if (indata1) {
      std::cout << "File found\n";
    } else {
      std::cout << "File not found\n";
    }
  } catch (std::ifstream::failure e) {
    std::cerr << "Error while opening the input share file.\n";
    return EXIT_FAILURE;
  }

  try {
    if (is_empty(indata1)) {
      // file is empty
    }
  } catch (std::ifstream::failure e) {
    std::cerr << "Image share input file is empty.\n";
    return EXIT_FAILURE;
  }

  int num_elements, column_size;
  try {
    num_elements = read_file(indata1);
    column_size = read_file(indata1);
  } catch (std::ifstream::failure e) {
    std::cerr << "Error while reading columns from image shares.\n";
    return EXIT_FAILURE;
  }

  options->num_elements = num_elements;
  options->column_size = column_size;

  auto k = 0;
  while (k < options->num_elements * options->column_size * 2) {
    std::uint64_t num = read_file(indata1);
    if (indata1.eof()) {
      std::cerr << "File contains less number of elements" << std::endl;
      return EXIT_FAILURE;
    }
    k++;
  }

  indata1.close();
  indata2.open(p);

  options->num_elements = read_file(indata2);
  options->column_size = read_file(indata2);
  // std::cout << options->num_elements << " " << options->column_size << "\n";
  for (int i = 0; i < options->num_elements; ++i) {
    try {
      uint64_t m1 = read_file(indata2);
      options->input.Delta.push_back(m1);
      uint64_t m2 = read_file(indata2);
      options->input.delta.push_back(m2);
    } catch (std::ifstream::failure e) {
      std::cerr << "Error while reading columns from image shares.\n";
      return EXIT_FAILURE;
    }
  }
  indata2.close();
}

int file_read(Options* options) {
  std::string path = options->currentpath;
  // std::string path = std::filesystem::current_path();
  std::string t1 = path + "/" + options->filepath_frombuild;

  std::ifstream file1;

  std::cout << t1 << "\n";
  try {
    file1.open(t1);
    if (file1) {
      std::cout << "File found\n";
    } else {
      std::cout << "File not found\n";
    }
  } catch (std::ifstream::failure e) {
    std::cerr << "Error while opening config_model file.\n";
    return EXIT_FAILURE;
  }

  try {
    if (is_empty(file1)) {
      std::cout << "File is empty\n";
    }
  } catch (std::ifstream::failure e) {
    std::cerr << "config_file_model is empty.\n";
    return EXIT_FAILURE;
  }

  std::string str;
  char t;
  while (file1 >> std::noskipws >> t) {
    if (t != ' ' && t != '\n') {
      str.push_back(t);
    } else {
      break;
    }
  }
  std::cout << str << "\n";

  input_shares(options, str);
  file1.close();
}

//////////////////////////////////////////////////////////////////////
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
    ("filepath", po::value<std::string>()->required(), "Path of the shares file from build_debwithrelinfo folder")
    ("repetitions", po::value<std::size_t>()->default_value(1), "number of repetitions")
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
  options.filepath_frombuild = vm["filepath"].as<std::string>();
  options.currentpath = vm["current-path"].as<std::string>();
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
    auto obj = MOTION::Statistics::to_json("tensor_gt_relu", run_time_stats, comm_stats);
    obj.emplace("party_id", options.my_id);
    obj.emplace("arithmetic_protocol", MOTION::ToString(options.arithmetic_protocol));
    obj.emplace("boolean_protocol", MOTION::ToString(options.boolean_protocol));
    obj.emplace("simd", options.num_simd);
    obj.emplace("threads", options.threads);
    obj.emplace("sync_between_setup_and_online", options.sync_between_setup_and_online);
    // std::cout << obj << "\n";
  } else {
    std::cout << MOTION::Statistics::print_stats("tensor_gt_relu", run_time_stats, comm_stats);
  }
}

auto create_composite_circuit(const Options& options, MOTION::TwoPartyTensorBackend& backend) {
  // retrieve the gate factories for the chosen protocols
  auto& arithmetic_tof = backend.get_tensor_op_factory(options.arithmetic_protocol);
  auto& yao_tof = backend.get_tensor_op_factory(options.boolean_protocol);
  auto& boolean_tof = backend.get_tensor_op_factory(MOTION::MPCProtocol::BooleanBEAVY);

  auto frac_bits = options.fractional_bits;
  std::vector<float> arr = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
  // std::vector<float> arr(700);
  // std::iota(arr.begin(), arr.end(), 1);
  std::vector<uint64_t> indx_vec;

  for (int i = 0; i < arr.size(); i++) {
    auto temp = MOTION::new_fixed_point::encode<uint64_t, float>(arr[i], frac_bits);
    indx_vec.push_back(temp);
  }

  std::cout << "The indexes are :\n";
  for (int i = 0; i < indx_vec.size(); i++) {
    std::cout << indx_vec[i] << " ";
  }
  std::cout << "\n";

  MOTION::tensor::TensorDimensions tensor_dims;
  tensor_dims.batch_size_ = 1;
  tensor_dims.num_channels_ = 1;
  tensor_dims.height_ = options.num_elements;
  tensor_dims.width_ = 1;

  const MOTION::tensor::MaxPoolOp maxpool_op = {.input_shape_ = {1, options.num_elements, 1},
                                                .output_shape_ = {1, 1, 1},
                                                .kernel_shape_ = {options.num_elements, 1},
                                                .strides_ = {1, 1}};

  const MOTION::tensor::GemmOp gemm_op1 = {.input_A_shape_ = {options.num_elements, 1},
                                           .input_B_shape_ = {1, 1},
                                           .output_shape_ = {options.num_elements, 1}};

  const MOTION::tensor::GemmOp gemm_op2 = {.input_A_shape_ = {1, options.num_elements},
                                           .input_B_shape_ = {options.num_elements, 1},
                                           .output_shape_ = {1, 1}};

  std ::vector<float> constant;
  constant.assign(10, 1);
  std ::vector<uint64_t> constant_encoded(10, 0);

  std::transform(
      std::begin(constant), std::end(constant), std::begin(constant_encoded),
      [frac_bits](auto j) { return MOTION::new_fixed_point::encode<std::uint64_t, float>(j, 13); });

  const auto input_A_dims = maxpool_op.get_input_tensor_dims();
  const auto input_B_dims = maxpool_op.get_input_tensor_dims();
  const auto output_dims = maxpool_op.get_output_tensor_dims();

  /////////////////////////////////////////////////////////////////////////
  MOTION::tensor::TensorCP tensor_input;
  auto pair_input = arithmetic_tof.make_arithmetic_64_tensor_input_shares(tensor_dims);
  std::vector<ENCRYPTO::ReusableFiberPromise<MOTION::IntegerValues<uint64_t>>> input_vector =
      std::move(pair_input.first);
  tensor_input = pair_input.second;
  assert(tensor_input);

  ///////////////////////////////////////////////////////////////
  input_vector[0].set_value(options.input.Delta);
  input_vector[1].set_value(options.input.delta);
  ///////////////////////////////////////////////////////////////////

  std::function<MOTION::tensor::TensorCP(const MOTION::tensor::TensorCP&)> make_maxPool;
  make_maxPool = [&](const auto& input) {
    const auto yao_tensor = yao_tof.make_tensor_conversion(MOTION::MPCProtocol::Yao, input);
    const auto maxPool_tensor = yao_tof.make_tensor_maxpool_op(maxpool_op, yao_tensor);
    return yao_tof.make_tensor_conversion(options.arithmetic_protocol, maxPool_tensor);
  };

  std::function<MOTION::tensor::TensorCP(const MOTION::tensor::TensorCP&)> make_activation,
      make_relu;
  std::function<MOTION::tensor::TensorCP(const MOTION::tensor::TensorCP&, std::size_t)>
      make_indicator;
  // -RELU(-X)u
  make_activation = [&](const auto& input) {
    //  const auto negated_tensor = arithmetic_tof.make_tensor_negate(input);
    const auto boolean_tensor = yao_tof.make_tensor_conversion(MOTION::MPCProtocol::Yao, input);
    const auto relu_tensor = yao_tof.make_tensor_relu_op(boolean_tensor);
    return yao_tof.make_tensor_conversion(options.arithmetic_protocol, relu_tensor);
  };
  //  RELU(X)
  make_relu = [&](const auto& input) {
    const auto negated_tensor = arithmetic_tof.make_tensor_negate(input);
    const auto boolean_tensor =
        yao_tof.make_tensor_conversion(MOTION::MPCProtocol::Yao, negated_tensor);
    const auto relu_tensor = yao_tof.make_tensor_relu_op(boolean_tensor);  // -RELU(-X)
    const auto finBoolean_tensor =
        yao_tof.make_tensor_conversion(options.arithmetic_protocol, relu_tensor);
    return arithmetic_tof.make_tensor_negate(finBoolean_tensor);
  };

  make_indicator = [&](const auto& input, std::size_t input_size) {
    const auto first_relu_output = make_activation(input);  // Returns -RELU(-X)

    // Declaring a constant uint64 vector of same size as input and initializing every element
    // with encoded 9000
    std::vector<uint64_t> const_vector(
        input_size, MOTION::new_fixed_point::encode<uint64_t, float>(9000, frac_bits));

    // Multiplying the tensor with the constant vector (element wise)
    const auto mult_output =
        arithmetic_tof.make_tensor_constMul_op(first_relu_output, const_vector, frac_bits);
    // Reached 9000 * -RELU(-X)
    // Adding an encoded one to the tensor
    std::vector<uint64_t> const_vector2(input_size,
                                        MOTION::new_fixed_point::encode<uint64_t, float>(1, 13));

    const auto add_output = arithmetic_tof.make_tensor_constAdd_op(mult_output, const_vector2);
    // Reached 1 + 9000 * -RELU(-X)

    return make_relu(add_output);  // make_relu returns RELU(Y)
    // Returning RELU( 1 + 9000 * -RELU(-X) )
  };

  /////////////////////////////////////////////////////////////////////

  // Uisng indicator funtion to get value 1 at the postion of the maximum element
  // index_array = [1 2 3 4]
  // input_vetor-max_vector
  // eg. input = [4 -2 4 3]
  // output1 = 4 //max elemnt
  // output2 = [4 4 4 4]
  //  we negate output2 and add it to the input
  // output3 = [1 -2 4 3]- [4 4 4 4] = [0 -6 0 -1]
  //  we give this as an put to indicator function and gives output4 = [1 0 1 0]
  //  we do hadamard prodoctconstanta matrix  indx_array with output5 = [1 0 3 0]
  // maxpool on output 5 gives argmax as 3

  /////////////////////////////////////////////////////////////////////////

  // finding the maximum elemnet m
  auto output1 = make_maxPool(tensor_input);

  // In this function call, when 'true' or 'false' is set as the fourth argument,
  // 'true' multiplies tensor * constant, while 'false' does constant * tensor.
  auto output2 = arithmetic_tof.make_tensor_constMatrix_Mul_op(gemm_op1, constant_encoded, output1,
                                                               false, frac_bits);

  auto negated_tensor = arithmetic_tof.make_tensor_negate(output2);

  auto output3 = arithmetic_tof.make_tensor_add_op(tensor_input, negated_tensor);

  auto output4 = make_indicator(output3, options.num_elements);

  auto output5 = arithmetic_tof.make_tensor_constMul_op(output4, indx_vec, options.fractional_bits);

  auto output6 = make_maxPool(output5);

  ENCRYPTO::ReusableFiberFuture<std::vector<std::uint64_t>> main_output_future;

  if (options.my_id == 0) {
    arithmetic_tof.make_arithmetic_tensor_output_other(output6);
  } else {
    main_output_future = arithmetic_tof.make_arithmetic_64_tensor_output_my(output6);
  }

  return std::move(main_output_future);
}

void run_composite_circuit(const Options& options, MOTION::TwoPartyTensorBackend& backend) {
  auto output_future = create_composite_circuit(options, backend);
  backend.run();
  if (options.my_id == 1) {
    auto main = output_future.get();

    for (int i = 0; i < main.size(); ++i) {
      long double temp =
          MOTION::new_fixed_point::decode<uint64_t, long double>(main[i], options.fractional_bits);

      std::cout << "The index is :" << temp << "\n";
    }
  }
}
int main(int argc, char* argv[]) {
  // testMemoryOccupied();
  // std::cout << "Inside main";
  bool WriteToFiles = 1;
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
    testMemoryOccupied(WriteToFiles, options->my_id, options->currentpath);
    comm_layer->shutdown();
    print_stats(*options, run_time_stats, comm_stats);
    if (WriteToFiles == 1) {
      /////// Generate path for the AverageTimeDetails file and MemoryDetails file
      // std::string path = std::filesystem::current_path();
      std::string path = options->currentpath;
      std::string t1 = path + "/" + "AverageTimeDetails" + std::to_string(options->my_id);
      std::string t2 = path + "/" + "MemoryDetails" + std::to_string(options->my_id);

      ///// Write to the AverageMemoryDetails files
      std::ofstream file2;
      file2.open(t2, std::ios_base::app);
      std::string time_str =
          MOTION::Statistics::print_stats_short("tensor_gt_relu", run_time_stats, comm_stats);
      // std::cout << "Execution time string:" << time_str << "\n";
      double exec_time = std::stod(time_str);
      std::cout << "Execution time:" << exec_time << "\n";
      file2 << "Execution time - " << exec_time << "msec\n";
      file2.close();

      std::ofstream file1;
      file1.open(t1, std::ios_base::app);
      file1 << exec_time;
      file1 << "\n";
      file1.close();
    }
  } catch (std::runtime_error& e) {
    std::cerr << "ERROR OCCURRED: " << e.what() << "\n";
    std::cerr << "ERROR Caught !!"
              << "\n";
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
