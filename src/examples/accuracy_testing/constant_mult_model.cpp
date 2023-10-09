/*
inputpath changed to imageprovider
imageprovider is file name inside server 0 and server 1 respectively
eg. (ip1_0 , ip1_1) , (ip2_0,ip2_1) and so on..
from bash file we are giving ip1 and in this file it is appended to ip1_0 and ip1_1

At the argument "--filepath " give the path of the file containing shares from build_deb.... folder
Server-0
./bin/read_image_shares --my-id 0 --party 0,::1,7002 --party 1,::1,7000 --fractional-bits 13
--layer-id 1 --current-path ${BASE_DIR}/build_debwithrelinfo_gcc --config-file-input
remote_image_shares --weights-file newW1 --bias-file newB1


Server-1
./bin/read_image_shares --my-id 1 --party 0,::1,7002 --party 1,::1,7000 --fractional-bits 13
--layer-id 1 --current-path ${BASE_DIR}/build_debwithrelinfo_gcc --config-file-input
remote_image_shares --weights-file newW1 --bias-file newB1

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
#include <parallel/algorithm>

#include "communication/communication_layer.h"
#include "communication/tcp_transport.h"
#include "compute_server/compute_server.h"
#include "statistics/analysis.h"
#include "utility/logger.h"

#include <boost/thread/thread.hpp>
#include <chrono>
#include "base/two_party_tensor_backend.h"
#include "communication/message_handler.h"
#include "protocols/beavy/tensor.h"
#include "tensor/tensor.h"
#include "tensor/tensor_op.h"
#include "tensor/tensor_op_factory.h"
#include "utility/linear_algebra.h"
#include "utility/new_fixed_point.h"

namespace po = boost::program_options;

int j = 0;
// global variable
std::vector<std::uint64_t> Delta_otherid;
std::vector<std::uint64_t> delta_1;
bool flag1, flag2, flag0 = false;
bool server1_ready_flag, server0_ready_flag = false;

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
  std::size_t fractional_bits;
  std::string imageprovider;
  std::string modelpath;
  std::size_t layer_id;
  std::string currentpath;
  std::size_t my_id;
  MOTION::Communication::tcp_parties_config tcp_config;
  bool no_run = false;
  Matrix image_file;
  Matrix row;
  Matrix col;
  std::vector<float> B_data;
  std::vector<float> W_data;
  std::uint64_t W_row = 0, W_col = 0;
  std::uint64_t B_row = 0, B_col = 0;
  std::vector<std::uint64_t> finalDelta;
  std::string weights_file;
  std::string bias_file;
};

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
      std::cout << "Image shares File found\n";
    } else {
      std::cout << "Image shares File not found\n";
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
    std::cout << "Image r " << rows << " ";
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
      // std::cout << options->image_file.Delta[i] << " " << options->image_file.delta[i] << "\n";
    } catch (std::ifstream::failure e) {
      std::cerr << "Error while reading columns from image shares.\n";
      return EXIT_FAILURE;
    }
  }
  temps.close();
}

int read_W_csv(Options* options, std::string W_path) {
  std::ifstream file;
  std::cout << "W: " << W_path << "\n";
  try {
    file.open(W_path);
    if (file) {
      std::cout << "W_csv File found\n";
    } else {
      std::cout << "W_csv File not found\n";
    }
  } catch (std::ifstream::failure e) {
    std::cerr << "Error while opening the weight csv file.\n";
    return EXIT_FAILURE;
  }

  try {
    if (is_empty(file)) {
      // //file is empty
    }
  } catch (std::ifstream::failure e) {
    std::cerr << "W_csv file is empty.\n";
    return EXIT_FAILURE;
  }

  // //read weights
  std::string line;
  while (std::getline(file, line)) {
    std::stringstream lineStream(line);
    std::string cell;

    while (std::getline(lineStream, cell, ',')) {
      options->W_data.push_back(stof(cell));
    }
  }

  file.close();
  file.open(W_path);

  while (std::getline(file, line)) {
    float temp;
    std::istringstream iss(line);
    if (iss >> temp) {
      options->W_row++;
    }
  }
  int total_size = options->W_data.size();
  options->W_col = total_size / (options->W_row);

  file.close();
}

int read_B_csv(Options* options, std::string B_path) {
  std::ifstream file;
  std::cout << "B: " << B_path << "\n";
  try {
    file.open(B_path);
    if (file) {
      std::cout << "B_csv File found\n";
    } else {
      std::cout << "B_csv File not found\n";
    }
  } catch (std::ifstream::failure e) {
    std::cerr << "Error while opening the Bias csv file.\n";
    return EXIT_FAILURE;
  }

  try {
    if (is_empty(file)) {
      // //file is empty
    }
  } catch (std::ifstream::failure e) {
    std::cerr << "B_csv file is empty.\n";
    return EXIT_FAILURE;
  }

  // //read bias
  std::string line;
  while (std::getline(file, line)) {
    std::stringstream lineStream(line);
    std::string cell;

    while (std::getline(lineStream, cell, ',')) {
      options->B_data.push_back(stof(cell));
    }
  }

  file.close();
  file.open(B_path);

  while (std::getline(file, line)) {
    float temp;
    std::istringstream iss(line);
    if (iss >> temp) {
      options->B_row++;
    }
  }
  // std::cout << "B rows:" << options->B_row << "\n";
  options->B_col = 1;

  file.close();
}

int file_read(Options* options) {
  std::string path = options->currentpath;
  std::string t1, i;
  // //if layer_id=1 then read filename inside server
  // //else read file_config_input (id is appended)
  if (options->layer_id == 1) {
    t1 = path + "/server" + std::to_string(options->my_id) + "/Image_shares/" +
         options->imageprovider;
  } else if (options->layer_id > 1) {
    // outputshare_0/1 inside server 1/0
    t1 = path + "/server" + std::to_string(options->my_id) + "/" + options->imageprovider + "_" +
         std::to_string(options->my_id);
  }

  image_shares(options, t1);

  //////////////////////////////////////////////////////////////////////////////////////////////////////////
  std::string home_dir = getenv("BASE_DIR");
  std::string model_path = home_dir + "/data/ModelProvider/";
  std::string W1_path, W2_path, B1_path, B2_path;
  std::cout << model_path << "\n";

  W1_path = model_path + options->weights_file + ".csv";
  B1_path = model_path + options->bias_file + ".csv";

  read_W_csv(options, W1_path);
  read_B_csv(options, B1_path);
}

//////////////////////////////////////////////////////////////////////
std::optional<Options> parse_program_options(int argc, char* argv[]) {
  Options options;
  boost::program_options::options_description desc("Allowed options");
  // clang-format off
  desc.add_options()
    ("help,h", po::bool_switch()->default_value(false),"produce help message")
    ("config-file-input", po::value<std::string>()->required(), "config file containing options")
    ("my-id", po::value<std::size_t>()->required(), "my party id")
    ("layer-id", po::value<std::size_t>()->required(), "layer id")
    ("party", po::value<std::vector<std::string>>()->multitoken(),
     "(party id, IP, port), e.g., --party 1,127.0.0.1,7777")
    ("threads", po::value<std::size_t>()->default_value(0), "number of threads to use for gate evaluation")
    ("json", po::bool_switch()->default_value(false), "output data in JSON format")
    ("fractional-bits", po::value<std::size_t>()->default_value(16),
     "number of fractional bits for fixed-point arithmetic")
    ("repetitions", po::value<std::size_t>()->default_value(1), "number of repetitions")
    ("num-simd", po::value<std::size_t>()->default_value(1), "number of SIMD values")
    ("current-path",po::value<std::string>()->required(), "current path build_debwithrelinfo")
    ("sync-between-setup-and-online", po::bool_switch()->default_value(false),
     "run a synchronization protocol before the online phase starts")
    ("no-run", po::bool_switch()->default_value(false), "just build the circuit, but not execute it")
    ("weights-file", po::value<std::string>()->required(), "weights csv file")
    ("bias-file", po::value<std::string>()->required(), "biass csv file")


    ;
  // //clang-format on

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
  options.layer_id = vm["layer-id"].as<std::size_t>();
  options.weights_file = vm["weights-file"].as<std::string>();
  options.bias_file = vm["bias-file"].as<std::string>();
  /////////////////////////////////////////////////////////////////
  options.fractional_bits = vm["fractional-bits"].as<std::size_t>();
  if (options.my_id > 1) {
    std::cerr << "my-id must be one of 0 and 1\n";
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
    obj.emplace("simd", options.num_simd);
    obj.emplace("threads", options.threads);
    obj.emplace("sync_between_setup_and_online", options.sync_between_setup_and_online);
    // std::cout << obj << "\n";
  } else {
    std::cout << MOTION::Statistics::print_stats("read_image_shares", run_time_stats, comm_stats);
  }
}

template <typename E>
std::uint64_t RandomNumDistribution(E& engine) {
  std::uniform_int_distribution<unsigned long long> distribution(
      std::numeric_limits<std::uint64_t>::min(), std::numeric_limits<std::uint64_t>::max());
  return distribution(engine);
}

int read_shares(const Options& options,
                 std::pair<std::vector<uint64_t>, std::vector<uint64_t>>& message) {
  auto frac_bits = options.fractional_bits;
  auto my_id = options.my_id;

  try {
      if(options.W_col!=options.image_file.row)
  {
     std::cout<<"Weight's column and image's row donot match ! \n";
  }
  } catch (std::ifstream::failure e) {
    std::cerr << "Error in dimensions of matrices.\n";
    return EXIT_FAILURE;
  }

  int size_of_weights = options.W_col * options.W_row;
  std::vector<uint64_t> W_encoded(size_of_weights, 0);
  std::transform(std::begin(options.W_data), std::end(options.W_data), std::begin(W_encoded),
                 [frac_bits](auto j) {
                   return MOTION::new_fixed_point::encode<std::uint64_t, float>(j, frac_bits);
                 });

  // //gmw share generation
  std::vector<uint64_t> Delta_encoded(options.W_row * 1, 0);
  Delta_encoded =
      MOTION::matrix_multiply(options.W_row, options.W_col, 1, W_encoded, options.image_file.Delta);
  std::vector<uint64_t> delta_encoded(options.W_row * 1, 0);
  delta_encoded =
      MOTION::matrix_multiply(options.W_row, options.W_col, 1, W_encoded, options.image_file.delta);

  std::vector<uint64_t> gmw_x_encoded(options.W_row * 1, 0);
  std::transform(Delta_encoded.begin(), Delta_encoded.end(), gmw_x_encoded.begin(),
                 [&my_id](auto& c) { return c * my_id; });

  // //gmw share = gmw_x_encoded
  __gnu_parallel::transform(gmw_x_encoded.begin(), gmw_x_encoded.end(), delta_encoded.begin(),
                            gmw_x_encoded.begin(), std::minus{});

  // //truncate gmw share
  std::vector<uint64_t> gmw_x_decoded(options.W_row * 1, 0);
  std::transform(std::begin(gmw_x_encoded), std::end(gmw_x_encoded), std::begin(gmw_x_decoded),
                 [frac_bits](auto j) { return MOTION::new_fixed_point::truncate(j, frac_bits); });

  // //generation of delta shares
  std::random_device rd;
  std::mt19937 gen(rd());
  std::vector<uint64_t> delta_decoded(options.W_row * 1, 0);
  std::transform(delta_decoded.begin(), delta_decoded.end(), delta_decoded.begin(),
                 [&gen](auto j) { return RandomNumDistribution(gen); });

  std::vector<uint64_t> Delta_decoded(options.W_row * 1, 0);
  __gnu_parallel::transform(gmw_x_decoded.begin(), gmw_x_decoded.end(), delta_decoded.begin(),
                            Delta_decoded.begin(), std::plus{});

  message.first = Delta_decoded;
  message.second = delta_decoded;
  // std::cout << "Size of weights:" << size_of_weights << "\n";
  // std::cout << "length of W_encoded:" << W_encoded.size() << "\n";
  // std::cout << "length of B_encoded:" << B_encoded.size() << "\n";
  // std::cout << "length of gmw_x_encoded:" << gmw_x_encoded.size() << "\n";
  // std::cout << "length of gmw_x_decoded:" << gmw_x_decoded.size() << "\n";
  // std::cout << "length of Delta_decoded:" << Delta_decoded.size() << "\n";
  // std::cout << "length of delta_decoded:" << delta_decoded.size() << "\n";
}

std::uint64_t getuint64(std::vector<std::uint8_t>& message, int index) {
  // //Converts 8->64
  std::uint64_t num = 0;
  for (auto i = 0; i < 8; i++) {
    num = num << 8;
    num = num | message[(index + 1) * 8 - 1 - i];
  }
  return num;
}

void adduint64(std::uint64_t num, std::vector<std::uint8_t>& message) {
  // //Converts 64->8
  for (auto i = 0; i < sizeof(num); i++) {
    std::uint8_t byte = num & 0xff;
    message.push_back(byte);
    num = num >> 8;
  }
}

class TestMessageHandler : public MOTION::Communication::MessageHandler {
  void received_message(std::size_t party_id, std::vector<std::uint8_t>&& message) override {
    std::cout << "Message received from party " << party_id << "\n";
    std::cout << "Message size:" << message.size() << "\n";
    auto message_size = message.size();

    if (flag0 == false && flag1 == false) {
      std::cout << "Message1 after receiving from party 0 :\n";
      for (int i = 0; i < message_size / 8; i++) {
        auto temp = getuint64(message, i);
        Delta_otherid.push_back(temp);
        // std::cout << "Delta_otherid: " << Delta_otherid[i] << "\n";
      }
      flag1 = true;
    } else if (flag0 == false && flag1 == true) {
      std::cout << "Message1 after receiving from party 1 :\n";
      for (int i = 0; i < message_size / 8; i++) {
        auto temp = getuint64(message, i);
        Delta_otherid.push_back(temp);
        // std::cout << "Delta_otherid: " << Delta_otherid[i] << "\n";
      }
      flag0 = true;
    }
  }
};

int compute_arithmetic_shares(
    std::pair<std::vector<uint64_t>, std::vector<std::uint64_t>> myshares, Options& options) {
  std::vector<std::uint64_t> Delta_myid = myshares.first;
  std::vector<std::uint64_t> delta_myid = myshares.second;
  auto frac_bits = options.fractional_bits;
  // //global variables : Delta_otherid , Delta
  options.finalDelta = Delta_myid;
  // // Delta of both the parties
  __gnu_parallel::transform(Delta_myid.begin(), Delta_myid.end(), Delta_otherid.begin(),
                            options.finalDelta.begin(), std::plus{});

  std::vector<std::uint64_t> encoded_B(options.B_row, 1);
  std::transform(options.B_data.begin(), options.B_data.end(), encoded_B.begin(),
                 [frac_bits](auto j) {
                   return MOTION::new_fixed_point::encode<std::uint64_t, float>(j, frac_bits);
                 });

  // //adding bias to weights
  __gnu_parallel::transform(options.finalDelta.begin(), options.finalDelta.end(), encoded_B.begin(),
                            options.finalDelta.begin(), std::plus{});

  std::ofstream file;
  auto output_file_path = options.currentpath + "/server" + std::to_string(options.my_id) +
                          "/outputshare_" + std::to_string(options.my_id);

  std::cout << "output file path : " << output_file_path << "\n";
  

  try {
    file.open(output_file_path, std::ios_base::out);

    if (file) {
      std::cout << "outputshare file found\n";
    } else {
      std::cout << "outputshare file not found\n";
    }
  } catch (std::ifstream::failure e) {
    std::cerr << "Error while opening the image share file.\n";
    return EXIT_FAILURE;
  }

  file << options.B_row << " " << options.B_col << "\n";
  for (int i = 0; i < options.finalDelta.size(); i++) {
    file << options.finalDelta[i] << " " << delta_myid[i] << "\n";
  }
  file.close();
  
  //file_config_input 
  std::string path_for_next_operation;
  path_for_next_operation=options.currentpath+"/file_config_input"+std::to_string(options.my_id);
  
  file.open(path_for_next_operation,std::ios_base::out);
  file<<output_file_path;
  
  file.close();
}

void reconstruct_ans(std::pair<std::vector<uint64_t>, std::vector<std::uint64_t>> myshares,
                     Options& options) {
  std::vector<std::uint64_t> delta_0 = myshares.second;
  auto frac_bits = options.fractional_bits;

  // // global variables : delta1 , Delta
  std::vector<std::uint64_t> output_share(options.W_row, 0);
  std::vector<std::uint64_t> temp(options.W_row, 0);
  __gnu_parallel::transform(options.finalDelta.begin(), options.finalDelta.end(), delta_0.begin(),
                            temp.begin(), std::minus{});
  __gnu_parallel::transform(temp.begin(), temp.end(), delta_1.begin(), output_share.begin(),
                            std::minus{});

  std::vector<float> final_share(options.W_row, 0.0);
  std::transform(std::begin(output_share), std::end(output_share), std::begin(final_share),
                 [frac_bits](auto j) {
                   return MOTION::new_fixed_point::decode<std::uint64_t, float>(j, frac_bits);
                 });

  std::cout << "Output:-\n";
  for (int i = 0; i < final_share.size(); i++) {
    std::cout << final_share[i] << "\n";
  }
}

int main(int argc, char* argv[]) {
  std::cout<<"Inside constant_mult_model.cpp\n";
  auto start = std::chrono::high_resolution_clock::now();
  auto options = parse_program_options(argc, argv);
  if (!options.has_value()) {
    return EXIT_FAILURE;
  }

  std::unique_ptr<MOTION::Communication::CommunicationLayer> comm_layer;
  std::shared_ptr<MOTION::Logger> logger;
  try {
    try {
      MOTION::Communication::TCPSetupHelper helper(options->my_id, options->tcp_config);
      comm_layer = std::make_unique<MOTION::Communication::CommunicationLayer>(
          options->my_id, helper.setup_connections());
    } catch (std::runtime_error& e) {
      std::cerr << "Error occurred during connection setup: " << e.what() << "\n";
      return EXIT_FAILURE;
    }
    try {
      logger = std::make_shared<MOTION::Logger>(options->my_id,
                                                boost::log::trivial::severity_level::trace);
      comm_layer->set_logger(logger);
    } catch (std::runtime_error& e) {
      std::cerr << "Error occurred during logger setup: " << e.what() << "\n";
      return EXIT_FAILURE;
    }
    try {
      comm_layer->start();
    } catch (std::runtime_error& e) {
      std::cerr << "Error occurred while starting the communication: " << e.what() << "\n";
      return EXIT_FAILURE;
    }

    comm_layer->register_fallback_message_handler(
        [](auto party_id) { return std::make_shared<TestMessageHandler>(); });

    // //read shares from compute server
    std::pair<std::vector<uint64_t>, std::vector<uint64_t>> message;
    std::cout << "before read_shares \n";
    read_shares(*options, message);
    std::cout << "after read_shares \n";

    // //convert Delta(uint64_t) to Delta(std::vector<std::uint8_t>)
    std::vector<std::uint64_t> Delta_i = message.first;
    std::vector<std::uint8_t> message_byte;

    for (int i = 0; i < Delta_i.size(); i++) {
      // std::cout << Delta_i[i] << " ";
      adduint64(Delta_i[i], message_byte);
    }

    try {
      // std::cout << "message size before sending:" << ackn.size() << "\n";
      if (options->my_id == 0) {
        std::cout << "sending message from party 0 \n";
        comm_layer->send_message(1, message_byte);
        std::cout << "message from party 0 sent \n";

      } else if (options->my_id == 1) {
        while (!flag1) {
          std::cout << ".";
          boost::this_thread::sleep_for(boost::chrono::milliseconds(100));
        }

        std::cout << "message(shares) recieved from party 0\n";

        std::cout << "sending message from party 1 \n";
        comm_layer->send_message(0, message_byte);
        std::cout << "message from party 1 sent \n";
      }

    } catch (std::runtime_error& e) {
      std::cerr << "Error occurred while sending the weight shares to helper node: " << e.what()
                << "\n";
      return EXIT_FAILURE;
    }

    std::cout << "Parties waiting\n";
    while (!flag0 && !flag1) {
      std::cout << ".";
      boost::this_thread::sleep_for(boost::chrono::milliseconds(100));
    }
    std::cout << "Both parties ready to compute\n";
    compute_arithmetic_shares(message, *options);

    comm_layer->shutdown();
    auto stop = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start);

    std::cout << "Duration:" << duration.count() << "\n";

    std::string t1 = options->currentpath + "/" + "AverageTimeDetails0";
    std::string t2 = options->currentpath + "/" + "MemoryDetails0";

    std::ofstream file2;
    file2.open(t2, std::ios_base::app);
    if (!file2.is_open()) {
      std::cerr << "Unable to open the MemoryDetails file.\n";
    } else {
      file2 << "Execution time - " << duration.count() << "msec";
      file2 << "\n";
    }
    file2.close();

    std::ofstream file1;
    file1.open(t1, std::ios_base::app);
    if (!file1.is_open()) {
      std::cerr << "Unable to open the AverageTimeDetails file.\n";
    } else {
      file1 << duration.count();
      file1 << "\n";
    }
    file1.close();
  } catch (std::runtime_error& e) {
    std::cerr << "ERROR OCCURRED: " << e.what() << "\n";
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}