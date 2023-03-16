/*
./bin/final_output_provider --my-id 0 --connection-port 1234 --config-input X1 --current-path
path_upto_build_deb..
./bin/final_output_provider --my-id 1 --connection-port 1235 --config-input X1 --current-path
path_upto_build_deb..
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
#include <optional>
#include <regex>
#include <stdexcept>

#include <boost/algorithm/string.hpp>
#include <boost/asio.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/program_options.hpp>

#include "communication/communication_layer.h"
#include "communication/tcp_transport.h"
#include "utility/logger.h"

using namespace boost::asio;
using ip::tcp;
using std::cout;
using std::endl;
using std::string;
namespace po = boost::program_options;

struct Options {
  std::size_t my_id;
  // std::string current-path ;
  // std::string filepath;
  std::string inputfilename;
  int port_number;
  int x;
  std::string fullfilepath;
  std::string currentpath;
};

// struct Shares {
//     int Delta, delta;
// };

std::uint64_t read_file(std::ifstream& indata) {
  std::string str;
  char num;
  while (indata >> std::noskipws >> num) {
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

void read_message(tcp::socket& socket) {
  boost::asio::streambuf buf;
  boost::system::error_code ec;

  boost::asio::read_until(socket, buf, "\n");
  if (ec && ec != boost::asio::error::eof) {
    cout << "Server: receive failed: " << ec.message() << endl;
  } else {
    string data = boost::asio::buffer_cast<const char*>(buf.data());
    std::cout << data << std::endl;
  }
}

std::optional<Options> parse_program_options(int argc, char* argv[]) {
  std::cout << "Parse program options \n";
  Options options;
  boost::program_options::options_description desc("Allowed options");
  // clang-format off
    desc.add_options()
    ("help,h", po::bool_switch()->default_value(false),"produce help message")
    // ("config-file", po::value<std::string>(), "config file containing options")
    // ("config-filename", po::value<std::string>()->required(), "Path of the shares file from build_debwithrelinfo folder")
    ("config-input", po::value<std::string>()->required(), "Path of the input from build_debwithrelinfo folder")
    ("my-id", po::value<std::size_t>()->required(), "my party id")
    ("connection-port", po::value<int>()->required(), "Port number on which to send request for connection")
     ("current-path",po::value<std::string>()->required(), "current path build_debwithrelinfo")
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
  options.port_number = vm["connection-port"].as<int>();
  options.currentpath = vm["current-path"].as<std::string>();
  options.inputfilename = vm["config-input"].as<std::string>();
  if (options.my_id > 1) {
    std::cerr << "my-id must be one of 0 and 1\n";
    return std::nullopt;
  }

  std::string path = options.currentpath;

  options.fullfilepath = path + "/server" + std::to_string(options.my_id) +
                         "/Boolean_Output_Shares/Final_Boolean_Shares_server" +
                         std::to_string(options.my_id) + "_" + options.inputfilename + ".txt";
  std::cout << "path " << options.fullfilepath << "\n";

  if (std::ifstream(options.fullfilepath)) {
    cout << "File found";
  } else
    cout << "File not found";

  return options;
}
struct Shares {
  bool Delta, delta;
};

// sends the shares stored in a data structure to the image provider.
void write_struct(tcp::socket& socket, std::vector<Shares>& data, int num_elements) {
  for (int i = 0; i < num_elements; i++) {
    boost::system::error_code error;
    boost::asio::write(socket, boost::asio::buffer(&data[i], sizeof(data[i])), error);

    if (!error) {
      cout << "successfully sent\n";
    } else {
      cout << "send failed: " << error.message() << endl;
    }
    sleep(1);
  }
}

void write_struct(tcp::socket& socket, Shares* data, int num_elements) {
  boost::system::error_code error;

  for (int i = 0; i < 10; i++) {
    bool arr[2];
    arr[0] = data[i].Delta;
    arr[1] = data[i].delta;
    std::cout << arr[0] << " " << arr[1] << "\n";
    // boost::system::error_code error;
    boost::asio::write(socket, boost::asio::buffer(&arr, sizeof(arr)), error);

    if (!error) {
      continue;
    } else {
      cout << "send of shares failed: " << error.message() << endl;
    }
  }
}
int main(int argc, char* argv[]) {
  std::cout << "inside main\n";
  auto options = parse_program_options(argc, argv);
  // Reading contents from file
  std::ifstream indata;
  indata.open(options->fullfilepath);
  if ((std::ifstream(options->fullfilepath)))
    cout << "File found";
  else
    cout << "File not found";
  Shares shares_data[10];
  // get input data
  // std::vector<float> data;
  int i = 0;
  int number_of_elements;
  std::string line;
  indata >> number_of_elements;
  for (i = 0; i < number_of_elements; i++) {
    indata >> shares_data[i].Delta;
    indata >> shares_data[i].delta;
  }
  for (i = 0; i < number_of_elements; i++) {
    std::cout << shares_data[i].Delta << " " << shares_data[i].delta << "\n";
  }
  cout << "\nStart of send \n";

  boost::asio::io_service io_service;

  // socket creation
  tcp::socket socket(io_service);

  auto port = options->port_number;

  socket.connect(tcp::endpoint(boost::asio::ip::address::from_string("127.0.0.1"), port));

  //////////Send data///////////////////////
  boost::system::error_code error;
  write_struct(socket, shares_data, number_of_elements);

  socket.close();
}
