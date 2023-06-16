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
namespace po = boost::program_options;
namespace fs = std::filesystem;

struct Options {
  std::size_t my_id;
  int listening_port_number;
  std::string currentpath;
};

struct Shares {
    bool Delta, delta;
};

std::vector<Shares> read_struct(tcp::socket& socket, int num_elements) {
  std::vector<Shares> data;
  for (int i = 0; i < num_elements; i++) {
    boost::system::error_code read_error;
    bool arr[2];
    read(socket, boost::asio::buffer(&arr, sizeof(arr)), read_error);
    if(read_error)
      {
        std::cerr<<"Error while receiving share "<<i+1<<"\n";
        throw std::runtime_error("Error while receiving the shares.\n");
      }
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
         ;
  // clang-format on
    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    bool help = vm["help"].as<bool>();
    if (help) {
      std::cerr << desc << "\n";
      return std::nullopt;
    }
    try {
      po::notify(vm);
    } 
    catch (std::exception& e) {
      std::cerr << "Error in the input arguments:" << e.what() << "\n\n";
      std::cerr << desc << "\n";
      return std::nullopt;
    }

  options.my_id = vm["my-id"].as<std::size_t>();
  options.listening_port_number = vm["listening-port"].as<int>();
  options.currentpath = vm["current-path"].as<std::string>();
  // ----------------- Input Validation ----------------------------------------//
  if (options.my_id > 1) {
    std::cerr << "my-id must be 0 or 1\n";
    return std::nullopt;
  }
  // Check if the port numbers are within the valid range (1-65535)
  if ((options.listening_port_number < 1) || (options.listening_port_number > std::numeric_limits<unsigned short>::max())) {
      std::cerr<<"Invalid port "<<options.listening_port_number<<"\n";
      return std::nullopt;  // Out of range
  }
  //Check if currentpath directory exists
  if(!std::filesystem::is_directory(options.currentpath))
    {
      std::cerr<<"Directory ("<<options.currentpath<<") does not exist.\n";
      std::cout<<"Creating folder "<<options.currentpath<<std::endl;
      std::filesystem::create_directories(options.currentpath);
    }
  // ----------------------------------------------------------------------------------//
  return options;
}


int main(int argc, char* argv[]) {
  std::cout<<"Output shares receiver starts.\n";
  auto options = parse_program_options(argc, argv);
  if (!options.has_value()) {
    std::cerr<<"Error while parsing the given input options.\n";
    return EXIT_FAILURE;
  }
  int num_elements = 10; //hardcoded.
  boost::asio::io_service io_service;
  // listen for new connection
  tcp::acceptor acceptor_(io_service, tcp::endpoint(tcp::v4(), options->listening_port_number));
  tcp::socket socket_(io_service);

  // waiting for the connection
  try {
  std::cout<<"Waiting for the final output provider at port "<<options->listening_port_number<<"\n";
  acceptor_.accept(socket_);
  std::cout<<"Accepted a connection at port "<<options->listening_port_number<<"\n";
  }
  catch (const boost::system::system_error& error) {
    std::cout << "Error accepting connection: " << error.what() << std::endl;
    return EXIT_FAILURE;
  }
  boost::system::error_code ec;
  // Read the data in the required format
  std::vector<Shares> data;
  try{  
    data = read_struct(socket_, num_elements);
    }
  catch(std::runtime_error& e){
    std::cerr<<e.what()<<std::endl;
    return EXIT_FAILURE;
    }
  std::cout << "Finished receiving the shares.\n";
  std::string path = options->currentpath;
  std::string shares_file_path = path + "/server" + std::to_string(options->my_id) + "_shares_X";

  // Write to files
  std::ofstream shares_file;
  shares_file.open(shares_file_path, std::ios_base::out);
  if (!shares_file) {
    std::cerr << "Unable to create file "<<shares_file_path<<"\n";
    shares_file.close();
    return EXIT_FAILURE;
  }
  try{
  for(int i=0;i<num_elements;i++) {
    shares_file << data[i].Delta;
    shares_file << " ";
    shares_file << data[i].delta;
    shares_file << "\n";
    }
  }
  catch(std::exception& e){
    std::cerr<<"Error while writing shares to file. Error: "<<e.what()<<std::endl;
  }
  shares_file.close();
//--------------------------------------------------------------------
}
