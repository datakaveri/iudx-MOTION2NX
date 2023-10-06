/*
inputpath changed to imageprovider
imageprovider is file name inside server 0 and server 1 respectively
eg. (ip1_0 , ip1_1) , (ip2_0,ip2_1) and so on..
from bash file we are giving ip1 and in this file it is appended to ip1_0 and ip1_1

At the argument "--filepath " give the path of the file containing shares from build_deb.... folder
Server-0
./bin/training_all_labels --my-id 0 --party 0,::1,7000 --party 1,::1,7001 --arithmetic-protocol beavy --boolean-protocol yao --fractional-bits 13 --config-file-input Sample_shares --config-file-model file_config_model0 --actual-labels Actual_all_labels --current-path ${BASE_DIR}/build_debwithrelinfo_gcc --sample-size 2 --w1t-filename SharesForW1T0 --w2t-filename SharesForW2T0

Server-1
./bin/training_all_labels --my-id 1 --party 0,::1,7000 --party 1,::1,7001 --arithmetic-protocol beavy --boolean-protocol yao --fractional-bits 13 --config-file-input Sample_shares --config-file-model file_config_model1 --actual-labels Actual_all_labels --current-path ${BASE_DIR}/build_debwithrelinfo_gcc --sample-size 2 --w1t-filename SharesForW1T1 --w2t-filename SharesForW2T1

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

#include <algorithm>  // for copy() and assign()
#include <boost/algorithm/string.hpp>
#include <boost/json/serialize.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/log/trivial.hpp>
#include <boost/program_options.hpp>
#include <iterator>  // for back_inserter

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
int j = 0;
int count = 1;

static std::vector<uint64_t> generate_inputs(const MOTION::tensor::TensorDimensions dims) {
  return MOTION::Helpers::RandomVector<uint64_t>(dims.get_data_size());
}

bool is_empty(std::ifstream& file) { return file.peek() == std::ifstream::traits_type::eof(); }

void testMemoryOccupied(int WriteToFiles, int my_id, std::string path) {
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
    file2 << "Multiplication layer : \n";
    file2 << "RSS - " << rss << " kB\n";
    file2 << "Shared Memory - " << shared_mem << " kB\n";
    file2 << "Private Memory - " << rss - shared_mem << "kB\n";
    file2.close();
  }
}
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

struct Shares {
  uint64_t Delta, delta;
};
struct Matrix {
  std::vector<uint64_t> Delta;
  std::vector<uint64_t> delta;
  int row;
  int col;
};

class Matrix_input {
 private:
  uint64_t rows, columns, num_elements;
  std::string fullFileName;
  std::vector<uint64_t> file_data;

 public:
  struct Shares* Data;
  Matrix_input() {}
  uint64_t get_rows() { return rows; }
  uint64_t get_columns() { return columns; }
  void Set_data(std::string fileName) {
    fullFileName = fileName;
    std::cout << fullFileName << std::endl;
    readMatrix();
  }

  void readMatrix() {
    std::cout << "Reading the matrix from file :- " << fullFileName << std::endl;

    int count = 0;
    uint64_t temp;
    float temp2;

    std::ifstream indata;
    indata.open(fullFileName);

    rows = read_file(indata);
    columns = read_file(indata);
    num_elements = rows * columns;
    Data = (struct Shares*)malloc(sizeof(struct Shares) * num_elements);
    for (int i = 0; i < num_elements; ++i) {
      Data[i].Delta = read_file(indata);
      Data[i].delta = read_file(indata);
      j = j + 2;
      // std::cout << Data[i].Delta << " " << Data[i].delta << "\n";
    }
    // std::cout << "Size of file_data:" << file_data.size() << "\n";
  }
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
  std::string imageprovider, actuallabels, W1T_filename, W2T_filename;
  std::string modelpath;
  std::size_t layer_id;
  std::string currentpath;
  std::size_t my_id;
  MOTION::Communication::tcp_parties_config tcp_config;
  bool no_run = false;
  int sample_size;
  Matrix image_file[20], actual_labels;
  Matrix X, Xtranspose;
  Matrix W1T; //theta
  Matrix W2T;
  Matrix row;
  Matrix col;
};

//////////////////////////////////////////////////////////////////////
std::optional<Options> parse_program_options(int argc, char* argv[]) {
  Options options;
  boost::program_options::options_description desc("Allowed options");
  // clang-format off
  desc.add_options()
    ("help,h", po::bool_switch()->default_value(false),"produce help message")
    ("config-file-input", po::value<std::string>()->required(), "config file containing options")
    ("actual-labels", po::value<std::string>()->required(), "Name of the file with actual labels")
    ("w1t-filename", po::value<std::string>()->required(), "Name of the file with weights 1 transpose")
    ("w2t-filename", po::value<std::string>()->required(), "Name of the file with weights 2 transpose")
    ("config-file-model", po::value<std::string>()->required(), "config file containing options")
    ("my-id", po::value<std::size_t>()->required(), "my party id")
    ("sample-size", po::value<int>()->required(), "sample size")
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
  options.sample_size = vm["sample-size"].as<int>();
  options.currentpath = vm["current-path"].as<std::string>();
  //////////////////////////////////////////////////////////////////
  options.imageprovider = vm["config-file-input"].as<std::string>();
  options.actuallabels = vm["actual-labels"].as<std::string>();
  options.W1T_filename = vm["w1t-filename"].as<std::string>();
  options.W2T_filename = vm["w2t-filename"].as<std::string>();
  options.modelpath = vm["config-file-model"].as<std::string>();
  ///////////////////////////////////////////////////////////////////
  std::string path = options.currentpath;
  Matrix_input Xi[20], Yi, Theta1_n, Theta2_n;
  for (int i = 0; i < options.sample_size; i++) {
    auto t1 = path + "/server" + std::to_string(options.my_id) + "/Image_shares/" +
              options.imageprovider + std::to_string(i + 1);
    std::cout << "Path from where image files are read:" << t1 << "\n";
    Xi[i].Set_data(t1);
    ////////////////////////////////////////////////////////////////

    options.image_file[i].row = Xi[i].get_rows();
    options.image_file[i].col = Xi[i].get_columns();
    // options.image_file[i].Delta.assign(Xt[i].Data->begin(), Xt[i].Data.end());
    // copy(Xt[i].Data->begin(), Xt[i].Data.end(), back_inserter(options.image_file[i].Delta));
    for (int j = 0; j < options.image_file[i].row * options.image_file[i].col; ++j) {
      options.image_file[i].Delta.push_back(Xi[i].Data[j].Delta);
      options.image_file[i].delta.push_back(Xi[i].Data[j].delta);
    }
  }
  auto t1 =
      path + "/server" + std::to_string(options.my_id) + "/Image_shares/" + options.actuallabels;
  std::cout << "Path from where actual label files are read:" << t1 << "\n";
  Yi.Set_data(t1);
  // Read 10*m actual labels
  options.actual_labels.row = Yi.get_rows();
  options.actual_labels.col = Yi.get_columns();
  for (int j = 0; j < options.actual_labels.row * options.actual_labels.col; ++j) {
    options.actual_labels.Delta.push_back(Yi.Data[j].Delta);
    options.actual_labels.delta.push_back(Yi.Data[j].delta);
  }
  std::cout << "labels: " << options.actual_labels.col << " " << options.sample_size << std::endl;
  if (options.actual_labels.col != options.sample_size) {
    std::cerr << "Sample size should match actual labels \n";
  }

    t1 = path + "/" + options.W1T_filename;
  //t1 = path + "/server" + std::to_string(options.my_id) + "/Image_shares/" + options.W1T_filename;
  // Read 10 * 784 size theta
  std::cout << "Path from where W1 transpose files are read:" << t1 << "\n";
  Theta1_n.Set_data(t1);
  options.W1T.row = Theta1_n.get_rows();
  options.W1T.col = Theta1_n.get_columns();
  for (int j = 0; j < options.W1T.row * options.W1T.col; ++j) {
    options.W1T.Delta.push_back(Theta1_n.Data[j].Delta);
    options.W1T.delta.push_back(Theta1_n.Data[j].delta);
  }

  t1 = path + "/" + options.W2T_filename;
  // Read 10 * 784 size theta
  std::cout << "Path from where W2 transpose files are read:" << t1 << "\n";
  Theta2_n.Set_data(t1);
  options.W2T.row = Theta2_n.get_rows();
  options.W2T.col = Theta2_n.get_columns();
  for (int j = 0; j < options.W2T.row * options.W2T.col; ++j) {
    options.W2T.Delta.push_back(Theta2_n.Data[j].Delta);
    options.W2T.delta.push_back(Theta2_n.Data[j].delta);
  }
  std::cout << "Generating X \n";

  // [x(1)  x(2)  x(3)...............x(m)]
  //   |     |     |                   |
  //   |     |     |                   |
  //   |     |     |                   |
  //   |     |     |                   |
  //  These columns are of size 784.
  //  Reading these elements rowwise into X

  options.X.row = options.image_file[0].row;
  options.X.col = options.sample_size;
  for (int i = 0; i < options.image_file[0].row * options.image_file[0].col; ++i) {
    for (int j = 0; j < options.sample_size; j++) {
      // std::cout << i <<" " << j << "   ";
      options.X.Delta.push_back(options.image_file[j].Delta[i]);
      options.X.delta.push_back(options.image_file[j].delta[i]);
    }
    // std::cout << "\n";
  }
  std::cout << "\n";
  std::cout << "End of generating X \n";
  std::cout << "Generating X transpose \n";
  // [x(1)------------------------------]
  //   x(2)------------------------------
  //   x(3)------------------------------
  //   ----------------------------------
  //   x(m)------------------------------
  //  These rows are of size 784.
  //  Reading these elements rowwise into Xtranspose
  options.Xtranspose.row = options.sample_size;
  options.Xtranspose.col = options.image_file[0].row;
  for (int j = 0; j < options.sample_size; j++) {
    for (int i = 0; i < options.image_file[0].row * options.image_file[0].col; ++i) {
      // std::cout << i <<" " << j << "   ";
      options.Xtranspose.Delta.push_back(options.image_file[j].Delta[i]);
      options.Xtranspose.delta.push_back(options.image_file[j].delta[i]);
    }
    // std::cout << "\n";
  }

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
    // std::cout << obj << "\n";
  } else {
    std::cout << MOTION::Statistics::print_stats("tensor_gt_mul", run_time_stats, comm_stats);
  }
}

auto create_composite_circuit(const Options& options, MOTION::TwoPartyTensorBackend& backend) {
  // std::cout << "Inside create_composite"
  //           << "\n";
  // retrieve the gate factories for the chosen protocols
  auto& arithmetic_tof = backend.get_tensor_op_factory(options.arithmetic_protocol);
  auto& boolean_tof = backend.get_tensor_op_factory(MOTION::MPCProtocol::Yao);

  std::cout << "Theta:" << options.W1T.row << " " << options.W1T.col << "\n";
  std::cout << "X:" << options.X.row << " " << options.X.col << "\n";

  const MOTION::tensor::GemmOp gemm_op1 = {.input_A_shape_ = {options.W1T.row, options.W1T.col},
                                           .input_B_shape_ = {options.X.row, options.X.col},
                                           .output_shape_ = {options.W1T.row, options.X.col}};

  // const auto W1_dims = gemm_op1.get_input_A_tensor_dims();

  // const auto X_dims = gemm_op1.get_input_B_tensor_dims();
  MOTION::tensor::TensorDimensions Y_dims, X0_dims, Xtranspose_dims;

  X0_dims.batch_size_ = 1;
  X0_dims.num_channels_ = 1;
  X0_dims.height_ = options.image_file[0].row;
  X0_dims.width_ = options.image_file[0].col;

  // theta_dims.batch_size_ = 1;
  // theta_dims.num_channels_ = 1;
  // theta_dims.height_ = options.theta.row * options.theta.col;
  // theta_dims.width_ = 1;
  const auto W1T_dims = gemm_op1.get_input_A_tensor_dims();
  const auto X_dims = gemm_op1.get_input_B_tensor_dims();
  const auto gemmop1_dims = gemm_op1.get_output_tensor_dims();
  std::cout << "X dims:" << X_dims.height_ << " " << X_dims.width_ << "\n";
  std::cout << "W1 T dims:" << W1T_dims.height_ << " " << W1T_dims.width_ << "\n";
  std::cout << "Output dimensions of first gemm operation:" << gemmop1_dims.height_ << " "
            << gemmop1_dims.width_ << "\n";

  Xtranspose_dims.batch_size_ = 1;
  Xtranspose_dims.num_channels_ = 1;
  Xtranspose_dims.height_ = X_dims.width_;
  Xtranspose_dims.width_ = X_dims.height_;

  Y_dims.batch_size_ = 1;
  Y_dims.num_channels_ = 1;
  Y_dims.height_ = options.actual_labels.row;
  Y_dims.width_ = options.actual_labels.col;
  std::cout << Y_dims.height_ << " " << Y_dims.width_ << std::endl;
  const MOTION::tensor::GemmOp gemm_op2 = {
      .input_A_shape_ = {options.W2T.row, options.W2T.col},
      .input_B_shape_ = gemm_op1.compute_output_shape(),
      .output_shape_ = {Y_dims.height_, Y_dims.width_}};
  const auto W2T_dims = gemm_op2.get_input_A_tensor_dims();

  const MOTION::tensor::GemmOp gemm_op3 = {
      .input_A_shape_ = {Y_dims.height_, Y_dims.width_},
      .input_B_shape_ = gemm_op1.compute_output_shape(),
      .output_shape_ = {W2T_dims.height_, W2T_dims.width_},
      .transB_ = true};
  
  std::cout << "Gemm op 3 dimensions: \n";
  std::cout << "Input A: " << Y_dims.height_ << " " << Y_dims.width_ << "\n";
  std::cout << "Input B: " << gemm_op1.compute_output_shape()[0] << " " << gemm_op1.compute_output_shape()[1] << "\n";
  std::cout << "Output: " << W2T_dims.height_<< " " << W2T_dims.width_ << "\n";

  const MOTION::tensor::GemmOp gemm_op4 = {
      .input_A_shape_ = {W2T_dims.height_, W2T_dims.width_},
      .input_B_shape_ = {Y_dims.height_, Y_dims.width_},
      .output_shape_ = {W2T_dims.width_, Y_dims.width_},
      .transA_ = true};

  std::cout << "Gemm op 4 dimensions: \n";
  std::cout << "Input A: " << W2T_dims.height_ << " " << W2T_dims.width_ << "\n";
  std::cout << "Input B: " << Y_dims.height_ << " "  << Y_dims.width_ << "\n";
  std::cout << "Output: " << W2T_dims.width_ << " " << Y_dims.height_ << "\n";


  const MOTION::tensor::HammOp hamm_op1 = {
      .input_A_shape_ = gemm_op4.compute_output_shape(),
      .input_B_shape_ = gemm_op1.compute_output_shape(),
      .output_shape_ = gemm_op1.compute_output_shape()};
  
  const MOTION::tensor::GemmOp gemm_op5 = {
      .input_A_shape_ = hamm_op1.compute_output_shape(),
      .input_B_shape_ = {options.X.row, options.X.col},
      .output_shape_ = {options.W1T.row, options.W1T.col},
      .transB_ = true};

  /////////////////////////////////////////////////////////////////////////
  MOTION::tensor::TensorCP tensor_X, tensor_Y, tensor_W1T, tensor_W2T, tensor_X0, tensor_Xtranspose;
  MOTION::tensor::TensorCP gemm_output1, add_output1;

  std::cout << "Make tensors \n";
  auto pairX = arithmetic_tof.make_arithmetic_64_tensor_input_shares(X_dims);
  std::vector<ENCRYPTO::ReusableFiberPromise<MOTION::IntegerValues<uint64_t>>> input_promises_X =
      std::move(pairX.first);
  tensor_X = pairX.second;
  std::cout << "Tensor X created \n";
  auto pairXtranspose = arithmetic_tof.make_arithmetic_64_tensor_input_shares(Xtranspose_dims);
  std::vector<ENCRYPTO::ReusableFiberPromise<MOTION::IntegerValues<uint64_t>>>
      input_promises_Xtranspose = std::move(pairXtranspose.first);
  tensor_Xtranspose = pairXtranspose.second;
  std::cout << "Tensor Xtranspose created \n";
  auto pairX0 = arithmetic_tof.make_arithmetic_64_tensor_input_shares(X0_dims);
  std::vector<ENCRYPTO::ReusableFiberPromise<MOTION::IntegerValues<uint64_t>>> input_promises_X0 =
      std::move(pairX0.first);
  tensor_X0 = pairX0.second;
  std::cout << "Tensor X0 created \n";
  auto pairY = arithmetic_tof.make_arithmetic_64_tensor_input_shares(Y_dims);
  std::vector<ENCRYPTO::ReusableFiberPromise<MOTION::IntegerValues<uint64_t>>> input_promises_Y =
      std::move(pairY.first);
  tensor_Y = pairY.second;
  std::cout << "Tensor Y created \n";
  auto pairtheta1 = arithmetic_tof.make_arithmetic_64_tensor_input_shares(W1T_dims);
  std::vector<ENCRYPTO::ReusableFiberPromise<MOTION::IntegerValues<uint64_t>>>
      input_promises_theta1 = std::move(pairtheta1.first);
  tensor_W1T = pairtheta1.second;
  auto pairtheta2 = arithmetic_tof.make_arithmetic_64_tensor_input_shares(W2T_dims);
  std::vector<ENCRYPTO::ReusableFiberPromise<MOTION::IntegerValues<uint64_t>>>
      input_promises_theta2 = std::move(pairtheta2.first);
  tensor_W2T = pairtheta2.second;
  std::cout << "End of tensor creation \n";
  ///////////////////////////////////////////////////////////////
  input_promises_X[0].set_value(options.X.Delta);
  input_promises_X[1].set_value(options.X.delta);
  std::cout << "Input promise set \n";
  input_promises_Y[0].set_value(options.actual_labels.Delta);
  input_promises_Y[1].set_value(options.actual_labels.delta);

  input_promises_theta1[0].set_value(options.W1T.Delta);
  input_promises_theta1[1].set_value(options.W1T.delta);

  input_promises_theta2[0].set_value(options.W2T.Delta);
  input_promises_theta2[1].set_value(options.W2T.delta);

  input_promises_Xtranspose[0].set_value(options.Xtranspose.Delta);
  input_promises_Xtranspose[1].set_value(options.Xtranspose.delta);

  input_promises_X0[0].set_value(options.image_file[0].Delta);
  input_promises_X0[1].set_value(options.image_file[0].delta);
  /////////////////////////////Sigmoid//////////////////////////////////

  
  std::function<MOTION::tensor::TensorCP(const MOTION::tensor::TensorCP&)> make_activation,make_relu;
  std::function<MOTION::tensor::TensorCP(const MOTION::tensor::TensorCP&), std::size_t> make_indicator, make_sigmoid;
  // -RELU(-X)u
  make_activation = [&](const auto& input) {
  //  const auto negated_tensor = arithmetic_tof.make_tensor_negate(input);
    const auto boolean_tensor =
        boolean_tof.make_tensor_conversion(MOTION::MPCProtocol::Yao, input);
    const auto relu_tensor = boolean_tof.make_tensor_relu_op(boolean_tensor);
    return boolean_tof.make_tensor_conversion(options.arithmetic_protocol, relu_tensor);
  };

  //  RELU(X)
  make_relu = [&](const auto& input) {
    const auto negated_tensor = arithmetic_tof.make_tensor_negate(input);
    const auto boolean_tensor =
        boolean_tof.make_tensor_conversion(MOTION::MPCProtocol::Yao, negated_tensor);
    const auto relu_tensor = boolean_tof.make_tensor_relu_op(boolean_tensor); // -RELU(-X)
    const auto finBoolean_tensor =
        boolean_tof.make_tensor_conversion(options.arithmetic_protocol, relu_tensor);
    return arithmetic_tof.make_tensor_negate(finBoolean_tensor);
  };
  
  make_sigmoid = [&](const auto& input, std::size_t input_size) {
    const std::vector<uint64_t>constant_vector1(input_size, MOTION::new_fixed_point::encode<uint64_t, float>(-0.5, options.fractional_bits));
    const auto input_const_add = arithmetic_tof.make_tensor_constAdd_op(
        input, constant_vector1);
    const auto first_relu_output = make_activation(input_const_add);
    const std::vector<uint64_t>constant_vector2(input_size, MOTION::new_fixed_point::encode<uint64_t, float>(1, options.fractional_bits));
    const auto input_const_add2 = arithmetic_tof.make_tensor_constAdd_op(
        first_relu_output, constant_vector2);
    const auto negated_tensor = arithmetic_tof.make_tensor_negate(input_const_add2);
    const auto final_relu_output = make_activation(negated_tensor);
    return arithmetic_tof.make_tensor_negate(final_relu_output);
  };

  make_indicator = [&](const auto& input, std::size_t input_size) {
    const auto first_relu_output = make_activation(input);    // Returns -RELU(-X)

    // Declaring a constant uint64 vector of same size as input and initializing every element with encoded 9000
    std::vector<uint64_t> const_vector(input_size, MOTION::new_fixed_point::encode<uint64_t, float>(9000, options.fractional_bits));

    // Multiplying the tensor with the constant vector (element wise)
    const auto mult_output = arithmetic_tof.make_tensor_constMul_op(first_relu_output, const_vector, options.fractional_bits);
    // Reached 9000 * -RELU(-X)
    // Adding an encoded one to the tensor
    std::vector<uint64_t> const_vector2(input_size, MOTION::new_fixed_point::encode<uint64_t, float>(1, options.fractional_bits));
    const auto add_output = arithmetic_tof.make_tensor_constAdd_op(mult_output,const_vector2);
    // Reached 1 + 9000 * -RELU(-X)

    return make_relu(add_output); // make_relu returns RELU(Y)
    // Returning RELU( 1 + 9000 * -RELU(-X) )
  };

  ///////////////////////////////////////////////////////////////////
  std::cout << "Gemm operation starts \n";
  auto tensor_Z1 =
      arithmetic_tof.make_tensor_gemm_op(gemm_op1, tensor_W1T, tensor_X, options.fractional_bits);
  auto tensor_A1 = make_relu(tensor_Z1);
  auto tensor_Z2 = 
      arithmetic_tof.make_tensor_gemm_op(gemm_op2, tensor_W2T, tensor_A1, options.fractional_bits);
  std::cout << "gemm op 2" << std::endl;
  auto tensor_A2 = make_sigmoid(tensor_Z2, gemm_op2.compute_output_size());
  std::cout << "sigmoid" << std::endl;
  auto negated_Y = arithmetic_tof.make_tensor_negate(tensor_Y);
  std::cout << "negation" << std::endl;
  auto tensor_H = arithmetic_tof.make_tensor_add_op(tensor_A2, negated_Y);
  std::cout << "addition" << std::endl;
  auto tensor_dLdW2T = arithmetic_tof.make_tensor_gemm_op(gemm_op3, tensor_H, tensor_A1,
                                                      options.fractional_bits);
  std::cout << "gemm op 3" << std::endl;
  auto tensor_gemm_op4_output = arithmetic_tof.make_tensor_gemm_op(gemm_op4, tensor_W2T, tensor_H,
                                                      options.fractional_bits);
  std::cout << "gemm op 4" << std::endl;
  auto tensor_indicator_Z1 = make_indicator(tensor_Z1, gemm_op1.compute_output_size());
  std::cout << "indicator matrix" << std::endl;
  auto tensor_hadamard_output = arithmetic_tof.make_tensor_hamm_op(hamm_op1, tensor_gemm_op4_output, tensor_indicator_Z1,
                                                      options.fractional_bits);                                                                    
  std::cout << "Hadamard" << std::endl;
  auto tensor_dLdW1T = arithmetic_tof.make_tensor_gemm_op(gemm_op5, tensor_hadamard_output, tensor_X,
                                                      options.fractional_bits);
  std::cout << "Gemm op 5" << std::endl;
  float alpham = 0.005;
  auto encoded_alpham = MOTION::new_fixed_point::encode<uint64_t, float>(alpham, options.fractional_bits);
  std::cout << "Encoded alpha m:" << encoded_alpham << "\n";
  std::vector<uint64_t> constant_vector2(gemm_op3.get_output_tensor_dims().height_ * gemm_op3.get_output_tensor_dims().width_, encoded_alpham);
  auto tensor_dLdW2_alpham = arithmetic_tof.make_tensor_constMul_op(tensor_dLdW2T, constant_vector2, options.fractional_bits);
  auto tensor_negated_dLdW2_alpham = arithmetic_tof.make_tensor_negate(tensor_dLdW2_alpham);
  auto Updated_W2T = arithmetic_tof.make_tensor_add_op(tensor_W2T, tensor_negated_dLdW2_alpham);

  std::vector<uint64_t> constant_vector1(gemm_op5.get_output_tensor_dims().height_ * gemm_op5.get_output_tensor_dims().width_, encoded_alpham);
  auto tensor_dLdW1_alpham = arithmetic_tof.make_tensor_constMul_op(tensor_dLdW1T, constant_vector1, options.fractional_bits);
  auto tensor_negated_dLdW1_alpham = arithmetic_tof.make_tensor_negate(tensor_dLdW1_alpham);
  auto Updated_W1T = arithmetic_tof.make_tensor_add_op(tensor_W1T, tensor_negated_dLdW1_alpham);
  
  std::cout << "Gemm operation ends \n";
  ENCRYPTO::ReusableFiberFuture<std::vector<std::uint64_t>> output_future, main_output_future,
      main_output;

  if (options.my_id == 0) {
    arithmetic_tof.make_arithmetic_tensor_output_other(Updated_W2T);
  } else {
    main_output_future = arithmetic_tof.make_arithmetic_64_tensor_output_my(Updated_W2T);
  }

  tensor_W2T = Updated_W2T;
  tensor_W1T = Updated_W1T;

  return std::move(main_output_future);
}

void run_composite_circuit(const Options& options, MOTION::TwoPartyTensorBackend& backend) {
  int iterations = 3;
  ENCRYPTO::ReusableFiberFuture<std::vector<std::uint64_t>> output_future;
  for (int i = 0; i < iterations; i++) {
    std::cout << "iteration " << i + 1 << "  *****************\n";
    output_future = create_composite_circuit(options, backend);
  }
  std::cout << "Before backend.run\n";
  backend.run();
  std::cout << "Backend.run ends \n";
  if (options.my_id == 1) {
    auto main = output_future.get();
    //   std::vector<long double> mod_x;
    //   // std::string path = std::filesystem::current_path();
    //   std::string path = options.currentpath;
    //   string filename = path + "/" + "output_tensor";
    //   std::ofstream x;
    //   x.open(filename, std::ios_base::app);
    //   x << options.imageprovider << "\n";
    int k = 783;
    int z = 1;
    for (int i = 0; i < main.size(); ++i) {
      long double temp =
          MOTION::new_fixed_point::decode<uint64_t, long double>(main[i], options.fractional_bits);
      std::cout << temp << ",";
      if (i > k) {
        std::cout
            << "********************************\n \n \n*************************************";
        z = z + 1;
        k = z * 784 - 1;
      }
    }

    // if (options.layer_id == 2) {
    //   std::cout << temp << ",";
    // }
  }
}

int main(int argc, char* argv[]) {
  // std::cout << "Inside main";
  auto options = parse_program_options(argc, argv);
  int WriteToFiles = 1;
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
          MOTION::Statistics::print_stats_short("tensor_gt_mul_test", run_time_stats, comm_stats);
      std::cout << "Execution time string:" << time_str << "\n";
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
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
