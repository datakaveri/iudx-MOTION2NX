//./bin/tensor_dot_product_new --my-id 0 --party 0,::1,7000 --party 1,::1,7001 --arithmetic-protocol beavy --boolean-protocol yao --repetitions 1

//./bin/tensor_dot_product_new --my-id 1 --party 0,::1,7000 --party 1,::1,7001 --arithmetic-protocol beavy --boolean-protocol yao --repetitions 1

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
  std::uint64_t Delta;
  std::vector<std::uint64_t>Delta_T;
  std::vector<std::uint64_t>delta_T;
  std::uint64_t delta;
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
    
   options.num_elements=read_file(indata);

   std::cout<<options.num_elements<<std::endl;

   for(int i=0;i<options.num_elements;i++)
   {
     options.Delta= read_file(indata);
     options.Delta_T.push_back(options.Delta);
     //std::cout<<"Delta "<<i<<" :- "<< options.Delta<<" ";
      std::cout<<"Delta "<<i<<" :- "<< options.Delta_T[i]<<" ";
     options.delta= read_file(indata);
     options.delta_T.push_back(options.delta);
     //std::cout<<"delta "<<i<<" :- "<< options.delta<<std::endl;
       std::cout<<"delta "<<i<<" :- "<< options.delta_T[i]<<std::endl;
   }


  
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

  std::vector<ENCRYPTO::ReusableFiberPromise<MOTION::IntegerValues<uint64_t>>> input_1;
  std::vector<ENCRYPTO::ReusableFiberPromise<MOTION::IntegerValues<uint64_t>>> promises_1;
   //std::vector<std::vector<ENCRYPTO::ReusableFiberPromise<MOTION::IntegerValues<uint64_t>>>>input_final;
   // MOTION::WireVector input_0_arith, input_1_arith;
  std::vector<MOTION::WireVector>input_bool,input_bool_1;
for(int i=0;i<options.num_elements;i++)
 { auto pair= gate_factory_arith.make_arithmetic_64_input_gate_shares(1);
  auto promises=std::move(pair.first);
  auto input_a_arith=std::move(pair.second);

  auto Delta=std::move(promises[0]);
  input_1.push_back(std::move(Delta));
  auto delta=std::move(promises[1]);
  input_1.push_back(std::move(delta));
  
  
   auto temp1=backend.convert(options.boolean_protocol,input_a_arith);
   auto temp2=backend.convert(options.boolean_protocol,input_a_arith);
   input_bool.push_back(std::move(temp1));
   input_bool_1.push_back(std::move(temp2));

 }

 MOTION::CircuitLoader circuit_loader,circuit_loader_1;
  auto& gt_circuit =
      circuit_loader.load_gt_circuit(64, options.boolean_protocol != MOTION::MPCProtocol::Yao);
 auto& gtmux_circuit =
     circuit_loader.load_gtmux_circuit(64, options.boolean_protocol != MOTION::MPCProtocol::Yao);
  
  auto max = std::move(input_bool[0]);
   //auto output_future = gate_factory_arith.make_arithmetic_64_output_gate_my(MOTION::ALL_PARTIES, max);
    //auto output_bool_0 = gate_factory_bool.make_boolean_output_gate_my(MOTION::ALL_PARTIES, max);
  auto n=input_bool.size();
  for(int i=1;i<n;++i){
       auto intermediate_wire = std::move(input_bool[i]);
        max = backend.make_circuit(gtmux_circuit, intermediate_wire , max);

  }
  
  std::vector<MOTION::WireVector>output_final;
  int n1=input_bool_1.size();
  for(int j=0;j<n1;++j){
      
      auto temp_0=std::move(input_bool_1[j]);
      auto output1=backend.make_circuit(gt_circuit,max,temp_0);
      output_final.push_back(output1);
  }

   
 //auto temp1=backend.convert(options.arithmetic_protocol,max);
 //auto output_future = gate_factory_arith.make_arithmetic_64_output_gate_my(MOTION::ALL_PARTIES, temp1);

 std::array<ENCRYPTO::ReusableFiberFuture<MOTION::BitValues>,26> *outputbool=new std::array<ENCRYPTO::ReusableFiberFuture<MOTION::BitValues>,26>();
       for(int i=0;i<options.num_elements;i++)
      {(*outputbool)[i]= gate_factory_bool.make_boolean_output_gate_my(MOTION::ALL_PARTIES, output_final[i]);}
    


return std::make_pair(std::move(*outputbool),std::move(input_1));

  
}

void run_composite_circuit(const Options& options, MOTION::TwoPartyBackend& backend) {
    auto [output_futures,input_promises] = create_composite_circuit(options, backend);
    auto& gate_factory_bool = backend.get_gate_factory(options.boolean_protocol);
    auto& gate_factory_arith = backend.get_gate_factory(options.arithmetic_protocol);
  if (options.no_run) {
    return;
  }
    std::cout<<input_promises.size()<<std::endl;
    int j=0;
   for(int i=0;i<options.num_elements;i++)
    {
         std::cout<<options.Delta_T[i]<<std::endl;
         input_promises[j].set_value({options.Delta_T[i]});
      
   
         std::cout<<options.delta_T[i]<<std::endl;
         input_promises[j+1].set_value({options.delta_T[i]});
      
      j+=2;
    }
     
    
    backend.run();
   
       // execute the protocol
      
    
     std::vector<bool>gt_result;
     for(int i=0;i<options.num_elements;i++)
     {
       auto temp=output_futures[i].get().at(0).Get(0);
       gt_result.push_back(temp);
     }
  
       
      //delete output_futures;
     //auto bvs=output_futures.get();
     //auto gt_result=bvs.at(0);
     //auto mult_result = bvs.at(0);
    // auto temp=bvs.at(0).Get(0);

  if (!options.json) {

     std::cout << "Result is" << std::endl;
    for(int i=0;i<options.num_elements;i++)
    {
      std::cout<<gt_result[i]<<" ";
    }
     //std::cout<<gt_result<<std::endl;
     std::cout<<std::endl;
  }
  
  
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