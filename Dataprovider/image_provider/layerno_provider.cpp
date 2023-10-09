#include <boost/asio.hpp>
#include <boost/program_options.hpp>
#include <cstdint>
#include <iostream>
#include <optional>

using namespace boost::asio;
namespace po = boost::program_options;
using ip::tcp;
using std::cout;
using std::endl;
using std::string;

struct Options {
  std::string cs0_ip;
  int cs0_port;
  std::string cs1_ip;
  int cs1_port;
  int number_of_layer;
};

std::optional<Options> parse_program_options(int argc, char* argv[]) {
  Options options;
  po::options_description desc("Allowed options");
  // clang-format off
  desc.add_options()
    ("help,h", po::bool_switch()->default_value(false),"produce help message")
    ("compute-server0-ip", po::value<std::string>()->default_value("127.0.0.1"), "IP address of compute server 0")
    ("compute-server0-port", po::value<int>()->required(), "Port number of compute server 0")
    ("compute-server1-ip", po::value<std::string>()->default_value("127.0.0.1"), "IP address of compute server 1")
    ("compute-server1-port", po::value<int>()->required(), "Port number of compute server 1")
    ("number_of_layer", po::value<int>()->required(), "Number of layer")
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
  } catch (std::exception& e) {
    std::cerr << "Input parse error:" << e.what() << std::endl;
    return std::nullopt;
  }
  options.cs0_ip = vm["compute-server0-ip"].as<std::string>();
  options.cs0_port = vm["compute-server0-port"].as<int>();
  options.cs1_ip = vm["compute-server1-ip"].as<std::string>();
  options.cs1_port = vm["compute-server1-port"].as<int>();
  options.number_of_layer = vm["number_of_layer"].as<int>();

  return options;
}

int main(int argc, char* argv[]) {
  auto options = parse_program_options(argc, argv);

  if (!options.has_value()) {
    std::cerr << "Error while parsing the given input options.\n";
    return EXIT_FAILURE;
  }

  for (int id = 0; id < 2; id++) {
        boost::asio::io_service io_service;
    tcp::socket socket(io_service);

    auto ip = options->cs0_ip;
    auto port = options->cs0_port;
    if (id == 1) {
      ip = options->cs1_ip;
      port = options->cs1_port;
    }
    // connection
    socket.connect(tcp::endpoint(boost::asio::ip::address::from_string(ip), port));
    // request/message from client
    const int msg = options->number_of_layer;
    boost::system::error_code send_error, read_error;
    boost::asio::write(socket, boost::asio::buffer(&msg, sizeof(msg)), send_error);
    if (!send_error) {
      cout << "Client sent number of layers" << endl;
    } else {
      cout << "Error: Could not send the number of layers:" << send_error.message() << endl;
      socket.close();
      return EXIT_FAILURE;
    }

    // getting a response from the server
    boost::asio::streambuf receive_buffer_init;
    boost::asio::read_until(socket, receive_buffer_init, "\n", read_error);
    if (read_error) {
      if (read_error == boost::asio::error::eof) {
        std::cerr << "Connection closed by layer number receivers. Read error: "
                  << read_error.message() << std::endl;
      } else {
        std::cerr << "Error while receiving acknowledgment from layer number receivers for "
                     "Read error: "
                  << read_error.message() << std::endl;
      }
      socket.close();
      return EXIT_FAILURE;
    }

    const char* data = boost::asio::buffer_cast<const char*>(receive_buffer_init.data());
    std::cout << "Received response: " << data << std::endl;

    socket.close();
  }
  return EXIT_SUCCESS;
}
