//./bin/3PC_Relu_0 --party 0,127.0.0.1,4009 --party 1,127.0.0.1,4010 --helper_node 127.0.0.1,4011 --current-path ${BASE_DIR}/build_debwithrelinfo_gcc


#include <unistd.h>
#include <filesystem>
#include <fstream>
#include <random>
#include <regex>
#include <stdexcept>
#include <utility>
#include "communication/communication_layer.h"
#include "communication/message_handler.h"
#include "communication/tcp_transport.h"
#include "utility/logger.h"

#include <boost/algorithm/string.hpp>
#include <boost/json/serialize.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/log/trivial.hpp>
#include <boost/program_options.hpp>

#include <iostream>
#include <iterator>
#include <parallel/algorithm>
#include <vector>
#include "utility/new_fixed_point.h"

#include <boost/chrono.hpp>
#include <boost/thread/thread.hpp>
#include <chrono>

using namespace std::chrono;

/************************************************************************************************/
// Aim : Read the Arithmatic shares and convert them into ABY shares
//Input :  "/home/iudx/Desktop/Haritha/iudx-MOTION2NX-1/build_debwithrelinfo_gcc/3PC_Relu/OutputShare_0"
//********************************************************************************************

//Global variables
bool helpernode_ready_flag = false;
bool ABY_receive_flag = false;
std::vector<std::uint64_t> X_arith, X_ABY_public, X_ABY_private;
std::vector<std::uint8_t> msg_ABY_shares_for_mult;
std::vector<std::uint64_t> for_test;


std::uint64_t fractional_bits;
namespace po = boost::program_options;

void testMemoryOccupied(int WriteToFiles, int my_id, std::string path) {
  int tSize = 0, resident = 0, share = 0;
  std::ifstream buffer("/proc/self/statm");
  buffer >> tSize >> resident >> share;
  buffer.close();

  long page_size_kb =
      sysconf(_SC_PAGE_SIZE) / 1024;  // in case x86-64 is configured to use 2MB pages
  double rss = resident * page_size_kb;
  std::cout << "RSS - " << rss << " kB\n";
  double shared_mem = share * page_size_kb;
  std::cout << "Shared Memory - " << shared_mem << " kB\n";
  std::cout << "Private Memory - " << rss - shared_mem << "kB\n";
  std::cout << std::endl;
  if (WriteToFiles == 1) {
    // Generate path for the AverageMemoryDetails file and MemoryDetails file
    std::string t1 = path + "/" + "AverageMemoryDetails" + std::to_string(my_id);
    std::string t2 = path + "/" + "MemoryDetails" + std::to_string(my_id);

    // Write to the AverageMemoryDetails files
    std::ofstream file1;
    file1.open(t1, std::ios_base::app);
    file1 << rss;
    file1 << "\n";
    file1.close();

    std::ofstream file2;
    file2.open(t2, std::ios_base::app);
    file2 << "Helper Node Multiplication layer : \n";
    file2 << "RSS - " << rss << " kB\n";
    file2 << "Shared Memory - " << shared_mem << " kB\n";
    file2 << "Private Memory - " << rss - shared_mem << "kB\n";
    file2.close();
  }
}

struct Options {
  std::string current_path;
  MOTION::Communication::tcp_parties_config tcp_config;
  std::string arith_shares_file;
  std::string aby_shares_file;
  std::size_t fractional_bits;
};

std::optional<Options> parse_program_options(int argc, char* argv[]) {
  Options options;
  boost::program_options::options_description desc("Allowed options");
  // clang-format off
  desc.add_options()
    ("help,h", po::bool_switch()->default_value(false),"produce help message")
    
    ("party", po::value<std::vector<std::string>>()->multitoken(),
     "(party id, IP, port), e.g., --party 1,127.0.0.1,7777")
    ("helper_node", po::value<std::string>()->multitoken(),
     "(helpernode IP, port), e.g., --helper_node 127.0.0.1,7777") 
    ("current-path", po::value<std::string>()->required(), "currentpath") 
    ("fractional-bits", po::value<std::size_t>()->default_value(13), "Number of fractional bits") 
  ;
 
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
    std::cerr << "error:" << e.what() << "\n\n";
    std::cerr << desc << "\n";
    return std::nullopt;
  }

  options.current_path = vm["current-path"].as<std::string>();
  options.fractional_bits = vm["fractional-bits"].as<std::size_t>();
  fractional_bits = options.fractional_bits;
  std::cout<<"Fractional bits: "<< options.fractional_bits<<std::endl;
  // clang-format on;

  const auto parse_helpernode_info =
      [](const auto& s) -> MOTION::Communication::tcp_connection_config {
    const static std::regex party_argument_re("([^,]+),(\\d{1,5})");
    std::smatch match;
    if (!std::regex_match(s, match, party_argument_re)) {
      throw std::invalid_argument("Invalid party argument: "+s);
    }
    auto host = match[1];
    auto port = boost::lexical_cast<std::uint16_t>(match[2]);
    return {host, port};
  };

  const auto parse_party_argument =
      [](const auto& s) -> std::pair<std::size_t, MOTION::Communication::tcp_connection_config> {
    const static std::regex party_argument_re("([01]),([^,]+),(\\d{1,5})");
    std::smatch match;
    if (!std::regex_match(s, match, party_argument_re)) {
      throw std::invalid_argument("invalid party argument");
    }
    auto id = boost::lexical_cast<std::size_t>(match[1]);
    auto host = match[2];
    auto port = boost::lexical_cast<std::uint16_t>(match[3]);
    return {id, {host, port}};
  };

  const std::vector<std::string> party_infos = vm["party"].as<std::vector<std::string>>();
  std::cout << party_infos[0] << "\n";
  std::cout << party_infos[1] << "\n";
  if (party_infos.size() != 2) {
    std::cerr << "expecting two --party options\n";
    return std::nullopt;
  }
  const auto [id0, conn_info0] = parse_party_argument(party_infos[0]);
  const auto [id1, conn_info1] = parse_party_argument(party_infos[1]);
  if (id0 == id1) {
    std::cerr << "Need party arguments for both party 0 and party 1\n";
    return std::nullopt;
  }
  const std::string helper_node_info = vm["helper_node"].as<std::string>();
  const auto conn_info_helpernode = parse_helpernode_info(helper_node_info);

  options.tcp_config.resize(3);
  options.tcp_config[id0] = conn_info0;
  options.tcp_config[id1] = conn_info1;
  options.tcp_config[2] = conn_info_helpernode;
  options.arith_shares_file = "/home/iudx/Desktop/Haritha/iudx-MOTION2NX-1/build_debwithrelinfo_gcc/3PC_Relu/OutputShare_0";
  options.aby_shares_file = "/home/iudx/Desktop/Haritha/iudx-MOTION2NX-1/build_debwithrelinfo_gcc/3PC_Relu/ABY_shares_0";
  
return options;
}

void print_message(std::vector<std::uint8_t>& message) {
  for (auto i = 0; i < message.size(); i++) {
    std::cout << std::hex << (int)message[i] << " ";
  }
  return;
}

std::uint64_t getuint64(std::vector<std::uint8_t>& message, int index) {
  //Converts 8->64
  std::uint64_t num = 0;
  for (auto i = 0; i < 8; i++) {
    num = num << 8;
    num = num | message[(index + 1) * 8 - 1 - i];
  }
  return num;
}

void adduint64(std::uint64_t num, std::vector<std::uint8_t>& message) {
  //Converts 64->8
  for (auto i = 0; i < sizeof(num); i++) {
    std::uint8_t byte = num & 0xff;
    message.push_back(byte);
    num = num >> 8;
  }
}

template <typename E>
std::uint64_t RandomNumGenerator(E &engine)
{
     std::uniform_int_distribution<unsigned long long> dis(
         std::numeric_limits<std::uint64_t>::min(),
         std::numeric_limits<std::uint64_t>::max());
     return dis(engine);
}

//ReadSharesIntoVec : Reads arithmatic shares from file_path into a vector
//first 2 values are dimensions, remaning are share values
int ReadSharesIntoVec(std::string file_path, std::vector<uint64_t>&vec)
{
std::ifstream input_file;
std::cout << "\n #### Entered ReadSharesIntoVec ##### \n";

  try {
    input_file.open(file_path);
    if (!input_file) {
      std::cerr << "Unable to open Arithmatic share file.\n";
      throw std::ifstream::failure("Error opening Arithmatic share file.");
    }
  }
  catch(std::exception& e){
      std::cerr<<"Error during opening Arithmatic share file: "<<e.what()<<std::endl;
      return EXIT_FAILURE;
    }
  int num_r = 0; 
  int num_c = 0;
  
  // reading the first line and getting the number of rows and columns
  try{
    input_file >> num_r >> num_c;
    }
    catch (std::ifstream::failure e) {
      std::cerr << "Error while reading rows and columns from input shares file.\n";
      exit(1);
    }
    if (input_file.eof()) {
      std::cerr << "Input shares file doesn't contain rows and columns" << std::endl;
      exit(1);
    }
  std::cout << "Total num elements : " << num_r*num_c << "\n";
  vec.push_back(num_r);
  vec.push_back(num_c);
  int num_vals = num_r*num_c;
  std::uint64_t temp;
  int k = 0;

  while (k < num_r*num_c) {
      try{    
        input_file >> temp;
        vec.push_back(temp);
      }
      catch (std::ifstream::failure e) {
      std::cerr << "Error while reading the Arithmatic shares.\n";
      exit(1);
      }
      if (input_file.eof()) {
        std::cerr << "Arithmatic shares file contains less number of elements" << std::endl;
        exit(1);
      }
      k++;
    }
  std :: cout << "Number of elements read into vector : " << vec.size() << "\n";
  input_file.close();
  // if (k != num_r*num_c) 
  //   std::cout << "The number of elements expeted are : " << num_r*num_c << ", but present in file are : " << k << "\n";
  std :: cout << "#### Exiting ReadSharesIntoVec ##### \n\n";
  return 1;
}

//ConvertVectorIntoMessage : Converts the input vector into a message 
//that is ready to send over socket
//mes_type : To differentiate received messages we  
void ConvertVetorIntoMessage(std::vector<uint64_t>&vec, std::vector<uint8_t>&msg, std::uint8_t msg_type = 0)
{
std::cout << "\n ##### Entered ConvertVetorIntoMessage... ##### \n\n"; 
//  std::cout << "Start, Size of message : " << msg.size() << "\n";
//  std::cout << "Vetor size : " << vec.size() << "\n";
 msg.push_back(msg_type);
 for(int i = 0;i < vec.size();i++)
  {
    adduint64(vec[i], msg); 
  }
std::cout << "\n ##### Exiting ConvertVetorIntoMessage... ##### \n\n"; 
}

void ConvertMessageIntoVector(std::vector<uint8_t>&msg, std::vector<uint64_t>&vec )
{
  std::cout << "\n #####Entered ConvertMessageIntoVector... \n"; 
  int i = 0;
  msg.erase(msg.begin());
  int k = msg.size()/8;
  std::cout << msg.size() << "\n";
  for(i = 0; i < k; i++)
      {
        auto t = getuint64(msg, i);
        vec.push_back(t);
      }
  std::cout << "Exting ConvertMessageIntoVector... \n\n"; 
}

//ConvertFileIntoMessage(): converts the content in the input file into 
//a message that can be send over socket
void ConvertFileIntoMessage(){
  std::cout<< "Entered ConvertFileIntoMessage... \n";
  int i;
}

// Method which is called with received messages of the type the handler is
// registered for and the id of the sending party.
// This method may be called concurrently with different values of party_id.
// The client is responsible for the necessary synchronization,see  message_handler.h for more info
//for this reason we have to use synchronization on our own
// virtual void received_message(std::size_t party_id, std::vector<std::uint8_t>&& message) = 0;
//Note that above is a virtual function
class TestMessageHandler : public MOTION::Communication::MessageHandler {
  void received_message(std::size_t party_id, std::vector<std::uint8_t>&& message) override {
    
    if(message.size() == 1 && message[0] == (std::uint8_t)1)
      {
        if(party_id == 2)
          {
            std::cout<<"\nHelper node acknowledged message 1.\n";
            helpernode_ready_flag = true;
            return;
          }
        else
          {
            std::cerr<<"Received the message \"1\" from unknown party "<<party_id<<std::endl;
            return;
          }
      }
    while(!helpernode_ready_flag)
      {
        std::cout<<"#";
        boost::this_thread::sleep_for(boost::chrono::milliseconds(200));
      }
    if (message[0] == (std::uint8_t)0)
      {
        if(party_id == 1)
          {
            std::cout << "Received message from : " << party_id << "\n";
            //MessageToVector
            std::cout << "message size : " << message.size() << "\n";
            ConvertMessageIntoVector(message, X_ABY_public);
            ABY_receive_flag = true;
            return;
          }
        else
          {
            std::cerr<<"Received the message '0' from unknown party "<<party_id<<std::endl;
            return;
          }
      }
    std::cout<<"\n";
    while(!ABY_receive_flag)
      {
        std::cout<<".";
        boost::this_thread::sleep_for(boost::chrono::milliseconds(200));
      }
  }
}; 

int main(int argc, char* argv[]) {
  auto start = high_resolution_clock::now();
  auto options = parse_program_options(argc, argv);

  if (!options.has_value()) {
    std::cerr<<"No options given.\n";
    return EXIT_FAILURE;
  }
  
  int my_id = 0, helpernode_id=2;
  int WriteToFiles = 1;
  
  std::cout << "My party id: " << my_id << "\n";
  std::unique_ptr<MOTION::Communication::CommunicationLayer> comm_layer;
  std::shared_ptr<MOTION::Logger> logger;
  try{
    try{
      MOTION::Communication::TCPSetupHelper helper(my_id, options->tcp_config);
      comm_layer = std::make_unique<MOTION::Communication::CommunicationLayer>(
          my_id, helper.setup_connections());
    }
    catch (std::runtime_error& e) {
      std::cerr << "Error occurred during connection setup: " << e.what() << "\n";
      return EXIT_FAILURE;
    }
     try{
      comm_layer->start();
    }
    catch (std::runtime_error& e) {
      std::cerr << "Error occurred while starting the communication: " << e.what() << "\n";
      return EXIT_FAILURE;
    }
   //Read_Arithmatic shares from  "/home/iudx/Desktop/Haritha/iudx-MOTION2NX-1/build_debwithrelinfo_gcc/3PC_Relu/OutputShare_0"
   
   if (ReadSharesIntoVec(options->arith_shares_file, X_arith))
    std::cout<< "complted Reading Arithmatic shares \n";
   else std::cout << "Could not read Arithmatic shares into a vector \n";
   std::cout << "X_arith.size() : " <<X_arith.size() << "\n";
   
   ConvertVetorIntoMessage(X_arith, msg_ABY_shares_for_mult);
   std::cout << "msg_ABY_shares_for_mult.size()" << msg_ABY_shares_for_mult.size()  << "\n";
   
   //ConvertMessageIntoVector(msg_ABY_shares_for_mult, for_test);
  //  for(int i = 0; i<for_test.size();i++)
  //    std::cout << for_test[i] << "\n";

   std::vector<std::uint8_t> started{(std::uint8_t)1};
   std::cout<<"Sending message 1 the helper node.\n";
   try{
      comm_layer->send_message(helpernode_id, started);
   }
   catch (std::runtime_error& e) {
      std::cerr << "Error occurred while sending the start message to helper node: " << e.what() << "\n";
      return EXIT_FAILURE;
   }
   comm_layer->register_fallback_message_handler([](auto party_id) { return std::make_shared<TestMessageHandler>(); });
   
  //Waiting to receive the acknowledgement from helpernode
   while(!helpernode_ready_flag)
      {
        std::cout<<"*";
        boost::this_thread::sleep_for(boost::chrono::milliseconds(200));
      }
  std::cout << "sending message to Server : " << 1-my_id << "\n";
  try{
      comm_layer->send_message(1-my_id, msg_ABY_shares_for_mult);
    }
  catch (std::runtime_error& e) {
      std::cerr << "Error occurred while sending msg_ABY_shares_for_mult to node other part: " << e.what() << "\n";
      return EXIT_FAILURE;
    }
  while(!ABY_receive_flag)
    {
      std::cout<<"*";
      boost::this_thread::sleep_for(boost::chrono::milliseconds(200));
    }
  for(int i = 0; i<X_ABY_public.size();i++)
     std::cout << X_ABY_public[i] << "\n";


  testMemoryOccupied(WriteToFiles,0, options->current_path);
  std::cout<<std::endl;
  std::cout <<"**************************************************************\n"; 
  auto stop = high_resolution_clock::now();
  auto duration = duration_cast<milliseconds>(stop - start);
  
  std::cout<<"Duration:"<<duration.count()<<"\n";

  std::string t1 = options->current_path + "/" + "AverageTimeDetails0";
  std::string t2 = options->current_path + "/" + "MemoryDetails0";

  std::ofstream file2;
  file2.open(t2, std::ios_base::app);
  if(!file2.is_open())
    {
      std::cerr<<"Unable to open the MemoryDetails file.\n";
    }
  else{
  file2 << "Execution time - " << duration.count() << "msec";
  file2 << "\n";
  }
  file2.close();

  std::ofstream file1;
  file1.open(t1, std::ios_base::app);
  if(!file1.is_open())
    {
      std::cerr<<"Unable to open the AverageTimeDetails file.\n";
    }
  else
    {
    file1 << duration.count();
    file1 << "\n";
    }
  file1.close();
  }
  catch (std::runtime_error& e) {
    std::cerr << "ERROR OCCURRED: " << e.what() << "\n";
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}