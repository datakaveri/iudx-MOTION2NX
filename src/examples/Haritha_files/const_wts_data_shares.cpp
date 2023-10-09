/*
inputpath changed to imageprovider
imageprovider is file name inside server 0 and server 1 respectively
eg. (ip1_0 , ip1_1) , (ip2_0,ip2_1) and so on..
from bash file we are giving ip1 and in this file it is appended to ip1_0 and ip1_1

At the argument "--filepath " give the path of the file containing shares from build_deb.... folder
Server-0
./bin/tensor_gt_mul_test --my-id 0 --party 0,::1,7002 --party 1,::1,7000 --arithmetic-protocol beavy
--boolean-protocol yao --fractional-bits 13 --config-file-model file_config_model0 --layer-id 1
--current-path ${BASE_DIR}/build_debwithrelinfo_gcc

Server-1
./bin/tensor_gt_mul_test --my-id 1 --party 0,::1,7002 --party 1,::1,7000 --arithmetic-protocol beavy
--boolean-protocol yao --fractional-bits 13 --config-file-model file_config_model1 --layer-id 1
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

#include "utility/linear_algebra.h"

namespace po = boost::program_options;
namespace fp = MOTION::new_fixed_point;
int j = 0;

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
  //std::cout << "in read_file in cnn : " << str << std::endl;
  std::string::size_type sz = 0;
  std::uint64_t ret = (uint64_t)std::stoull(str, &sz, 0);
  return ret;
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
  MOTION::Communication::tcp_parties_config tcp_config;
  bool no_run = false;
  Matrix W_file;
  Matrix B_file;
  // Files
  Matrix image_file;
  Matrix CW1_file;
  Matrix CB1_file;
  Matrix CW2_file;
  Matrix CB2_file;
  Matrix row;
  Matrix col;
};

template <typename E>
std::uint64_t blah(E& engine) {
  std::uniform_int_distribution<unsigned long long> dis(std::numeric_limits<std::uint64_t>::min(),
                                                        std::numeric_limits<std::uint64_t>::max());
  return dis(engine);
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

int image_shares(Options* options, std::string p) {
  std::cout << "In image_shares : " << std::endl;
  std::ifstream temps;
  try {
    temps.open(p);
    if (temps) {
      std::cout << "File found\n";
    } else {
      std::cout << "Image shares file not found at "+p+"\n";
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

  for (int i = 0; i < rows * cols; ++i) {
    try {
      uint64_t m1 = read_file(temps);
      options->image_file.Delta.push_back(m1);
      uint64_t m2 = read_file(temps);
      options->image_file.delta.push_back(m2);
     // std::cout << "D : " << m1 << "d : "<< m2 << "--";
    } catch (std::ifstream::failure e) {
      std::cerr << "Error while reading columns from image shares.\n";
      return EXIT_FAILURE;
    }
  }
  temps.close();
}
int CW1_shares(Options* options, std::string p) {
  std::ifstream indata;
  std::cout << "In CW1_Shares" << std::endl;
  try {
    indata.open(p);
    if (std::ifstream(p)) {
      std::cout << "File found\n";
    } else {
      std::cout << "Weight shares file not found at " + p + "\n";
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
    std::cerr << "Weight share input file is empty.\n";
    return EXIT_FAILURE;
  }

  int kernels, channels, rows, cols;
  try {
    rows = read_file(indata);
    options->CW1_file.row = rows;
    std::cout << "r " << rows << " ";
    cols = read_file(indata);
    options->CW1_file.col = cols;
    std::cout << "c " << cols << "\n";
  } catch (std::ifstream::failure e) {
    std::cerr << "Error while reading rows and columns from weight shares.\n";
    return EXIT_FAILURE;
  }

  for (int i = 0; i < rows * cols; ++i) {
    try {
      uint64_t m1 = read_file(indata);
      options->CW1_file.Delta.push_back(m1);
      uint64_t m2 = read_file(indata);
      options->CW1_file.delta.push_back(m2);
     // std::cout << "D : " << m1 << "d : "<< m2 << "--";
    } catch (std::ifstream::failure e) {
      std::cerr << "Error while reading input weight shares from file.\n";
      return EXIT_FAILURE;
    }
  }
  indata.close();
}

int CB1_shares(Options* options, std::string p) {
  std::cout << "In CB1_Shares" << std::endl;
  std::ifstream indata;

  try {
    if (std::ifstream(p)) {
      std::cout << "File found\n";
    } else {
      std::cout << "Bias shares file not found at " + p +"\n";
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
    options->CB1_file.row = rows;
    std::cout << "r " << rows << " ";
    cols = read_file(indata);
    options->CB1_file.col = cols;
    std::cout << "c " << cols << "\n";
     
  } catch (std::ifstream::failure e) {
    std::cerr << "Error while reading rows and columns from input bias share file.\n";
    return EXIT_FAILURE;
  }
  
  for (int i = 0; i < rows * cols; ++i) {
    //std::cout << "335 - I am into for loop \n";
    try {
      //std::cout << "337 - I am in try \n";
      //std:: cout << "path : "<< p << std::endl;
      uint64_t m1 = read_file(indata);
      
      //std::cout << "339 - I am in try after reading 1st elemnt\n";
      
      options->CB1_file.Delta.push_back(m1);
      uint64_t m2 = read_file(indata);
      options->CB1_file.delta.push_back(m2);
      //std::cout << m1 << "---" << m2 << std::endl;
    } catch (std::ifstream::failure e) {
      std::cerr << "Error while reading the input bias share file.\n";
      return EXIT_FAILURE;
    }
  }
  indata.close();
}


int CW2_shares(Options* options, std::string p) {
  std::ifstream indata;
  std::cout << "In CW2_Shares" << std::endl;
  try {
    indata.open(p);
    if (std::ifstream(p)) {
      std::cout << "File found\n";
    } else {
      std::cout << "Weight shares file not found at " + p + "\n";
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
    std::cerr << "Weight share input file is empty.\n";
    return EXIT_FAILURE;
  }

  int kernels, channels, rows, cols;
  try {
    rows = read_file(indata);
    options->CW2_file.row = rows;
    std::cout << "r " << rows << " ";
    cols = read_file(indata);
    options->CW2_file.col = cols;
    std::cout << "c " << cols << "\n";
  } catch (std::ifstream::failure e) {
    std::cerr << "Error while reading rows and columns from weight shares.\n";
    return EXIT_FAILURE;
  }

  for (int i = 0; i < rows * cols; ++i) {
    try {
      uint64_t m1 = read_file(indata);
      options->CW2_file.Delta.push_back(m1);
      uint64_t m2 = read_file(indata);
      options->CW2_file.delta.push_back(m2);
     // std::cout << "D : " << m1 << "d : "<< m2 << "--";
    } catch (std::ifstream::failure e) {
      std::cerr << "Error while reading input weight shares from file.\n";
      return EXIT_FAILURE;
    }
  }
  indata.close();
}

int CB2_shares(Options* options, std::string p) {
  std::cout << "In CB2_Shares" << std::endl;
  std::ifstream indata;

  try {
    if (std::ifstream(p)) {
      std::cout << "File found\n";
    } else {
      std::cout << "Bias shares file not found at " + p +"\n";
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
    options->CB2_file.row = rows;
    std::cout << "r " << rows << " ";
    cols = read_file(indata);
    options->CB2_file.col = cols;
    std::cout << "c " << cols << "\n";
     
  } catch (std::ifstream::failure e) {
    std::cerr << "Error while reading rows and columns from input bias share file.\n";
    return EXIT_FAILURE;
  }
  
  for (int i = 0; i < rows * cols; ++i) {
    //std::cout << "335 - I am into for loop \n";
    try {
      //std::cout << "337 - I am in try \n";
      //std:: cout << "path : "<< p << std::endl;
      uint64_t m1 = read_file(indata);
      
      //std::cout << "339 - I am in try after reading 1st elemnt\n";
      
      options->CB2_file.Delta.push_back(m1);
      uint64_t m2 = read_file(indata);
      options->CB2_file.delta.push_back(m2);
      //std::cout << m1 << "---" << m2 << std::endl;
    } catch (std::ifstream::failure e) {
      std::cerr << "Error while reading the input bias share file.\n";
      return EXIT_FAILURE;
    }
  }
  indata.close();
}


int W_shares(Options* options, std::string p) {
  std::ifstream indata;
  std::cout << "In W_Shares" << std::endl;
  try {
    indata.open(p);
    if (std::ifstream(p)) {
      std::cout << "File found\n";
    } else {
      std::cout << "Weight shares file not found at " + p + "\n";
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
    std::cerr << "Weight share input file is empty.\n";
    return EXIT_FAILURE;
  }

  int kernels, channels, rows, cols;
  try {
    rows = read_file(indata);
    options->W_file.row = rows;
    std::cout << "r " << rows << " ";
    cols = read_file(indata);
    options->W_file.col = cols;
    std::cout << "c " << cols << "\n";
  } catch (std::ifstream::failure e) {
    std::cerr << "Error while reading rows and columns from weight shares.\n";
    return EXIT_FAILURE;
  }

  for (int i = 0; i < rows * cols; ++i) {
    try {
      uint64_t m1 = read_file(indata);
      options->W_file.Delta.push_back(m1);
      uint64_t m2 = read_file(indata);
      options->W_file.delta.push_back(m2);
     // std::cout << "D : " << m1 << "d : "<< m2 << "--";
    } catch (std::ifstream::failure e) {
      std::cerr << "Error while reading input weight shares from file.\n";
      return EXIT_FAILURE;
    }
  }
  indata.close();
}

int B_shares(Options* options, std::string p) {
  std::cout << "In B_Shares" << std::endl;
  std::ifstream indata;

  try {
    if (std::ifstream(p)) {
      std::cout << "File found\n";
    } else {
      std::cout << "Bias shares file not found at " + p +"\n";
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
    options->B_file.row = rows;
    std::cout << "r " << rows << " ";
    cols = read_file(indata);
    options->B_file.col = cols;
    std::cout << "c " << cols << "\n";
     
  } catch (std::ifstream::failure e) {
    std::cerr << "Error while reading rows and columns from input bias share file.\n";
    return EXIT_FAILURE;
  }
  
  for (int i = 0; i < rows * cols; ++i) {
    //std::cout << "335 - I am into for loop \n";
    try {
      //std::cout << "337 - I am in try \n";
      //std:: cout << "path : "<< p << std::endl;
      uint64_t m1 = read_file(indata);
      
      //std::cout << "339 - I am in try after reading 1st elemnt\n";
      
      options->B_file.Delta.push_back(m1);
      uint64_t m2 = read_file(indata);
      options->B_file.delta.push_back(m2);
      //std::cout << m1 << "---" << m2 << std::endl;
    } catch (std::ifstream::failure e) {
      std::cerr << "Error while reading the input bias share file.\n";
      return EXIT_FAILURE;
    }
  }
  indata.close();
}

// changes made in this function
int file_read(Options* options) {
  std::string path = options->currentpath;
  std::string t1, i;
  // if layer_id=1 then read filename inside server
  // else read file_config_input (id is appended)
  if (options->layer_id == 1) {
    // std::cout << "hello\n";
    t1 = path + "/server" + std::to_string(options->my_id) + "/Image_shares/" +
         options->imageprovider;
  } else if (options->layer_id > 1) {
    // outputshare_0/1 inside server 1/0
    t1 = path + "/server" + std::to_string(options->my_id) + "/" + options->imageprovider + "_" +
         std::to_string(options->my_id);
  }

  image_shares(options, t1);

  //////////////////////////////////////////////////////////////////////////////////////////////////////////
  // model path --> builddebwithrelinfo_gcc/file_config_model0/1
  std::string t2 = path + "/" + options->modelpath;
  std::ifstream file2;
  try {
    file2.open(t2);
    if (file2) {
      std::cout << "File found\n";
    } else {
      std::cout << "File not found at "+ t2 +"\n";
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

  // std::cout << "w:" << w << "\n";
  // std::cout << "j:" << j << "\n";
  std::string b = read_filepath(file2, c + 1);
  // std::cout << "b:" << b << "\n";
  file2.close();
  if (options->my_id ==1 )
    {
     std::string cw1 = "/home/iudx/iudx-MOTION2NX/build_debwithrelinfo_gcc/server1/CW1";
     std::string cb1 = "/home/iudx/iudx-MOTION2NX/build_debwithrelinfo_gcc/server1/CB1";
     std::string cw2 = "/home/iudx/iudx-MOTION2NX/build_debwithrelinfo_gcc/server1/CW2";
     std::string cb2 = "/home/iudx/iudx-MOTION2NX/build_debwithrelinfo_gcc/server1/CB2";
     CW1_shares(options, cw1);
     CB1_shares(options, cb1);
     CW2_shares(options, cw2);
     CB2_shares(options, cb2);

    }
  else
    {
    std::string cw1 = "/home/iudx/iudx-MOTION2NX/build_debwithrelinfo_gcc/server0/CW1";
    std::string cb1 = "/home/iudx/iudx-MOTION2NX/build_debwithrelinfo_gcc/server0/CB1";
    std::string cw2 = "/home/iudx/iudx-MOTION2NX/build_debwithrelinfo_gcc/server0/CW2";
    std::string cb2 = "/home/iudx/iudx-MOTION2NX/build_debwithrelinfo_gcc/server0/CB2";
     CW1_shares(options, cw1);
     CB1_shares(options, cb1);
     CW2_shares(options, cw2);
     CB2_shares(options, cb2);
    }
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
  options.currentpath = vm["current-path"].as<std::string>();
  //////////////////////////////////////////////////////////////////
  options.imageprovider = vm["config-file-input"].as<std::string>();
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
  //file_read(&options);
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
/*******************************************************************/

  auto& arithmetic_tof = backend.get_tensor_op_factory(options.arithmetic_protocol);
  auto& boolean_tof = backend.get_tensor_op_factory(MOTION::MPCProtocol::Yao);
  std::cout << "Inside Build CNN" << std::endl;

  const MOTION::tensor::TensorDimensions input_dims{
      .batch_size_ = 1, .num_channels_ = 1, .height_ = 28, .width_ = 28};

  const MOTION::tensor::TensorDimensions conv_weights_dims{
      .batch_size_ = 5, .num_channels_ = 1, .height_ = 5, .width_ = 5};

  const MOTION::tensor::TensorDimensions input_dims_conv_2{
      .batch_size_ = 1, .num_channels_ = 5, .height_ = 13, .width_ = 13};

  const MOTION::tensor::TensorDimensions conv_weights_dims_2{
      .batch_size_ = 3, .num_channels_ = 5, .height_ = 4, .width_ = 4};

  const MOTION::tensor::TensorDimensions squashed_weights_dims{
      .batch_size_ = 1, .num_channels_ = 1, .height_ = 845, .width_ = 100};
  const MOTION::tensor::TensorDimensions fully_connected_weights_dims{
      .batch_size_ = 1, .num_channels_ = 1, .height_ = 100, .width_ = 10};

  MOTION::tensor::TensorDimensions CBias1_dims;
   CBias1_dims.batch_size_=  5;
   CBias1_dims.num_channels_ = 1;
   CBias1_dims.height_ = 1;
   CBias1_dims.width_ = 1;

   MOTION::tensor::TensorDimensions CBias2_dims;
   CBias2_dims.batch_size_=  3;
   CBias2_dims.num_channels_ = 1;//Haritha : should number of channels be 1 right??? 
   CBias2_dims.height_ = 1;
   CBias2_dims.width_ = 1;

  const MOTION::tensor::Conv2DOp conv_op = {.kernel_shape_ = {5, 1, 5, 5},
                                            .input_shape_ = {1, 28, 28},
                                            .output_shape_ = {5, 13, 13},
                                            .dilations_ = {1, 1},
                                            .pads_ = {1, 1, 0, 0},
                                            .strides_ = {2, 2}};

  const MOTION::tensor::Conv2DOp conv_op_2 = {.kernel_shape_ = {3, 5, 4, 4},
                                            .input_shape_ = {5, 13, 13},
                                            .output_shape_ = {3, 6, 6},
                                            .dilations_ = {1, 1},
                                            .pads_ = {1, 1, 0, 0},
                                            .strides_ = {2, 2}};
  const MOTION::tensor::GemmOp gemm_op_1 = {
      .input_A_shape_ = {1, 845}, .input_B_shape_ = {845, 100}, .output_shape_ = {1, 100}};
  const MOTION::tensor::GemmOp gemm_op_2 = {
      .input_A_shape_ = {1, 100}, .input_B_shape_ = {100, 10}, .output_shape_ = {1, 10}};

  std::cout << "----------------------------------" << std::endl << "Input Dimensions:" << std::endl;
  std::cout << "Batch Size: " << input_dims.batch_size_ << "\nNumber of Channels: " << input_dims.num_channels_
    << "\nHeight of Matrix: " << input_dims.height_ << "\nWidth of Matrix: " << input_dims.width_ << std::endl;
  std::cout << "----------------------------------" << std::endl;
  std::cout << "Convolutional Weights:\n"<< "Batch Size: " << conv_weights_dims.batch_size_ << "\nNumber of Channels: "
    << conv_weights_dims.num_channels_ << "\nHeight of Matrix: " << conv_weights_dims.height_ << "\nWidth of Matrix: "
    << conv_weights_dims.width_ << std::endl;
  std::cout << "----------------------------------" << std::endl;

//   MOTION::tensor::TensorCP input_tensor;
//   MOTION::tensor::TensorCP conv_weights_tensor;
//   MOTION::tensor::TensorCP squashed_weights_tensor;
//   MOTION::tensor::TensorCP fully_connected_weights_tensor;

//  make_activation = [&](const auto& input) {
//     const auto negated_tensor = arithmetic_tof.make_tensor_negate(input);
//     const auto boolean_tensor =
//         boolean_tof.make_tensor_conversion(MOTION::MPCProtocol::Yao, negated_tensor);
//     const auto relu_tensor = boolean_tof.make_tensor_relu_op(boolean_tensor);
//     const auto finBoolean_tensor =
//         boolean_tof.make_tensor_conversion(options.arithmetic_protocol, relu_tensor);
//     return arithmetic_tof.make_tensor_negate(finBoolean_tensor);
//   };

  

MOTION::tensor::TensorCP tensor_X, tensor_CW1, tensor_CB1, tensor_CW2, tensor_CB2;
MOTION::tensor::TensorCP gemm_output1, add_output1;

  auto pairX = arithmetic_tof.make_arithmetic_64_tensor_input_shares(input_dims);
  std::vector<ENCRYPTO::ReusableFiberPromise<MOTION::IntegerValues<uint64_t>>> input_promises_X =
      std::move(pairX.first);
  tensor_X = pairX.second;
std::cout<<"tensor_X ok \n";

  auto pairCW1 = arithmetic_tof.make_arithmetic_64_tensor_input_shares(conv_weights_dims);
  std::vector<ENCRYPTO::ReusableFiberPromise<MOTION::IntegerValues<uint64_t>>> input_promises_CW1 =
      std::move(pairCW1.first);
  tensor_CW1 = pairCW1.second;
std::cout<<"tensor_CW1 ok \n";

  auto pairCB1 = arithmetic_tof.make_arithmetic_64_tensor_input_shares(CBias1_dims);
  std::vector<ENCRYPTO::ReusableFiberPromise<MOTION::IntegerValues<uint64_t>>> input_promises_CB1 =
      std::move(pairCB1.first);
  tensor_CB1 = pairCB1.second;
std::cout<<"tensor_CB1 ok \n";


  auto pairCW2 = arithmetic_tof.make_arithmetic_64_tensor_input_shares(conv_weights_dims_2);
  std::vector<ENCRYPTO::ReusableFiberPromise<MOTION::IntegerValues<uint64_t>>> input_promises_CW2 =
      std::move(pairCW2.first);
  tensor_CW2 = pairCW2.second;
std::cout<<"tensor_CW2 ok \n";

  auto pairCB2 = arithmetic_tof.make_arithmetic_64_tensor_input_shares(CBias2_dims);
//   std::cout<<"pairCB2 ok \n";
  std::vector<ENCRYPTO::ReusableFiberPromise<MOTION::IntegerValues<uint64_t>>> input_promises_CB2 =
      std::move(pairCB2.first);
//   std::cout<<"input_promises_CB2 ok \n";
  tensor_CB2 = pairCB2.second;
std::cout<<"tensor_CB2 ok \n";


std :: cout << "Enetring into input_promises_ok \n";


  ///////////////////////////////////////////////////////////////
  input_promises_X[0].set_value(options.image_file.Delta);
  input_promises_X[1].set_value(options.image_file.delta);

  input_promises_CW1[0].set_value(options.CW1_file.Delta);
  input_promises_CW1[1].set_value(options.CW1_file.delta);

  input_promises_CB1[0].set_value(options.CB1_file.Delta);
  input_promises_CB1[1].set_value(options.CB1_file.delta);

//   input_promises_X2[0].set_value(options.image_file.Delta);
//   input_promises_X2[1].set_value(options.image_file.delta);

  input_promises_CW2[0].set_value(options.CW2_file.Delta);
  input_promises_CW2[1].set_value(options.CW2_file.delta);

  input_promises_CB2[0].set_value(options.CB2_file.Delta);
  input_promises_CB2[1].set_value(options.CB2_file.delta);
std :: cout << "input_promises_ok \n";

//***********************************************************************
std::function<MOTION::tensor::TensorCP(const MOTION::tensor::TensorCP&)> make_activation;

  make_activation = [&](const auto& input) {
    const auto negated_tensor = arithmetic_tof.make_tensor_negate(input);
    const auto boolean_tensor =
        boolean_tof.make_tensor_conversion(MOTION::MPCProtocol::Yao, negated_tensor);
    const auto relu_tensor = boolean_tof.make_tensor_relu_op(boolean_tensor);
    const auto finBoolean_tensor =
        boolean_tof.make_tensor_conversion(options.arithmetic_protocol, relu_tensor);
    return arithmetic_tof.make_tensor_negate(finBoolean_tensor);
  };
 
  /* checking what input values are */


  auto conv_output = arithmetic_tof.make_tensor_conv2d_op(
      conv_op, tensor_X, tensor_CW1, tensor_CB1, options.fractional_bits);
  auto cnn_relu_output = make_activation(conv_output);
  auto conv_output2 = arithmetic_tof.make_tensor_conv2d_op(
      conv_op_2, cnn_relu_output, tensor_CW2, tensor_CB2, options.fractional_bits);
  auto cnn_relu_output2 = make_activation(conv_output2);

  ENCRYPTO::ReusableFiberFuture<std::vector<std::uint64_t>> output_future;
  if (options.my_id == 0) {
    arithmetic_tof.make_arithmetic_tensor_output_other(cnn_relu_output2);
  } else {
    output_future = arithmetic_tof.make_arithmetic_64_tensor_output_my(cnn_relu_output2);
  }
  return output_future;
/////////////////////////////////////////////////////////////////////////////////////

}

void run_composite_circuit(const Options& options, MOTION::TwoPartyTensorBackend& backend) {
  auto output_future = create_composite_circuit(options, backend);
  backend.run();
  if (options.my_id == 1) {
    auto main = output_future.get();
    //   std::vector<long double> mod_x;
    //   // std::string path = std::filesystem::current_path();
    //   std::string path = options.currentpath;
    //   string filename = path + "/" + "output_tensor";
    //   std::ofstream x;
    //   x.open(filename, std::ios_base::app);
    //   x << options.imageprovider << "\n";
    int i = 1;
    for (int k = 0; k < main.size(); ++k)
          {
            long double temp =
          MOTION::new_fixed_point::decode<uint64_t, long double>(main[k], options.fractional_bits);
          if (k%6 == 0)
          {
            std::cout << i << " ::";
            i = i+1; 
          } 
          std::cout << std::fixed << std::setprecision(3) << temp << " ";
          
          if (k%6 == 5) 
            {
              std::cout << "\n";
            }

          }
    std::cout << "******************************************" << std::endl;
    std::cout << "print output of the CNN " << main.size() << std::endl;
    std::cout << "******************************************" << std::endl;
    // for (int i = 0; i < 5; ++i) 
    // {
    //   std::cout << i << "******" << std::endl;
    //   for (int j = 0; j < 5; ++j) 
    //   {
    //     for (int k = 0; k < 5; ++k)
    //       {
    //         long double temp =
    //       MOTION::new_fixed_point::decode<uint64_t, long double>(main[(i*5) + (j*13) + k ], options.fractional_bits);
      
    //       std::cout << temp << ",";

    //       }
    //       std::cout<<std::endl;
    //   }
    //   std::cout<<"*****************************************************"<<std::endl;
    // }
  }
}

int main(int argc, char* argv[]) {
  
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
    //run_composite_circuit(*options, backend);
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
  std :: cout << "CONST_model_data_shares \n";
  //Weights and bias matrices read from the files
  std :: vector<float> W_const;
  std :: vector<float> B_const;
  //Encoded versions of weights and biases
  std :: vector<uint64_t> W_encoded(4, 0);
  std :: vector<uint64_t> B_encoded(2, 0);

  //Read public shares of data into Delta
  std :: vector<uint64_t> Delta;
  //W_encoded X Delta
  std :: vector<uint64_t> Delta_encoded(2, 0);
  
  //manuallay reading model parameters (weights and biases)
  W_const.push_back(1);
  W_const.push_back(0);
  W_const.push_back(0);
  W_const.push_back(1);
  B_const.push_back(-1.2);
  B_const.push_back(0.8);


 //manuallay reading public shares of data
  Delta.push_back(13196712866701121397); //Delta for 1
  Delta.push_back(10960966305359638832); //Delta for 2

  std::size_t fractional_bits = options->fractional_bits;
 
// encoding Weights and biases (W_const and B_const)
// All public and private shares are in uint_64 format and are encoded, so to generate shares of WX
// i.e., multiplying W_const and Public shares we have to convert W_cost and B_const into encoded world
  std::transform(std::begin(W_const), std::end(W_const), std::begin(W_encoded), [fractional_bits](auto j) { return fp::encode<std::uint64_t, float>(j, 13); });
  std::transform(std::begin(B_const), std::end(B_const), std::begin(B_encoded), [fractional_bits](auto j) { return fp::encode<std::uint64_t, float>(j, 13); });
  
// Matrix multiply given 2 matrices A and B it gives A * B
// while multiplying Weights and public shares give the dimensuons appropriately
// In NN: we have (Transpose of W) * X
  Delta_encoded = MOTION :: matrix_multiply(2, 2, 1, W_encoded, Delta);
if (options->my_id ==0)
{
// read size and privateshares from files
        std :: vector<uint64_t> delta;
        delta.push_back(14063790581080738872); //delta for 1
        delta.push_back(5304746145521377138);  //delta for 2
        
        std :: vector<uint64_t> delta_encoded(2, 0);
        delta_encoded = MOTION :: matrix_multiply(2, 2, 1, W_encoded, delta);
        std :: vector<uint64_t> gmw_X_encoded(2, 0);
        std :: vector<uint64_t> gmw_X_decoded(2, 0);

        for (int i = 0; i < Delta_encoded.size(); i++)  
        {  
           
        gmw_X_encoded[i] =  - delta_encoded[i]; 
          
        }  
        std::transform(std::begin(gmw_X_encoded), std::end(gmw_X_encoded), std::begin(gmw_X_decoded), [fractional_bits](auto j) { return fp::truncate(j, 13); });

        std::random_device rd;
        std::mt19937 gen(rd());
        std :: vector<uint64_t> Delta_decoded;
        std :: vector<uint64_t> delta_decoded(2, 0);
        std :: cout << "%%%%%% In Party 0  %%%%%%%%%%%%%%%%%%%%%%\n " ; 
        for(int i = 0; i < gmw_X_decoded.size(); i++)
        {
        std::uint64_t del_decode = blah(gen);
        delta_decoded[i] = del_decode;
        Delta_decoded.push_back(gmw_X_decoded[i] + del_decode);
        std :: cout << Delta_decoded[i] << "   " << delta_decoded[i] << std::endl;
        }

}

if (options->my_id == 1)
{
        std :: vector<uint64_t> delta;
        delta.push_back(17579666359329925949);  //delta1 for 1
        delta.push_back(5656220159838245310);   //delta1 for 2

        std :: vector<uint64_t> delta_encoded(2, 0);
        delta_encoded = MOTION :: matrix_multiply(2, 2, 1, W_encoded, delta);

        std :: vector<uint64_t> gmw_X_encoded(2, 0);
        std :: vector<uint64_t> gmw_X_decoded(2, 0);
        for (int i = 0; i < Delta_encoded.size(); i++)  
        {     
        gmw_X_encoded[i] = Delta_encoded[i] - delta_encoded[i];  
        }  
        std::transform(std::begin(gmw_X_encoded), std::end(gmw_X_encoded), std::begin(gmw_X_decoded), [fractional_bits](auto j) { return fp::truncate(j, 13); });

        // std::random_device rd;
        // std::mt19937 gen(rd());
        std :: vector<uint64_t> Delta_decoded(2, 0);
        std :: vector<uint64_t> delta_decoded(2, 0);
        std::random_device rd;
        std::mt19937 gen(rd());
        std :: cout << "%%%%%% In Party 1  %%%%%%%%%%%%%%%%%%%%%%\n " ;
        for(int i = 0; i < gmw_X_decoded.size(); i++)
        {
        std::uint64_t del_decode = blah(gen);
        delta_decoded[i] = del_decode;
        Delta_decoded[i] = gmw_X_decoded[i] + del_decode;
        std :: cout << Delta_decoded[i] << "   " << del_decode << std::endl;
        }
        
}
// std :: vector<uint64_t> ans_encoded(2, 0);
// for(int i = 0; i < Delta1_decoded.size(); i++)
//         {
       
//         ans_encoded[i] = Delta1_decoded[i] + Delta0_decoded[i]- delta1_decoded[i]-delta0_decoded[i];
//         std :: cout << fp::decode<uint64_t, float>(ans_encoded[i],13) << std::endl;
//         }


return EXIT_SUCCESS;
}