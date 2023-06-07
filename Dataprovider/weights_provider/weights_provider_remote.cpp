#include <boost/archive/text_oarchive.hpp>
#include <boost/asio.hpp>
#include <boost/program_options.hpp>
#include <boost/serialization/string.hpp>
#include <boost/chrono.hpp>
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
using namespace std::chrono;
using namespace boost::asio;
namespace po = boost::program_options;
using ip::tcp;

struct Options {
  std::string cs0_ip;
  int cs0_port;
  std::string cs1_ip;
  int cs1_port;
  std::size_t fractional_bits;
  std::filesystem::path filename;
  std::string fullfilepath;
};

struct Shares {
  uint64_t Delta, delta;
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


std::optional<Options> parse_program_options(int argc, char* argv[]) {
  Options options;
  boost::program_options::options_description desc("Allowed options");
  // clang-format off
  desc.add_options()
    ("help,h", po::bool_switch()->default_value(false),"produce help message")
    ("compute-server0-ip", po::value<std::string>()->default_value("127.0.0.1"), "IP address of compute server 0")
    ("compute-server0-port", po::value<int>()->required(), "Port number of compute server 0")
    ("compute-server1-ip", po::value<std::string>()->default_value("127.0.0.1"), "IP address of compute server 1")
    ("compute-server1-port", po::value<int>()->required(), "Port number of compute server 1")
    ("fractional-bits", po::value<size_t>()->required(), "Number of fractional bits")
    ("filepath", po::value<std::string>()->required(), "Path to the folder containing weight and bias files for which shares should be created")
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

  options.cs0_ip = vm["compute-server0-ip"].as<std::string>();
  options.cs0_port = vm["compute-server0-port"].as<int>();

  options.cs1_ip = vm["compute-server1-ip"].as<std::string>();
  options.cs1_port = vm["compute-server1-port"].as<int>();

  options.fractional_bits = vm["fractional-bits"].as<size_t>();
  options.fullfilepath = vm["filepath"].as<std::string>();

  // options.filename = std::filesystem::current_path();

  // if (vm["dp-id"].as<int>() == 0) {
  //   options.filename += "/input_dp0.txt";
  // } else if (vm["dp-id"].as<int>() == 1) {
  //   options.filename += "/input_dp1.txt";
  // } else {
  //   std::cerr << "Invalid data provider ID\n";
  //   return std::nullopt;
  // }
  // --------------------------------- Input Validation ---------------------------------------//
  // Check whether IP addresses are valid
  if ((!is_valid_IP(options.cs0_ip)) || (!is_valid_IP(options.cs1_ip))) {
    std::cerr << "Invalid IP address." << std::endl;
    return std::nullopt;
  } 
  
  // Check if the port numbers are within the valid range (1-65535)
  if ((options.cs0_port < 1 || options.cs0_port > std::numeric_limits<unsigned short>::max())
  ||(options.cs1_port < 1 || options.cs1_port > std::numeric_limits<unsigned short>::max())) {
      std::cerr<<"Invalid port.\n";
      return std::nullopt;  // Out of range
  }
  //--------------------------------------------------------------------------------------------//
  return options;
}

template <typename E>
std::uint64_t RandomNumDistribution(E& engine) {
  std::uniform_int_distribution<unsigned long long> distribution(std::numeric_limits<std::uint64_t>::min(),  std::numeric_limits<std::uint64_t>::max());
  return distribution(engine);
}


int share_generation(std::ifstream& indata, int num_elements, Shares* cs0_data, Shares* cs1_data,
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

  // while (count < num_elements) {
  //   data.push_back(0.0);
  // }

  // for (int i = 0; i < data.size(); i++) {
  //   std::cout << data[i] << " ";
  // }
  // std::cout << "\n";

  // Now that we have data, need to generate the shares
  std::cout << "Generating model shares. \n";
  try{
    for (int i = 0; i < data.size(); i++) {
      std::random_device rd;
      std::mt19937 gen(rd());
      std::uint64_t del0 = RandomNumDistribution(gen);
      std::uint64_t del1 = RandomNumDistribution(gen);

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

      }
    }
  catch(std::exception& e){
      std::cerr<<"Error during model share generation: "<<e.what()<<std::endl;
      return EXIT_FAILURE;
    }
  return EXIT_SUCCESS;
}
///////////////////////////////////////////////////////////////
void write_struct_vector(tcp::socket& socket, Shares* data, int num_elements) {
  uint64_t arr[2 * num_elements];
  int j = 0;
  for (int i = 0; i < num_elements; i++) {
    // uint64_t arr[2];
    arr[j] = data[i].Delta;
    j = j + 1;
    arr[j] = data[i].delta;
    j = j + 1;
  }
  boost::system::error_code error;
  std::cout << "Start sending shares operation " << std::endl;
  boost::asio::write(socket, boost::asio::buffer(&arr, sizeof(arr)), error);
  if (error) {
      std::cerr << "Unable to send shares. Error: "<<error.message() << std::endl;
      }
}

///////////////////////////////////////////////////////////////////
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
      std::cout << "send failed: " << error.message() << std::endl;
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
  Matrix(int row, int col, std::string fileName, std::size_t fraction_bits,
         const Options& options) {
    rows = row;
    columns = col;
    num_elements = row * col;
    fractional_bits = fraction_bits;

    fullFileName = fileName;
    std::cout << fullFileName << std::endl;
    cs0_data = (struct Shares*)malloc(sizeof(struct Shares) * num_elements);
    cs1_data = (struct Shares*)malloc(sizeof(struct Shares) * num_elements);
  }
  /////////Function to access rows and columns of a Matrix, Ramya May 3,2023
  std::vector<int> getRowsColumns() {
    std::vector<int> size;
    size.push_back(rows);
    size.push_back(columns);
    return size;
  }

  int getNumberofElements() { return num_elements; }
  std::string getFileName() { return fullFileName; }
  /////////Function to aceess data from the Matrix, Ramya May 3,2023
  Shares* getData(int a) {
    if (a == 0) {
      return cs0_data;
    } else {
      return cs1_data;
    }
  }

  void readMatrix() {
    auto start = high_resolution_clock::now();

    std::cout << "Reading the matrix from file : " << fullFileName << std::endl;

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
      auto stop = high_resolution_clock::now();
      auto duration = duration_cast<milliseconds>(stop - start);
      // To get the value of duration use the count()
      // member function on the duration object
      std::cout << "Duration for reading from the file:" << duration.count() << std::endl;
    }

    // CHANGE FOR FRACTIONAL BITS
    while (count < num_elements) {
      data.push_back(0.0);
    }
  }

  void readMatrixCSV() {
    auto start = high_resolution_clock::now();

    std::cout << "Reading the matrix from file : " << fullFileName << std::endl;

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
    auto stop = high_resolution_clock::now();
    auto duration = duration_cast<milliseconds>(stop - start);
    std::cout << "Duration for reading from the file:" << duration.count() << "msec" << std::endl;
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
    for (int i = 0; i < data.size(); i++) {
      std::random_device rd;
      std::mt19937 gen(rd());
      std::uint64_t del0 = RandomNumDistribution(gen);
      std::uint64_t del1 = RandomNumDistribution(gen);
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

      // std::cout << "Data = " << data[i] << " Delta = " << cs0.Delta << " delta0 = " << cs0.delta
      // << " delta1 = " << cs1.delta << "\n";
    }

    return;
  }
};

void send_confirmation(const Options& options) {
  int validation_bit = 1;

  // for (int i = 0; i < 2; i++) {

  // }
  // std::cin.ignore();
  sleep(2);
  std::cout << "\nStart of send to compute server\n";

  boost::asio::io_service io_service;

  // socket creation
  tcp::socket socket(io_service);

  auto port = options.cs0_port;
  auto ip = options.cs0_ip;
  // if (i) {
  //   port = options.cs1_port;
  //   ip = options.cs1_ip;
  //  }

  // connection
  if(!establishConnection(socket,ip,port)){
      std::cerr<<"Connection could not established to send the validation bit\n";
      // socket.close();
    }
  else{
      std::cout<<"Connection established to send validation bit.\n";
      boost::system::error_code error_init;
      boost::asio::write(socket, boost::asio::buffer(&validation_bit, sizeof(validation_bit)),
                          error_init);
    }  
  
  // socket.connect(tcp::endpoint(boost::asio::ip::address::from_string(ip), port));
  socket.close();
}

int main(int argc, char* argv[]) {
  auto options = parse_program_options(argc, argv);

  if (!options.has_value()) {
    std::cerr<<"Error while parsing the given input options.\n";
    return EXIT_FAILURE;
  }
  std::string p1 = options->fullfilepath + "/newW1.csv";
  // dimensions should be 512*784
  Matrix weightW1 = Matrix(256, 784, p1, options->fractional_bits, *options);
  weightW1.readMatrixCSV();
  weightW1.generateShares();

  p1 = options->fullfilepath + "/newB1.csv";
  // dimensions should be 512*1
  Matrix biasB1 = Matrix(256, 1, p1, options->fractional_bits, *options);
  biasB1.readMatrixCSV();
  biasB1.generateShares();

  p1 = options->fullfilepath + "/newW2.csv";
  // dimensions should be 256*512
  Matrix weightW2 = Matrix(10, 256, p1, options->fractional_bits, *options);
  weightW2.readMatrixCSV();
  weightW2.generateShares();

  p1 = options->fullfilepath + "/newB2.csv";
  // dimensions should be 256*1
  Matrix biasB2 = Matrix(10, 1, p1, options->fractional_bits, *options);
  biasB2.readMatrixCSV();
  biasB2.generateShares();
  ///////////////////// To add a new matrix WeightW3, use the following piece of code -Ramya May
  /// 9,2023 /////////
  // p1 = options->fullfilepath + "name of the file WeightW3";

  // Matrix WeightW3 = Matrix(No. of rows of W3, No. of columns of W3, p1, options->fractional_bits,
  // *options); WeightW3.readMatrixCSV(); WeightW3.generateShares();

  std::vector<Matrix*> allData;
  allData.push_back(&weightW1);
  allData.push_back(&biasB1);
  allData.push_back(&weightW2);
  allData.push_back(&biasB2);
  //////////////////// Add new Matrix to allData as shown below
  // allData.push_back(&weightW3);
  ////All new weights and biases to be sent should be a part of allData and
  ////then it is automatically sent to compute_server.cpp

  //////////////////////Sending data directly from main //////////////////////////
  std::cout << "Sending to servers from main\n";

  for (int i = 0; i < 2; i++) {
    std::cout << "\nStart of send to compute server\n";

    boost::asio::io_service io_service;

    // socket creation
    tcp::socket socket(io_service);

    uint64_t frac_bits = options->fractional_bits;
    std::cout << "Frac bits:" << options->fractional_bits << "\n";
    auto port = options->cs0_port;
    auto ip = options->cs0_ip;
    if (i) {
      port = options->cs1_port;
      ip = options->cs1_ip;
    }

    // connection
    if(establishConnection(socket,ip,port)){
        std::cout<<"Connection established successfully\n";
      }
    else{
        std::cerr<<"Connection could not established with the weight share receiver in server "<<i<<std::endl;
        socket.close();
        return EXIT_FAILURE;
      }
    
    // socket.connect(tcp::endpoint(boost::asio::ip::address::from_string(ip), port));

    /////////Sending 2*number of layers or number of vectors to be expected
    int numberLayers = 2;
    int numberOfVectors = 2 * numberLayers;
    boost::system::error_code error_layer;
    boost::asio::write(socket, boost::asio::buffer(&numberOfVectors, sizeof(numberOfVectors)),
                       error_layer);
    if (!error_layer) {
      std::cout << "Not an error" << std::endl;
    } else {
      std::cout << "send failed: " << error_layer.message() << std::endl;
    }

    // getting a response from the server
    boost::asio::streambuf receive_buffer_layer;
    boost::asio::read_until(socket, receive_buffer_layer, "\n");
    if (error_layer && error_layer != boost::asio::error::eof) {
      std::cout << "receive failed: " << error_layer.message() << std::endl;
    } else {
      const char* data = boost::asio::buffer_cast<const char*>(receive_buffer_layer.data());
      std::cout << data << std::endl;
    }

    //////////////////////////////////////

    // First send the number of fractional bits to the server
    boost::system::error_code error_init;
    boost::asio::write(socket, boost::asio::buffer(&frac_bits, sizeof(frac_bits)), error_init);
    if (!error_init) {
      std::cout << "Not an error" << std::endl;
    } else {
      std::cout << "send failed: " << error_init.message() << std::endl;
    }

    // getting a response from the server
    boost::asio::streambuf receive_buffer_init;
    boost::asio::read_until(socket, receive_buffer_init, "\n");
    if (error_init && error_init != boost::asio::error::eof) {
      std::cout << "receive failed: " << error_init.message() << std::endl;
    } else {
      const char* data = boost::asio::buffer_cast<const char*>(receive_buffer_init.data());
      std::cout << data << std::endl;
    }

    for (auto Vector : allData) {
      std::cout << "File name:" << Vector->getFileName() << "\n";

      // Send rows and columns to compute server
      int rows, columns;
      auto arr1 = Vector->getRowsColumns();
      int arr[2];
      arr[0] = arr1[0];
      arr[1] = arr1[1];
      std::cout << "Rows, columns" << arr[0] << " " << arr[1] << std::endl;
      boost::system::error_code error;
      boost::asio::write(socket, boost::asio::buffer(&arr, sizeof(arr)), error);
      if (!error) {
        std::cout << "Not an error" << std::endl;
      } else {
        std::cout << "send failed: " << error.message() << std::endl;
      }

      // getting a response from the server
      boost::asio::streambuf receive_buffer;
      boost::asio::read_until(socket, receive_buffer, "\n");
      if (error && error != boost::asio::error::eof) {
        std::cout << "receive failed: " << error.message() << std::endl;
      } else {
        const char* data = boost::asio::buffer_cast<const char*>(receive_buffer.data());
        std::cout << data << std::endl;
      }

      // Sending CS0 data to compute_server0
      auto data = Vector->getData(0);
      if (i) {
        data = Vector->getData(1);
      }
      // std::cout << "inside main size of cs 0 data:" << sizeof(cs0_data)<< "\n";
      // std::cout << "inside main size of data:" << sizeof(data)<< "\n";
      //  Use auto keyword to avoid typing long
      //  type definitions to get the timepoint
      //  at this instant use function now()
      auto start = high_resolution_clock::now();
      std::cout << "Number of elements:" << Vector->getNumberofElements() << std::endl;
      std::cout << "....Sending data...."
           << "\n";
      write_struct_vector(socket, data, arr[0] * arr[1]);
      std::cout << "Data sent"
           << "\n";

      // getting a response from the server
      boost::asio::streambuf receive_buff;
      boost::asio::read_until(socket, receive_buff, "\n");
      if (error && error != boost::asio::error::eof) {
        std::cout << "receive failed: " << error.message() << std::endl;
      } else {
        const char* data = boost::asio::buffer_cast<const char*>(receive_buff.data());
        std::cout << data << std::endl;
      }
      auto stop = high_resolution_clock::now();
      auto duration = duration_cast<milliseconds>(stop - start);
      std::cout << "Duration to write the vector:" << duration.count() << "msec" << std::endl;
    }
    std::cout << "Data sent to Server " << i << std::endl;
    socket.close();
  }
  send_confirmation(*options);
  return 0;
}
