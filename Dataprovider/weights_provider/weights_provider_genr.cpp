//  ./bin/weights_provider_genr --compute-server0-ip 127.0.0.1 --compute-server0-port 1234
//  --compute-server1-ip 127.0.0.1 --compute-server1-port 1235 --dp-id 0 --fractional-bits 13
//  --filepath ${BASE_DIR}/data/ModelProvider --config-file-path ${BASE_DIR}/config_files/model_config.json

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
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>

using namespace std::chrono;
namespace pt = boost::property_tree;

#include "./fixed-point.h"

using namespace boost::asio;
namespace po = boost::program_options;
namespace fs = std::filesystem; 
using ip::tcp;
using std::cout;
using std::endl;
using std::string;

struct Options {
  std::string cs0_ip;
  int cs0_port;
  std::string cs1_ip;
  int cs1_port;
  std::size_t fractional_bits;
  std::string fullfilepath;
  std::filesystem::path config_file;
  int numberOfLayers;
};

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
    ("dp-id", po::value<int>()->required(), "Id of the data provider")
    ("filepath", po::value<string>()->required(), "Directory where the weights and biases are stored")
    ("config-file-path", po::value<string>()->required(), "File path which has the name of all the weights and bias csv files")
    ;
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
  options.config_file = vm["config-file-path"].as<std::string>();

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

///////////////////////////////////////////////////////////////
void write_struct_vector(tcp::socket& socket, Shares* data, int num_elements) {
  std::cout << "Inside write struct \n";
  uint64_t arr[2 * num_elements];
  int j = 0;
  for (int i = 0; i < num_elements; i++) {
    // uint64_t arr[2];
    arr[j] = data[i].Delta;
    j++;
    arr[j] = data[i].delta;
    j++;
  }
  boost::system::error_code error;
  //  cout << "Start write operation " << endl;
  boost::asio::write(socket, boost::asio::buffer(&arr, sizeof(arr)), error);

  if (error) {
    cout << "Send failed: " << error.message() << endl;
  } else {
    cout << "Sent successfully \n";
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
  std::pair<int, int> getShareDimensions() {
    std::pair<int, int> dimensions;
    dimensions.first = rows;
    dimensions.second = columns;
    return dimensions;
  }

  int getNumberofElements() { return num_elements; }
  string getFileName() { return fullFileName; }
  /////////Function to aceess data from the Matrix, Ramya May 3,2023
  Shares* getData(int a) {
    if (a == 0) {
      return cs0_data;
    } else {
      return cs1_data;
    }
  }

  void readMatrixCSV() {
    auto start = high_resolution_clock::now();

    std::cout << "Reading the matrix from file :- " << fullFileName << std::endl;

    std::ifstream indata;
    indata.open(fullFileName);
    if(!indata) {
      // std::cout << "Error: There was an issue in opening the weights and bias csv file\n";
      throw std::ifstream::failure("Error opening the weights and bias csv file: " + fullFileName);      
    }
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
    cout << "Duration for reading from the file:" << duration.count() << "msec" << endl;
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

      // std::cout << "Data = " << data[i] << " Delta = " << cs0.Delta << " delta0 = " << cs0.delta
      // << " delta1 = " << cs1.delta << "\n";
    }

    return;
  }
};

std::vector<Matrix> read_Data(Options& options) {
  std::ifstream jsonFile(options.config_file);

  std::vector<Matrix> allData, nullData;
  fs::path data_dir(options.fullfilepath);

  if (jsonFile.is_open()){   //checking whether the file is open
      pt::ptree pt;
      pt::read_json(jsonFile, pt);
      
      string data_file_name;
      int num_layers, num_rows, num_columns;

      fs::path data_file_path;

      int curr_layer_id = 0;

      for (auto &json_elem : pt) {  //read data from file object and put it into string.
        if (json_elem.first == "no_of_layers") {
          options.numberOfLayers = stoi(json_elem.second.data());
        }
        else if(json_elem.first == "Layers") {
          for (auto &layer_id : json_elem.second) {
            curr_layer_id++;
            for (auto &data : layer_id.second) {
              
              for(auto &property : data.second) {
                
                if (property.first == "rows") {
                  num_rows = stoi(property.second.data());
                }
                else if (property.first == "columns") {
                  num_columns = stoi(property.second.data());
                }
                else if (property.first == "file_name") {
                  std::string file_name = property.second.data();
                  fs::path data_file_name(file_name);
                  data_file_path = data_dir/data_file_name;
                }
                else {
                  throw std::runtime_error("There is an error in the field names given inside the json file.");
                }
              }

              // std::cout << curr_layer_id << " " << options.numberOfLayers << " " << num_rows << " " << num_columns << " " << data_file_path << " " << std::endl;
              
              Matrix vector_data = Matrix(num_rows, num_columns, data_file_path, options.fractional_bits, options);
              try {
                vector_data.readMatrixCSV();                
              }
              catch(const std::ifstream::failure& e) {
                std::cout << "Error: " << e.what() << std::endl;
                return nullData;
              }
              vector_data.generateShares();
              allData.push_back(vector_data);
            }
            if(curr_layer_id > options.numberOfLayers) {
              // cout << "Error: Number of layers in model config exceed number of layers stated." << std::endl;
              throw std::runtime_error("Number of layers in model config exceed number of layers stated.");
            }                  
          }
        }
        else {
          throw std::runtime_error("There is an error in the field names given inside the json file.");
        }
      }
      if(curr_layer_id < options.numberOfLayers) {
        // cout << "Error: Details of the layers provided are leser than number of layers. Please provide all the details of all the layers." << std::endl;
        throw std::runtime_error("Details of the layers provided are leser than the number of layers. Please provide all the details of all the layers."); 
      }
      jsonFile.close();   //close the file object.
  }
  else {
    // std::cout << "Error in opening file" << std::endl;
    throw std::ifstream::failure("Error in opening json config file");
  }
  return allData;
}

void send_shares_to_servers(std::vector<Matrix> data_shares, const Options& options) {
  std::cout << "Sending shares to the compute servers." << std::endl;

  for(int i = 0; i < 2; ++i) {
    boost::asio::io_service io_service;

    // socket creation
    tcp::socket socket(io_service);

    uint64_t frac_bits = options.fractional_bits;
    std::cout << "Frac bits:" << options.fractional_bits << "\n";
    auto port = options.cs0_port;
    auto ip = options.cs0_ip;
    if (i) {
      port = options.cs1_port;
      ip = options.cs1_ip;
    }

    // connection
    socket.connect(tcp::endpoint(boost::asio::ip::address::from_string(ip), port));

    /////////Sending number of layers to be expected
    boost::system::error_code error_layer;
    boost::asio::write(socket, boost::asio::buffer(&options.numberOfLayers, sizeof(options.numberOfLayers)),
                       error_layer);
    if (!error_layer) {
      cout << "Not an error" << endl;
    } else {
      cout << "send failed: " << error_layer.message() << endl;
    }

    // getting a response from the server
    boost::asio::streambuf receive_buffer_layer;
    boost::asio::read_until(socket, receive_buffer_layer, "\n");
    if (error_layer && error_layer != boost::asio::error::eof) {
      cout << "receive failed: " << error_layer.message() << endl;
    } else {
      const char* data = boost::asio::buffer_cast<const char*>(receive_buffer_layer.data());
      cout << data << endl;
    }

    //////////////////////////////////////

    // First send the number of fractional bits to the server
    boost::system::error_code error_init;
    boost::asio::write(socket, boost::asio::buffer(&frac_bits, sizeof(frac_bits)), error_init);
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

    for (auto data_array : data_shares) {
      std::cout << "File name:" << data_array.getFileName() << "\n";

      // Send rows and columns to compute server
      int rows, columns;
      auto vector_dimensions = data_array.getShareDimensions();
      cout << "Rows, columns: " << vector_dimensions.first << " " << vector_dimensions.second << endl;
      boost::system::error_code error;
      boost::asio::write(socket, boost::asio::buffer(&vector_dimensions, sizeof(vector_dimensions)), error);
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
      auto data = data_array.getData(0);
      if (i) {
        data = data_array.getData(1);
      }
      // std::cout << "inside main size of cs 0 data:" << sizeof(cs0_data)<< "\n";
      // std::cout << "inside main size of data:" << sizeof(data)<< "\n";
      //  Use auto keyword to avoid typing long
      //  type definitions to get the timepoint
      //  at this instant use function now()
      auto start = high_resolution_clock::now();
      int num_of_elements = data_array.getNumberofElements();
      cout << "Number of elements:" << num_of_elements << endl;
      cout << "....Sending data...." << "\n";
      write_struct_vector(socket, data, num_of_elements);
      cout << "Data sent" << "\n";

      // getting a response from the server
      boost::asio::streambuf receive_buff;
      boost::asio::read_until(socket, receive_buff, "\n");
      if (error && error != boost::asio::error::eof) {
        cout << "receive failed: " << error.message() << endl;
      } else {
        const char* data = boost::asio::buffer_cast<const char*>(receive_buff.data());
        cout << data << endl;
      }
      auto stop = high_resolution_clock::now();
      auto duration = duration_cast<milliseconds>(stop - start);
      cout << "Duration to write the vector:" << duration.count() << "msec" << endl;
    }
    std::cout << "Data sent to Server " << i << endl;
    socket.close();
  }
}

void send_confirmation(const Options& options) {
  int validation_bit = 1;

  // for (int i = 0; i < 2; i++) {

  // }
  // std::cin.ignore();
  sleep(2);
  cout << "\nStart of send to compute server\n";

  boost::asio::io_service io_service;

  // socket creation
  tcp::socket socket(io_service);

  auto port = options.cs0_port;
  auto ip = options.cs0_ip;

  // connection
  socket.connect(tcp::endpoint(boost::asio::ip::address::from_string(ip), port));
  boost::system::error_code error_init;
  boost::asio::write(socket, boost::asio::buffer(&validation_bit, sizeof(validation_bit)),
                     error_init);

  socket.close();
}

int main(int argc, char* argv[]) {
  auto options = parse_program_options(argc, argv);

  if (!options.has_value()) {
    return EXIT_FAILURE;
  }
  
  try {
    auto model_share_data = read_Data(*options);
    if(model_share_data.size() == 0) {
      std::cout << "Error: There was an error in reading the data\n";
      return EXIT_FAILURE;
    }
    send_shares_to_servers(model_share_data, *options);
  }
  catch(std::exception const& e) {
    std::cout << "Runtime Error: " << e.what() << std::endl;
  }
  catch(const std::ifstream::failure& e) {
    std::cout << "Error in file: " << e.what() <<std::endl;
  }

  send_confirmation(*options);

  return EXIT_SUCCESS;
}