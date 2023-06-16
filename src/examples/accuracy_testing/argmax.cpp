/*
./bin/argmax --my-id 0 --party 0,::1,7000 --party 1,::1,7001 --arithmetic-protocol
beavy --boolean-protocol yao --repetitions 1 --config-filename file_config_input0 --config-input X
./bin/argmax --my-id 1 --party 0,::1,7000 --party 1,::1,7001 --arithmetic-protocol
beavy --boolean-protocol yao --repetitions 1 --config-filename file_config_input1 --config-input X
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
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <random>
#include <regex>
#include <stdexcept>

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

namespace po = boost::program_options;

static std::vector<uint64_t> generate_inputs(const MOTION::tensor::TensorDimensions dims) {
  return MOTION::Helpers::RandomVector<uint64_t>(dims.get_data_size());
}

struct Matrix {
  std::vector<std::uint64_t> Delta_T;
  std::vector<std::uint64_t> delta_T;
};

struct Options {
  std::size_t threads;
  bool json;
  std::size_t num_repetitions;
  std::size_t num_simd;
  bool sync_between_setup_and_online;
  MOTION::MPCProtocol arithmetic_protocol;
  MOTION::MPCProtocol boolean_protocol;
  std::uint64_t num_elements;
  std::uint64_t col;
  std::size_t my_id;
  std::string currentpath;
  MOTION::Communication::tcp_parties_config tcp_config;
  std::string filepath;
  std::string inputfilename;
  // int x;
  bool no_run = false;
  Matrix input;
};

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
    file2 << "Argmax : \n";
    file2 << "RSS - " << rss << " kB\n";
    file2 << "Shared Memory - " << shared_mem << " kB\n";
    file2 << "Private Memory - " << rss - shared_mem << "kB\n";
    file2.close();
  }
}

std::uint64_t read_file(std::ifstream& indata) {
  std::cout << "Inside read file uint\n";
  std::string str;
  char num;
  while (indata >> std::noskipws >> num) {
    if (num != ' ' && num != '\n') {
      // std::cout<<"Read "<<num<<std::endl;
      str.push_back(num);
    } else {
      break;
    }
  }
  std::cout << "Read " << str << std::endl;
  std::string::size_type sz = 0;
  std::uint64_t ret = (uint64_t)std::stoull(str, &sz, 0);
  return ret;
}

std::string read_filepath(std::ifstream& indata) {
  // std::cout << "Inside read file path\n";
  std::string str;

  char num;
  while (indata) {
    // std::cout << "inside while \n";
    std::getline(indata, str);
    // std::cout << str << std::endl;
  }
  // std::cout << "string read filepath:" << str << " \n";
  return str;
}

int input_shares(Options* options, std::string p) {
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
    std::cerr << "Input share file is empty.\n";
    return EXIT_FAILURE;
  }

  int num_elements, col;
  try {
    num_elements = read_file(indata);
    std::cout << "num_elements: " << num_elements << " ";
    col = read_file(indata);
    std::cout << "col: " << col << "\n";
  } catch (std::ifstream::failure e) {
    std::cerr << "Error while reading rows and columns from input share file.\n";
    return EXIT_FAILURE;
  }

  options->num_elements = num_elements;
  options->col = col;

  for (int i = 0; i < options->num_elements; i++) {
    try {
      uint64_t m1 = read_file(indata);
      options->input.Delta_T.push_back(m1);
      uint64_t m2 = read_file(indata);
      options->input.delta_T.push_back(m2);
    } catch (std::ifstream::failure e) {
      std::cerr << "Error while reading the input share file.\n";
      return EXIT_FAILURE;
    }
  }
  indata.close();
}

int file_read(Options* options) {
  std::string path = options->currentpath;
  // std::string path = std::filesystem::current_path();
  std::string t1 = path + "/" + options->filepath;
  std::cout << t1 << std::endl;

  // std::string t2 = path + "/" + "server" + std::to_string(options->my_id) + "/Actual_label/" +
  //                  "remote_share_actual_answer";
  //  std::cout << t2 << std::endl;

  std::ifstream file1;

  try {
    if (std::ifstream(t1)) {
      std::cout << "File found\n";
    } else {
      std::cout << "File not found\n";
    }
    file1.open(t1);
  } catch (std::ifstream::failure e) {
    std::cerr << "Error while opening the file_config_input.\n";
    return EXIT_FAILURE;
  }

  try {
    if (is_empty(file1)) {
      // file is empty
    }
  } catch (std::ifstream::failure e) {
    std::cerr << "Input share file is empty.\n";
    return EXIT_FAILURE;
  }

  std::string i = read_filepath(file1);
  // std::cout << "i:" << i << "\n";

  file1.close();
  input_shares(options, i);

  // reading input number for test case

  // file1.open(t2);
  // if (file1) {
  //   std::cout << "File found\n";
  // } else {
  //   std::cout << "File not found\n";
  // }
  // options->x = read_file(file1);
  // file1.close();
}

std::optional<Options> parse_program_options(int argc, char* argv[]) {
  Options options;
  boost::program_options::options_description desc("Allowed options");
  // clang-format off
  desc.add_options()
    ("help,h", po::bool_switch()->default_value(false),"produce help message")
    ("config-file", po::value<std::string>(), "config file containing options")
    ("config-filename", po::value<std::string>()->required(), "Path of the shares file from build_debwithrelinfo folder")
    ("config-input", po::value<std::string>()->required(), "Path of the input from build_debwithrelinfo folder")
    ("my-id", po::value<std::size_t>()->required(), "my party id")
    ("party", po::value<std::vector<std::string>>()->multitoken(),
     "(party id, IP, port), e.g., --party 1,127.0.0.1,7777")
    ("threads", po::value<std::size_t>()->default_value(0), "number of threads to use for gate evaluation")
    ("json", po::bool_switch()->default_value(false), "output data in JSON format")
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
  options.filepath = vm["config-filename"].as<std::string>();
  options.inputfilename = vm["config-input"].as<std::string>();
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

  file_read(&options);

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
    auto obj = MOTION::Statistics::to_json("argmax", run_time_stats, comm_stats);
    obj.emplace("party_id", options.my_id);
    obj.emplace("arithmetic_protocol", MOTION::ToString(options.arithmetic_protocol));
    obj.emplace("boolean_protocol", MOTION::ToString(options.boolean_protocol));
    obj.emplace("simd", options.num_simd);
    obj.emplace("threads", options.threads);
    obj.emplace("sync_between_setup_and_online", options.sync_between_setup_and_online);
    // std::cout << obj << "\n";
  } else {
    std::cout << MOTION::Statistics::print_stats("argmax", run_time_stats, comm_stats);
  }
}

auto create_composite_circuit(const Options& options, MOTION::TwoPartyBackend& backend) {
  auto& gate_factory_arith = backend.get_gate_factory(options.arithmetic_protocol);  // gmw
  auto& gate_factory_bool = backend.get_gate_factory(options.boolean_protocol);      // beavy

  // share the inputs using the arithmetic protocol
  // NB: the inputs need to always be specified in the same order:
  // here we first specify the input of party 0, then that of party 1

  std::vector<ENCRYPTO::ReusableFiberPromise<MOTION::IntegerValues<uint64_t>>> input_1;
  std::vector<ENCRYPTO::ReusableFiberPromise<MOTION::IntegerValues<uint64_t>>> promises_1;
  // std::vector<std::vector<ENCRYPTO::ReusableFiberPromise<MOTION::IntegerValues<uint64_t>>>>input_final;
  //  MOTION::WireVector input_0_arith, input_1_arith;
  std::vector<MOTION::WireVector> input_bool, input_bool_1;
  for (int i = 0; i < options.num_elements; i++) {
    auto pair = gate_factory_arith.make_arithmetic_64_input_gate_shares(1);
    auto promises = std::move(pair.first);
    auto input_a_arith = std::move(pair.second);

    auto Delta = std::move(promises[0]);
    input_1.push_back(std::move(Delta));
    auto delta = std::move(promises[1]);
    input_1.push_back(std::move(delta));

    auto temp1 = backend.convert(options.boolean_protocol, input_a_arith);
    auto temp2 = backend.convert(options.boolean_protocol, input_a_arith);
    input_bool.push_back(std::move(temp1));
    input_bool_1.push_back(std::move(temp2));
  }

  MOTION::CircuitLoader circuit_loader, circuit_loader_1;
  auto& gt_circuit =
      circuit_loader.load_gt_circuit(64, options.boolean_protocol != MOTION::MPCProtocol::Yao);
  auto& gtmux_circuit =
      circuit_loader.load_gtmux_circuit(64, options.boolean_protocol != MOTION::MPCProtocol::Yao);

  auto max = std::move(input_bool[0]);
  // auto output_future = gate_factory_arith.make_arithmetic_64_output_gate_my(MOTION::ALL_PARTIES,
  // max); auto output_bool_0 = gate_factory_bool.make_boolean_output_gate_my(MOTION::ALL_PARTIES,
  // max);

  auto n = input_bool.size();
  for (int i = 1; i < n; ++i) {
    auto intermediate_wire = std::move(input_bool[i]);
    max = backend.make_circuit(gtmux_circuit, intermediate_wire, max);
  }

  std::vector<MOTION::WireVector> output_final;
  int n1 = input_bool_1.size();
  for (int j = 0; j < n1; ++j) {
    auto temp_0 = std::move(input_bool_1[j]);
    auto output1 = backend.make_circuit(gt_circuit, max, temp_0);
    output_final.push_back(output1);
  }

  // const int k=26;
  // auto temp1=backend.convert(options.arithmetic_protocol,max);
  // auto output_future = gate_factory_arith.make_arithmetic_64_output_gate_my(MOTION::ALL_PARTIES,
  // temp1);

  int k = options.num_elements;

  // std::array<ENCRYPTO::ReusableFiberFuture<MOTION::BitValues>, 26>* outputbool =
  // new std::array<ENCRYPTO::ReusableFiberFuture<MOTION::BitValues>, 26>();
  std::vector<size_t> gate_ids(10);
  for (int i = 0; i < options.num_elements; i++) {
    // (*outputbool)[i] =
    //     gate_factory_bool.make_boolean_output_gate_my(MOTION::ALL_PARTIES, output_final[i]);
    gate_ids[i] = gate_factory_bool.make_boolean_output_gate_my_wo_getting_output(
        MOTION::ALL_PARTIES, output_final[i]);
    // std::cout << gate_ids[i] << std::endl;
  }

  return std::make_pair(gate_ids, std::move(input_1));
}

// Reads the public and private share from each gate file and consolidates it to a single file to
// share to the image provider.
int consolidate_share_files(const Options& options, size_t num_outputs, size_t first_gate_number) {
  std::string op = getenv("BASE_DIR");
  std::ofstream outdata;

  if (options.my_id == 0) {
    op += "/build_debwithrelinfo_gcc/server0/Boolean_Output_Shares/Final_Boolean_Shares_server0_";
    op += options.inputfilename;
    op += ".txt";
  } else {
    op += "/build_debwithrelinfo_gcc/server1/Boolean_Output_Shares/Final_Boolean_Shares_server1_";
    op += options.inputfilename;
    op += ".txt";
  }
  outdata.open(op);
  assert(outdata);
  // outdata << options.my_id <<"\n";

  // outdata << options.inputfilename << "\n";

  outdata << num_outputs << "\n";

  for (size_t i = 0; i < num_outputs; ++i) {
    std::filesystem::__cxx11::path ip = getenv("BASE_DIR");
    std::ifstream indata;

    if (options.my_id == 0) {
      ip += "/build_debwithrelinfo_gcc/server0/Boolean_Output_Shares/output_share_for_server0_gate";
      ip += std::to_string(first_gate_number + i);
      ip += ".txt";
    } else {
      // ip += "/server1/Boolean_Output_Shares/output_share_for_server1_gate";
      ip += "/build_debwithrelinfo_gcc/server1/Boolean_Output_Shares/output_share_for_server1_gate";
      ip += std::to_string(first_gate_number + i);
      ip += ".txt";
    }

    try {
      indata.open(ip);
      if (indata) {
        std::cout << "File found\n";
      } else {
        std::cout << "File not found\n";
      }
    } catch (std::ifstream::failure e) {
      std::cerr << "Error while opening the boolean output share with gate file.\n";
      return EXIT_FAILURE;
    }

    auto output_Delta = read_file(indata);
    auto output_delta = read_file(indata);

    // outdata << first_gate_number+i << " ";

    // std::cout << output_Delta << " " << output_delta << "\n";

    outdata << output_Delta << " " << output_delta << "\n";

    remove(ip);
  }

  outdata.close();
}

void run_composite_circuit(const Options& options, MOTION::TwoPartyBackend& backend) {
  // auto [output_futures, input_promises] = create_composite_circuit(options, backend);
  auto [gate_ids, input_promises] = create_composite_circuit(options, backend);
  auto& gate_factory_bool = backend.get_gate_factory(options.boolean_protocol);
  auto& gate_factory_arith = backend.get_gate_factory(options.arithmetic_protocol);
  if (options.no_run) {
    return;
  }

  int j = 0;
  for (int i = 0; i < options.num_elements; ++i) {
    input_promises[j].set_value({options.input.Delta_T[i]});

    input_promises[j + 1].set_value({options.input.delta_T[i]});

    j += 2;
  }

  // backend.run_wo_broadcast();
  backend.run();

  std::sort(gate_ids.begin(), gate_ids.end());

  // std::cout << gate_ids[0] << std::endl;

  consolidate_share_files(options, gate_ids.size(), gate_ids[0]);

  // execute the protocol
  // int ans = 0;
  // std::vector<bool> gt_result;
  // for (int i = 0; i < options.num_elements; i++) {
  //   auto temp = output_futures[i].get().at(0).Get(0);

  //   if (temp == 0) {
  //     ans = i;
  //     break;
  //   }
  //   gt_result.push_back(temp);
  // }

  // std::string fn = options.inputfilename;
  // delete [] output_futures;
  // delete output_futures;
  // auto bvs=output_futures.get();
  // auto gt_result=bvs.at(0);
  // auto mult_result = bvs.at(0);
  // auto temp=bvs.at(0).Get(0);

  // if (!options.json) {
  //   // std::cout << "Input is " << options.x << " and output is " << ans;
  //   if (options.x == ans && options.my_id == 1) {
  //     /////// Read the counter number from counter file
  //     // std::string path = std::filesystem::current_path();
  //     std::string path = options.currentpath;
  //     std::string t1 = path + "/" + "count";
  //     std::ifstream file1;
  //     file1.open(t1);
  //     if (file1) {
  //       std::cout << "File found\n";
  //     } else {
  //       std::cout << "File not found\n";
  //     }
  //     int counter;
  //     file1 >> counter;

  //     file1.close();

  //     ///// Increment the number by 1 and rewrite it to the counter file.
  //     std::ofstream file2;
  //     file2.open(t1, std::ios_base::out);
  //     counter = counter + 1;
  //     file2 << counter;
  //     file2.close();
  //   } else if (options.x != ans) {
  //     std::string path = options.currentpath;
  //     // std::string path = std::filesystem::current_path();
  //     std::string t1 = path + "/" + "nomatch";
  //     std::ofstream file2;
  //     file2.open(t1, std::ios_base::app);
  //     file2 << options.inputfilename << "\n";
  //   }
  // }
}

int main(int argc, char* argv[]) {
  auto options = parse_program_options(argc, argv);
  std::cout << " Server id ";
  std::cout << options->my_id;
  std::cout << "\n";
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
    MOTION::TwoPartyBackend backend(*comm_layer, options->threads,
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
          MOTION::Statistics::print_stats_short("argmax", run_time_stats, comm_stats);
      // std::cout << "Execution time string:" << time_str << "\n";
      double exec_time = std::stod(time_str);
      // std::cout << "Execution time:" << exec_time << "\n";
      file2 << "Execution time - " << exec_time << "msec\n";
      file2 << "\n";
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
