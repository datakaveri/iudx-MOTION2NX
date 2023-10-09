//  ./bin/my_share_generator --compute-server0-ip 127.0.0.1 --compute-server0-port 1234 --compute-server1-ip 127.0.0.1 --compute-server1-port 1235 --dp-id 0 --fractional-bits 13 --filepath ${BASE_DIR}/Dataprovider/weights_provider

#include <boost/archive/text_oarchive.hpp>
#include <boost/asio.hpp>
#include <boost/program_options.hpp>
#include <boost/serialization/string.hpp>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <random>
#include <stdexcept>
#include <type_traits>

#include "./fixed-point.h"

using namespace boost::asio;
namespace po = boost::program_options;
using ip::tcp;
using std::cin;
using std::cout;
using std::endl;
using std::string;

struct Options {
  std::string cs0_ip;
  int cs0_port;
  std::string cs1_ip;
  int cs1_port;
  std::size_t fractional_bits;
  std::filesystem::path filename;
  std::string fullfilepath;
};

std::optional<Options> parse_program_options(int argc, char* argv[]) {
  Options options;
  boost::program_options::options_description desc("Allowed options");
  // clang-format off
  desc.add_options()
    ("help,h", po::bool_switch()->default_value(false),"produce help message")
    ("compute-server0-ip", po::value<std::string>()->default_value("127.0.0.1"), "IP address of compute server 0")
    ("compute-server0-port", po::value<int>()->default_value(1234), "Port number of compute server 0")
    ("compute-server1-ip", po::value<std::string>()->default_value("127.0.0.1"), "IP address of compute server 1")
    ("compute-server1-port", po::value<int>()->default_value(1235), "Port number of compute server 1")
    ("fractional-bits", po::value<size_t>()->default_value(13), "Number of fractional bits")
    ("dp-id", po::value<int>()->default_value(0), "Id of the data provider")
    ("filepath", po::value<string>()->default_value("/home/iudx/iudx-MOTION2NX/data/ModelProvider"), "Name of the image file for which shares should be created")
    ;
//  desc.add_options()
//     ("help,h", po::bool_switch()->default_value(false),"produce help message")
//     ("compute-server0-ip", po::value<std::string>()->default_value("127.0.0.1"), "IP address of compute server 0")
//     ("compute-server0-port", po::value<int>()->default_value(1234), "Port number of compute server 0")
//     ("compute-server1-ip", po::value<std::string>()->default_value("127.0.0.1"), "IP address of compute server 1")
//     ("compute-server1-port", po::value<int>()->default_value(1235), "Port number of compute server 1")
//     ("fractional-bits", po::value<size_t>()->default_value(13), "Number of fractional bits")
//     ("dp-id", po::value<int>()->default_value(0), "Id of the data provider")
//     ("filepath", po::value<string>()->default_value("temp"), "Name of the image file for which shares should be created")
//     ;
  // clang-format on

  po::variables_map vm;
  po::store(po::parse_command_line(argc, argv, desc), vm);
  bool help = vm["help"].as<bool>();
  if (help) {
    std::cerr << desc << "\n";
    return std::nullopt;
  }

  options.cs0_ip = vm["compute-server0-ip"].as<std::string>();
  options.cs0_port = vm["compute-server0-port"].as<int>();

  options.cs1_ip = vm["compute-server1-ip"].as<std::string>();
  options.cs1_port = vm["compute-server1-port"].as<int>();

  options.fractional_bits = vm["fractional-bits"].as<size_t>();
  options.fullfilepath = vm["filepath"].as<std::string>();

  options.filename = std::filesystem::current_path();

  if (vm["dp-id"].as<int>() == 0) {
    options.filename += "/input_dp0.txt";
  } else if (vm["dp-id"].as<int>() == 1) {
    options.filename += "/input_dp1.txt";
  } else {
    std::cerr << "Invalid data provider ID\n";
    return std::nullopt;
  }

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
        del0 + del1 +
        MOTION::new_fixed_point::encode<uint64_t, long double>(data[i], fractional_bits);

    // For each data, creating 2 shares variables - 1 for CS0 and another for CS1
    Shares cs0, cs1;

    cs0.Delta = Del;
    cs0.delta = del0;

    cs1.Delta = Del;
    cs1.delta = del1;

    cs0_data[i] = cs0;
    cs1_data[i] = cs1;

    // std::cout << "Data = " << data[i] << " Delta = " << cs0.Delta << " delta0 = " << cs0.delta <<
    // " delta1 = " << cs1.delta << "\n";
  }

  return;
}

void write_struct(tcp::socket& socket, Shares* data, int num_elements) {
  for (int i = 0; i < num_elements; i++) {
    uint64_t arr[2];
    arr[0] = data[i].Delta;
    arr[1] = data[i].delta;

    boost::system::error_code error;
    boost::asio::write(socket, boost::asio::buffer(&arr, sizeof(arr)), error);

    if (!error) {
      continue;
    } else {
      cout << "send failed: " << error.message() << endl;
    }
  }
}

class Matrix {
 private:
  uint64_t rows, columns, num_elements, fractional_bits;
  std::string fullFileName;
  std::vector<float> data;
  struct Shares *cs0_data, *cs1_data;

 public:
  Matrix(int row, int col, std::string fileName, std::size_t fraction_bits, const Options& options) {
    rows = row;
    columns = col;
    num_elements = row * col;
    fractional_bits = fraction_bits;

    fullFileName = fileName;
    std::cout << fullFileName << std::endl;
    cs0_data = (struct Shares*)malloc(sizeof(struct Shares) * num_elements);
    cs1_data = (struct Shares*)malloc(sizeof(struct Shares) * num_elements);
  }

  void readMatrix() {
    std::cout << "Reading the matrix from file :- " << fullFileName << std::endl;

    int count = 0;
    float temp;

    std::ifstream indata;
    indata.open(fullFileName);

    while (indata >> temp) {
      data.push_back(temp);
      count++;
      if (count == num_elements) {
        break;
      }
    }

    // CHANGE FOR FRACTIONAL BITS
    while (count < num_elements) {
      data.push_back(0.0);
    }
  }

  void readMatrixCSV() {
    std::cout << "Reading the matrix from file :- " << fullFileName << std::endl;

    std::ifstream indata;
    indata.open(fullFileName);

    std::string line;
    while (std::getline(indata, line)) {
      std::stringstream lineStream(line);
      std::string cell;

      while (std::getline(lineStream, cell, ',')) {
        data.push_back(stold(cell));
      }
    }
    // for(int i=0; i<data.size(); ++i)
    // {
    //     std::cout << i+1 << " : " << data[i] << " , ";

    // }
    std::cout << "Reading the matrix from file complted " << fullFileName << std::endl;
  }
  /// write the weight to a file W1share0,
  void writeSharesToFile(std::string t, std::string shares_file_name, const Options& options)
  {
    std::cout << "write " << endl;
    
    std::string t0 = "0";
    std::string t1 = "1";
    std::string s0 = t + t0 + shares_file_name; 
    std::string s1 = t + t1 + shares_file_name;
    std::cout << "output shares file path" << std::endl;
    std::cout << s0 << std::endl;
    std::cout << s1 << std::endl;
    std::ofstream file0;
    std::ofstream file1;
    file0.open(s0, std::ios_base::out);
    file1.open(s1, std::ios_base::out);
    for (int i = 0; i < num_elements; i++)
    {
        file0 << cs0_data[i].Delta << " " << cs0_data[i].delta << "\n";
        file1 << cs1_data[i].Delta << " " << cs1_data[i].delta << "\n";
    }
    file0.close();
    file1.close();
  }
  void printData() {
    std::cout << "Printing data\n";

    for (int i = 0; i < data.size(); i++) {
      std::cout << data[i] << " ";
    }

    std::cout << "\n";
  }

  void generateShares() {
    // Now that we have data, need to generate the shares
    std :: cout << "generate shares started " << endl;
   

    for (int i = 0; i < data.size(); i++) {
      std::random_device rd;
      std::mt19937 gen(rd());
      std::uint64_t del0 = blah(gen);
      std::uint64_t del1 = blah(gen);
      // auto t=encode<uint64_t, float>(data[i], fractional_bits);
      // std::cout<<"Encoded value:"<<t<<std::endl;
      // auto p1=decode<uint64_t, float>(t, fractional_bits);
      // std::cout<<"Decoded value:"<<p1<<std::endl;
      std::uint64_t Del =
          del0 + del1 +
          MOTION::new_fixed_point::encode<uint64_t, long double>(data[i], fractional_bits);
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

  void sendToServers(const Options& options) {
    std::cout << "Sending to servers\n";

    for (int i = 0; i < 2; i++) {
      // std::cin.ignore();
      sleep(10);
      cout << "\nStart of send to compute server\n";

      boost::asio::io_service io_service;

      // socket creation
      tcp::socket socket(io_service);

      auto port = options.cs0_port;
      auto ip = options.cs0_ip;
      if (i) {
        port = options.cs1_port;
        ip = options.cs1_ip;
      }

      // connection
      socket.connect(tcp::endpoint(boost::asio::ip::address::from_string(ip), port));

      // First send the number of fractional bits to the server
      boost::system::error_code error_init;
      boost::asio::write(socket, boost::asio::buffer(&fractional_bits, sizeof(fractional_bits)),
                         error_init);
      if (!error_init) {
        cout << "Not an error" << endl;
      } else {
        cout << "send failed: " << error_init.message() << endl;
      }

      // getting a response from the server
      boost::asio::streambuf receive_buffer_init;
      boost::asio::read_until(socket, receive_buffer_init, "\n");
      if (error_init && error_init != boost::asio::error::eof) {
        cout << "receive failed: " << error_init.message() << endl;
      } else {
        const char* data = boost::asio::buffer_cast<const char*>(receive_buffer_init.data());
        cout << data << endl;
      }

      // Send rows and columns to compute server
      int arr[2] = {rows, columns};
      boost::system::error_code error;
      boost::asio::write(socket, boost::asio::buffer(&arr, sizeof(arr)), error);
      if (!error) {
        cout << "Not an error" << endl;
      } else {
        cout << "send failed: " << error.message() << endl;
      }

      // getting a response from the server
      boost::asio::streambuf receive_buffer;
      boost::asio::read_until(socket, receive_buffer, "\n");
      if (error && error != boost::asio::error::eof) {
        cout << "receive failed: " << error.message() << endl;
      } else {
        const char* data = boost::asio::buffer_cast<const char*>(receive_buffer.data());
        cout << data << endl;
      }

      // Sending CS0 data to compute_server0
      auto data = cs0_data;
      if (i) {
        data = cs1_data;
      }
      write_struct(socket, data, num_elements);

      socket.close();
    }
  }
};

void send_confirmation(const Options& options) {

  int validation_bit = 1;

  // for (int i = 0; i < 2; i++) {
      
  // }
  // std::cin.ignore();
      sleep(10);
      cout << "\nStart of send to compute server\n";

      boost::asio::io_service io_service;

      // socket creation
      tcp::socket socket(io_service);

      auto port = options.cs0_port;
      auto ip = options.cs0_ip;
      //if (i) {
      //  port = options.cs1_port;
      //  ip = options.cs1_ip;
      // }

      // connection
      socket.connect(tcp::endpoint(boost::asio::ip::address::from_string(ip), port));
      boost::system::error_code error_init;
      boost::asio::write(socket, boost::asio::buffer(&validation_bit, sizeof(validation_bit)),
                         error_init);

      socket.close();
}

int main(int argc, char* argv[]) {
  auto options = parse_program_options(argc, argv);
  int row, col;
  if (!options.has_value()) {
    return EXIT_FAILURE;
  }
  std::string p1 = options->fullfilepath + "/newW1.csv";
  // dimensions should be 512*784
  std::cout << "Please Enter rows and columns for CNN-1 weight matrix" << std::endl;
  std::cin >> row >> col;
  Matrix weightL1 = Matrix(row, col, p1, options->fractional_bits, *options);
  weightL1.readMatrixCSV();
  //weightL1.printData();
  weightL1.generateShares();
  weightL1.writeSharesToFile("/home/iudx/iudx-MOTION2NX/build_debwithrelinfo_gcc/server", "/CW1", *options);
  weightL1.writeSharesToFile("/home/iudx/motion2nx-LATEST/iudx-MOTION2NX/build_debwithrelinfo_gcc/server", "/CW1", *options);
  

  p1 = options->fullfilepath + "/newB1.csv";
  std::cout<< "Please Enter rows and columns for CNN bias-1 matrix" << std::endl;
  std::cin >> row >> col;
  Matrix biasL1 = Matrix(row, col, p1, options->fractional_bits, *options);
  biasL1.readMatrixCSV();
  biasL1.generateShares();
  biasL1.writeSharesToFile("/home/iudx/iudx-MOTION2NX/build_debwithrelinfo_gcc/server", "/CB1", *options);
  biasL1.writeSharesToFile("/home/iudx/motion2nx-LATEST/iudx-MOTION2NX/build_debwithrelinfo_gcc/server", "/CB1", *options);

  // p1 = options->fullfilepath + "/newW2.csv";
  // // dimensions should be 512*784
  // std::cout << "Please Enter rows and columns for CNN-2 Weight matrix" << std::endl;
  // std::cin >> row >> col;
  // Matrix weightW2 = Matrix(row, col, p1, options->fractional_bits, *options);
  // weightW2.readMatrixCSV();
  // //weightL1.printData();
  // weightW2.generateShares();
  // weightW2.writeSharesToFile("/home/iudx/iudx-MOTION2NX/build_debwithrelinfo_gcc/server", "/CW2", *options);
  
  // p1 = options->fullfilepath + "/newB2.csv";
  // std::cout<< "Please Enter rows and columns for NN layer-1 Bias matrix" << std::endl;
  // std::cin >> row >> col;
  // Matrix biasB2 = Matrix(row, col, p1, options->fractional_bits, *options);
  // biasB2.readMatrixCSV();
  // biasB2.generateShares();
  // biasB2.writeSharesToFile("/home/iudx/iudx-MOTION2NX/build_debwithrelinfo_gcc/server", "/CB2", *options);
 

p1 = options->fullfilepath + "/X0.csv";
  std::cout<< "Please Enter rows and columns for Image matrix" << std::endl;
  std::cin >> row >> col;
  Matrix im_X0 = Matrix(row, col, p1, options->fractional_bits, *options);
  im_X0.readMatrixCSV();
  im_X0.generateShares();
  im_X0.writeSharesToFile("/home/iudx/iudx-MOTION2NX/build_debwithrelinfo_gcc/server", "/Image_shares/X0", *options);
  im_X0.writeSharesToFile("/home/iudx/motion2nx-LATEST/iudx-MOTION2NX/build_debwithrelinfo_gcc/server", "/Image_shares/X0", *options);

  return 0;
}
