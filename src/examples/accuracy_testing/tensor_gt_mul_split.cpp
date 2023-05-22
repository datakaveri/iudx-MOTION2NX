/*
inputpath changed to imageprovider
imageprovider is file name inside server 0 and server 1 respectively
eg. (ip1_0 , ip1_1) , (ip2_0,ip2_1) and so on..
from bash file we are giving ip1 and in this file it is appended to ip1_0 and ip1_1

At the argument "--filepath " give the path of the file containing shares from build_deb.... folder
Server-0
./bin/tensor_gt_mul_split --my-id 0 --party 0,::1,7002 --party 1,::1,7000 --arithmetic-protocol
beavy --boolean-protocol yao --fractional-bits 13 --config-file-input ip1 --config-file-model
file_config_model0 --layer-id 1 --row_start 1 --row_end 784

Server-1
./bin/tensor_gt_mul_split --my-id 1 --party 0,::1,7002 --party 1,::1,7000 --arithmetic-protocol
beavy --boolean-protocol yao --fractional-bits 13  --config-file-input ip1 --config-file-model
file_config_model1 --layer-id 1 --row_start 1 --row_end 784

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

#include <math.h>
#include "base/two_party_tensor_backend.h"
#include "protocols/beavy/tensor.h"
#include "tensor/tensor.h"
#include "tensor/tensor_op.h"
#include "tensor/tensor_op_factory.h"
#include "utility/fixed_point.h"

namespace po = boost::program_options;
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
  std::string imageprovider;
  std::string modelpath;
  std::size_t layer_id;
  std::string currentpath;
  std::size_t my_id;
  // std::string filepath_frombuild;
  MOTION::Communication::tcp_parties_config tcp_config;
  bool no_run = false;
  Matrix image_file;
  Matrix W_file;
  Matrix B_file;
  Matrix row;
  Matrix col;
  /////////////////////////////////////////////////////////
  int row_start;
  int row_end;
  int splits;
};

void testMemoryOccupied(bool WriteToFiles, int my_id, int layer_id, int p, std::string path) {
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

    std::ostringstream ss;
    ss << std::fixed << rss;
    ///// Write to the AverageMemoryDetails files
    std::ofstream file1;
    file1.open(t1, std::ios_base::app);
    file1 << std::fixed << std::stod(ss.str());
    file1 << "\n";
    file1.close();

    std::ofstream file2;
    file2.open(t2, std::ios_base::app);
    file2 << "Multiplication layer : \n";
    file2 << "RSS - " << std::fixed << std::stod(ss.str()) << " kB\n";
    file2 << "Shared Memory - " << shared_mem << " kB\n";
    file2 << "Private Memory - " << rss - shared_mem << "kB\n";
    file2.close();
  }
}

bool is_empty(std::ifstream& file) { return file.peek() == std::ifstream::traits_type::eof(); }

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
///////////// New function that reads from a specific point in file ////////

std::fstream& GotoLine(std::fstream& file, unsigned int num) {
  file.seekg(std::ios::beg);
  for (int i = 0; i < num - 1; ++i) {
    file.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
  }
  return file;
}

///////////////////////////////////////////////////////////////////////////

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

int image_shares(Options* options, std::string p) {
  std::ifstream temps;

  try {
    temps.open(p);
    if (temps) {
      std::cout << "File found\n";
    } else {
      std::cout << "File not found\n";
    }
  } catch (std::ifstream::failure e) {
    std::cerr << "Error while opening the image share file.\n";
    return EXIT_FAILURE;
  }

  try {
    if (is_empty(temps)) {
      // file is empty
    }
  } catch (std::ifstream::failure e) {
    std::cerr << "Image share input file is empty.\n";
    return EXIT_FAILURE;
  }

  std::uint64_t rows, cols;
  try {
    rows = read_file(temps);
    options->image_file.row = rows;
    std::cout << "r " << rows << " ";
    cols = read_file(temps);
    options->image_file.col = cols;
    std::cout << "c " << cols << "\n";
  } catch (std::ifstream::failure e) {
    std::cerr << "Error while reading columns from image shares.\n";
    return EXIT_FAILURE;
  }

  // ////////////////////////////Reading from specific rows////////////////////
  //    // t1 is the path of the file that is to be read
  //    //std::ifstream file(t1);
  //     temps.seekg(std::ios::beg);
  //     for(int i=0; i < options->row_start; ++i){
  //         temps.ignore(std::numeric_limits<std::streamsize>::max(),'\n');
  //     }

  //     for (int i = options->row_start ; i < options->row_end +1 ; ++i) {
  //       std::cout << "\n" << i << "\n";
  //       uint64_t m1 = read_file(temps);
  //     // std::cout << m1 << " ";
  //     options->image_file.Delta.push_back(m1);
  //     uint64_t m2 = read_file(temps);
  //     // std::cout << m2 << "\n";
  //     options->image_file.delta.push_back(m2);
  //    // std::cout << "\n" << i << "\n";
  //    }
  //    //////////////////////////////////////////////////////////////////////////

  for (int i = 0; i < rows * cols; ++i) {
    try {
      uint64_t m1 = read_file(temps);
      options->image_file.Delta.push_back(m1);
      uint64_t m2 = read_file(temps);
      options->image_file.delta.push_back(m2);
    } catch (std::ifstream::failure e) {
      std::cerr << "Error while reading columns from image shares.\n";
      return EXIT_FAILURE;
    }
  }
  temps.close();
}

int W_shares(Options* options, std::string p) {
  std::ifstream indata;
  try {
    indata.open(p);
    if (std::ifstream(p)) {
      std::cout << "File found\n";
    } else {
      std::cout << "File not found\n";
    }
  } catch (std::ifstream::failure e) {
    std::cerr << "Error while opening the input weight share file.\n";
    return EXIT_FAILURE;
  }

  try {
    if (is_empty(indata)) {
      // file is empty
    }
  } catch (std::ifstream::failure e) {
    std::cerr << "Weight share file is empty.\n";
    return EXIT_FAILURE;
  }

  int rows, cols;
  try {
    rows = read_file(indata);
    cols = read_file(indata);
  } catch (std::ifstream::failure e) {
    std::cerr << "Error while reading rows and columns from weight shares.\n";
    return EXIT_FAILURE;
  }

  int effective_rows = (options->row_end - options->row_start) + 1;
  options->W_file.row = effective_rows;
  // options->W_file.row = rows;
  std::cout << "Effective rows: " << effective_rows << "\n ";
  std::cout << "Actual rows,r: " << rows << " ";
  options->W_file.col = cols;
  std::cout << "Actual columns,c: " << cols << "\n";

  ////////////////////////////Reading from specific rows////////////////////
  // t1 is the path of the file that is to be read
  // std::ifstream file(t1);
  indata.seekg(std::ios::beg);
  for (int i = 0; i < ((options->row_start - 1) * cols) + 1; ++i) {
    // std::cout << "i is " << i << "\n";
    indata.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
  }

  for (int i = ((options->row_start) * cols); i < ((options->row_end + 1) * cols); ++i) {
    try {
      uint64_t m1 = read_file(indata);
      options->W_file.Delta.push_back(m1);
      uint64_t m2 = read_file(indata);
      options->W_file.delta.push_back(m2);
    } catch (std::ifstream::failure e) {
      std::cerr << "Error while reading input weight shares from file.\n";
      return EXIT_FAILURE;
    }
  }
  indata.close();
}

int B_shares(Options* options, std::string p) {
  std::ifstream indata;
  try {
    if (std::ifstream(p)) {
      std::cout << "File found\n";
    } else {
      std::cout << "File not found\n";
    }

    indata.open(p);
  } catch (std::ifstream::failure e) {
    std::cerr << "Error while opening the input bias share file.\n";
    return EXIT_FAILURE;
  }

  try {
    if (is_empty(indata)) {
      // file is empty
    }
  } catch (std::ifstream::failure e) {
    std::cerr << "Bias share file is empty.\n";
    return EXIT_FAILURE;
  }

  int rows, cols;
  try {
    rows = read_file(indata);
    cols = read_file(indata);
  } catch (std::ifstream::failure e) {
    std::cerr << "Error while reading rows and columns from input bias share file.\n";
    return EXIT_FAILURE;
  }

  int effective_rows = (options->row_end - options->row_start) + 1;
  options->B_file.row = effective_rows;
  // options->B_file.row = rows;
  std::cout << "r " << rows << " ";
  options->B_file.col = cols;
  std::cout << "c " << cols << "\n";
  ////////////////////////////Reading from specific rows////////////////////
  // t1 is the path of the file that is to be read
  // std::ifstream file(t1);
  indata.seekg(std::ios::beg);
  for (int i = 0; i < ((options->row_start)); ++i) {
    // std::cout << "i is " << i << "\n";
    indata.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
  }

  for (int i = ((options->row_start)); i < ((options->row_end + 1)); ++i) {
    try {
      uint64_t m1 = read_file(indata);
      options->B_file.Delta.push_back(m1);
      uint64_t m2 = read_file(indata);
      options->B_file.delta.push_back(m2);
    } catch (std::ifstream::failure e) {
      std::cerr << "Error while reading the input bias share file.\n";
      return EXIT_FAILURE;
    }
  }
  //////////////////////////////////////////////////////////////////////////

  // for (int i = 0; i < rows * cols; ++i) {
  //   uint64_t m1 = read_file(indata);
  //   options->B_file.Delta.push_back(m1);
  //   uint64_t m2 = read_file(indata);
  //   options->B_file.delta.push_back(m2);
  // }
  indata.close();
}

// changes made in this function
int file_read(Options* options) {
  std::string path = options->currentpath;
  // std::string path = std::filesystem::current_path();
  std::string t1, i;
  // if layer_id=1 then read filename inside server
  // else read file_config_input (id is appended)
  if (options->layer_id == 1) {
    t1 = path + "/server" + std::to_string(options->my_id) + "/Image_shares/" +
         options->imageprovider;
  } else if (options->layer_id > 1) {
    t1 = path + "/server" + std::to_string(options->my_id) + "/" + options->imageprovider + "_" +
         std::to_string(options->my_id);
  }
  std::cout << "\nt1: " << t1 << std::endl;
  image_shares(options, t1);

  //////////////////////////////////////////////////////////////////////////////////////////////////////////
  // model path --> builddebwithrelinfo_gcc/file_config_model0/1
  std::string t2 = path + "/" + options->modelpath;
  std::cout << "t2: " << t2 << std::endl;
  std::ifstream file2;
  try {
    file2.open(t2);
    if (file2) {
      std::cout << "File found\n";
    } else {
      std::cout << "File not found\n";
    }
  } catch (std::ifstream::failure e) {
    std::cerr << "Error while opening config_model file.\n";
    return EXIT_FAILURE;
  }

  try {
    if (is_empty(file2)) {
      // file is empty
    }
  } catch (std::ifstream::failure e) {
    std::cerr << "file_config_model is empty.\n";
    return EXIT_FAILURE;
  }

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
    ("current-path",po::value<std::string>()->required(), "current path build_debwithrelinfo")
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
    ("row_start", po::value<int>()->required(), "Row start number for split multiplication operation")
    ("row_end", po::value<int>()->required(), "Row end number for split multiplication operation")
     ("split", po::value<int>()->required(), "Type of split")
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
  options.currentpath = vm["current-path"].as<std::string>();
  //////////////////////////////////////////////////////////////////
  options.imageprovider = vm["config-file-input"].as<std::string>();
  options.modelpath = vm["config-file-model"].as<std::string>();
  options.layer_id = vm["layer-id"].as<std::size_t>();
  options.splits = vm["split"].as<int>();
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
  options.row_start = vm["row_start"].as<int>();
  options.row_end = vm["row_end"].as<int>();
  if (options.row_start > options.row_end) {
    std::cerr << "\n Row end number should be greater than row start.\n";
    return std::nullopt;
  }
  if ((options.row_start < 1) || (options.row_end < 2)) {
    std::cerr << "\n Either row start or row end is not a feasible number.\n";
    return std::nullopt;
  }
  if ((options.row_end > 784)) {
    std::cerr << "\n Row end is not a feasible number it should be less than 784.\n";
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
    auto obj = MOTION::Statistics::to_json("tensor_gt_mul_split", run_time_stats, comm_stats);
    obj.emplace("party_id", options.my_id);
    obj.emplace("arithmetic_protocol", MOTION::ToString(options.arithmetic_protocol));
    obj.emplace("boolean_protocol", MOTION::ToString(options.boolean_protocol));
    obj.emplace("simd", options.num_simd);
    obj.emplace("threads", options.threads);
    obj.emplace("sync_between_setup_and_online", options.sync_between_setup_and_online);
    std::cout << obj << "\n";
  } else {
    std::cout << MOTION::Statistics::print_stats("tensor_gt_mul_split", run_time_stats, comm_stats);
  }
}

auto create_composite_circuit(const Options& options, MOTION::TwoPartyTensorBackend& backend) {
  // retrieve the gate factories for the chosen protocols
  auto& arithmetic_tof = backend.get_tensor_op_factory(options.arithmetic_protocol);
  auto& boolean_tof = backend.get_tensor_op_factory(MOTION::MPCProtocol::Yao);

  ////////////////Using below to set size of input tensor/////////////////////
  ////////Looks clumsy but could not fix it
  /*const MOTION::tensor::GemmOp gemm_op1 = {
      .input_A_shape_ = {256, 784}, .input_B_shape_ = {784, 1}, .output_shape_ = {256, 1}};
*/

  std::cout << options.W_file.row << " " << options.W_file.col << "\n";
  std::cout << options.image_file.row << " " << options.image_file.col << "\n";

  const MOTION::tensor::GemmOp gemm_op1 = {
      .input_A_shape_ = {options.W_file.row, options.W_file.col},
      .input_B_shape_ = {options.image_file.row, options.image_file.col},
      .output_shape_ = {options.W_file.row, options.image_file.col}};

  const auto W1_dims = gemm_op1.get_input_A_tensor_dims();

  const auto X_dims = gemm_op1.get_input_B_tensor_dims();
  const auto B1_dims = gemm_op1.get_output_tensor_dims();

  /////////////////////////////////////////////////////////////////////////
  MOTION::tensor::TensorCP tensor_X, tensor_W1, tensor_B1;
  MOTION::tensor::TensorCP gemm_output1, add_output1;

  auto pairX = arithmetic_tof.make_arithmetic_64_tensor_input_shares(X_dims);
  std::vector<ENCRYPTO::ReusableFiberPromise<MOTION::IntegerValues<uint64_t>>> input_promises_X =
      std::move(pairX.first);
  tensor_X = pairX.second;

  auto pairW1 = arithmetic_tof.make_arithmetic_64_tensor_input_shares(W1_dims);
  std::vector<ENCRYPTO::ReusableFiberPromise<MOTION::IntegerValues<uint64_t>>> input_promises_W1 =
      std::move(pairW1.first);
  tensor_W1 = pairW1.second;

  auto pairB1 = arithmetic_tof.make_arithmetic_64_tensor_input_shares(B1_dims);
  std::vector<ENCRYPTO::ReusableFiberPromise<MOTION::IntegerValues<uint64_t>>> input_promises_B1 =
      std::move(pairB1.first);
  tensor_B1 = pairB1.second;

  ///////////////////////////////////////////////////////////////
  input_promises_X[0].set_value(options.image_file.Delta);
  input_promises_X[1].set_value(options.image_file.delta);

  input_promises_W1[0].set_value(options.W_file.Delta);
  input_promises_W1[1].set_value(options.W_file.delta);

  input_promises_B1[0].set_value(options.B_file.Delta);
  input_promises_B1[1].set_value(options.B_file.delta);
  ///////////////////////////////////////////////////////////////////

  gemm_output1 =
      arithmetic_tof.make_tensor_gemm_op(gemm_op1, tensor_W1, tensor_X, options.fractional_bits);
  add_output1 = arithmetic_tof.make_tensor_add_op(gemm_output1, tensor_B1);

  ENCRYPTO::ReusableFiberFuture<std::vector<std::uint64_t>> output_future, main_output_future,
      main_output;

  if (options.my_id == 0) {
    arithmetic_tof.make_arithmetic_tensor_output_other(add_output1);
    // arithmetic_tof.make_arithmetic_tensor_output_other(tensor_B1);
  } else {
    main_output_future = arithmetic_tof.make_arithmetic_64_tensor_output_my(add_output1);
    // main_output_future = arithmetic_tof.make_arithmetic_64_tensor_output_my(tensor_B1);
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
          MOTION::fixed_point::decode<uint64_t, long double>(main[i], options.fractional_bits);

      std::cout << temp << " , ";
    }
  }
}

int main(int argc, char* argv[]) {
  // testMemoryOccupied();
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
    testMemoryOccupied(WriteToFiles, options->my_id, options->layer_id, options->splits,
                       options->currentpath);
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
          MOTION::Statistics::print_stats_short("tensor_gt_mul_split", run_time_stats, comm_stats);
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