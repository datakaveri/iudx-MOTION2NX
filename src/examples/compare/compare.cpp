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

struct Options {
  std::size_t threads;
  bool json;
  std::size_t num_repetitions;
  std::size_t num_simd;
  bool sync_between_setup_and_online;
  MOTION::MPCProtocol arithmetic_protocol;
  MOTION::MPCProtocol boolean_protocol;
  std::uint64_t num_elements;
  std::uint64_t DeltaA;
  std::uint64_t deltaA;
  std::uint64_t DeltaB;
  std::uint64_t deltaB;
  std::uint64_t DeltaC;
  std::uint64_t deltaC;
  std::vector<std::uint64_t> Delta;
  std::vector<std::uint64_t> delta;
  std::size_t my_id;
  MOTION::Communication::tcp_parties_config tcp_config;
  bool no_run = false;
};

std::uint64_t read_file(std::ifstream & indata) {
  std::string str;
  char num;
  while(indata >> std::noskipws >> num) {
    if(num != ' ' && num != '\n'){
    str.push_back(num);
    } else {
      break;
    }
  }
  std::string::size_type sz = 0;
  std::uint64_t ret = (uint64_t)std::stoull (str,&sz,0);
  return ret;
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

auto p = std::filesystem::current_path();
    if(options.my_id == 0) {
        p += "/../src/examples/tensor_split_functionality/A.txt";
    } else {
        p += "/../src/examples/tensor_split_functionality/B.txt";
    }

    std::ifstream indata;
    indata.open(p);

    if(!indata) {
        std::cerr << " Error in reading file\n";
        return std::nullopt;
    }

    std::uint64_t temp_Delta, temp_delta;
    options.num_elements = read_file(indata);
    // options.DeltaA = read_file(indata);
    // options.deltaA = read_file(indata);
    // options.Delta.push_back(options.DeltaA);
    // //options.Delta[0] = options.DeltaA;
    // // options.delta[0] = options.deltaA;
    // options.DeltaB = read_file(indata);
    // options.deltaB = read_file(indata);
    // options.Delta.push_back(options.DeltaB);
    // // options.Delta[1] = options.DeltaB;
    // // options.delta[1] = options.deltaB;
    // options.DeltaC = read_file(indata);
    // options.deltaC = read_file(indata);
    // options.Delta.push_back(options.DeltaC);
    // options.Delta[2] = options.DeltaC;
    // options.delta[2] = options.deltaC;
    //options.Delta.push_back(options.DeltaA);

  for (auto i = 0; i<options.num_elements ; ++i)
    {
     temp_Delta = read_file(indata); 
     options.Delta.push_back(temp_Delta);
     temp_delta = read_file(indata); 
     options.delta.push_back(temp_delta);
     std::cout << "Delta :- " << options.Delta[i];
     std::cout << " delta :- " << i << options.delta[i];
     std::cout << "\n";
    }
    options.DeltaA = options.Delta[0];
    options.deltaA = options.delta[0];
    options.DeltaB = options.Delta[1];
    options.deltaB = options.delta[1];
    options.DeltaC = options.Delta[2];
    options.deltaC = options.delta[2];

    // std::cout << "Delta A :- " << options.DeltaA;
    // std::cout << "\n";

    // std::cout << "delta A :- " << options.deltaA;
    // std::cout << "\n";

    // std::cout << "Delta B :- " << options.DeltaB;
    // std::cout << "\n";

    // std::cout << "delta B :- " << options.deltaB;
    // std::cout << "\n";

    // std::cout << "Delta C :- " << options.DeltaC;
    // std::cout << "\n";

    // std::cout << "delta C :- " << options.deltaC;
    // std::cout << "\n";
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

auto create_composite_circuit(const Options& options, MOTION::TwoPartyBackend& backend) {
auto& gate_factory_arith = backend.get_gate_factory(options.arithmetic_protocol);//gmw
  auto& gate_factory_bool = backend.get_gate_factory(options.boolean_protocol);// beavy

  // share the inputs using the arithmetic protocol
  // NB: the inputs need to always be specified in the same order:
  // here we first specify the input of party 0, then that of party 1
  std::vector<ENCRYPTO::ReusableFiberPromise<MOTION::IntegerValues<uint64_t>>> input_promises;
  // MOTION::WireVector input_0_arith, input_1_arith;
  // for (auto i = 0; i<options.num_elements ; ++i)
  //   {
  //    auto [temp_promises,input_0_arith_temp] = gate_factory_arith.make_arithmetic_64_input_gate_shares(1);
  //    auto DeltaA = std::move(A_promises[0]);
  //   }
  
  auto [A_promises,input_0_arith] = gate_factory_arith.make_arithmetic_64_input_gate_shares(1);
  auto [B_promises,input_1_arith] = gate_factory_arith.make_arithmetic_64_input_gate_shares(1);
  auto [C_promises,input_2_arith] = gate_factory_arith.make_arithmetic_64_input_gate_shares(1);

  auto DeltaA = std::move(A_promises[0]);
  auto deltaA = std::move(A_promises[1]);
  auto DeltaB = std::move(B_promises[0]);
  auto deltaB = std::move(B_promises[1]);
  auto DeltaC = std::move(C_promises[0]);
  auto deltaC = std::move(C_promises[1]);

    // convert the arithmetic shares into Boolean shares
  auto input_0_bool = backend.convert(options.boolean_protocol, input_0_arith);
  auto input_1_bool = backend.convert(options.boolean_protocol, input_1_arith);
  auto input_2_bool = backend.convert(options.boolean_protocol, input_2_arith);

  // load a boolean circuit for to compute 'greater-than'
  MOTION::CircuitLoader circuit_loader,circuit_loader_1;
  auto& gt_circuit =
      circuit_loader.load_gt_circuit(64, options.boolean_protocol != MOTION::MPCProtocol::Yao);
 auto& gtmux_circuit =
     circuit_loader.load_gtmux_circuit(64, options.boolean_protocol != MOTION::MPCProtocol::Yao);
  // apply the circuit to the Boolean sahres
  auto output_intermediate = backend.make_circuit(gtmux_circuit, input_0_bool, input_1_bool);
  auto max = backend.make_circuit(gtmux_circuit, output_intermediate, input_2_bool);
  //auto output = gate_factory_arith.make_binary_gate(ENCRYPTO::PrimitiveOperationType::MUL, input_0_arith, input_1_arith);
  auto output_0 = backend.make_circuit(gt_circuit, max, input_0_bool);
  auto output_1 = backend.make_circuit(gt_circuit, max, input_1_bool);
  auto output_2 = backend.make_circuit(gt_circuit, max, input_2_bool);
  std::vector< MOTION::WireVector > bool_output;
  bool_output.push_back(output_0);
  bool_output.push_back(output_1);
  bool_output.push_back(output_2);
  auto output_new = backend.convert(options.arithmetic_protocol, max);
  // create an output gates of the result
   //auto output_future = gate_factory_bool.make_boolean_output_gate_my(MOTION::ALL_PARTIES, bool_output);

  input_promises.push_back(std::move(DeltaA));
  input_promises.push_back(std::move(deltaA));
  input_promises.push_back(std::move(DeltaB));
  input_promises.push_back(std::move(deltaB));
  input_promises.push_back(std::move(DeltaC));
  input_promises.push_back(std::move(deltaC));

  //auto output_1 = gate_factory_arith.make_binary_gate(
     //ENCRYPTO::PrimitiveOperationType::ADD, input_0_arith, output_future_intermediate);
  
  //auto output_future = gate_factory_arith.make_arithmetic_64_output_gate_my(MOTION::ALL_PARTIES, output_new);
  // return promise and future to allow setting inputs and retrieving outputs
  return std::make_pair(std::move(bool_output), std::move(input_promises));

  
}

void run_composite_circuit(const Options& options, MOTION::TwoPartyBackend& backend) {
    auto [output_futures,input_promises] = create_composite_circuit(options, backend);
  auto& gate_factory_bool = backend.get_gate_factory(options.boolean_protocol);
     auto& gate_factory_arith = backend.get_gate_factory(options.arithmetic_protocol);
  if (options.no_run) {
    return;
  }

  input_promises[0].set_value({options.DeltaA});
  input_promises[1].set_value({options.deltaA});
  input_promises[2].set_value({options.DeltaB});
  input_promises[3].set_value({options.deltaB});
  input_promises[4].set_value({options.DeltaC});
  input_promises[5].set_value({options.deltaC});
  // execute the protocol
  // Error occurs below, possible fix - pass shares as a promise
  
  

     
     //auto [output_futures,input_final] = create_composite_circuit(options,backend);
  
      
      auto output_wire_0 = std::move(output_futures[0]);
      auto output_wire_1 = std::move(output_futures[1]);
      auto output_wire_2 =std::move(output_futures[2]);
     
     // create an output gates of the result
      
      auto output_bool_0 = gate_factory_bool.make_boolean_output_gate_my(MOTION::ALL_PARTIES, output_wire_0);
      auto output_bool_1 = gate_factory_bool.make_boolean_output_gate_my(MOTION::ALL_PARTIES, output_wire_1);
      auto output_bool_2 = gate_factory_bool.make_boolean_output_gate_my(MOTION::ALL_PARTIES, output_wire_2);


       // execute the protocol
      backend.run();

      // retrieve the result from the future
       auto bvs0 = output_bool_0.get();
       auto bvs1 = output_bool_1.get();
       auto bvs2 =  output_bool_2.get();
      // auto bvs = output_futures.get();
   
      auto gt_result0 = bvs0.at(0).Get(0);
      auto gt_result1 = bvs1.at(0).Get(0);
      auto gt_result2 = bvs2.at(0).Get(0);
  
  if (!options.json) {
    std::cout << "Result is" << std::endl;
    std::cout << gt_result0;
    std::cout << gt_result1;
    std::cout << gt_result2;
  }
  
  
  
  
  
  
  // backend.run();

  // // retrieve the result from the future
  // auto bvs = output_future.get();
  // auto gt_result = bvs.at(0);
  // // retrieve the result from the future
  // // auto bvs = output_future.get();
  // // auto gt_result = bvs.at(0);
  // if (!options.json) {
  //   std::cout << "Result is" << std::endl;
  //   std::cout << gt_result;
  //   // if (gt_result) {
  //   //   std::cout << "Party 0 has more money than Party 1" << std::endl;
  //   // } else {
  //   //   std::cout << "Party 1 has at least the same amount of money as Party 0" << std::endl;
  //   // }
  // }
}

int main(int argc, char* argv[]) {
  if (std::ifstream("/home/ramya/source-code/iudx-MOTION2NX/src/examples/tensor_split_functionality/Evaluator.txt")){
  remove("/home/ramya/source-code/iudx-MOTION2NX/src/examples/tensor_split_functionality/Evaluator.txt");}
  if (std::ifstream("/home/ramya/source-code/iudx-MOTION2NX/src/examples/tensor_split_functionality/Garbler.txt")){
  remove("/home/ramya/source-code/iudx-MOTION2NX/src/examples/tensor_split_functionality/Garbler.txt");}
  auto options = parse_program_options(argc, argv);
  std::cout << " Server id ";
  std::cout << options->my_id;
  std::cout << "\n";
  // auto temp_Delta = options->input_values_dp0_Delta;
  // std::cout << "a_Delta ";
  // for (auto i = temp_Delta.begin(); i != temp_Delta.end(); ++i) std::cout << *i << " ";
  // auto temp_delta = options->input_values_dp0_delta;
  // std::cout << "a_delta ";
  // for (auto j = temp_delta.begin(); j != temp_delta.end(); ++j) std::cout << *j << " ";
  // auto temp_Delta1 = options->input_values_dp1_Delta;
  // std::cout << "\n";
  // std::cout << "b_Delta ";
  // for (auto i = temp_Delta1.begin(); i != temp_Delta1.end(); ++i) std::cout << *i << " ";
  // auto temp_delta1 = options->input_values_dp1_delta;
  // std::cout << "b_delta ";
  // for (auto j = temp_delta1.begin(); j != temp_delta1.end(); ++j) std::cout << *j << " ";
  // std::cout << "\n";
  if (!options.has_value()) {
    return EXIT_FAILURE;
  }

  try {
    auto comm_layer = setup_communication(*options);
    auto logger = std::make_shared<MOTION::Logger>(options->my_id,
                                                   boost::log::trivial::severity_level::trace);
    comm_layer->set_logger(logger);
    MOTION::TwoPartyBackend backend(*comm_layer, options->threads,
                                          options->sync_between_setup_and_online, logger);
    run_composite_circuit(*options, backend);
    comm_layer->shutdown();
  } catch (std::runtime_error& e) {
    std::cerr << "ERROR OCCURRED: " << e.what() << "\n";
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
