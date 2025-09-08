// At Party 0:
// ./bin/3PC_ArithToABY --party 0,127.0.0.1,4009 --party 1,127.0.0.1,4010 --current-path ${BASE_DIR}/build_debwithrelinfo_gcc --party_id 0

// At Party 1:
// ./bin/3PC_ArithToABY --party 0,127.0.0.1,4009 --party 1,127.0.0.1,4010 --current-path ${BASE_DIR}/build_debwithrelinfo_gcc --party_id 1

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
#include <string>
#include <vector>
#include "utility/new_fixed_point.h"

#include <boost/chrono.hpp>
#include <boost/thread/thread.hpp>
#include <chrono>

using namespace std::chrono;
namespace po = boost::program_options;

/***** START GLOBAL VARIABLES ******/
std::uint64_t fractional_bits;
const auto LMINUS_ONE = std::numeric_limits<std::uint64_t>::max();
const uint64_t PRIME_NUM = 67;
uint64_t addModPrime[PRIME_NUM][PRIME_NUM];
uint64_t multModPrime[PRIME_NUM][PRIME_NUM];
uint64_t subModPrime[PRIME_NUM][PRIME_NUM];
const auto BIT_SIZE = 64;

std::unique_ptr<MOTION::Communication::CommunicationLayer> comm_layer;

int my_id;

enum MessageType {OtherPartySync, ArithToABY};

bool ArithToABY_Flag = false;
bool OtherPartySync_Flag = false;

std::vector<std::uint64_t> Relu_Public_Shares_Other_Party;
/*****  END GLOBAL VARIABLES ******/

//******** START: RandomNumberGenerators ***********************
template <typename E>
std::uint64_t RandomNumDistribution(E& engine) {
  std::uniform_int_distribution<unsigned long long> distribution(
      std::numeric_limits<std::uint64_t>::min(), std::numeric_limits<std::uint64_t>::max());
  return distribution(engine);
}

template <typename E>
std::uint64_t RandomNumOverPrime(E& engine, int l = 0, int pr = PRIME_NUM) {
  std::uniform_int_distribution<unsigned long long> distribution(
      l, pr-1);
  return distribution(engine);
}
template <typename E>
std::uint64_t RandomNonZeroNumOverPrime(E& engine, int l = 1, int pr = PRIME_NUM) {
  std::uniform_int_distribution<unsigned long long> distribution(
      l, pr-1);
  return distribution(engine);
}

template <typename E>
std::uint64_t RandomNumOverOddRing(E& engine, int l = 0, std::uint64_t p = std::numeric_limits<std::uint64_t>::max()-1) {
  std::uniform_int_distribution<unsigned long long> distribution(
      l, p);
  return distribution(engine);
}
//******** END: RandomNumberGenerators ***********************
//used at P0 and P1

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

//*********************** START PRIME operations*****************
// Initializes the addition, subtraction and multiplication tables over modulo prime
// a + b, a - b, a * b ; a,b {0,1, ... ,PRIME_NUM -1 }
void InitializeModuloPrimeOps()
{
  for (int i = 0; i< PRIME_NUM;  i++)
  {
    for (int j = 0; j< PRIME_NUM;  j++)
    {
     addModPrime[i][j] = (i+j) % PRIME_NUM;
     multModPrime[i][j] = (i*j) % PRIME_NUM;
    }
  }
  for (int i = 0; i< PRIME_NUM;  i++)
  {
    for (int j = 0; j< PRIME_NUM;  j++)
    {
     if (j == 0) subModPrime[i][j] = i; 
     else 
     {
      auto temp = PRIME_NUM - j;
      subModPrime[i][j] = addModPrime[i][temp]; 
     }
    }
  }

}
inline std::uint64_t AddModuloPrime(std::uint64_t a, std::uint64_t b)
{return addModPrime[a][b];}
inline std::uint64_t MultiplyModuloPrime(std::uint64_t a, std::uint64_t b)
{return multModPrime[a][b];}
inline std::uint64_t SubtractModuloPrime(std::uint64_t a, std::uint64_t b)
{return subModPrime[a][b];}
//*********************** END PRIME operations*****************

// ReadSharesIntoVec: Reads arithmetic shares from a given filepath and stores it into a vector
// First two values are dimensions of the matrix (rows, cols), rest are the share values
int ReadSharesIntoVec(std::string file_path, std::vector<uint64_t>&vec)
{
std::ifstream input_file;
std::cout << "##### Entered ReadSharesIntoVec ...##### \n";
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
  std :: cout << "##### Exiting ReadSharesIntoVec ##### \n\n";
  return 1;
}

//ConvertMessageIntoVector : Converts received message into a vector
//Input : std::vector<uint8_t>&msg; Output : std::vector<uint64_t>&vec
//Note that the first byte of received message contains the type of message
void ConvertMessageIntoVector(std::vector<uint8_t>&msg, std::vector<uint64_t>&vec )
{
  std::cout << "##### Entered ConvertMessageIntoVector... #####\n"; 
  int i = 0;
  msg.erase(msg.begin());//we erase the message type that is stored in 1st byte
  int k = msg.size()/8;
  std::cout << msg.size() << "\n";
  for(i = 0; i < k; i++)
      {
        auto t = getuint64(msg, i);
        vec.push_back(t);
      }
  std::cout << "##### Exting ConvertMessageIntoVector.\n\n"; 
}

//Generate ABY private shares required for to create ABY
//Input : Arithmatic share vector
//Output : private shares with same dimension as input
//Note that the first two values in any vector are the dimensions
int GeneratePrivateShares_ABY(std::vector<uint64_t> &vec, std::vector<uint64_t> &private_share)
{
std::cout << "##### Entered GeneratePrivateShares_ABY ##### \n";
int num_r = vec[0];
int num_c = vec[1];
// Note that the first two values in any vector are the dimensions
private_share.push_back(vec[0]);
private_share.push_back(vec[1]);
  try {
    for (int i = 0; i < num_r*num_c; i++) {
      std::random_device rd;
      std::mt19937 gen(rd());
      std::uint64_t t = RandomNumDistribution(gen);
      private_share.push_back(t);
    }
  } catch (std::exception& e) {
    std::cerr << "Error during share generation: " << e.what() << std::endl;
    return EXIT_FAILURE;
  }
  std::cout << "##### Exiting GeneratePrivateShares_ABY ##### \n\n";
  
  return 1;
}

//addition is performed parallelly ans = op1 + op2
//offset from which element the addition operation has to be performed
void ParallelAddition(std::vector<std::uint64_t> &op1, std::vector<std::uint64_t> &op2, std::vector<std::uint64_t> &ans, int offset = 2)
{
std::cout << "#### Entered ParallelAddition ##### \n" ;
ans.resize(op1.size());
auto op1_begin = op1.begin();
auto op1_end = op1.end();
auto op2_begin = op2.begin();
auto ans_begin = ans.begin();
if (offset)
   {
    for(int i = 0; i<offset;i++)
       ans[i] = op2[i];
   }
advance(op1_begin, offset);
advance(op2_begin, offset);
advance(ans_begin, offset);
__gnu_parallel::transform(op1_begin, op1_end, op2_begin, ans_begin, std::plus{});

std::cout << "##### Exiting ParallelAddition ##### \n\n";
}

//ConvertVectorIntoMessage : Converts the input vector into a message 
//that is ready to send over socket
//mes_type : To differentiate received messages we  
void ConvertVetorIntoMessage(std::vector<uint64_t>&vec, std::vector<uint8_t>&msg, std::uint8_t msg_type = 0)
{
std::cout << "##### Entered ConvertVetorIntoMessage... ##### \n"; 
//  std::cout << "Start, Size of message : " << msg.size() << "\n";
//  std::cout << "Vetor size : " << vec.size() << "\n";
 msg.push_back(msg_type);
 for(int i = 0;i < vec.size();i++)
  {
    adduint64(vec[i], msg); 
  }
std::cout << "##### Exiting ConvertVetorIntoMessage ##### \n\n"; 
}

int ArithToABYShareGenerator(std::vector<std::uint64_t> Relu_Arith_Shares, std::vector<std::uint64_t>& Relu_Public_Shares, std::vector<std::uint64_t>& Relu_Private_Shares) {
  auto start = std::chrono::high_resolution_clock::now();
  std::vector<std::uint8_t> otherPartySyncMessage{(std::uint8_t)OtherPartySync};
  try {
    comm_layer->send_message(1 - my_id, otherPartySyncMessage);
  }
  catch (std::exception& e) {
    std::cerr << "Error while establishing connection with other party: " << e.what() << std::endl;
    return EXIT_FAILURE;
  }

  while (!OtherPartySync_Flag) {
    std::cout << ".";
    boost::this_thread::sleep_for(boost::chrono::milliseconds(200));
  }
  auto end = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

  std::ofstream ackStatsFile;
  const std::string baseDirectory = (std::string)std::getenv("BASE_DIR");
  std::string filepath = baseDirectory + "/build_debwithrelinfo_gcc/stats/ackStats" + std::to_string(my_id);
  ackStatsFile.open(filepath, std::ios_base::app);
  if (!ackStatsFile.is_open()) {
    std::cerr << "Error: Unable to open filepath." << std::endl;
  }
  ackStatsFile << "Acknowledgement message for ArithToABY @ S" << my_id << " for ArithToABY: " << duration.count() << "\n" << std::endl;
  ackStatsFile.close();

  std::size_t len = Relu_Arith_Shares.size();

  std::vector<std::uint8_t> Relu_ABY_Public_Shares;

  GeneratePrivateShares_ABY(Relu_Arith_Shares, Relu_Private_Shares);

  Relu_Public_Shares.resize(len);
  ParallelAddition(Relu_Arith_Shares, Relu_Private_Shares, Relu_Public_Shares, 2);

  ConvertVetorIntoMessage(Relu_Public_Shares, Relu_ABY_Public_Shares, (std::uint8_t)ArithToABY);

  try {
    comm_layer->send_message(1 - my_id, Relu_ABY_Public_Shares);
  }
  catch (std::runtime_error& e) {
    std::cerr << "Error occurred while sending public shares to: " << 1 - my_id << std::endl;
    return EXIT_FAILURE;
  }

  while (!ArithToABY_Flag) {
    std::cout << ".";
    boost::this_thread::sleep_for(boost::chrono::milliseconds(200));
  }

  ParallelAddition(Relu_Public_Shares, Relu_Public_Shares_Other_Party, Relu_Public_Shares, 2);

  return 0;
}


// int WriteSharesToFile(std::string filepath, std::vector<std::uint64_t>& Relu_shares, std::size_t rows, std::size_t cols) {
//   std::cout << "Entered WriteSharesToFile." << std::endl;
//   std::ofstream ReluOutputFile;
//   try {
//     ReluOutputFile.open(filepath);
//     if (!ReluOutputFile) {
//       std::cerr << "Error: Error opening ReLU output file." << std::endl;
//     }
//   } catch (std::exception& error) {
//     std::cerr << "Error: Error opening ReLU output share file: " << error.what() << std::endl;
//   }
//   std::size_t rowsABYShares = Relu_shares[0];
//   std::size_t colsABYShares = Relu_shares[1];

//   // Verifying that the dimensions of the arithmetic shares and the ABY2.0 shares match
//   if ((rows != rowsABYShares) || (cols != colsABYShares)) {
//     std::cerr << "Dimensions of the arithmetic shares and the ABY shares do not match." << std::endl;
//     return EXIT_FAILURE;
//   }

//   ReluOutputFile << rows <<  " " << cols << std::endl;
//   for (int index = 2; index < Relu_shares.size(); index++) {
//     ReluOutputFile << Relu_shares[index] << std::endl;
//   }

//   if (ReluOutputFile.eof()) {
//     ReluOutputFile.close();
//   }
//   std::cout << "Exited WriteSharesToFile.\n" << std::endl;
//   return 0;
// }

int WriteSharesToFile(std::string filepath, std::vector<std::uint64_t>& Relu_public_shares, std::vector<std::uint64_t>& Relu_private_shares, std::size_t rows, std::size_t cols) {
  std::cout << "Entered WriteSharesToFile." << std::endl;
  std::ofstream ReluOutputFile;
  try {
    ReluOutputFile.open(filepath);
    if (!ReluOutputFile) {
      std::cerr << "Error: Error opening ReLU output file." << std::endl;
    }
  } catch (std::exception& error) {
    std::cerr << "Error: Error opening ReLU output share file: " << error.what() << std::endl;
  }
  std::size_t rowsABYShares = Relu_public_shares[0];
  std::size_t colsABYShares = Relu_public_shares[1];

  // Verifying that the dimensions of the arithmetic shares and the ABY2.0 shares match
  if ((rows != rowsABYShares) || (cols != colsABYShares)) {
    std::cerr << "Dimensions of the arithmetic shares and the ABY shares do not match." << std::endl;
    return EXIT_FAILURE;
  }

  ReluOutputFile << rows <<  " " << cols << std::endl;
  for (int index = 2; index < Relu_public_shares.size(); index++) {
    ReluOutputFile << Relu_public_shares[index] << " " << Relu_private_shares[index] << std::endl;
  }

  if (ReluOutputFile.eof()) {
    ReluOutputFile.close();
  }
  std::cout << "Exited WriteSharesToFile.\n" << std::endl;
  return 0;
}


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
    file2 << "3PC_ArithToABY: \n";
    file2 << "RSS - " << rss << " kB\n";
    file2 << "Shared Memory - " << shared_mem << " kB\n";
    file2 << "Private Memory - " << rss - shared_mem << "kB\n";
    file2.close();
  }
}

struct Options {
  std::string current_path;
  MOTION::Communication::tcp_parties_config tcp_config;
  std::size_t my_id;

  std::string Arith_Shares_File;
  std::string ABY_Shares_File;
  std::size_t fractional_bits;
};

std::optional<Options> parse_program_options(int argc, char* argv[]) {
  Options options;
  boost::program_options::options_description desc("Allowed options");
  // clang-format off
  desc.add_options()
    ("help,h", po::bool_switch()->default_value(false),"produce help message")
    ("party_id", po::value<std::size_t>(), "Party id")
    ("party", po::value<std::vector<std::string>>()->multitoken(),
     "(party id, IP, port), e.g., --party 1,127.0.0.1,7777")
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

  options.my_id = vm["party_id"].as<std::size_t>();
  options.current_path = vm["current-path"].as<std::string>();
  options.fractional_bits = vm["fractional-bits"].as<std::size_t>();
  fractional_bits = options.fractional_bits;
  std::cout<<"Fractional bits: "<< options.fractional_bits<<std::endl;
  // clang-format on;

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

  const std::string baseDirectory = (std::string)std::getenv("BASE_DIR");

  options.tcp_config.resize(2);
  options.tcp_config[id0] = conn_info0;
  options.tcp_config[id1] = conn_info1;

  options.Arith_Shares_File = baseDirectory + "/build_debwithrelinfo_gcc/server" + std::to_string(options.my_id) + "/outputshare_" + std::to_string(options.my_id);
  options.ABY_Shares_File = baseDirectory + "/build_debwithrelinfo_gcc/server" + std::to_string(options.my_id) + "/outputshare_" + std::to_string(options.my_id);
return options;
}

// Method which is called with received messages of the type the handler is
// registered for and the id of the sending party.
// This method may be called concurrently with different values of party_id.
// The client is responsible for the necessary synchronization,see  message_handler.h for more info
// for this reason we have to use synchronization on our own
// virtual void received_message(std::size_t party_id, std::vector<std::uint8_t>&& message) = 0;
// Note that above is a virtual function
class TestMessageHandler : public MOTION::Communication::MessageHandler
{
  void received_message(std::size_t party_id, std::vector<std::uint8_t>&& message) override
  {
    auto msg_type = +message[0];
    std::cout << "Message type: " << msg_type << "\n\n";

    switch (msg_type)
    {
      case OtherPartySync: // Received acknowledgement from other party
        if (party_id == (1 - my_id))
        {
          std::cout << "Message received from other party: " << 1 - my_id << ", Message type: OtherPartySync." << std::endl;
          ConvertMessageIntoVector(message, Relu_Public_Shares_Other_Party);
          OtherPartySync_Flag = true;
        }
        else {
          std::cout << "Message received from unknown party: " << party_id << ", Message type: OtherPartySync." << std::endl;
          return;
        }
        break;
      case ArithToABY: // Received public shares from other_party
        if (party_id == (1 - my_id))
        {
          std::cout << "Message received from other party: " << 1 - my_id << ", Message type: ArithToABY." << std::endl;
          ConvertMessageIntoVector(message, Relu_Public_Shares_Other_Party);
          ArithToABY_Flag = true;
        }
        else {
          std::cout << "Message received from unknown party: " << party_id << ", Message type: ArithToABY." << std::endl;
          return;
        }
        break;
    } // end case
  }
};

int main(int argc, char* argv[]) {
  auto start = high_resolution_clock::now();
  auto options = parse_program_options(argc, argv);
  std::cout << "My id: " << options->my_id << std::endl;
  int WriteToFiles = 1;
  my_id = options->my_id;

  std::vector<std::uint64_t> Relu_Arith_Shares;
  std::vector<std::uint64_t> Relu_Public_Shares, Relu_Private_Shares;

  // ******** Setting up the communication layer for sending and receiving messages ***********
  auto startComm = std::chrono::high_resolution_clock::now();

  try {
    MOTION::Communication::TCPSetupHelper helper(options->my_id, options->tcp_config);
    comm_layer = std::make_unique<MOTION::Communication::CommunicationLayer>(options->my_id, helper.setup_connections());
  }
  catch (std::runtime_error& e) {
    std::cerr << "Error occurred during connection setup: " << e.what() << "\n";
    return EXIT_FAILURE;
  }

  try {
    comm_layer->start();
  }
  catch (std::runtime_error& e) {
    std::cerr << "Error occurred while starting the communication layer: " << e.what() << "\n";
    return EXIT_FAILURE;
  }
  auto endComm = std::chrono::high_resolution_clock::now();
  
  comm_layer->register_fallback_message_handler([](auto party_id) { return std::make_shared<TestMessageHandler>(); });
  
  std::ofstream ackStatsFile;
  const std::string baseDirectory = (std::string)std::getenv("BASE_DIR");
  std::string filepath = baseDirectory + "/build_debwithrelinfo_gcc/stats/ackStats" + std::to_string(my_id);
  ackStatsFile.open(filepath, std::ios_base::app);
  if (!ackStatsFile.is_open()) {
    std::cerr << "Error: Unable to open filepath." << std::endl;
  }
  ackStatsFile << "Starting communication layer @ S" << my_id << " for ArithToABY: " << std::chrono::duration_cast<std::chrono::milliseconds>(endComm - startComm).count() << std::endl;
  ackStatsFile.close();


  ReadSharesIntoVec(options->Arith_Shares_File, Relu_Arith_Shares);

  std::size_t rows = Relu_Arith_Shares[0];
  std::size_t cols = Relu_Arith_Shares[1];

  InitializeModuloPrimeOps();

  // Generate ABY2.0 shares from arithmetic shares
  ArithToABYShareGenerator(Relu_Arith_Shares, Relu_Public_Shares, Relu_Private_Shares);

  WriteSharesToFile(options->ABY_Shares_File, Relu_Public_Shares, Relu_Private_Shares, rows, cols);

  comm_layer->shutdown();  
  auto stop = high_resolution_clock::now();
  auto duration = duration_cast<milliseconds>(stop - start);

  std::cout << "Duration: " << duration.count() << std::endl;

  testMemoryOccupied(WriteToFiles, options->my_id, options->current_path);

  std::string timeDetails = options->current_path + "/" + "AverageTimeDetails" + std::to_string(my_id);
  std::string memoryDetails = options->current_path + "/" + "MemoryDetails" + std::to_string(my_id);

  std::ofstream timeDetailsFile;
  timeDetailsFile.open(timeDetails, std::ios_base::app);
  if (!timeDetailsFile.is_open()) {
    std::cerr << "Error: Unable to open the AverageTimeDetails file." << std::endl;
  } else {
    timeDetailsFile << duration.count() << "\n";
  }
  timeDetailsFile.close();

  std::ofstream memoryDetailsFile;
  memoryDetailsFile.open(memoryDetails, std::ios_base::app);
  if (!memoryDetailsFile.is_open()) {
    std::cerr << "Error: Unable to open the MemoryDetails file." << std::endl;
  } else {
    memoryDetailsFile << "Execution Time - " << duration.count() << "\n\n";
  }
  memoryDetailsFile.close();

  return EXIT_SUCCESS;
}