//./bin/01_Msg_Generation_with_Id
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
  std::string arith_shares_file;
  std::string aby_shares_file;
  std::size_t fractional_bits;
};

template <typename E>
std::uint64_t RandomNumDistribution(E& engine) {
  std::uniform_int_distribution<unsigned long long> distribution(
      std::numeric_limits<std::uint64_t>::min(), std::numeric_limits<std::uint64_t>::max());
  return distribution(engine);
}

std::optional<Options> parse_program_options(int argc, char* argv[]) {
  Options options;
  boost::program_options::options_description desc("Allowed options");
  // clang-format off
  desc.add_options()
    ("help,h", po::bool_switch()->default_value(false),"produce help message")
    ("fractional-bits", po::value<std::size_t>()->default_value(13), "Number of fractional bits");
 
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
  msg.erase(msg.begin()+0);
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

//Generate private shares required for to create ABY
//Input : Arithmatic share vector
int GeneratePrivateShares_ABY(std::vector<uint64_t> &vec, std::vector<uint64_t> &private_share)
{
std::cout << "\n  #####Entered GeneratePrivateShares_ABY ... ##### \n";
int num_r = vec[0];
int num_c = vec[1];
private_share.push_back(vec[0]);
private_share.push_back(vec[1]);
  try {
    for (int i = 0; i < num_r*num_c; i++) {
      std::random_device rd;
      std::mt19937 gen(rd());
      std::uint64_t t = RandomNumDistribution(gen);
      private_share.push_back(t);
      //std::cout << private_share[i] << "\n";
    }
  } catch (std::exception& e) {
    std::cerr << "Error during image share generation: " << e.what() << std::endl;
    return EXIT_FAILURE;
  }
  std::cout << "##### Exiting GeneratePrivateShares_ABY ... ##### \n\n";
  
  return 1;
}

//addition is performed parallelly ans = op1 + op2
//offset from which element the addition operation has to be performed
void parallel_add(std::vector<std::uint64_t> &op1, std::vector<std::uint64_t> &op2, std::vector<std::uint64_t> &ans, int offset = 2)
{
std::cout << "##### Entered parallel_add... ##### \n" ;
auto op1_begin = op1.begin();
auto op1_end = op1.end();
auto op2_begin = op2.begin();
auto ans_begin = ans.begin();
if (offset)
   {
    for(int i = 0; i<offset;i++)
       ans[i] = op1[i];
   }
advance(op1_begin, offset);
advance(op2_begin, offset);
advance(ans_begin, offset);
__gnu_parallel::transform(op1_begin, op1_end, op2_begin, ans_begin, std::plus{});

std::cout << "##### Exiting parallel_add... ##### \n\n";
}

void ParallelVectorHadamardMult(std::vector<std::uint64_t> &op1, std::vector<std::uint64_t> &op2, std::vector<std::uint64_t> &ans, int offset = 2)
{
std::cout << "#### Entered ParallelMatrixMult... ##### \n" ;
auto op1_begin = op1.begin();
auto op1_end = op1.end();
auto op2_begin = op2.begin();
auto ans_begin = ans.begin();
if (offset)
   {
    for(int i = 0; i<offset;i++)
       ans[i] = op1[i];
   }
advance(op1_begin, offset);
advance(op2_begin, offset);
advance(ans_begin, offset);
__gnu_parallel::transform(op1_begin, op1_end, op2_begin, ans_begin, std::multiplies{});

std::cout << "##### Exiting ParallelMatrixMult ##### \n\n";
}

int MatrixMultiplication(std::vector<uint64_t> &a,std::vector<uint64_t> &b, std::vector<uint64_t> &ans)
{   
  std::cout << "##### Entering MatrixMultiplication ...#####\n";
  if(a[1]!=b[0])
    {
      std::cerr<<"Error : matrix multiplication. Num. of cols in A, Num. of rows in B are not equal." << a[1] << " , "<< b[0] <<"\n";
      return -1;
    }
  //Note that a and b are 2 Dimension matrices but stored in 1Dim vectors
  ans.resize(a[0]*b[1]+2);
  std::vector< std::vector<std::uint64_t>> M1(a[0], std::vector<std::uint64_t>(a[1], 0));
  std::vector< std::vector<std::uint64_t>> M2(b[0], std::vector<std::uint64_t>(b[1], 0));
  // std::vector< std::vector<std::uint64_t>> M3(a[0], std::vector<std::uint64_t>(b[1], 0));
  int n_row = a[0];
  int n_col = b[1];
  int n_mult = a[1];
  ans[0] = a[0]; ans[1] = b[1];
  int ind = 0;
  //copy a 1-D vector into M1 2-D matrix
  for(int i = 0; i< a[0];i++)
   {
      for(int j = 0; j<a[1]; j++)
        {
          M1[i][j] = a[ind+2];
          ind++;
          //std::cout << M1[i][j] << " , ";
        }
        //std::cout << "\n";
   }
    
  ind = 0;
  //copy b 1-D vector into M2 2-D matrix
  for(int i = 0; i< b[0];i++)
      for(int j = 0; j<b[1]; j++)
        {
          M2[i][j] = b[ind+2];
          ind++;
        }
  ind = 0;
  for(int i = 0; i < n_row; i++)
     {
      for(int j = 0; j < n_col; j++)
      {
        for(int k = 0; k < n_mult; k++)
        { 
          //  M3[i][j] = M3[i][j] + M1[i][k]*M2[k][j];
           ans[ind+2] = ans[ind+2] + M1[i][k]*M2[k][j]; 
        }
        // std::cout << M3[i][j] << " , ";
        ind = ind + 1;   
      }
      //std::cout << "\n";
     }
  std::cout << "##### Exiting matrix multiplication. ##### \n \n";
  return 1;
}

int main(int argc, char* argv[]) {
  
  auto options = parse_program_options(argc, argv);

  if (!options.has_value()) {
    std::cerr<<"No options given.\n";
    return EXIT_FAILURE;
  }
  
  int my_id = 0;
  int WriteToFiles = 1;
  
  std::cout << "My party id: " << my_id << "\n";
  
  //  if (ReadSharesIntoVec(options->arith_shares_file, X_arith))
  //   std::cout<< "complted Reading Arithmatic shares \n";
  //  else std::cout << "Could not read Arithmatic shares into a vector \n";
  //  std::cout << "X_arith.size() : " <<X_arith.size() << "\n";
  //  GeneratePrivateShares_ABY(X_arith, X_ABY_private);
   
  // X_ABY_public.resize(X_arith.size());
  //  X_ABY_public[0] = X_ABY_private[0];
  //  X_ABY_public[1] = X_ABY_private[1];
  //  auto Delta_begin = X_ABY_public.begin();
   
  //  auto delta_begin = X_ABY_private.begin();
  //  auto delta_end = X_ABY_private.end();
  //  auto arith_begin = X_arith.begin();
  //  advance(Delta_begin, 2);
  //  advance(delta_begin, 2);
  //  advance(arith_begin, 2);
  // __gnu_parallel::transform(delta_begin, delta_end, arith_begin, Delta_begin, std::plus{});
  // parallel_add(X_ABY_private, X_arith, X_ABY_public, 2);
  // for(int i = 0; i < X_ABY_public.size(); i++)
  //    {
  //     std::cout << X_arith[i] << ",   " <<  X_ABY_public[i] << ",   "<< X_ABY_private[i] << "\n";
  //    }
  //  ConvertVetorIntoMessage(X_ABY_public, msg_ABY_shares_for_mult);
  //  ConvertMessageIntoVector(msg_ABY_shares_for_mult, for_test);
  //  for(int i = 0; i<for_test.size();i++)
  //    std::cout << for_test[i] << "\n";
  std::vector<std::uint64_t> m1;
  std::vector<std::uint64_t> m2;
  std::vector<std::uint64_t> m3;
  m1.push_back(2);
  m1.push_back(4);
  m2.push_back(4);
  m2.push_back(2);
  for(int i=2; i<10; i++)
  {
    m1.push_back(i-2);
    m2.push_back(1);
    m3.push_back(0);
  } 
  MatrixMultiplication(m1, m2, m3);
  std::cout << "Multiplication output \n";
  for(int i = 0; i<m3.size(); i++)
  {
    std::cout << m3[i] << ",  " ;
  }
  // __gnu_parallel::transform(m1.begin(), m1.end(), m2.begin(), m3.begin(), std::multiplies{});
  // for(int i=0; i<5; i++)
  // {
  //   std::cout << m3[i];
  // } 
   std::cout << "\n";
  
  return EXIT_SUCCESS;
}