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
  std::uint64_t Delta1;
  std::uint64_t delta1;
  std::uint64_t Delta2;
  std::uint64_t delta2;
  std::uint64_t Delta3,Delta4,Delta5,Delta6,Delta7,Delta8,Delta9,Delta10;
  std::uint64_t delta3,delta4,delta5,delta6,delta7,delta8,delta9,delta10;
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

    options.num_elements = read_file(indata);
    options.Delta1 = read_file(indata);
    options.delta1 = read_file(indata);
    options.Delta2 = read_file(indata);
    options.delta2 = read_file(indata);
    options.Delta3 = read_file(indata);
    options.delta3 = read_file(indata);
    options.Delta4 = read_file(indata);
    options.delta4 = read_file(indata);
    options.Delta5 = read_file(indata);
    options.delta5 = read_file(indata);
    options.Delta6 = read_file(indata);
    options.delta6 = read_file(indata);
    options.Delta7 = read_file(indata);
    options.delta7 = read_file(indata);
    options.Delta8 = read_file(indata);
    options.delta8 = read_file(indata);
    options.Delta9 = read_file(indata);
    options.delta9 = read_file(indata);
    options.Delta10 = read_file(indata);
    options.delta10 = read_file(indata);

    std::cout << "Delta A :- " << options.Delta1;
    std::cout << "\n";

    std::cout << "delta A :- " << options.delta1;
    std::cout << "\n";

    std::cout << "Delta B :- " << options.Delta2;
    std::cout << "\n";

    std::cout << "delta B :- " << options.delta2;
    std::cout << "\n";

    std::cout << "Delta C :- " << options.Delta3;
    std::cout << "\n";

    std::cout << "delta C :- " << options.delta3;
    std::cout << "\n";

    std::cout << "Delta  :- " << options.Delta4;
    std::cout << "\n";

    std::cout << "delta  :- " << options.delta4;
    std::cout << "\n";

    std::cout << "Delta  :- " << options.Delta5;
    std::cout << "\n";

    std::cout << "delta  :- " << options.delta5;
    std::cout << "\n";

    std::cout << "Delta  :- " << options.Delta6;
    std::cout << "\n";

    std::cout << "delta  :- " << options.delta6;
    std::cout << "\n";

    std::cout << "Delta  :- " << options.Delta7;
    std::cout << "\n";

    std::cout << "delta  :- " << options.delta7;
    std::cout << "\n";

    std::cout << "Delta  :- " << options.Delta8;
    std::cout << "\n";

    std::cout << "delta  :- " << options.delta8;
    std::cout << "\n";

    std::cout << "Delta  :- " << options.Delta9;
    std::cout << "\n";

    std::cout << "delta  :- " << options.delta9;
    std::cout << "\n";

    std::cout << "Delta  :- " << options.Delta10;
    std::cout << "\n";

    std::cout << "delta  :- " << options.delta10;
    std::cout << "\n";
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
  
  auto [A_promises,input_0_arith] = gate_factory_arith.make_arithmetic_64_input_gate_shares(1);
  auto [B_promises,input_1_arith] = gate_factory_arith.make_arithmetic_64_input_gate_shares(1);
  auto [C_promises,input_2_arith] = gate_factory_arith.make_arithmetic_64_input_gate_shares(1);
  auto [D_promises,input_3_arith] = gate_factory_arith.make_arithmetic_64_input_gate_shares(1);
  auto [E_promises,input_4_arith] = gate_factory_arith.make_arithmetic_64_input_gate_shares(1);
  auto [F_promises,input_5_arith] = gate_factory_arith.make_arithmetic_64_input_gate_shares(1);
  auto [G_promises,input_6_arith] = gate_factory_arith.make_arithmetic_64_input_gate_shares(1);
  auto [H_promises,input_7_arith] = gate_factory_arith.make_arithmetic_64_input_gate_shares(1);
  auto [I_promises,input_8_arith] = gate_factory_arith.make_arithmetic_64_input_gate_shares(1);
  auto [J_promises,input_9_arith] = gate_factory_arith.make_arithmetic_64_input_gate_shares(1);


  auto Delta1 = std::move(A_promises[0]);
  auto delta1 = std::move(A_promises[1]);
  auto Delta2 = std::move(B_promises[0]);
  auto delta2 = std::move(B_promises[1]);
  auto Delta3 = std::move(C_promises[0]);
  auto delta3 = std::move(C_promises[1]);
  auto Delta4 = std::move(D_promises[0]);
  auto delta4 = std::move(D_promises[1]);
  auto Delta5 = std::move(E_promises[0]);
  auto delta5 = std::move(E_promises[1]);
  auto Delta6 = std::move(F_promises[0]);
  auto delta6 = std::move(F_promises[1]);
  auto Delta7 = std::move(G_promises[0]);
  auto delta7 = std::move(G_promises[1]);
  auto Delta8 = std::move(H_promises[0]);
  auto delta8 = std::move(H_promises[1]);
  auto Delta9 = std::move(I_promises[0]);
  auto delta9 = std::move(I_promises[1]);
  auto Delta10 = std::move(J_promises[0]);
  auto delta10 = std::move(J_promises[1]);


    // convert the arithmetic shares into Boolean shares
  auto input_0_bool = backend.convert(options.boolean_protocol, input_0_arith);
  auto input_1_bool = backend.convert(options.boolean_protocol, input_1_arith);
  auto input_2_bool = backend.convert(options.boolean_protocol, input_2_arith);
  auto input_3_bool = backend.convert(options.boolean_protocol, input_3_arith);
  auto input_4_bool = backend.convert(options.boolean_protocol, input_4_arith);
  auto input_5_bool = backend.convert(options.boolean_protocol, input_5_arith);
  auto input_6_bool = backend.convert(options.boolean_protocol, input_6_arith);
  auto input_7_bool = backend.convert(options.boolean_protocol, input_7_arith);
  auto input_8_bool = backend.convert(options.boolean_protocol, input_8_arith);
  auto input_9_bool = backend.convert(options.boolean_protocol, input_9_arith);



  // load a boolean circuit for to compute 'greater-than'
  MOTION::CircuitLoader circuit_loader,circuit_loader_1;
  auto& gt_circuit =
      circuit_loader.load_gt_circuit(64, options.boolean_protocol != MOTION::MPCProtocol::Yao);
 auto& gtmux_circuit =
     circuit_loader.load_gtmux_circuit(64, options.boolean_protocol != MOTION::MPCProtocol::Yao);
  // apply the circuit to the Boolean sahres
  auto output_intermediate = backend.make_circuit(gtmux_circuit, input_0_bool, input_1_bool);
   output_intermediate = backend.make_circuit(gtmux_circuit, output_intermediate, input_2_bool);
   output_intermediate = backend.make_circuit(gtmux_circuit, output_intermediate, input_3_bool);
   output_intermediate = backend.make_circuit(gtmux_circuit, output_intermediate, input_4_bool);
   output_intermediate = backend.make_circuit(gtmux_circuit, output_intermediate, input_5_bool);
   output_intermediate = backend.make_circuit(gtmux_circuit, output_intermediate, input_6_bool);
   output_intermediate = backend.make_circuit(gtmux_circuit, output_intermediate, input_7_bool);
   output_intermediate = backend.make_circuit(gtmux_circuit, output_intermediate, input_8_bool);
  auto max = backend.make_circuit(gtmux_circuit, output_intermediate, input_9_bool);
  //auto output = gate_factory_arith.make_binary_gate(ENCRYPTO::PrimitiveOperationType::MUL, input_0_arith, input_1_arith);
  auto output_0 = backend.make_circuit(gt_circuit, max, input_0_bool);
  auto output_1 = backend.make_circuit(gt_circuit, max, input_1_bool);
  auto output_2 = backend.make_circuit(gt_circuit, max, input_2_bool);
  auto output_3 = backend.make_circuit(gt_circuit, max, input_3_bool);
  auto output_4 = backend.make_circuit(gt_circuit, max, input_4_bool);
  auto output_5 = backend.make_circuit(gt_circuit, max, input_5_bool);
  auto output_6 = backend.make_circuit(gt_circuit, max, input_6_bool);
  auto output_7 = backend.make_circuit(gt_circuit, max, input_7_bool);
  auto output_8 = backend.make_circuit(gt_circuit, max, input_8_bool);
  auto output_9 = backend.make_circuit(gt_circuit, max, input_9_bool);
  std::vector< MOTION::WireVector > bool_output;
  bool_output.push_back(output_0);
  bool_output.push_back(output_1);
  bool_output.push_back(output_2);
  bool_output.push_back(output_3);
  bool_output.push_back(output_4);
  bool_output.push_back(output_5);
  bool_output.push_back(output_6);
  bool_output.push_back(output_7);
  bool_output.push_back(output_8);
  bool_output.push_back(output_9);
  auto output_new = backend.convert(options.arithmetic_protocol, max);
  // create an output gates of the result
   //auto output_future = gate_factory_bool.make_boolean_output_gate_my(MOTION::ALL_PARTIES, bool_output);

  input_promises.push_back(std::move(Delta1));
  input_promises.push_back(std::move(delta1));
  input_promises.push_back(std::move(Delta2));
  input_promises.push_back(std::move(delta2));
  input_promises.push_back(std::move(Delta3));
  input_promises.push_back(std::move(delta3));
  input_promises.push_back(std::move(Delta4));
  input_promises.push_back(std::move(delta4));
  input_promises.push_back(std::move(Delta5));
  input_promises.push_back(std::move(delta5));
  input_promises.push_back(std::move(Delta6));
  input_promises.push_back(std::move(delta6));
  input_promises.push_back(std::move(Delta7));
  input_promises.push_back(std::move(delta7));
  input_promises.push_back(std::move(Delta8));
  input_promises.push_back(std::move(delta8));
  input_promises.push_back(std::move(Delta9));
  input_promises.push_back(std::move(delta9));
  input_promises.push_back(std::move(Delta10));
  input_promises.push_back(std::move(delta10));


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

  input_promises[0].set_value({options.Delta1});
  input_promises[1].set_value({options.delta1});
  input_promises[2].set_value({options.Delta2});
  input_promises[3].set_value({options.delta2});
  input_promises[4].set_value({options.Delta3});
  input_promises[5].set_value({options.delta3});
  input_promises[6].set_value({options.Delta4});
  input_promises[7].set_value({options.delta4});
  input_promises[8].set_value({options.Delta5});
  input_promises[9].set_value({options.delta5});
  input_promises[10].set_value({options.Delta6});
  input_promises[11].set_value({options.delta6});
  input_promises[12].set_value({options.Delta7});
  input_promises[13].set_value({options.delta7});
  input_promises[14].set_value({options.Delta8});
  input_promises[15].set_value({options.delta8});
  input_promises[16].set_value({options.Delta9});
  input_promises[17].set_value({options.delta9});
  input_promises[18].set_value({options.Delta10});
  input_promises[19].set_value({options.delta10});
  // execute the protocol
  // Error occurs below, possible fix - pass shares as a promise
  
  

     
     //auto [output_futures,input_final] = create_composite_circuit(options,backend);
  
      
      auto output_wire_0 = std::move(output_futures[0]);
      auto output_wire_1 = std::move(output_futures[1]);
      auto output_wire_2 =std::move(output_futures[2]);
      auto output_wire_3 = std::move(output_futures[3]);
      auto output_wire_4 = std::move(output_futures[4]);
      auto output_wire_5 =std::move(output_futures[5]);
      auto output_wire_6 =std::move(output_futures[6]);
      auto output_wire_7 = std::move(output_futures[7]);
      auto output_wire_8 = std::move(output_futures[8]);
      auto output_wire_9 =std::move(output_futures[9]);
     
     // create an output gates of the result
      
      auto output_bool_0 = gate_factory_bool.make_boolean_output_gate_my(MOTION::ALL_PARTIES, output_wire_0);
      auto output_bool_1 = gate_factory_bool.make_boolean_output_gate_my(MOTION::ALL_PARTIES, output_wire_1);
      auto output_bool_2 = gate_factory_bool.make_boolean_output_gate_my(MOTION::ALL_PARTIES, output_wire_2);
      auto output_bool_3 = gate_factory_bool.make_boolean_output_gate_my(MOTION::ALL_PARTIES, output_wire_3);
      auto output_bool_4 = gate_factory_bool.make_boolean_output_gate_my(MOTION::ALL_PARTIES, output_wire_4);
      auto output_bool_5 = gate_factory_bool.make_boolean_output_gate_my(MOTION::ALL_PARTIES, output_wire_5);
      auto output_bool_6 = gate_factory_bool.make_boolean_output_gate_my(MOTION::ALL_PARTIES, output_wire_6);
      auto output_bool_7 = gate_factory_bool.make_boolean_output_gate_my(MOTION::ALL_PARTIES, output_wire_7);
      auto output_bool_8 = gate_factory_bool.make_boolean_output_gate_my(MOTION::ALL_PARTIES, output_wire_8);
      auto output_bool_9 = gate_factory_bool.make_boolean_output_gate_my(MOTION::ALL_PARTIES, output_wire_9);

       // execute the protocol
      backend.run();

      // retrieve the result from the future
       auto bvs0 = output_bool_0.get();
       auto bvs1 = output_bool_1.get();
       auto bvs2 =  output_bool_2.get();
       auto bvs3 = output_bool_3.get();
       auto bvs4 = output_bool_4.get();
       auto bvs5 =  output_bool_5.get();
       auto bvs6 =  output_bool_6.get();
       auto bvs7 = output_bool_7.get();
       auto bvs8 = output_bool_8.get();
       auto bvs9 =  output_bool_9.get();
      // auto bvs = output_futures.get();
   
      auto gt_result0 = bvs0.at(0).Get(0);
      auto gt_result1 = bvs1.at(0).Get(0);
      auto gt_result2 = bvs2.at(0).Get(0);
      auto gt_result3 = bvs3.at(0).Get(0);
      auto gt_result4 = bvs4.at(0).Get(0);
      auto gt_result5 = bvs5.at(0).Get(0);
      auto gt_result6 = bvs6.at(0).Get(0);
      auto gt_result7 = bvs7.at(0).Get(0);
      auto gt_result8 = bvs8.at(0).Get(0);
      auto gt_result9 = bvs9.at(0).Get(0);
      

      

  
  if (!options.json) {
    std::cout << "Result is" << std::endl;
    std::cout << gt_result0;
    std::cout << gt_result1;
    std::cout << gt_result2;
    std::cout << gt_result3;
    std::cout << gt_result4;
    std::cout << gt_result5;
    std::cout << gt_result6;
    std::cout << gt_result7;
    std::cout << gt_result8;
    std::cout << gt_result9;
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
