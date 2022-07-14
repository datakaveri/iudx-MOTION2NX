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
  std::size_t num_repetitions;
  std::size_t num_simd;
  bool sync_between_setup_and_online;
  MOTION::MPCProtocol arithmetic_protocol;
  MOTION::MPCProtocol boolean_protocol;
  Matrix image;
  Matrix weights[3];
  Matrix biases[3];  
  std::size_t fractional_bits;
  std::size_t my_id;
  MOTION::Communication::tcp_parties_config tcp_config;
  bool no_run = false;
};

void retrieve_shares(int port_number,Options* options) {
    auto pair1 = COMPUTE_SERVER::get_provider_mat_mul_data(port_number);
    std::vector<COMPUTE_SERVER::Shares>input_values_dp0 = pair1.second.first;
    for(int i=0;i<input_values_dp0.size();i++) {
      options->image.Delta.push_back(input_values_dp0[i].Delta);
      options->image.delta.push_back(input_values_dp0[i].delta);

    }
    options->image.row = pair1.second.second[0];
    options->image.col = pair1.second.second[1];
    options->fractional_bits = pair1.first;

    int i = 0;
    for(i=0;i<6;i++) {

      auto pair2 = COMPUTE_SERVER::get_provider_mat_mul_data(port_number);
      std::vector<COMPUTE_SERVER::Shares>input_values_dp1 = pair2.second.first;

      if(i%2 == 0) {
        for(int j=0;j<input_values_dp1.size();j++) {
          options->weights[i/2].Delta.push_back(input_values_dp1[j].Delta);
          options->weights[i/2].delta.push_back(input_values_dp1[j].delta);
        }
        options->weights[i/2].row = pair2.second.second[0];
        options->weights[i/2].col = pair2.second.second[1];  

      } else {
        for(int j=0;j<input_values_dp1.size();j++) {
          options->biases[i/2].Delta.push_back(input_values_dp1[j].Delta);
          options->biases[i/2].delta.push_back(input_values_dp1[j].delta);
        }
        options->biases[i/2].row = pair2.second.second[0];
        options->biases[i/2].col = pair2.second.second[1];  
      }
    }
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
    ("fractional-bits", po::value<std::size_t>()->default_value(16),
     "number of fractional bits for fixed-point arithmetic")
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

  if(options.my_id == 0) {
    retrieve_shares(1234,&options);
  }
  else {    
    retrieve_shares(1235,&options);
  }  

  if(options.weights[0].col != options.image.row) {
    std::cerr << "Invalid inputs, number of columns for dp0 must be equal to number of rows for dp1 \n";
    return std::nullopt;
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

auto create_composite_circuit(const Options& options, MOTION::TwoPartyTensorBackend& backend) {
  // retrieve the gate factories for the chosen protocols
  auto& arithmetic_tof = backend.get_tensor_op_factory(options.arithmetic_protocol);  
  auto& boolean_tof = backend.get_tensor_op_factory(MOTION::MPCProtocol::Yao);


  const MOTION::tensor::GemmOp gemm_op1 = {
      .input_A_shape_ = {options.weights[0].row, options.weights[0].col}, 
      .input_B_shape_ = {options.image.row, options.image.col}, 
      .output_shape_ = {options.weights[0].row, options.image.col}
      };
  
  // const MOTION::tensor::GemmOp gemm_op2 = {
  //     .input_A_shape_ = {options.weights[1].row, options.weights[1].col}, 
  //     .input_B_shape_ = {options.weights[0].row, options.image.col}, 
  //     .output_shape_ = {options.weights[1].row, options.image.col}
  //     };
  
  // const MOTION::tensor::GemmOp gemm_op3 = {
  //     .input_A_shape_ = {options.weights[2].row, options.weights[2].col}, 
  //     .input_B_shape_ = {options.weights[1].row, options.image.col}, 
  //     .output_shape_ = {options.weights[2].row, options.image.col}
  //     };

  // const MOTION::tensor::JoinOp join_op = {
  //     .input_A_shape_ = {1, 1}, 
  //     .input_B_shape_ = {1, 1}, 
  //     .output_shape_ = {1, 2}
  // };

  // const MOTION::tensor::GTOp gt_op = {
  //   .input_shape_ = {1,1,2},
  //   .output_shape_ = {1,1,1},
  //   .kernel_shape_ = {1,2},
  //   .strides_ = {1,1}
  // }; 

  const MOTION::tensor::MaxPoolOp maxpool_op = {
    .input_shape_ = {1,options.weights[0].row,options.image.col},
    .output_shape_ = {1,1,1},
    .kernel_shape_ = {options.weights[0].row,options.image.col},
    .strides_ = {1,1}
  }; 


  const auto W1_dims = gemm_op1.get_input_A_tensor_dims();
  const auto X_dims = gemm_op1.get_input_B_tensor_dims();
  const auto B1_dims = gemm_op1.get_output_tensor_dims();

  // const auto W2_dims = gemm_op2.get_input_A_tensor_dims();
  // const auto B2_dims = gemm_op2.get_output_tensor_dims();

  // const auto W3_dims = gemm_op3.get_input_A_tensor_dims();
  // const auto B3_dims = gemm_op3.get_output_tensor_dims();

  // share the inputs using the arithmetic protocol
  // NB: the inputs need to always be specified in the same order:
  // here we first specify the input of party 0, then that of party 1

  MOTION::tensor::TensorCP tensor_W1,tensor_B1, tensor_X, tensor_W2, tensor_B2, tensor_W3, tensor_B3;
  MOTION::tensor::TensorCP gemm_output1, gemm_output2, gemm_output3; 
  MOTION::tensor::TensorCP maxpool_output;
  MOTION::tensor::TensorCP add_output1, add_output2, add_output3;

  std::function<MOTION::tensor::TensorCP(const MOTION::tensor::TensorCP&)> make_maxPool;
  make_maxPool = [&](const auto& input) {
    const auto boolean_tensor = boolean_tof.make_tensor_conversion(MOTION::MPCProtocol::Yao, input);
    const auto maxPool_tensor = boolean_tof.make_tensor_maxpool_op(maxpool_op,boolean_tensor);
    return boolean_tof.make_tensor_conversion(options.arithmetic_protocol, maxPool_tensor);
  };

  // std::function<MOTION::tensor::TensorCP(const MOTION::tensor::TensorCP&,const MOTION::tensor::TensorCP&)> make_gt;  
  // make_gt = [&](const auto& input_a,const auto& input_b) {
  //   const auto join_tensor = arithmetic_tof.make_tensor_join_op(join_op,input_a,input_b);
  //   const auto boolean_join_tensor = boolean_tof.make_tensor_conversion(MOTION::MPCProtocol::Yao, join_tensor);
  //   const auto gt_tensor = boolean_tof.make_tensor_gt_op(maxpool_op,boolean_join_tensor);
  //   return boolean_tof.make_tensor_conversion(options.arithmetic_protocol, gt_tensor);
  // };

  auto pairX = arithmetic_tof.make_arithmetic_64_tensor_input_shares(X_dims);
  std::vector<ENCRYPTO::ReusableFiberPromise<MOTION::IntegerValues<uint64_t>>> input_promises_X = std::move(pairX.first);
  tensor_X = pairX.second;

  auto pairW1 = arithmetic_tof.make_arithmetic_64_tensor_input_shares(W1_dims);
  std::vector<ENCRYPTO::ReusableFiberPromise<MOTION::IntegerValues<uint64_t>>> input_promises_W1 = std::move(pairW1.first);
  tensor_W1 = pairW1.second;
  
  auto pairB1 = arithmetic_tof.make_arithmetic_64_tensor_input_shares(B1_dims);
  std::vector<ENCRYPTO::ReusableFiberPromise<MOTION::IntegerValues<uint64_t>>> input_promises_B1 = std::move(pairB1.first);
  tensor_B1 = pairB1.second;

  // auto pairW2 = arithmetic_tof.make_arithmetic_64_tensor_input_shares(W2_dims);
  // std::vector<ENCRYPTO::ReusableFiberPromise<MOTION::IntegerValues<uint64_t>>> input_promises_W2 = std::move(pairW2.first);
  // tensor_W2 = pairW2.second;
  
  // auto pairB2 = arithmetic_tof.make_arithmetic_64_tensor_input_shares(B2_dims);
  // std::vector<ENCRYPTO::ReusableFiberPromise<MOTION::IntegerValues<uint64_t>>> input_promises_B2 = std::move(pairB2.first);
  // tensor_B2 = pairB2.second;

  // auto pairW3 = arithmetic_tof.make_arithmetic_64_tensor_input_shares(W3_dims);
  // std::vector<ENCRYPTO::ReusableFiberPromise<MOTION::IntegerValues<uint64_t>>> input_promises_W3 = std::move(pairW3.first);
  // tensor_W3 = pairW3.second;
  
  // auto pairB3 = arithmetic_tof.make_arithmetic_64_tensor_input_shares(B3_dims);
  // std::vector<ENCRYPTO::ReusableFiberPromise<MOTION::IntegerValues<uint64_t>>> input_promises_B3 = std::move(pairB3.first);
  // tensor_B3 = pairB3.second;

  input_promises_X[0].set_value(options.image.Delta);
  input_promises_X[1].set_value(options.image.delta);
  
  input_promises_W1[0].set_value(options.weights[0].Delta);
  input_promises_W1[1].set_value(options.weights[0].delta);

  input_promises_B1[0].set_value(options.biases[0].Delta);
  input_promises_B1[1].set_value(options.biases[0].delta);

  // input_promises_W2[0].set_value(options.weights[1].Delta);
  // input_promises_W2[1].set_value(options.weights[1].delta);

  // input_promises_B2[0].set_value(options.biases[1].Delta);
  // input_promises_B2[1].set_value(options.biases[1].delta);

  // input_promises_W3[0].set_value(options.weights[2].Delta);
  // input_promises_W3[1].set_value(options.weights[2].delta);

  // input_promises_B3[0].set_value(options.biases[2].Delta);
  // input_promises_B3[1].set_value(options.biases[2].delta);

  

  gemm_output1 = arithmetic_tof.make_tensor_gemm_op(gemm_op1, tensor_W1, tensor_X,options.fractional_bits);
  add_output1 = arithmetic_tof.make_tensor_add_op(gemm_output1,tensor_B1);

  // gemm_output2 = arithmetic_tof.make_tensor_gemm_op(gemm_op2, tensor_W2, add_output1,options.fractional_bits);
  // add_output2 = arithmetic_tof.make_tensor_add_op(gemm_output2,tensor_B2);

  // gemm_output3 = arithmetic_tof.make_tensor_gemm_op(gemm_op3, tensor_W3, add_output2,options.fractional_bits);
  // add_output3 = arithmetic_tof.make_tensor_add_op(gemm_output3,tensor_B3);

  maxpool_output = make_maxPool(add_output1);
  
  // // TESTING TENSOR SPLITTING
  // const auto split_input_tensors = arithmetic_tof.make_tensor_split_op(tensor_a);

  // TESTING Join + GT OPERATION
  // std::vector< MOTION::tensor::TensorCP > output_fin_arr;

  // for(int i=0;i<10;i++) {
  //   auto output_fin = make_gt(maxpool_output,split_input_tensors[i]);
  //   output_fin_arr.push_back(output_fin);
  // }

  // auto output_fin = make_gt(split_input_tensors[0],maxpool_output);
  
  ENCRYPTO::ReusableFiberFuture<std::vector<std::uint64_t>> output_future;
  if (options.my_id == 0) {
    arithmetic_tof.make_arithmetic_tensor_output_other(add_output1);
  } else {
    output_future = arithmetic_tof.make_arithmetic_64_tensor_output_my(add_output1);
  }

  return output_future;
}

void run_composite_circuit(const Options& options, MOTION::TwoPartyTensorBackend& backend){
  auto output_future = create_composite_circuit(options, backend);
  backend.run();
  if (options.my_id == 1) {
    auto interm = output_future.get();
    std::cout << "The result is:\n[";
    for(int i=0; i<interm.size(); ++i)
    {
      float temp = MOTION::fixed_point::decode<uint64_t,float>(interm[i],options.fractional_bits);
      std::cout << temp << " , ";
    }
    std::cout << "]" << std::endl;
  }
}

int main(int argc, char* argv[]) {
  auto options = parse_program_options(argc, argv);
  if (!options.has_value()) {
    return EXIT_FAILURE;
  }

  try {
    auto comm_layer = setup_communication(*options);
    auto logger = std::make_shared<MOTION::Logger>(options->my_id,
                                                   boost::log::trivial::severity_level::trace);
    comm_layer->set_logger(logger);
    MOTION::TwoPartyTensorBackend backend(*comm_layer, options->threads,
                                      options->sync_between_setup_and_online, logger);
    run_composite_circuit(*options, backend);
    comm_layer->shutdown();
  } catch (std::runtime_error& e) {
    std::cerr << "ERROR OCCURRED: " << e.what() << "\n";
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
