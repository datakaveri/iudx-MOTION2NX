/*
./bin/Image_Share_Receiver --my-id 0 --fractional-bits $fractional_bits --file-names $image_config
--index $i --current-path $build_path

./bin/Image_Share_Receiver --my-id 1 --fractional-bits $fractional_bits --file-names $image_config
--index $i --current-path $build_path

./bin/image_provider_iudx --compute-server0-port 1234 --compute-server1-port 1235 --fractional-bits
$fractional_bits --NameofImageFile X$i --filepath $image_path

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
  std::size_t num_simd;
  bool sync_between_setup_and_online;
  Matrix image;
  std::size_t fractional_bits;
  std::size_t my_id;
  // MOTION::Communication::tcp_parties_config tcp_config;
  bool no_run = false;
  std::string filenames;
  std::vector<std::string> data;
  int actual_answer;
  std::vector<std::string> filepaths;
  std::string index;
  std::string currentpath;
  int port;
};

void read_filenames(Options* options) {
  // read file_config.txt
  // auto p = std::filesystem::current_path();
  auto p = options->filenames;
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
  // creation of  eg. server0
  std::error_code err;
  std::string dirname = "server" + std::to_string(options->my_id);
  // std::filesystem::create_directories(dirname, err);

  //.../server0/
  // std::string temp = std::filesystem::current_path();
  std::string dirname1 = options->currentpath + "/" + dirname + "/Actual_label/";
  std::filesystem::create_directories(dirname1, err);
  std::string dirname2 = options->currentpath + "/" + dirname + "/Image_shares/";
  std::filesystem::create_directories(dirname2, err);
  std::ofstream file;
  std::string fullfilename;

  //../server0/filenameshare_id0 / 1 only for image shares, for actual answer it is only X
  std::cout << "Size of data: " << options->data.size() << "\n";
  std::string n0 = options->data[0] + options->index;
  fullfilename = dirname1 + n0;
  options->filepaths.push_back(fullfilename);
  // std::string n1 = options->data[1] + "_" + std::to_string(options->my_id);
  std::string n1 = options->data[1] + options->index;
  fullfilename = dirname2 + n1;
  options->filepaths.push_back(fullfilename);
}

void retrieve_shares(Options* options) {
  std::ofstream file;
  // options->actual_answer=COMPUTE_SERVER::get_actual_answer(port_number);

  auto [p1, p2, p3] = COMPUTE_SERVER::get_provider_mat_mul_data_new(options->port);
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

  std::vector<COMPUTE_SERVER::Shares> input_values_dp0 = p3.first;

  // std::ofstream file;
  auto temp = options->filepaths[1];

  file.open(temp, std::ios_base::out);
  if (file) {
    std::cout << "File found\n";
  } else {
    std::cout << "File not found\n";
  }
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
    ("port" , po::value<int>()->required(), "Port number on which to listen")
    ("threads", po::value<std::size_t>()->default_value(0), "number of threads to use for gate evaluation")
    ("json", po::bool_switch()->default_value(false), "output data in JSON format")
    ("fractional-bits", po::value<std::size_t>()->default_value(16),
     "number of fractional bits for fixed-point arithmetic")
    ("num-simd", po::value<std::size_t>()->default_value(1), "number of SIMD values")
    ("file-names",po::value<std::string>()->required(), "filename")
    ("index",po::value<std::string>()->default_value("0"), "index")
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
  options.port = vm["port"].as<int>();
  options.threads = vm["threads"].as<std::size_t>();
  options.json = vm["json"].as<bool>();
  options.num_simd = vm["num-simd"].as<std::size_t>();
  options.sync_between_setup_and_online = vm["sync-between-setup-and-online"].as<bool>();
  options.no_run = vm["no-run"].as<bool>();
  options.fractional_bits = vm["fractional-bits"].as<std::size_t>();
  options.filenames = vm["file-names"].as<std::string>();
  options.index = vm["index"].as<std::string>();
  options.currentpath = vm["current-path"].as<std::string>();
  std::cout << "index" << options.index << "\n";
  if (options.my_id > 1) {
    std::cerr << "my-id must be one of 0 and 1\n";
    return std::nullopt;
  }

  read_filenames(&options);

  if (options.my_id == 0) {
    generate_filepaths(&options);
  } else if (options.my_id == 1) {
    generate_filepaths(&options);
  }

  // if (options.my_id == 0) {
  //   retrieve_shares(1234, &options);
  // } else {
  //   retrieve_shares(1235, &options);
  // }

  retrieve_shares(&options);

  return options;
}

int main(int argc, char* argv[]) {
  auto options = parse_program_options(argc, argv);
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
