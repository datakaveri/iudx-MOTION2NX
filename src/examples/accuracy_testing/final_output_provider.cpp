/*
./bin/final_output_provider --my-id 0 --connection-port 1234 --config-input X1
./bin/final_output_provider --my-id 1 --connection-port 1235 --config-input X1
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
    // std::string currentpath;
    // std::string filepath;
    std::string inputfilename;
    int port_number;
    int x;
};

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
    if (ec && ec != boost::asio::error::eof)
    {
        cout << "Server: receive failed: " << ec.message() << endl;
    }
    else
    {
        string data = boost::asio::buffer_cast<const char *>(buf.data());
        std::cout << data << std::endl;
    }
}

// std::string read_filepath(std::ifstream& indata) {
//     std::string str;

//     char num;
//     while (indata) {
//         std::getline(indata, str);
//     }
//     std::cout << str << std::endl;
//     return str;
// }

// void file_read(Options* options) {
//     std::string path = options->currentpath;
//     // std::string path = std::filesystem::current_path();
//     std::string t1 = path + "/" + options->filepath;

//     std::string t2 =
//         path + "/" + "server" + std::to_string(options->my_id) + "/" + options->inputfilename;

//     std::ifstream file1;
//     file1.open(t1);
//     if (file1) {
//         std::cout << "File found\n";
//     } else {
//         std::cout << "File not found\n";
//     }

//     std::string i = read_filepath(file1);
//     std::cout << "i:" << i << "\n";

//     file1.close();

//     // reading input number for test case

//     file1.open(t2);
//     if (file1) {
//         std::cout << "File found\n";
//     } else {
//         std::cout << "File not found\n";
//     }
//     options->x = read_file(file1);
//     file1.close();
// }

std::optional<Options> parse_program_options(int argc, char* argv[]) {
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
    // ("current-path",po::value<std::string>()->required(), "current path build_debwithrelinfo")
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
    // options.currentpath = vm["current-path"].as<std::string>();
    // options.filepath = vm["config-filename"].as<std::string>();
    options.inputfilename = vm["config-input"].as<std::string>();
    if (options.my_id > 1) {
        std::cerr << "my-id must be one of 0 and 1\n";
        return std::nullopt;
    }

    // file_read(&options);

    // const auto parse_party_argument =
    //   [](const auto& s) -> std::pair<std::size_t, MOTION::Communication::tcp_connection_config> {
    //     const static std::regex party_argument_re("([01]),([^,]+),(\\d{1,5})");
    //     std::smatch match;
    //     if (!std::regex_match(s, match, party_argument_re)) {
    //         throw std::invalid_argument("invalid party argument");
    //     }
    //     auto id = boost::lexical_cast<std::size_t>(match[1]);
    //     auto host = match[2];
    //     auto port = boost::lexical_cast<std::uint16_t>(match[3]);
    //     return {id, {host, port}};
    // };

    // const std::vector<std::string> party_infos = vm["party"].as<std::vector<std::string>>();
  
    return options;
}

void send_message(tcp::socket& socket, const string& message) {
    const string msg = message + "\n";
    boost::asio::write(socket, boost::asio::buffer(msg));
}

// sends the shares stored in a data structure to the image provider.
void write_struct(tcp::socket &socket, std::vector<std::vector<int>> &data, int num_elements)
{
    for (int i = 0; i < num_elements; i++)
    {
        // bool arr[2];

        int Delta, delta;
        Delta = data[i][0];
        delta = data[i][1];

        std::size_t size_Delta = sizeof(Delta), size_delta = sizeof(delta);

        // std::cout << size_Delta << "\t" << size_delta << std::endl;

        // std::string Delta_ = std::to_string(Delta), delta_ = std::to_string(delta);

        //std::cout << arr[0] << " " << arr[1] << "\n"; 

        boost::system::error_code error;
        // boost::asio::write(socket, boost::asio::buffer(&size_Delta, sizeof(size_Delta)), error);
        boost::asio::write(socket, boost::asio::buffer(&Delta, sizeof(Delta)), error);
        // boost::asio::write(socket, boost::asio::buffer(&size_delta, sizeof(size_delta)), error);
        boost::asio::write(socket, boost::asio::buffer(&delta, sizeof(delta)), error);

        if (!error)
        {
            // cout << Delta << " " << delta << " successfully sent\n";
            cout << "successfully sent\n";
        }
        else
        {
            cout << "send failed: " << error.message() << endl;
        }
    }
}

// Send the image provider shares via a tcp connection
void send_provider_shares(int server_num, int port_number, Options& options) {
    auto ip = std::filesystem::current_path();
    if(server_num == 0){
        ip += "/server0/Boolean_Output_Shares/Final_Boolean_Shares_server0_";
        ip += options.inputfilename;
        ip += ".txt";
    }
    else{
        ip += "/server1/Boolean_Output_Shares/Final_Boolean_Shares_server1_";
        ip += options.inputfilename;
        ip += ".txt";
    }

    std::ifstream indata;

    indata.open(ip);

    assert(indata);
    if (!indata) {
        std::cerr << " Error in reading file\n";
        return ;
    }

    //auto server_num_ = read_file(indata);    

    int num_outputs = read_file(indata);

    std::cout << "Sending connection request to the image provider on port " << port_number << std::endl;

    boost::asio::io_context io_context;

    // boost::asio::io_service io_service;

    tcp::resolver resolver(io_context);
    tcp::resolver::results_type endpoints = resolver.resolve(tcp::v4(), "localhost", std::to_string(port_number));

    tcp::socket socket(io_context);

    boost::asio::connect(socket, endpoints);

    //std::cout << "Wating for image provider to accept connection\n"; 

    boost::system::error_code ec;

    read_message(socket);

    std::string server_num_str = std::to_string(server_num) + '\n', num_outputs_str = std::to_string(num_outputs) + '\n';

    boost::asio::write(socket, boost::asio::buffer(&server_num, sizeof(server_num)), ec);
    if (!ec)
    {
        cout << "Server "<< server_num_str << ": Sent successfully" << endl;
    }
    else
    {
        cout << "Server "<< server_num_str << ": send failed: " << ec.message() << endl;
    }
  
    boost::asio::write(socket, boost::asio::buffer(&num_outputs, sizeof(num_outputs)), ec);
    if (!ec)
    {
        cout << "Server "<< server_num_str << ": Sent successfully " << num_outputs_str << endl;
    }
    else
    {
        cout << "Server "<< server_num_str << ": send failed: " << ec.message() << endl;
    }

    std::string message = "Going to start sending boolean shares";
    send_message(socket, message);

    std::vector<std::vector<int>> data(num_outputs);

    for(int i = 0; i < num_outputs; ++i) {
        data[i].push_back(read_file(indata));
        data[i].push_back(read_file(indata));
    }

    indata.close();

    write_struct(socket, data, num_outputs);
  
    read_message(socket);
    
    socket.close();

}

int main(int argc, char* argv[]) {
    auto options = parse_program_options(argc, argv);

    send_provider_shares(options->my_id, options->port_number, *options);
}
