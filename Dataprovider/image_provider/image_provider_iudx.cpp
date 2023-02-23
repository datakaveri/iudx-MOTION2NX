#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <random>
#include <stdexcept>
#include <type_traits>

#include <boost/archive/text_oarchive.hpp>
#include <boost/asio.hpp>
#include <boost/program_options.hpp>
#include <boost/serialization/string.hpp>

#include "./fixed-point.h"

using namespace boost::asio;
namespace po = boost::program_options;
using ip::tcp;
using std::cout;
using std::endl;
using std::string;

struct Options {
  int cs0_port;
  int cs1_port;
  std::size_t fractional_bits;
  std::string NameofImageFile;
  std::string fullfilepath;
};

std::optional<Options> parse_program_options(int argc, char* argv[]) {
  Options options;
  boost::program_options::options_description desc("Allowed options");
  // clang-format off
  desc.add_options()
    ("help,h", po::bool_switch()->default_value(false),"produce help message")
    ("compute-server0-port", po::value<int>()->required(), "Port number of compute server 0")
    ("compute-server1-port", po::value<int>()->required(), "Port number of compute server 1")
    ("NameofImageFile", po::value<string>()->required(), "Name of the image file for which shares should be created")
    ("fractional-bits", po::value<size_t>()->required(), "Number of fractional bits")
    ("dp-id", po::value<int>()->default_value(0), "Id of the data provider, not needed in this version")
    ("filepath", po::value<string>()->required(), "Name of the image file for which shares should be created")

    ;
  // clang-format on

  po::variables_map vm;
  po::store(po::parse_command_line(argc, argv, desc), vm);
  bool help = vm["help"].as<bool>();
  if (help) {
    std::cerr << desc << "\n";
    return std::nullopt;
  }

  options.cs0_port = vm["compute-server0-port"].as<int>();
  options.cs1_port = vm["compute-server1-port"].as<int>();
  options.NameofImageFile = vm["NameofImageFile"].as<std::string>();
  options.fullfilepath = vm["filepath"].as<std::string>();

  options.fractional_bits = vm["fractional-bits"].as<size_t>();

  if (vm["dp-id"].as<int>() == 0) {
    // string name = "/X4_n.csv";
    options.fullfilepath += "/images/" + options.NameofImageFile + ".csv";
    std::cout << "path " << options.fullfilepath << "\n";
  } else if (vm["dp-id"].as<int>() == 1) {
    options.fullfilepath += "/images/" + options.NameofImageFile + ".csv";
    std::cout << "path " << options.fullfilepath << "\n";
  } else {
    std::cerr << "Invalid data provider ID\n";
    return std::nullopt;
  }
  if (std::ifstream(options.fullfilepath)) {
    cout << "File found";
  } else
    cout << "File not found";
  return options;
}

template <typename E>
std::uint64_t blah(E& engine) {
  std::uniform_int_distribution<unsigned long long> dis(std::numeric_limits<std::uint64_t>::min(),
                                                        std::numeric_limits<std::uint64_t>::max());
  return dis(engine);
}

struct Shares {
  uint64_t Delta, delta;
};

void share_generation(std::ifstream& indata, int num_elements, Shares* cs0_data, Shares* cs1_data,
                      size_t fractional_bits) {
  // get input data
  std::vector<float> data;

  int count = 0;

  float temp;
  while (indata >> temp) {
    std::cout << temp << ",";
    data.push_back(temp);
    count++;
    if (count == num_elements) {
      break;
    }
  }

  while (count < num_elements) {
    data.push_back(0.0);
  }

  for (int i = 0; i < data.size(); i++) {
    cout << data[i] << " ";
  }
  cout << "\n";

  // Now that we have data, need to generate the shares
  for (int i = 0; i < data.size(); i++) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uint64_t del0 = blah(gen);
    std::uint64_t del1 = blah(gen);

    std::uint64_t Del =
        del0 + del1 + MOTION::new_fixed_point::encode<uint64_t, float>(data[i], fractional_bits);

    // For each data, creating 2 shares variables - 1 for CS0 and another for CS1
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

  return;
}

int share_generation_csv(std::ifstream& indata, int num_elements, Shares* cs0_data,
                         Shares* cs1_data, size_t fractional_bits) {
  // get input data
  std::vector<float> data;
  int i = 0;
  std::string line;

  while(indata)
  { float temp;
    indata>>temp;
    data.push_back(temp);
  }
  // while (std::getline(indata, line)) {
  //   std::stringstream lineStream(line);
  //   std::string cell;

  //   while (std::getline(lineStream, cell, ',')) {
  //     data.push_back(stof(cell));
  //     std::cout << i << " = " << data[i] << "\n";
  //     i++;
  //   }
  // }
  int actual_ans = data[0];
  std::cout << "\n";
  cout << "Share generation \n";
  // Now that we have data, need to generate the shares
  for (int i = 1; i < 785; i++) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uint64_t del0 = blah(gen);
    std::uint64_t del1 = blah(gen);

    std::uint64_t Del =
        del0 + del1 + MOTION::new_fixed_point::encode<uint64_t, float>(data[i], fractional_bits);

    // For each data, creating 2 shares variables - 1 for CS0 and another for CS1
    Shares cs0, cs1;

    cs0.Delta = Del;
    cs0.delta = del0;

    cs1.Delta = Del;
    cs1.delta = del1;

    cs0_data[i - 1] = cs0;
    cs1_data[i - 1] = cs1;

    cout << "Data = " << data[i] << " Delta = " << cs0.Delta << " delta0 = " << cs0.delta
         << " delta1 = " << cs1.delta << "\n";
  }

  return actual_ans;
}

// void write_struct(tcp::socket &socket, Shares *data, int num_elements)
// {
//      for (int i = 0; i < num_elements; i++)
//      {
//           uint64_t arr[2];
//           arr[0] = data[i].Delta;
//           arr[1] = data[i].delta;

//           boost::system::error_code error;
//           boost::asio::write(socket, boost::asio::buffer(&arr, sizeof(arr)), error);

//           if (!error)
//           {
//                continue;
//           }
//           else
//           {
//                cout << "send failed: " << error.message() << endl;
//           }
//      }
// }

void write_struct(tcp::socket& socket, Shares* data, int num_elements) {
  boost::system::error_code error;

  for (int i = 0; i < 784; i++) {
    uint64_t arr[2];
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
  auto options = parse_program_options(argc, argv);

  if (!options.has_value()) {
    return EXIT_FAILURE;
  }

  // Reading contents from file
  std::ifstream indata;
  indata.open(options->fullfilepath);
  if ((std::ifstream(options->fullfilepath)))
    cout << "File found";
  else
    cout << "File not found";
  int rows = 784, columns = 1;
  int actual_answer;

  int num_elements = rows * columns;

  cout << num_elements << "\n";
  cout << options->NameofImageFile << "\n";

  Shares cs0_data[num_elements];
  Shares cs1_data[num_elements];

  actual_answer =
      share_generation_csv(indata, num_elements, cs0_data, cs1_data, options->fractional_bits);

  cout << "Actual answer:" << actual_answer << "\n";
  indata.close();
  int i = 0;
  for (i = 0; i < 2; i++) {
    cout << "\nStart of send to compute server\n";

    boost::asio::io_service io_service;

    // socket creation
    tcp::socket socket(io_service);

    auto port = options->cs0_port;
    // if(i) {
    //      port = options->cs1_port;
    // }
    if (i) {
      port = options->cs1_port;
      // socket.connect( tcp::endpoint( boost::asio::ip::address::from_string("192.168.1.118"), port
      // )); socket.connect(tcp::endpoint(boost::asio::ip::address::from_string("192.168.1.137"),
      // port));
    }
    // else
    // {
    //      // socket.connect(tcp::endpoint(boost::asio::ip::address::from_string("192.168.1.118"),
    //      port));
    // }

    // connection
    socket.connect(tcp::endpoint(boost::asio::ip::address::from_string("127.0.0.1"), port));

    //////////Send actual answer first///////////////////////
    boost::system::error_code error;
    boost::asio::write(socket, boost::asio::buffer(&actual_answer, sizeof(actual_answer)), error);

    if (!error) {
      cout << "sent actual answer: " << endl;
      cout << "Actual answer3:" << actual_answer << "\n";
    } else {
      cout << "send of actual answer failed: " << error.message() << endl;
    }

    // getting a response from the server
    boost::asio::streambuf receive_buffer_actualanswer;
    boost::asio::read_until(socket, receive_buffer_actualanswer, "\n");
    if (error && error != boost::asio::error::eof) {
      cout << "receive failed: " << error.message() << endl;
    } else {
      const char* data = boost::asio::buffer_cast<const char*>(receive_buffer_actualanswer.data());
      cout << data << endl;
    }

    // First send the number of fractional bits to the server
    boost::system::error_code error_init;
    boost::asio::write(
        socket, boost::asio::buffer(&options->fractional_bits, sizeof(options->fractional_bits)),
        error_init);
    if (!error_init) {
      cout << "Not an error" << endl;
    } else {
      cout << "send of fractiona bits failed: " << error_init.message() << endl;
    }

    // getting a re1ponse from the server
    boost::asio::streambuf receive_buffer_init;
    boost::asio::read_until(socket, receive_buffer_init, "\n");
    if (error_init && error_init != boost::asio::error::eof) {
      cout << "receive failed: " << error_init.message() << endl;
    } else {
      const char* data = boost::asio::buffer_cast<const char*>(receive_buffer_init.data());
      cout << data << endl;
    }

    // Send rows and columns to compute server
    std::cout << "in image provider Rows:" << rows << " Columns:" << columns;
    int arr[2] = {rows, columns};
    // boost::system::error_code error;
    boost::asio::write(socket, boost::asio::buffer(&arr, sizeof(arr)), error);
    if (!error) {
      cout << "Not an error" << endl;
    } else {
      cout << "send of rows and columns failed: " << error.message() << endl;
    }

    // getting a response from the server
    boost::asio::streambuf receive_buffer;
    boost::asio::read_until(socket, receive_buffer, "\n");
    if (error && error != boost::asio::error::eof) {
      cout << "receive of rows and columns failed: " << error.message() << endl;
    } else {
      const char* data = boost::asio::buffer_cast<const char*>(receive_buffer.data());
      cout << data << endl;
    }

    // Sending CS0 data to compute_server0
    auto data1 = cs0_data;
    if (i) {
      data1 = cs1_data;
    }
    cout << "Actual answer2:" << actual_answer << "\n";
    write_struct(socket, data1, num_elements);

    socket.close();
  }
  return 0;
}
