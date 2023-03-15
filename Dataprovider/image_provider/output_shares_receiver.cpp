/*
./bin/output_shares_receiver --my-id 0 --listening-port 1234
./bin/output_shares_receiver --my-id 1 --listening-port 1235
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
};

struct Shares {
    int Delta, delta;
};

std::optional<Options> parse_program_options(int argc, char* argv[]) {
    Options options;
    boost::program_options::options_description desc("Allowed options");
  // clang-format off
    desc.add_options()
    ("help,h", po::bool_switch()->default_value(false),"produce help message")
    ("my-id", po::value<std::size_t>()->required(), "my party id")
    ("listening-port", po::value<int>()->required(), "Port number on which to listen for connection")
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
    options.listening_port_number = vm["listening-port"].as<int>();

    if (options.my_id > 1) {
        std::cerr << "my-id must be one of 0 and 1\n";
        return std::nullopt;
    }

    return options;
}

void send_message(tcp::socket& socket, const string& message) {
    const string msg = message + "\n";
    boost::asio::write(socket, boost::asio::buffer(msg));
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
        std::cout << "Server: " << data << std::endl;
    }
}

void read_write_to_file(tcp::socket& socket, std::ofstream &outdata, int num_elements) {

    cout << "Before reading of data\n";

    // std::vector<Shares> shares(num_elements);

    Shares shares[num_elements];

    boost::system::error_code ec;
    read(socket, boost::asio::buffer(&shares, sizeof(shares)), ec);
    if (ec) {
        cout << ec << "\n";
    } else {
        // cout << "No Error\t" << arr[0] << " " << arr[1] << std::endl;
        cout << "No Error\n"; 
    }

    // cout << sizeof(shares) << "\n";

    // for (int i = 0; i < num_elements; i++) {
    //     cout << i << ":\t";
    //     boost::system::error_code ec;
             
    //     // int Delta, delta;

    //     read(socket, boost::asio::buffer(&shares[i], sizeof(shares[i])), ec);
    //     if (ec) {
    //         cout << ec << "\n";
    //     } else {
    //         // cout << "No Error\t" << arr[0] << " " << arr[1] << std::endl;
    //         cout << "No Error\n"; 
    //     }

    //     // read(socket, boost::asio::buffer(&delta, sizeof(delta)), ec);
    //     // if (ec) {
    //     //     cout << ec << "\n";
    //     // } else {
    //     //     // cout << "No Error\t" << arr[0] << " " << arr[1] << std::endl;
    //     //     cout << "No Error" << std::endl; 
    //     // }
    // }

    for(int i = 0; i < num_elements; ++i) {
        // std::cout << shares[i].Delta << " " << shares[i].delta << "\n";
        outdata << shares[i].Delta << " " << shares[i].delta << "\n";
    }
}

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

// receving shares from the compute server via a tcp socket connection.
int receive_shares(int port_number){

    std::cout << "Server listening on port " << port_number << std::endl;

    boost::asio::io_context io_context;

    // boost::asio::io_service io_service;

    tcp::acceptor acceptor(io_context, tcp::endpoint(tcp::v4(), port_number));

    tcp::socket socket(io_context);

    std::cout << "Waiting to get request from client\n";
    acceptor.accept(socket);

    boost::system::error_code ec;

    std::string message = "The image provider is ready to receive the message\n";
    boost::asio::write(socket, boost::asio::buffer(message), ec);

    int server_num;
    boost::asio::read(socket, boost::asio::buffer(&server_num, sizeof(server_num)), ec);
    if (ec) {
        cout << ec << "\n";
    } else {
        cout << "No Error. The server is connected to client " << server_num << std::endl;
    }

    // std::cout << "The image provider has connected to server " << server_num << std::endl;

    int num_outputs;
    boost::asio::read(socket, boost::asio::buffer(&num_outputs, sizeof(num_outputs)), ec);
    if (ec) {
        cout << ec << "\n";
    } else {
        cout << "No Error. The number of outputs are " << num_outputs << std::endl;
    }

    int img_num;;
    boost::asio::read(socket, boost::asio::buffer(&img_num, sizeof(img_num)), ec);
    if (ec) {
        cout << ec << "\n";
    } else {
        cout << "No Error." << std::endl;
    }

    read_message(socket);

    std::ofstream outdata;

    //std::cout << op << std::endl;

    std::string op = getenv("BASE_DIR");

    // auto op = std::filesystem::current_path();

    // auto op = base_dir + "/Dataprovider/image_provider/Final_Output_Shares";
    op += "/Dataprovider/image_provider/Final_Output_Shares";

    if(!fs::is_directory(op)){
        std::filesystem::create_directories(op);
    }
    
    if(server_num == 0){
        op += "/server0_shares_X";
        op += std::to_string(img_num);
        op += ".txt";
    }
    else{
        op += "/server1_shares_X";
        op += std::to_string(img_num);
        op += ".txt";
    }

    outdata.open(op);

    cout << op << std::endl;

    outdata << num_outputs << "\n";

    read_write_to_file(socket, outdata, num_outputs);


    message = "The image provider has received the shares from server " + std::to_string(server_num);
    send_message(socket, message);

    outdata.close();    

    // socket.close();

    return img_num;

}
// reconstructs the final boolean answer based on the final output shares
void reconstruct(int img_num) {
    std::ifstream indata0, indata1;
    std::string base_dir = getenv("BASE_DIR");

    auto dir = base_dir + "/Dataprovider/image_provider/Final_Output_Shares/";

    auto ip0 = dir + "server0_shares_X" + std::to_string(img_num) + ".txt";
    auto ip1 = dir + "server1_shares_X" + std::to_string(img_num) + ".txt";

    auto answer_dir = base_dir + "/Dataprovider/image_provider/answer";

    if(!fs::is_directory(answer_dir)){
        std::filesystem::create_directories(answer_dir);
    }

    answer_dir += "/reconstructed_answer_img" + std::to_string(img_num) + ".txt";

    std::ofstream outdata;

    outdata.open(answer_dir);

    // cout << ip0 << "\n" << ip1 << "\n";
     
    indata0.open(ip0);
    indata1.open(ip1);

    int num_outputs;
    num_outputs = read_file(indata0);;
    num_outputs = read_file(indata1);
     
    //  cout << num_outputs << "\n";

    int answer;

    for(int i = 0; i < num_outputs; ++i){
        // cout << i << std::endl;
        int Delta, delta0, delta1;

        Delta = read_file(indata0);
        delta0 = read_file(indata0);

        Delta = read_file(indata1);
        delta1 = read_file(indata1);

        //   cout << Delta << "\t" << delta0 << "\t" << delta1 << "\n";

        int reconstructed_answer = Delta ^ delta0 ^ delta1;

        std::cout << reconstructed_answer << " ";
        outdata << reconstructed_answer << " ";

        if(reconstructed_answer == 0){
            answer = i;
        }

    }

    std::cout << "\n" << answer << "\n";;
    outdata << "\n" << answer;

    return;
}



int main(int argc, char* argv[]) {
    auto options = parse_program_options(argc, argv);

    int img_num = receive_shares(options->listening_port_number);
    cout << "Answer Reconstruction\n";
    sleep(5);
    reconstruct(img_num);

}
