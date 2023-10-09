#include <boost/archive/text_oarchive.hpp>
#include <boost/asio.hpp>
#include <boost/chrono.hpp>
#include <boost/program_options.hpp>
#include <boost/serialization/string.hpp>
#include <boost/thread.hpp>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <random>
#include <stdexcept>
#include <type_traits>
#include "./fixed-point.h"

#define MAX_CONNECT_RETRIES 50
using namespace boost::asio;
namespace po = boost::program_options;
using ip::tcp;

struct Options {
  std::string cs0_ip;
  int cs0_port;
  std::string cs1_ip;
  int cs1_port;
  std::size_t fractional_bits;
  int index;
  std::string NameofImageFile;
  std::string fullfilepath;
};

struct Shares {
  uint64_t Delta, delta;
};

bool is_valid_IP(const std::string& ip) {
  ip::address ipAddress;
  try {
    ipAddress = boost::asio::ip::make_address(ip);
  } catch (const boost::system::system_error&) {
    return false;  // Failed to create boost::asio::ip::address
  }
  return ipAddress.is_v4() || ipAddress.is_v6();
}

bool establishConnection(ip::tcp::socket& socket, std::string& host, int& port) {
  for (int retry = 0; retry < MAX_CONNECT_RETRIES; ++retry) {
    try {
      socket.connect(tcp::endpoint(ip::address::from_string(host), port));
      std::cout << "Connected to " << host << ":" << port << std::endl;
      return true;  // Connection successful
    } catch (const boost::system::system_error& e) {
      std::cout << "Connection attempt " << retry + 1 << " failed: " << e.what() << std::endl;
      boost::this_thread::sleep_for(boost::chrono::milliseconds(500));
    }
  }
  std::cout << "Failed to establish connection after " << MAX_CONNECT_RETRIES << " retries."
            << std::endl;
  return false;  // Connection unsuccessful
}

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
    ("index", po::value<int>()->required(), "Index of image file")
    ("fractional-bits", po::value<size_t>()->required(), "Number of fractional bits")
    ("filepath", po::value<std::string>()->required(), "Name of the image file for which shares should be created")
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
  options.index = vm["index"].as<int>();
  options.NameofImageFile = "X" + std::to_string(options.index);
  options.fullfilepath = vm["filepath"].as<std::string>();
  options.fractional_bits = vm["fractional-bits"].as<size_t>();
  options.fullfilepath += "/images/" + options.NameofImageFile + ".csv";
  std::cout << "Image Path: " << options.fullfilepath << "\n";
  // --------------------------------- Input Validation ---------------------------------------//
  if (std::ifstream(options.fullfilepath)) {
    std::cout << "Image file found.";
  } else {
    std::cout << "No image file found at " << options.fullfilepath << std::endl;
    return std::nullopt;
  }
  // Check whether IP addresses are valid
  if ((!is_valid_IP(options.cs0_ip)) || (!is_valid_IP(options.cs1_ip))) {
    std::cerr << "Invalid IP address." << std::endl;
    return std::nullopt;
  }

  // Check if the port numbers are within the valid range (1-65535)
  if ((options.cs0_port < 1 || options.cs0_port > std::numeric_limits<unsigned short>::max()) ||
      (options.cs1_port < 1 || options.cs1_port > std::numeric_limits<unsigned short>::max())) {
    std::cerr << "Invalid port.\n";
    return std::nullopt;  // Out of range
  }
  //--------------------------------------------------------------------------------------------//
  return options;
}

template <typename E>
std::uint64_t RandomNumDistribution(E& engine) {
  std::uniform_int_distribution<unsigned long long> distribution(
      std::numeric_limits<std::uint64_t>::min(), std::numeric_limits<std::uint64_t>::max());
  return distribution(engine);
}

int share_generation(std::ifstream& image_data, int num_elements, Shares* cs0_data,
                     Shares* cs1_data, size_t fractional_bits) {
  // get input data
  std::vector<float> data;
  std::string line;
  while (std::getline(image_data, line)) {
    float temp;
    std::istringstream iss(line);
    if (iss >> temp) {
      data.push_back(temp);
    }
  }
  auto data_size = data.size();
  if (data_size != (num_elements)) {
    std::cerr << "Image file should have " << num_elements << " values. It has " << data_size
              << " values." << std::endl;
    return EXIT_FAILURE;
  }

  std::cout << "Generating image shares. \n";
  // Now that we have the data, need to generate the shares
  try {
    for (int i = 0; i < data_size; i++) {
      std::random_device rd;
      std::mt19937 gen(rd());
      std::uint64_t del0 = RandomNumDistribution(gen), del1 = RandomNumDistribution(gen);
      std::uint64_t Del =
          del0 + del1 + MOTION::new_fixed_point::encode<uint64_t, float>(data[i], fractional_bits);

      // For each data, creating 2 shares variables - 1 for computer server 0  and another for
      // compute server 1
      Shares cs0, cs1;

      cs0.Delta = Del;
      cs0.delta = del0;

      cs1.Delta = Del;
      cs1.delta = del1;

      cs0_data[i] = cs0;
      cs1_data[i] = cs1;
      std::cout << "Data = " << data[i] << " Delta = " << cs0.Delta << " delta0 = " << cs0.delta
                << " delta1 = " << cs1.delta << "\n";
    }
  } catch (std::exception& e) {
    std::cerr << "Error during image share generation: " << e.what() << std::endl;
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}

void write_struct(tcp::socket& socket, Shares* data, int num_elements) {
  boost::system::error_code error;
  for (int i = 0; i < num_elements; i++) {
    uint64_t arr[2];
    arr[0] = data[i].Delta;
    arr[1] = data[i].delta;
    // std::cout << arr[0] << " " << arr[1] << "\n";
    boost::asio::write(socket, boost::asio::buffer(&arr, sizeof(arr)), error);
    if (error) {
      std::cerr << "Unable to send share " << i + 1 << ".\nError: " << error.message() << std::endl;
    }
  }
}

int main(int argc, char* argv[]) {
  auto options = parse_program_options(argc, argv);

  if (!options.has_value()) {
    std::cerr << "Error while parsing the given input options.\n";
    return EXIT_FAILURE;
  }
  int rows = 784, columns = 1;  // hardcoded
  int num_elements = rows * columns;
  Shares cs0_data[num_elements];
  Shares cs1_data[num_elements];
  // Reading contents from image file
  std::ifstream image_file;
  try {
    image_file.open(options->fullfilepath);
    if (!image_file) {
      std::cerr << "Unable to open the image file.\n";
      throw std::ifstream::failure("Error opening the image file.");
    }
    std::cout << "Image: " << options->NameofImageFile << "\n";
    int is_error =
        share_generation(image_file, num_elements, cs0_data, cs1_data, options->fractional_bits);
    if (is_error) {
      image_file.close();
      return EXIT_FAILURE;
    }
    image_file.close();
  } catch (const std::ifstream::failure& e) {
    std::cerr << "Error in image share generation: " << e.what() << std::endl;
    image_file.close();
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
    if (establishConnection(socket, ip, port)) {
      std::cout << "Connection established successfully\n";
    } else {
      std::cerr << "Connection could not established with the image share receiver in server " << id
                << std::endl;
      socket.close();
      return EXIT_FAILURE;
    }

    // ------------------First send the number of fractional bits to the server
    // ------------------------
    std::cout << "\nStart sending to compute server " << id << "\n";
    boost::system::error_code send_error, read_error;
    boost::asio::write(
        socket, boost::asio::buffer(&options->fractional_bits, sizeof(options->fractional_bits)),
        send_error);
    if (send_error) {
      std::cerr << "Error: Could not send the number of fractional bits: " << send_error.message()
                << std::endl;
      socket.close();
      return EXIT_FAILURE;
    }

    // getting a response from the server
    boost::asio::streambuf receive_buffer_init;
    boost::asio::read_until(socket, receive_buffer_init, "\n", read_error);
    if (read_error) {
      if (read_error == boost::asio::error::eof) {
        std::cerr << "Connection closed by image share receivers. Read error: "
                  << read_error.message() << std::endl;
      } else {
        std::cerr << "Error while receiving acknowledgment from image share receivers for "
                     "fractional bits message. Read error: "
                  << read_error.message() << std::endl;
      }
      socket.close();
      return EXIT_FAILURE;
    }
    const char* data = boost::asio::buffer_cast<const char*>(receive_buffer_init.data());
    std::cout << "Received response: " << data << std::endl;
    //-----------------------------Send rows and columns to compute
    // server-------------------------------------------------
    std::cout << "Image size: Rows=" << rows << " Columns=" << columns << std::endl;
    int arr[2] = {rows, columns};
    boost::asio::write(socket, boost::asio::buffer(&arr, sizeof(arr)), send_error);
    if (send_error) {
      std::cerr << "Unable to send image size (rows,columns). Error: " << send_error.message()
                << std::endl;
      socket.close();
      return EXIT_FAILURE;
    }

    // getting a response from the server
    boost::asio::streambuf receive_buffer;
    boost::asio::read_until(socket, receive_buffer, "\n", read_error);
    if (read_error) {
      if (read_error == boost::asio::error::eof) {
        std::cerr << "Connection closed by image share receivers. Read error: "
                  << read_error.message() << std::endl;
      } else {
        std::cerr << "Error while receiving acknowledgment from image share receivers for the "
                     "image size message. Read error: "
                  << read_error.message() << std::endl;
      }
      socket.close();
      return EXIT_FAILURE;
    }
    data = boost::asio::buffer_cast<const char*>(receive_buffer.data());
    std::cout << "Received response: " << data << std::endl;
    // --------------------------------- Sending shares to
    // compute_server----------------------------------------------------
    try {
      auto data1 = cs0_data;
      if (id == 1) {
        data1 = cs1_data;
      }
      write_struct(socket, data1, num_elements);
      socket.close();
      std::cout << "Finished sending the shares to server " << id << "\n";

    } catch (std::exception& e) {
      std::cerr << "Error while sending image shares to server " << id << ". Error: " << e.what()
                << std::endl;
      socket.close();
      return EXIT_FAILURE;
    }
  }
  return EXIT_SUCCESS;
}