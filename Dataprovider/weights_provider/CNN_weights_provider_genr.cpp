//  ./bin/weights_provider_genr --compute-server0-ip 127.0.0.1 --compute-server0-port 1234
//  --compute-server1-ip 127.0.0.1 --compute-server1-port 1235 --fractional-bits 13
//  --filepath ${BASE_DIR}/data/ModelProvider --config-file-path ${BASE_DIR}/config_files/model_config.json

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <random>
#include <stdexcept>
#include <type_traits>
#include <boost/chrono.hpp>
#include <boost/thread.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/asio.hpp>
#include <boost/program_options.hpp>
#include <boost/serialization/string.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include "./fixed-point.h"

#define MAX_CONNECT_RETRIES 50
using namespace std::chrono;
namespace pt = boost::property_tree;
using namespace boost::asio;
namespace po = boost::program_options;
using ip::tcp;

struct Options {
  std::string cs0_ip;
  int cs0_port;
  std::string cs1_ip;
  int cs1_port;
  std::size_t fractional_bits;
  std::string model_directory;
  std::filesystem::path config_file;
  int numberOfLayers;
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
    ("filepath", po::value<std::string>()->required(), "Directory where the weights and biases are stored")
    ("config-file-path", po::value<std::string>()->required(), "File path which has the name of all the weights and bias csv files")
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
  options.model_directory = vm["filepath"].as<std::string>();
  options.config_file = vm["config-file-path"].as<std::string>();
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
  //Check if currentpath directory exists
  if(!std::filesystem::is_directory(options.model_directory))
    {
      std::cerr<<"Directory ("<<options.model_directory<<") does not exist.\nUnable to read the weights and bias from folder.\n";
      return std::nullopt;
    }
  if(!std::filesystem::exists(options.config_file))
    {
      std::cerr<<"Unable to find the model config json file at "<<options.config_file<<".\n";
      return std::nullopt;
    }
  //--------------------------------------------------------------------------------------------//
  return options;
}

template <typename E>
std::uint64_t RandomNumDistribution(E& engine) {
  std::uniform_int_distribution<unsigned long long> distribution(std::numeric_limits<std::uint64_t>::min(),
                                                        std::numeric_limits<std::uint64_t>::max());
  return distribution(engine);
}

struct Shares {
  uint64_t Delta, delta;
};

//----------------------------------Sending shares------------------------------------------//
void write_struct_vector(tcp::socket& socket, Shares* data, int num_elements) {
  std::cout << "here" << std::endl;
  uint64_t *arr = (uint64_t *) malloc(2 * num_elements * sizeof(uint64_t));
  if (!arr) {
    throw std::runtime_error("Error while allocating memory.\n");
  }
  // std::cout << sizeof(arr) << std::endl;
  int j = 0;
  for (int i = 0; i < num_elements; i++) {
    // uint64_t arr[2];
    arr[j] = data[i].Delta;
    j++;
    arr[j] = data[i].delta;
    j++;
  }
  boost::system::error_code error;
  std::cout << "Started sending the weight shares." <<std::endl;
  boost::asio::write(socket, boost::asio::buffer(arr, (2*num_elements*sizeof(uint64_t))), error);
  if (error) 
      {
      throw std::runtime_error("Unable to send shares.\nError:"+ error.message() + "\n");
      }
  std::cout<<"Sent successfully\n";
  free(arr);
}

void write_struct(tcp::socket& socket, Shares* data, int num_elements) {
  for (int i = 0; i < num_elements; i++) {
    uint64_t arr[2];
    arr[0] = data[i].Delta;
    arr[1] = data[i].delta;

    boost::system::error_code error;
    boost::asio::write(socket, boost::asio::buffer(&arr, sizeof(arr)), error);
    if (error) 
          {
          std::cerr << "Unable to send share "<< i+1<<".\nError: " << error.message() << std::endl;
          return;
          }
    }
    std::cout<<"Sent successfully\n";
}

class Matrix {
 private:
  uint64_t type, kernel, channels, rows, columns, num_elements, fractional_bits;
  uint64_t pads[4];
  uint64_t strides[2];
  std::string fullFileName;
  std::vector<float> data;
  struct Shares *cs0_data, *cs1_data;

 public:
  Matrix(int row, int col, std::string fileName, std::size_t fraction_bits,
         const Options& options) {
    type = 0;
    rows = row;
    columns = col;
    num_elements = row * col;
    fractional_bits = fraction_bits;

    fullFileName = fileName;
    std::cout << fullFileName << std::endl;
    std::cout << "No. of elements: " << num_elements << std::endl;
    cs0_data = (struct Shares*) malloc(num_elements * sizeof(struct Shares));
    cs1_data = (struct Shares*) malloc(num_elements * sizeof(struct Shares));
    if (!cs0_data || !cs1_data) {
      std::cout << "Error while allocating memory" << std::endl;
    }
  }

   // Constructor for CNN Vectors
  Matrix(int ker, int chnls, int row, int col, int pad_arr[4], int strd[2],
         std::string fileName, std::size_t fraction_bits, const Options& options) {
    type = 1;       // If CNN
    kernel = ker;
    channels = chnls;
    rows = row;
    columns = col;
    std::copy(pad_arr, pad_arr+4, pads);
    std::copy(strd, strd+2, strides); 
    num_elements = row * col * ker * chnls;
    fractional_bits = fraction_bits;

    fullFileName = fileName;
    std::cout << fullFileName << std::endl;
    std::cout << "No. of elements: " << num_elements << std::endl;
    cs0_data = (struct Shares*) malloc(num_elements * sizeof(struct Shares));
    cs1_data = (struct Shares*) malloc(num_elements * sizeof(struct Shares));
    if (!cs0_data || !cs1_data) {
      std::cout << "Error while allocating memory" << std::endl;
    } 
  }

  void free_csdata(int i) {
    if (i==0)
      free(cs0_data);
    else if (i==1)
      free(cs1_data);
    return;
  }
  
  int getType() { return type; }

  //------- Function to access rows and columns of a Matrix, Ramya May 3,2023----------
  std::vector<int> getDimensions() {
    std::vector<int> dimensions;
    if (type == 1) {
      dimensions.push_back(kernel);
      dimensions.push_back(channels);
    }
    dimensions.push_back(rows);
    dimensions.push_back(columns);
    if (type == 1) {
      for (int j=0; j<4; j++)
        dimensions.push_back(pads[j]);
      for (int j=0; j<2; j++)
        dimensions.push_back(strides[j]);
    }
    return dimensions;
  }

  int getNumberofElements() { return num_elements; }
  std::string getFileName() { return fullFileName; }
  //------------ Function to aceess data from the Matrix, Ramya May 3,2023 -------------
  Shares* getData(int a) {
    if (a == 0) {
      return cs0_data;
    } 
    else {
      return cs1_data;
    }
  }

  void readMatrixCSV() {
    auto start = high_resolution_clock::now();
    std::cout << "Reading the matrix from file: " << fullFileName << std::endl;
    std::ifstream indata;
    indata.open(fullFileName);
    if(!indata) {
      throw std::ifstream::failure("Error opening the weights and bias csv file: " + fullFileName);      
    }

    std::string line, cell;
    while (std::getline(indata, line)) {
      std::stringstream lineStream(line);
      std::string cell;
      while (std::getline(lineStream, cell, ',')) {
        data.push_back(stof(cell));
      }
    }
    if (data.size() != num_elements) {
      throw std::range_error("No. of elements in file (" + std::to_string(data.size()) + ") do not match with config file (" + std::to_string(num_elements) + ")\n");
    }

    auto stop = high_resolution_clock::now();
    auto duration = duration_cast<milliseconds>(stop - start);
    std::cout << "Time taken to read the file:" << duration.count() << "msec" << std::endl;
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
    try{
        for (int i = 0; i < data.size(); i++) {
          std::random_device rd;
          std::mt19937 gen(rd());
          std::uint64_t del0 = RandomNumDistribution(gen), del1 = RandomNumDistribution(gen);
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
        data.clear();
      }
    catch(std::exception& e){
      std::string err = e.what();
      throw std::runtime_error("Error during share generation: " + err + "\n");
    }
    // return EXIT_SUCCESS;
  }
};

std::vector<Matrix> read_Data(Options& options) {
  std::ifstream jsonFile(options.config_file);
  std::vector<Matrix> allData, nullData;
  std::filesystem::path data_dir(options.model_directory);
  std::string data_file_name;
  int type;
  int num_pads[4];
  int num_strides[2];
  int num_layers, num_kernels, num_channels, num_rows, num_columns;
  std::filesystem::path data_file_path;
  int curr_layer_id = 0;
  if (jsonFile.is_open()){   //checking whether the file is open
      try{
        pt::ptree pt;
        pt::read_json(jsonFile, pt);
        for (auto &json_elem : pt) {  //read data from file object and put it into string.
          if (json_elem.first == "no_of_layers") {
            options.numberOfLayers = stoi(json_elem.second.data());
          }
          else if(json_elem.first == "Layers") {
            for (auto &layer_id : json_elem.second) {
              curr_layer_id++;
              std::cout << curr_layer_id << std::endl;
              for (auto &data : layer_id.second) {
                type = 0;
                for(auto &property : data.second) {
                  if (property.first == "type") {
                    if (property.second.data() == "NN")
                      type = 0;
                    else if (property.second.data() == "CN")
                      type = 1;
                    else if (property.second.data() == "MP")
                      type = 2;
                    else
                      throw std::runtime_error("There is an error in the type of layer given inside the json file.");
                  }
                  else if (property.first == "kernels") {
                    num_kernels = stoi(property.second.data());
                  }
                  else if (property.first == "channels") {
                    num_channels = stoi(property.second.data());
                  }
                  else if (property.first == "rows") {
                    num_rows = stoi(property.second.data());
                  }
                  else if (property.first == "columns") {
                    num_columns = stoi(property.second.data());
                  }
                  else if (property.first == "pads") {
                    int i=0;
                    for (auto &pad:property.second) {
                      num_pads[i] =stoi(pad.second.data());
                      i++;
                    }
                  }
                  else if (property.first == "strides") {
                    int i=0;
                    for (auto &stride:property.second) {
                      num_strides[i] = stoi(stride.second.data());
                      i++;
                    }
                  }
                  else if (property.first == "file_name") {
                    std::string file_name = property.second.data();
                    std::filesystem::path data_file_name(file_name);
                    data_file_path = data_dir/data_file_name;
                  }
                  else {
                    throw std::runtime_error("There is an error in the field names given inside the json file.");
                  }
                }
                try {
                  if (type == 1) {
                    std::cout << "Convolution Layer recognised" << std::endl;
                    Matrix vector_data = Matrix(num_kernels, num_channels, num_rows, num_columns, num_pads, num_strides, data_file_path, options.fractional_bits, options);
                    vector_data.readMatrixCSV();
                    vector_data.generateShares();
                    allData.push_back(vector_data);
                  }
                  else if (type == 0) {
                    Matrix vector_data = Matrix(num_rows, num_columns, data_file_path, options.fractional_bits, options);
                    vector_data.readMatrixCSV();
                    vector_data.generateShares();
                    allData.push_back(vector_data);
                  }
                }
                catch(const std::ifstream::failure& e) {
                  std::cout << "Error: " << e.what() << std::endl;
                  return nullData;
                }
                catch(std::runtime_error const& e) {
                  std::cerr << "*****Runtime Error*****\n " << e.what() << std::endl;
                  return nullData;
                }
                
              }
              if(curr_layer_id > options.numberOfLayers) {
                jsonFile.close();
                throw std::runtime_error("Number of layers in model config exceed number of layers stated.");
              }                  
            }
          }
          else {
            jsonFile.close();
            throw std::runtime_error("There is an error in the field names given inside the json file.");
          }
        }
      }
      catch (const boost::property_tree::json_parser::json_parser_error& e) {
        jsonFile.close(); 
        std::string error_ = e.what();
        std::string error_msg = "Model Config JSON parser error: " +  error_ + ".\nError message: "+ e.message() + ".\nLine: " + std::to_string(e.line()) + "\n";
        throw std::runtime_error(error_msg);
      } 
  
      if(curr_layer_id < options.numberOfLayers) {
        jsonFile.close();
        throw std::runtime_error("Details of the layers provided are lesser than the number of layers. Please provide all the details of all the layers."); 
      }
      jsonFile.close();   //close the file object.
  }
  else {
    throw std::ifstream::failure("Error in opening json config file");
  }
  return allData;
}

void send_shares_to_servers(std::vector<Matrix> data_shares, const Options& options) {
  std::cout << "Sending shares to the compute servers." << std::endl;

  for(int i = 1; i >= 0; --i) {
    boost::asio::io_service io_service;

    // socket creation
    tcp::socket socket(io_service);

    uint64_t frac_bits = options.fractional_bits;
    std::cout << "Fractional bits:" << options.fractional_bits << "\n";
    auto port = options.cs0_port;
    auto ip = options.cs0_ip;
    if (i) {
      port = options.cs1_port;
      ip = options.cs1_ip;
    }

    // -------------------- Establishing socket connection with the servers -----------------------------
    if(establishConnection(socket,ip,port)){
        std::cout<<"Connection established successfully\n";
    }
    else{
        socket.close();
        throw std::runtime_error("Connection could not established with the weights share receiver in server "+ std::to_string(i) +"\n");
    }
    int numberOfVectors = 2*options.numberOfLayers;
    boost::system::error_code send_error, read_error;
    //------------------------ Sending number of layers to be expected -----------------------------------
    boost::asio::write(socket, boost::asio::buffer(&numberOfVectors, sizeof(numberOfVectors)),send_error);
    if (send_error) {
        socket.close();
        throw std::runtime_error("Could not send the number of layers. Error: " + send_error.message()+"\n");
    }
    std::cout<<"Sent the number of layers to weight share receivers.\n";

    // getting a response from the server
    boost::asio::streambuf receive_buffer_layer;
    boost::asio::read_until(socket, receive_buffer_layer, "\n",read_error);
    if (read_error) {
        if(read_error == boost::asio::error::eof) {
          socket.close();
          throw std::runtime_error("Connection closed by weight share receivers. Read error: "+ read_error.message()+"\n");
        } 
        else{
          socket.close();
          throw std::runtime_error("Error while receiving acknowledgment from weight share receivers for number of layers message. Read error: "+ read_error.message()+"\n");
        }
    }
    const char* data = boost::asio::buffer_cast<const char*>(receive_buffer_layer.data());
    std::cout << "Received response: "<< data << std::endl;

    // -------------------- Sending the number of fractional bits to the server --------------------------
    boost::asio::write(socket, boost::asio::buffer(&frac_bits, sizeof(frac_bits)), send_error);
    if (send_error) {
      socket.close();
      throw std::runtime_error("Could not send the number of fractional bits: "+send_error.message() + "\n");
    }

    // getting a response from the server
    boost::asio::streambuf receive_buffer_init;
    boost::asio::read_until(socket, receive_buffer_init, "\n", read_error);
    if (read_error) {
        socket.close();
        if(read_error == boost::asio::error::eof) {  
          throw std::runtime_error("Connection closed by weight share receivers. Read error: " + read_error.message() + "\n");
        } 
        else{
          throw std::runtime_error("Error while receiving acknowledgment from weight share receivers for fractional bits message. Read error: " + read_error.message() + "\n");
        }   
    }
    data = boost::asio::buffer_cast<const char*>(receive_buffer_init.data());
    std::cout << "Received response: "<< data << std::endl;
    // ------------------------------------ Sending shares ------------------------------------------------
    std::cout<<"Start sending all the weight and bias shares\n";
    for (auto &data_array : data_shares) {
      std::cout << "File name:" << data_array.getFileName() << "\n";
      // ----Send rows and columns to compute server----
      int rows, columns;
      int typeOfVector = data_array.getType();

      // Type of Vector
      boost::asio::write(socket, boost::asio::buffer(&typeOfVector, sizeof(typeOfVector)), send_error);
      if (send_error) {
      socket.close();
      throw std::runtime_error("Unable to send data size (rows,columns). Error: " + send_error.message() + "\n");
      }

      // getting a response from the server
      boost::asio::streambuf receive_buffer_type;
      boost::asio::read_until(socket, receive_buffer_type, "\n", read_error);
      if (read_error) {
        socket.close();
        if(read_error == boost::asio::error::eof) {
          throw std::runtime_error("Connection closed by weight share receivers. Read error: " + read_error.message() + "\n");
        } 
        else{
          throw std::runtime_error("Error while receiving acknowledgment from weight share receivers for the type message. Read error: " + read_error.message() + "\n");
        }
      }
      data = boost::asio::buffer_cast<const char*>(receive_buffer_type.data());
      std::cout <<"Received Response: " << data << std::endl;
      
      std::vector<int> vector_dimensions = data_array.getDimensions();
      int dimensions[vector_dimensions.size()];
      copy(vector_dimensions.begin(), vector_dimensions.end(), dimensions);
      
      std::cout << "Dimensions: ";
      for (auto iter:vector_dimensions)
        std::cout << iter << " ";
      std::cout << std::endl;

      // Dimensions
      boost::asio::write(socket, boost::asio::buffer(&dimensions, sizeof(dimensions)), send_error);
      if (send_error) {
      socket.close();
      throw std::runtime_error("Unable to send data size (rows,columns). Error: " + send_error.message() + "\n");
      }

      boost::asio::streambuf receive_buffer;
      boost::asio::read_until(socket, receive_buffer, "\n", read_error);
      if (read_error) {
        socket.close();
        if(read_error == boost::asio::error::eof) {
          throw std::runtime_error("Connection closed by weight share receivers. Read error: " + read_error.message() + "\n");
        } 
        else{
          throw std::runtime_error("Error while receiving acknowledgment from weight share receivers for the data size message. Read error: " + read_error.message() + "\n");
        }
      }
      data = boost::asio::buffer_cast<const char*>(receive_buffer.data());
      std::cout <<"Received Response: " << data << std::endl;
      
      // -----Sending share data to compute_server------
      auto share_data = data_array.getData(0);
      if (i) {
        share_data = data_array.getData(1);
      }
      // std::cout << "inside main size of cs 0 data:" << sizeof(cs0_data)<< "\n";
      // std::cout << "inside main size of data:" << sizeof(share_data)<< "\n";
      //  Use auto keyword to avoid typing long
      //  type definitions to get the timepoint
      //  at this instant use function now()
      auto start = high_resolution_clock::now();
      int num_of_elements = data_array.getNumberofElements();
      std::cout << "Number of elements:" << num_of_elements << std::endl;
      std::cout << "....Sending data...." << "\n";
      try{
        // Shares
        write_struct_vector(socket, share_data, num_of_elements);
      }
      catch(std::runtime_error& e){
          socket.close();
          throw std::runtime_error(e.what());  
      }
      std::cout << "Data sent.\n";

      // getting a response from the server
      boost::asio::streambuf receive_buff;
      boost::asio::read_until(socket, receive_buff, "\n",read_error);
      if (read_error) {
        socket.close();
        if(read_error == boost::asio::error::eof) {
          throw std::runtime_error("Connection closed by weight share receivers. Read error: " + read_error.message() + "\n");
        } 
        else{
          throw std::runtime_error("Error while receiving acknowledgment from weight share receivers for the weight shares. Read error: " + read_error.message() + "\n");
        }
      }
      data = boost::asio::buffer_cast<const char*>(receive_buff.data());
      std::cout << "Received response: " << data << std::endl;

      data_array.free_csdata(i);
    
      auto stop = high_resolution_clock::now();
      auto duration = duration_cast<milliseconds>(stop - start);
      std::cout << "Time taken to send the shares:" << duration.count() << "msec" << std::endl;
    }
    std::cout << "Data sent to server " << i << std::endl;
    socket.close();
  }
}

void send_confirmation(const Options& options) {
  int validation_bit = 1;
  sleep(2);
  std::cout << "\nStart of send to compute server\n";

  boost::asio::io_service io_service;

  // socket creation
  tcp::socket socket(io_service);

  auto port = options.cs0_port;
  auto ip = options.cs0_ip;

  // connection
  if(establishConnection(socket,ip,port)){
        std::cout<<"Connection established successfully\n";
  }
  else{
        // std::cerr<<"Connection could not established with the weight share receiver.\n";
        socket.close();
        throw std::runtime_error("Connection could not established with the weight share receiver to send the validation bit.\n");
        // return EXIT_FAILURE;
  }
  boost::system::error_code error_init;
  boost::asio::write(socket, boost::asio::buffer(&validation_bit, sizeof(validation_bit)),
                     error_init);
  if(error_init){
      std::cerr<<"Could not send the validation bit. Error: "<<error_init.message()<<"\n";
      socket.close();
      throw std::runtime_error("Could not send the validation bit. Error: "+error_init.message()+"\n");
      // return EXIT_FAILURE;
    }
  socket.close();
  // return EXIT_SUCCESS;
}

int main(int argc, char* argv[]) {
  auto options = parse_program_options(argc, argv);

  if (!options.has_value()) {
    std::cerr<<"Error while parsing the given input options. \nExiting...\n";
    return EXIT_FAILURE;
  }
  
  try {
    auto model_share_data = read_Data(*options);
    if(model_share_data.size() == 0) {
      std::cout << "Error while reading data and generating shares. \nExiting..\n";
      return EXIT_FAILURE;
    }
    send_shares_to_servers(model_share_data, *options);
    // send_confirmation(*options);
  }
  catch(const std::ifstream::failure& e) {
    std::cerr << "Error in file: " << e.what() <<std::endl;
    std::cerr <<"Exiting..\n";
    return EXIT_FAILURE;
  }
  catch(std::runtime_error const& e) {
    std::cerr << "*****Runtime Error*****\n " << e.what() << std::endl;
    std::cerr <<"Exiting..\n";
    return EXIT_FAILURE;
  }
  std::cout<<"Weights provider finished its work successfully.\n";
  return EXIT_SUCCESS;
}