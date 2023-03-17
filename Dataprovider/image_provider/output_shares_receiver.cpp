/*
./bin/output_shares_receiver --my-id 0 --listening-port 1234 --index i --current-path path_upto_build_deb..
./bin/output_shares_receiver --my-id 1 --listening-port 1235 --index i --current-path path_upto_build_deb..
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
#include <regex>
#include <stdexcept>
#include <optional>

#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/program_options.hpp>
#include <boost/asio.hpp>

using namespace boost::asio;
using ip::tcp;
using std::cout;
using std::endl;
using std::string;
namespace po = boost::program_options;
namespace fs = std::filesystem;

struct Options {
    std::size_t my_id;
    int listening_port_number;
    std::string currentpath  ;
    std::string index  ;
};

struct Shares {
    bool Delta, delta;
};

std::vector<Shares> read_struct(tcp::socket& socket, int num_elements) {
  cout << "Before reading of data\n";
  std::vector<Shares> data;
  for (int i = 0; i < num_elements; i++) {
    boost::system::error_code ec;
    bool arr[2];
    read(socket, boost::asio::buffer(&arr, sizeof(arr)), ec);
    Shares temp;
    temp.Delta = arr[0];
    std::cout << temp.Delta << " ";
    temp.delta = arr[1];
    std::cout << temp.delta << "\n";
    data.push_back(temp);
  }

  socket.close();
  return data;
}
std::optional<Options> parse_program_options(int argc, char* argv[]) {
    Options options;
    boost::program_options::options_description desc("Allowed options");
  // clang-format off
    desc.add_options()
    ("help,h", po::bool_switch()->default_value(false),"produce help message")
    ("my-id", po::value<std::size_t>()->required(), "my party id")
    ("listening-port", po::value<int>()->required(), "Port number on which to listen for connection")
     ("current-path",po::value<std::string>()->required(), "current path build_debwithrelinfo")
      ("index",po::value<std::string>()->required(), "index of image")
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
    options.listening_port_number = vm["listening-port"].as<int>();
    options.currentpath  = vm["current-path"].as<std::string>();
    options.index  = vm["index"].as<std::string>();
    if (options.my_id > 1) {
        std::cerr << "my-id must be one of 0 and 1\n";
        return std::nullopt;
    }

    return options;
}



int main(int argc, char* argv[]) {
    auto options = parse_program_options(argc, argv);
// //////////////////////////////////////////////////////
  int num_elements = 10;
  boost::asio::io_service io_service;
  cout << "Inside new function \n";
  // listen for new connection
  tcp::acceptor acceptor_(io_service, tcp::endpoint(tcp::v4(), options->listening_port_number));

  // socket creation
  tcp::socket socket_(io_service);

  // waiting for the connection
  acceptor_.accept(socket_);

  //int actualanswer;
  boost::system::error_code ec;
  // Read the data in the reuired format
  std::vector<Shares> data = read_struct(socket_, num_elements);

  std::cout << "Finished reading input \n\n";
  //////////////////////////////////////////
std::string path = options->currentpath  ;
     
      std::string t1 = path +"/server"+std::to_string(options->my_id)+"_shares_X"+options->index;
      

      ///// Write to files
      std::ofstream file1;
      file1.open(t1, std::ios_base::out);
      for(int i=0;i<10;i++)
      {
      file1 << data[i].Delta;
      file1 << " ";
      file1 << data[i].delta;
      file1 << "\n";
}
      file1.close();

/////////////////////////////////////////////////////////
  
}

