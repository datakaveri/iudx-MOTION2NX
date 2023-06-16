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
#include <boost/chrono.hpp>
#include <boost/thread.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/program_options.hpp>

#include "utility/logger.h"

#define MAX_CONNECT_RETRIES 50
// #include "communication/communication_layer.h"
// #include "communication/tcp_transport.h"

using namespace boost::asio;
using ip::tcp;
namespace po = boost::program_options;

struct Options {
  std::size_t my_id;
  std::string inputfilename;
  int port_number;
  int x;
  std::string fullfilepath;
  std::string currentpath;
  std::string ip;
};

bool is_valid_IP(const std::string& ip) {
  ip::address ipAddress;
  try 
  {
      ipAddress = boost::asio::ip::make_address(ip);
  } 
  catch (const boost::system::system_error&) {
      return false;  // Failed to create boost::asio::ip::address
  }
  return ipAddress.is_v4() || ipAddress.is_v6();
}

bool establishConnection(ip::tcp::socket& socket, std::string& host, int& port)
{
    for (int retry = 0; retry < MAX_CONNECT_RETRIES; ++retry)
    {
        try
        {
            socket.connect(tcp::endpoint(ip::address::from_string(host), port));
            std::cout << "Connected to " << host << ":" << port << std::endl;
            return true; // Connection successful
        }
        catch (const boost::system::system_error& e)
        {
            std::cout << "Connection attempt " << retry + 1 << " failed: " << e.what() << std::endl;
            boost::this_thread::sleep_for(boost::chrono::milliseconds(500));   
        }     
    }
    std::cout << "Failed to establish connection after " << MAX_CONNECT_RETRIES << " retries." << std::endl;
    return false; // Connection unsuccessful
}


// std::uint64_t read_file(std::ifstream& indata) {
//   std::string str;
//   char num;
//   while (indata >> std::noskipws >> num) {
//     if (num != ' ' && num != '\n') {
//       str.push_back(num);
//     } else {
//       break;
//     }
//   }
//   std::string::size_type sz = 0;
//   std::uint64_t ret = (uint64_t)std::stoull(str, &sz, 0);
//   return ret;
// }

// void read_message(tcp::socket& socket) {
//   boost::asio::streambuf buf;
//   boost::system::error_code ec;

//   boost::asio::read_until(socket, buf, "\n");
//   if (ec && ec != boost::asio::error::eof) {
//     std::cout << "Server: receive failed: " << ec.message() << std::endl;
//   } else {
//     std::string data = boost::asio::buffer_cast<const char*>(buf.data());
//     std::cout << data << std::endl;
//   }
// }

std::optional<Options> parse_program_options(int argc, char* argv[]) {
  std::cout << "Parse program options \n";
  Options options;
  boost::program_options::options_description desc("Allowed options");
  // clang-format off
    desc.add_options()
    ("help,h", po::bool_switch()->default_value(false),"produce help message")
    ("config-input", po::value<std::string>()->required(), "Path of the input from the build folder")
    ("my-id", po::value<std::size_t>()->required(), "my party id")
    ("connection-port", po::value<int>()->required(), "Port number on which to send request for connection")
    ("current-path",po::value<std::string>()->required(), "current path build_debwithrelinfo")
    ("connection-ip",po::value<std::string>()->default_value("127.0.0.1"), "current path build_debwithrelinfo")

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
    std::cerr << "Input parse error:" << e.what() << std::endl;
    return std::nullopt;
  }

  options.my_id = vm["my-id"].as<std::size_t>();
  options.port_number = vm["connection-port"].as<int>();
  options.currentpath = vm["current-path"].as<std::string>();
  // options.filepath = vm["config-filename"].as<std::string>();
  options.inputfilename = vm["config-input"].as<std::string>();
  options.ip = vm["connection-ip"].as<std::string>();
  if (options.my_id > 1) {
    std::cerr << "my-id must be 0 or 1\n";
    return std::nullopt;
  }

  std::string path = options.currentpath;

  options.fullfilepath = path + "/server" + std::to_string(options.my_id) +
                         "/Boolean_Output_Shares/Final_Boolean_Shares_server" +
                         std::to_string(options.my_id) + "_" + options.inputfilename + ".txt";
  std::cout << "Output shares path: " << options.fullfilepath << "\n";
  //----------------------------------- Input Validation -----------------------------------//
  if (std::ifstream(options.fullfilepath)) {
    std::cout << "Found the final output shares file.";
  } 
  else{
      std::cout << "Final output shares file not found at "<<options.fullfilepath<<std::endl;
      return std::nullopt;
  }
  // Check whether IP addresses are valid
  if (!is_valid_IP(options.ip)) {
    std::cerr << "Invalid IP address." << std::endl;
    return std::nullopt;
  } 
  
  // Check if the port numbers are within the valid range (1-65535)
  if ((options.port_number < 1) || (options.port_number > std::numeric_limits<unsigned short>::max())) {
      return std::nullopt;  // Out of range
  }
  //--------------------------------------------------------------------------------------------//
  return options;
}
struct Shares {
  bool Delta, delta;
};

// sends the shares stored in a data structure to the image provider.
void write_struct(tcp::socket& socket, std::vector<Shares>& data, int num_elements) {
  boost::system::error_code error;
  for (int i = 0; i < num_elements; i++) 
  {  
    boost::asio::write(socket, boost::asio::buffer(&data[i], sizeof(data[i])), error);
    if (error) 
      {
      std::cerr << "Unable to send share "<< i+1<<".\nError: " << error.message() << std::endl;
      }
    // sleep(1); //Commented on 19/5/23 by Rashmi
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
      std::cout << "send of shares failed: " << error.message() << std::endl;
    }
  }
}

int main(int argc, char* argv[]) {
  auto options = parse_program_options(argc, argv);
  if (!options.has_value()) {
    return EXIT_FAILURE;
  }
  // Reading contents from file
  Shares shares_data[10]; //hardcoded
  int i, number_of_elements;
  std::ifstream output_shares_file;
  
  try{
    output_shares_file.open(options->fullfilepath);
    if (!output_shares_file)
      {
        std::cerr<<"Unable to open the output shares file.\n";
        throw std::ifstream::failure("Error opening the output shares file.");  
      }
    
    std::string line;
    output_shares_file >> number_of_elements;
    for (i = 0; i < number_of_elements; i++) {
      output_shares_file >> shares_data[i].Delta;
      output_shares_file >> shares_data[i].delta;
      }
    }
  catch(const std::ifstream::failure& e){
    std::cerr << e.what() << std::endl;
    output_shares_file.close();
    return EXIT_FAILURE;  
    }
  output_shares_file.close();

  // for (i = 0; i < number_of_elements; i++) {
  //   std::cout << shares_data[i].Delta << " " << shares_data[i].delta << "\n";
  // }
  
  boost::asio::io_service io_service;
  tcp::socket socket(io_service);
  
  if(establishConnection(socket,options->ip,options->port_number))
      {
        std::cout<<"Connection established successfully\n";
      }
    else
      {
        std::cerr<<"Connection could not be established with the receiver"<<std::endl;
        socket.close();
        return EXIT_FAILURE;
      }
  
  // socket.connect(tcp::endpoint(boost::asio::ip::address::from_string(options->ip), options->port_number));

  //----------------Send data---------------------------//
  std::cout<<"Sending the final output shares.\n";
  try{
    write_struct(socket, shares_data, number_of_elements);
  }
  catch(std::exception& e)
      {
        std::cerr<<"Error while sending final output shares: "<<e.what()<<std::endl;
        socket.close();
        return EXIT_FAILURE;
      }
  socket.close();
  return EXIT_SUCCESS;
}