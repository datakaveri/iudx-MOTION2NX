#ifndef FUNCTIONS_CNN_NN_2_H
#define FUNCTIONS_CNN_NN_2_H

#pragma oncep_config
#include <bits/stdc++.h>
#include <filesystem>
#include <fstream>
#include <random>
#include <regex>
#include <stdexcept>
#include <unistd.h>
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
#include "utility/linear_algebra.h"
#include "utility/new_fixed_point.h"

#include <boost/chrono.hpp>
#include <boost/thread/thread.hpp>
#include <chrono>

#include "GlobalVar_3PC_CNN_NN_2.h"

using namespace std::chrono;

struct Options {
  std::size_t my_id;
  std::uint16_t my_port;
  std::size_t layers;
  std::vector<int> layer_types;
  std::size_t fractional_bits;
  MOTION::Communication::tcp_parties_config tcp_config;
  std::string current_path;
};

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
std::uint64_t RandomNumOverOddRing(E& engine, int l = 0, std::uint64_t p = std::numeric_limits<std::uint64_t>::max()-1) {
  std::uniform_int_distribution<unsigned long long> distribution(
      l, p);
  return distribution(engine);
}

void print_message(std::vector<std::uint8_t>& message) {
  for (auto i = 0; i < message.size(); i++) {
    std::cout << std::hex << (int)message[i] << " ";
  }
  return;
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

void AddVectorsOverL(std::vector<uint64_t>& a, std::vector<uint64_t>& b, std::vector<uint64_t>& ans, std::size_t len )
{
  std:: cout << " AddVectorsOverL \n ";
  for(int i = 0;i < len; i++) 
  {
    ans[i] = a[i] + b[i];
    // std::cout << ans[i] << "\n";
  }
    
}

inline std::uint64_t WrapAround(std::uint64_t a, std::uint64_t b) {
  return (a > LMINUS_ONE - b);} 

void WrapAround(std::vector<std::uint64_t> &a0, std::vector<std::uint64_t> &a1, std::vector<std::uint64_t> &ans, std::size_t len = 1)
{
    std::cout << "Entered WrapAround \n ";
    for(int i = 0; i<len; i++)
    {
        ans[i] = WrapAround(a0[i], a1[i]);
    }
}

void GenerateSharesOverL(std::vector<uint64_t>& a0, std::vector<uint64_t>& a1, std::vector<uint64_t>& a, std::size_t len)
{
std:: cout << " Entered GenerateSharesOverL \n";
std::random_device rd;
std::mt19937 gen(rd());
for(int i = 0; i < len; i++)
  {
    a0[i] = RandomNumDistribution(gen);
    a1[i] = a[i] - a0[i];
  }
}

void GenerateLSBSharesOverL(std::vector<std::uint64_t> &lsb_shares_x0, std::vector<std::uint64_t> &lsb_shares_x1, std::vector<std::uint64_t> &x, std::size_t len = 1)
{
std:: cout << " Entered GenerateLSBSharesOverL \n";
std::random_device rd;
std::mt19937 gen(rd());
//bit_shares_x1[i*BIT_SIZE + k] = bt - bit_shares_x0[i*BIT_SIZE + k]
for(int i = 0; i< len; i++)
 {
   lsb_shares_x0[i] = RandomNumDistribution(gen);
   auto lsb = x[i] & 1;
  //  std :: cout << "lsb of : " << std::hex << x[i] << " is : " << lsb << "\n"; 
  //  std :: cout << "lsb of : " << std::hex << x[i] << " is : " << x[i]%2 << "\n"; 
   std::uint64_t temp = lsb * (1 << FIXED_POINT);
   lsb_shares_x1[i] = temp - lsb_shares_x0[i];
   if (lsb_shares_x0[i] + lsb_shares_x1[i] != temp)
       std::cout << "Error in GenerateLSBSharesOverL" << lsb_shares_x0[i] + lsb_shares_x1[i] << " , " << temp <<"\n";
   //std::cout << lsb_shares_x0[i] + lsb_shares_x1[i] << " , " << temp <<"\n";
 }
}

void GenerateBitSharesOverPrime(std::vector<std::uint64_t> &bit_shares_x0, std::vector<std::uint64_t> &bit_shares_x1, std::vector<std::uint64_t> &x, std::size_t len = 1)
{
std:: cout << " Entered GenerateBitSharesOverPrime \n";
std::uint64_t t;
std::random_device rd;
std::mt19937 gen(rd());
//bit_shares_x1[i*BIT_SIZE + k] = bt - bit_shares_x0[i*BIT_SIZE + k]
for(int i = 0; i< len; i++)
{
  for(int k = 0; k < BIT_SIZE; k++)
  {
    t = RandomNumOverPrime(gen);
    bit_shares_x0[i*BIT_SIZE + k] = t;
    //Note that we are starting from MSB so that in PrivateCompare, for loop starts k = 0, k++; instead k = 64 and k-- 
    auto bt = (x[i] >> BIT_SIZE-1 - k) & 1; 
    //bit_shares_x1[i*BIT_SIZE + k] = bt - bit_shares_x0[i*BIT_SIZE + k]
    bit_shares_x1[i*BIT_SIZE + k] = subModPrime[bt][t];
  }
}
}

void AddModuloOdd(std::vector<std::uint64_t> &a0, std::vector<std::uint64_t> &a1, std::vector<std::uint64_t> &ans, std::size_t len = 1)
{
  std::cout << "Entered AddModuloOdd \n";
for(int i = 0; i < len; i++)
 {
  if ((a0[i] == LMINUS_ONE) and (a1[i] == LMINUS_ONE))
      ans[i] = 0;
  else 
    
    {
    ans[i] = (a0[i] + a1[i] + WrapAround(a0[i], a1[i])) % LMINUS_ONE;
    //std::cout << "a0 : " << a0[i] << ", a1 : " << a1[i] << ", ans : " << ans[i] <<"\n";
 }
 
}
}
//ans = a0 - a1
void SubtractModuloOdd(std::vector<std::uint64_t> &a0, std::vector<std::uint64_t> &a1, std::vector<std::uint64_t> &ans, std::size_t len = 1)
{ 
    std::cout << "Entered SubtractModuloOdd \n";
    std::vector<std::uint64_t> temp(len, 0);
    for(int i = 0; i < len; i++)
    {
      temp[i] = LMINUS_ONE - a1[i];
      //std::cout << "input : " << a1[i] << " , Negation : " << temp[i] << ", input + negation : " << temp[i]+a1[i] << "\n";
    }
    AddModuloOdd(a0, temp, ans, len);
}

void GenerateModuloOddShares(std::vector<std::uint64_t> &x0, std::vector<std::uint64_t> &x1, std::vector<std::uint64_t> &x, std::size_t len = 1)
{ 
  std::cout << " Entered GenerateModuloOddShares \n";
  std::random_device rd;
  std::mt19937 gen(rd()); 
  for(int i = 0; i < len; i++)
     // x0[i] = RandomNumOverOddRing(gen);
     x0[i] = 0;
  SubtractModuloOdd(x, x0, x1, len);
}

void GenerateRandVecModOdd(std::vector<std::uint64_t>& x, std::size_t len) {
  std::cout << "Entered GenerateRandVecModOdd" << std::endl;
  std::random_device rd;
  std::mt19937 gen(rd());
  for (int i = 0; i < len; i++) {
    // x[i] = RandomNumOverOddRing(gen);
    x[i] = 0;
  }
}

//ReadSharesIntoVec : Reads arithmatic shares from file_path into a vector
//first 2 values are dimensions, remaning are share values
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

//WriteABYToFile : Write shares into a file specified as the third argument
//Input : Publicshares, Private shares and file path
//Output : After writing to file succefully returns zero
int WriteABYToFile(std::vector<uint64_t>&pub, std::vector<uint64_t>&pri, std::string file_path)
{
std::ofstream output_file;
std::cout << "##### Entered  WriteABYToFile ...##### \n";
  if (pub.size() != pri.size())
     {
      std::cout << " The number of shares in public and priveate are not equal.\n";
      return -1;
     }

  try {
    output_file.open(file_path);
    if (!output_file) {
      std::cerr << "Unable to open file to write ABY.\n";
      throw std::ifstream::failure("Error opening ABY share file to write.");
    }
  }
  catch(std::exception& e){
      std::cerr<<"Error during opening ABY share file: "<<e.what()<<std::endl;
      return EXIT_FAILURE;
    }
  output_file << pub[0] << " " << pub[1] << std::endl;
  for(int i = 2; i < pub.size(); i++)
     {
      output_file << pub[i] << std::endl;
      output_file << pri[i] << std::endl;
     }
  std::cout << "##### Exiting  WriteABYToFile ##### \n\n";
  return 0;
}
//WriteArithToFile : Write shares into a file specified as second argument
//Input : Arith shares and file path
//Output : After writing to file succefully returns zero
int WriteArithToFile(std::vector<uint64_t>&pub, std::string file_path)
{
std::ofstream output_file;
std::cout << "##### Entered  WriteArithToFile ...##### \n";
  

  try {
    output_file.open(file_path);
    if (!output_file) {
      std::cerr << "Unable to open file to write ABY.\n";
      throw std::ifstream::failure("Error opening ABY share file to write.");
    }
  }
  catch(std::exception& e){
      std::cerr<<"Error during opening ABY share file: "<<e.what()<<std::endl;
      return EXIT_FAILURE;
    }
  output_file << pub[0] << " " << pub[1] << std::endl;
  for(int i = 2; i < pub.size(); i++)
     {
      output_file << pub[i] << std::endl;
     }
  std::cout << "##### Exiting  WriteArithToFile ##### \n\n";
  return 0;
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
std::cout << "#####Entered GeneratePrivateShares_ABY ... ##### \n";
int num_r = vec[0];
int num_c = vec[1];
//Note that the first two values in any vector are the dimensions
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
  std::cout << "##### Exiting GeneratePrivateShares_ABY ##### \n\n";
  
  return 1;
}

//ConvertFileIntoMessage(): converts the content in the input file into 
//a message that can be send over socket
void ConvertFileIntoMessage(){
  std::cout<< "Entered ConvertFileIntoMessage... \n";
  int i;
}

//addition is performed parallelly ans = op1 + op2
//offset from which element the addition operation has to be performed
void ParallelAddition(std::vector<std::uint64_t> &op1, std::vector<std::uint64_t> &op2, std::vector<std::uint64_t> &ans, int offset = 2)
{
std::cout << "#### Entered ParallalAddition... ##### \n" ;
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

std::cout << "##### Exiting ParallalAddition ##### \n\n";
}

void ParallelSubtraction(std::vector<std::uint64_t> &op1, std::vector<std::uint64_t> &op2, std::vector<std::uint64_t> &ans, int offset = 2)
{
std::cout << "#### Entered ParallalSubtraction... ##### \n" ;
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
__gnu_parallel::transform(op1_begin, op1_end, op2_begin, ans_begin, std::minus{});

std::cout << "##### Exiting ParallalSubtraction ##### \n\n";
}
//MatrixMultiplication is performed parallelly ans = op1 * op2
//offset from which element the addition operation has to be performed
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

int HelperNodeSyncFunction() {
  while((!server0_ready_flag) || (!server1_ready_flag)) {
    std::cout<<".";
    boost::this_thread::sleep_for(boost::chrono::milliseconds(400));
  }

  // Sending acknowledgement message to server 0 and 1, after receiving the start message.
  std::cout<<"Sending acknowledgement message to server 0 and 1\n";
  std::vector<std::uint8_t> ack{(std::uint8_t)HelperNodeSync};
  try {
    comm_layer->send_message(server1, ack); 
    comm_layer->send_message(server0, ack);
  }
  catch (std::runtime_error& e) {
    std::cerr << "Error occurred while sending the ack message to server 0 or 1: " << e.what() << "\n";
    return EXIT_FAILURE;
  }
  std::cout<<"Sent acknowledgement message to server 0 and 1\n";
  return EXIT_SUCCESS;
}


//matrix Multiplication  ans = a * b
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
  //copy a, 1-D vector into M1 2-D matrix
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
  //copy b, 1-D vector into M2 2-D matrix
  for(int i = 0; i< b[0];i++)
      for(int j = 0; j<b[1]; j++)
        {
          M2[i][j] = b[ind+2];
          ind++;
        }
  ind = 0;
  //Acutual multiplication
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
  return 0;
}

// HadamardMatrixMultiplication: Computes the hadamard product of a and b, first two elements of a and b store the dimensions of the matrix
// Input  : Two vectors a and b of std::uint64_t, and an empty vector result of type std::uint64_t
// Output : Updates result to store the product with the first two elements being the dimensions of the matrix.
int HadamardMatrixMultiplication(std::vector<std::uint64_t>& a, std::vector<std::uint64_t>& b, std::vector<std::uint64_t>& result) {
  std::cout << "Entering HadamardMatrixMultiplication." << std::endl;
  // Checking for dimension match of matrices a and b
  if (a[0] != b[0]) {
    std::cerr << "Error: Hadamard multiplication. Number of rows in matrix A: " << a[0] << " do not match the number of rows in matrix B: " << b[0] << std::endl;
    return 0;
  } else if (a[1] != b[1]) {
    std::cerr << "Error: Hadamard multiplication. Number of columns in matrix A: " << a[1] << " do not match the number of columns in matrix B: " << b[1] << std::endl;
    return 0;
  }

  // Defining the matrix result size
  result.resize(a[0] * a[1] + 2);

  auto rows = a[0];
  auto cols = a[1];

  result[0] = rows;
  result[1] = cols;

  auto aBegin = a.begin();
  auto aEnd = a.end();
  advance(aBegin, 2);

  auto bBegin = b.begin();
  advance(bBegin, 2);

  auto resultBegin = result.begin();
  advance(resultBegin, 2);

  __gnu_parallel::transform(aBegin, aEnd, bBegin, resultBegin, std::multiplies{});
  std::cout << "Exiting HadamardMatrixMultiplicaton." << std::endl;
  return 1;
}

void OTGeneration(std::vector<uint64_t> &v1, std::vector<uint64_t> &v2, std::vector<uint64_t> &ot0, std::vector<uint64_t> &ot1)
{
  std::cout << " \n ###### Entered OTGeneration ..  ##### \n ";
  GeneratePrivateShares_ABY(v1, ot0);
  ot1.resize(ot0.size());
  ot1[0] = ot0[0];
  ot1[1] = ot0[1];
  for(int i =2; i<v1.size();i++)
     ot1[i] = v1[i] + v2[i] - ot0[i]; 
  std::cout << " ###### Exiting OTGeneration ..  ##### \n\n ";
}

int HelperArithMatrixMultiplication()
{
   // Waiting for all the private shares to be received 
    while(!X0_Priv_Share_Flag || !X1_Priv_Share_Flag || !Y0_Priv_Share_Flag || !Y1_Priv_Share_Flag)
    {
     std::cout<<".";
     boost::this_thread::sleep_for(boost::chrono::milliseconds(400)); 
    }
  
  std::vector<std::uint64_t> X0Y1, X1Y0;
  std::vector<std::uint64_t> OT_0, OT_1; 
  std::vector<std::uint8_t> OT0_msg, OT1_msg;

  if (HadamardMatrixMultiplication(X0_Priv, Y1_Priv, X0Y1))
      std::cout << "";
  else std::cout << "Could Not Perform Matrix multiplication \n ";
  std::cout << " \nX0Y1 \n";
  
  if (HadamardMatrixMultiplication(X1_Priv, Y0_Priv, X1Y0))
      std::cout << "";
  else std::cout << "Could Not Perform Matrix multiplication \n ";
  
  OTGeneration(X0Y1, X1Y0, OT_0, OT_1);
  ConvertVetorIntoMessage(OT_0, OT0_msg, OT);
  ConvertVetorIntoMessage(OT_1, OT1_msg, OT);

  try{
      comm_layer->send_message(server0, OT0_msg);
    }
  catch (std::runtime_error& e) {
      std::cerr << "Error while sending OT message to server 0: " << e.what() << "\n";
      return EXIT_FAILURE;
    }
  
  try{
      comm_layer->send_message(server1, OT1_msg);
    }
  catch (std::runtime_error& e) {
      std::cerr << "Error while sending OT to server 0: " << e.what() << "\n";
      return EXIT_FAILURE;
    }
return 0;
}
int SendToP0P1(std::vector<uint64_t> &p0, std::vector<uint64_t> &p1, uint8_t msg_type)
{
std::vector<std::uint8_t> p0_msg, p1_msg;
ConvertVetorIntoMessage(p0, p0_msg, msg_type);
ConvertVetorIntoMessage(p1, p1_msg, msg_type);
try{
      comm_layer->send_message(server0, p0_msg);
      comm_layer->send_message(server1, p1_msg);
    }
catch (std::runtime_error& e) {
      std::cerr << "Error occurred while sending message type " << msg_type << e.what() << "\n";
      return EXIT_FAILURE;
    }
return 0;
}

int PrivateCompare(std::vector<std::uint64_t>& c0, std::vector<std::uint64_t>& c1, std::vector<std::uint64_t>& beta_prime, int startRange, int endRange)
{
std::cout << "Entered PrivateCompare. \n";
std::uint64_t ind, k;
std::size_t len = c0.size()/BIT_SIZE;
for(int i = startRange; i < endRange;  i++)
  {
    for (k = 0; k < BIT_SIZE; k++)
    {
      ind = i*BIT_SIZE +k;
      if (AddModuloPrime(c0[ind], c1[ind]) == 0) //x>r
        {
          beta_prime[i] = 1;
          break;
        }
    }
 
  }
return 0;
}

int ShareConvert()
{
  std::cout << "Entered ShareConvert  \n";
  // send x0_bit, delta_0 to P0 and  x1_bit,delta_1 to P1
  // and call PrivateCompare_2() that is waiting receiv shares from P0 and P1

   //wait for a_tilede, a1_tilde from P0 and P1
   while((!SC0_flag) || (!SC1_flag))
      {
        std::cout<<".";
        boost::this_thread::sleep_for(boost::chrono::milliseconds(400));
      }
    
  //Received a0_tilde and a1_tilde from P0 and P1
  std::size_t len = a0_tilde.size();  
  std::vector<uint64_t> x(len, 0), delta(len, 0);
  std::vector<uint64_t> del0_odd(len, 0), del1_odd(len, 0); //delta shares to be sent to P0 and dP1
  std::vector<uint64_t> x0_pr_bit(len*BIT_SIZE, 0), x1_pr_bit(len*BIT_SIZE, 0); // Prime shares to be send to P0 and dP1

  AddVectorsOverL(a0_tilde, a1_tilde, x, len);
  WrapAround(a0_tilde, a1_tilde, delta, len);
  GenerateBitSharesOverPrime(x0_pr_bit, x1_pr_bit, x, len);
  GenerateModuloOddShares(del0_odd, del1_odd, delta, len);
  SendToP0P1(x0_pr_bit, x1_pr_bit, (uint8_t)SC_P);
  SendToP0P1(del0_odd, del1_odd, (uint8_t)SC_D);
 
  //Wait for the c0 and c1 from P0 and P1 to compute beta' in PrivateCompare
  while((!SC_PC0_flag) || (!SC_PC1_flag))
      {
        std::cout<<".";
        boost::this_thread::sleep_for(boost::chrono::milliseconds(400));
      }
  std::vector<std::uint64_t> betaP(len, 0);
  // PrivateCompare(c0, c1, betaP);
  
  int partition = len / 8;
  std::thread threadPC_0_1(PrivateCompare, std::ref(c0), std::ref(c1), std::ref(betaP), 0, partition);
  std::thread threadPC_0_2(PrivateCompare, std::ref(c0), std::ref(c1), std::ref(betaP), partition, 2 * partition);
  std::thread threadPC_0_3(PrivateCompare, std::ref(c0), std::ref(c1), std::ref(betaP), 2 * partition, 3 * partition);
  std::thread threadPC_0_4(PrivateCompare, std::ref(c0), std::ref(c1), std::ref(betaP), 3 * partition, 4 * partition);
  std::thread threadPC_0_5(PrivateCompare, std::ref(c0), std::ref(c1), std::ref(betaP), 4 * partition, 5 * partition);
  std::thread threadPC_0_6(PrivateCompare, std::ref(c0), std::ref(c1), std::ref(betaP), 5 * partition, 6 * partition);
  std::thread threadPC_0_7(PrivateCompare, std::ref(c0), std::ref(c1), std::ref(betaP), 6 * partition, 7 * partition);
  std::thread threadPC_0_8(PrivateCompare, std::ref(c0), std::ref(c1), std::ref(betaP), 7 * partition, len);
  
  threadPC_0_1.join();
  threadPC_0_2.join();
  threadPC_0_3.join();
  threadPC_0_4.join();
  threadPC_0_5.join();
  threadPC_0_6.join();
  threadPC_0_7.join();
  threadPC_0_8.join();

  //Generate odd shares for beta_p and send to P0, P1 
  std::vector<std::uint64_t> betaP0_odd(len, 0), betaP1_odd(len,0);
  GenerateModuloOddShares(betaP0_odd, betaP1_odd, betaP, len);
  SendToP0P1(betaP0_odd, betaP1_odd, (uint8_t)SC_PC);
  return 0;
}

int ComputeMSB() {
  std::cout << "Entered ComputeMSB." << "\n";

  // Wait for initial message from P0 and P1 and send acknowledgement to P0 and P1
  while ((!server0_compute_msb_ready_flag) || (!server1_compute_msb_ready_flag)) {
    std::cout << ".";
    boost::this_thread::sleep_for(boost::chrono::milliseconds(200));
  }

  std::cout<<"Sending acknowledgement message to server 0 and 1\n";
  std::vector<std::uint8_t> ack{(std::uint8_t)ComputeMSBReady};
  try {
    comm_layer->send_message(server1, ack); 
    comm_layer->send_message(server0, ack);
  }
  catch (std::runtime_error& e) {
    std::cerr << "Error occurred while sending the ack message to server 0 or 1: " << e.what() << "\n";
    return EXIT_FAILURE;
  }
  std::cout << "Sent message successfully to server 0 and server 1." << std::endl;

  if (MSB_len0[0] != MSB_len1[0]) {
    std::cerr << "Dimensions of the shares do not match." << std::endl;
  }

  // Step 1
  // Select x over odd ring
  std::size_t len = MSB_len0[0];      
  std::vector<std::uint64_t> xOverOdd(len, 0);
  std::vector<std::uint64_t> xOddShares0(len, 0), xOddShares1(len, 0);
  std::vector<std::uint64_t> xPrimeBitShares0(len * BIT_SIZE, 0), xPrimeBitShares1(len * BIT_SIZE, 0);
  std::vector<std::uint64_t> xLSBBitShares0(len, 0), xLSBBitShares1(len, 0);

  // Generating odd shares of x over L - 1 and generate random value of x over L - 1
  GenerateRandVecModOdd(xOddShares0, len);
  GenerateRandVecModOdd(xOddShares1, len);
  AddModuloOdd(xOddShares0, xOddShares1, xOverOdd, len);
  // Generating bit shares over p = 67
  GenerateBitSharesOverPrime(xPrimeBitShares0, xPrimeBitShares1, xOverOdd, len);
  // Generating LSB shares of x over L
  GenerateLSBSharesOverL(xLSBBitShares0, xLSBBitShares1, xOverOdd, len);

  // Send respective shares to P0 and P1
  // Sending odd shares
  SendToP0P1(xOddShares0, xOddShares1, (std::uint8_t)MSB_Odd);
  // Sending prime shares
  SendToP0P1(xPrimeBitShares0, xPrimeBitShares1, (std::uint8_t)MSB_P);
  // Sending LSB shares to party 0 and party 1
  SendToP0P1(xLSBBitShares0, xLSBBitShares1, (std::uint8_t)MSB_L_LSB);

  // Step 4-5
  // Private Compare @ P2, followed by generating shares of betaP and sending them
  // to P0 and P1 respectively
  while ((!MSB_PC_0_flag) || (!MSB_PC_1_flag)) {
    std::cout << ".";
    boost::this_thread::sleep_for(boost::chrono::milliseconds(400));
  }
  std::vector<std::uint64_t> betaP(len, 0);
  // PrivateCompare(MSB_c0, MSB_c1, betaP);
  int partition = len / 8;
  std::thread threadPC_0_1(PrivateCompare, std::ref(MSB_c0), std::ref(MSB_c1), std::ref(betaP), 0, partition);
  std::thread threadPC_0_2(PrivateCompare, std::ref(MSB_c0), std::ref(MSB_c1), std::ref(betaP), partition, 2 * partition);
  std::thread threadPC_0_3(PrivateCompare, std::ref(MSB_c0), std::ref(MSB_c1), std::ref(betaP), 2 * partition, 3 * partition);
  std::thread threadPC_0_4(PrivateCompare, std::ref(MSB_c0), std::ref(MSB_c1), std::ref(betaP), 3 * partition, 4 * partition);
  std::thread threadPC_0_5(PrivateCompare, std::ref(MSB_c0), std::ref(MSB_c1), std::ref(betaP), 4 * partition, 5 * partition);
  std::thread threadPC_0_6(PrivateCompare, std::ref(MSB_c0), std::ref(MSB_c1), std::ref(betaP), 5 * partition, 6 * partition);
  std::thread threadPC_0_7(PrivateCompare, std::ref(MSB_c0), std::ref(MSB_c1), std::ref(betaP), 6 * partition, 7 * partition);
  std::thread threadPC_0_8(PrivateCompare, std::ref(MSB_c0), std::ref(MSB_c1), std::ref(betaP), 7 * partition, len);
  
  threadPC_0_1.join();
  threadPC_0_2.join();
  threadPC_0_3.join();
  threadPC_0_4.join();
  threadPC_0_5.join();
  threadPC_0_6.join();
  threadPC_0_7.join();
  threadPC_0_8.join();

  for (int i = 0; i < len; i++) {
    betaP[i] = MOTION::new_fixed_point::encode<uint64_t, long double>(betaP[i], 13);
  }

  std::vector<std::uint64_t> betaP_0(len, 0), betaP_1(len, 0);
  GenerateSharesOverL(betaP_0, betaP_1, betaP, len);
  SendToP0P1(betaP_0, betaP_1, (std::uint8_t)MSB_PC);

  // Step 9
  // Hadamard product of ABY2.0 shares of gammaL and deltaL using helper node
  HelperArithMatrixMultiplication();

  return 0;
}

std::vector<std::uint64_t> multiplicate(std::vector<uint64_t>& w0, std::vector<uint64_t>& x0) {
  //z=(256*784 * 784*1)= 256*1
  if (w0.size()<=2 || x0.size()<=2) {
    std::cerr<<"Shares unavailable to perform computations. Weight shares size is "<<w0.size()<<"  and input shares size is "<<x0.size()<<std::endl;
    exit(1);
  }
    
  auto x0_begin = x0.begin();
  advance(x0_begin, 2);

  std::vector<std::uint64_t>z;// Output shares.
  z.push_back(w0[0]); // Output row size = Weights row size
  z.push_back(x0[1]); // Output column size =  Input column size

  std::vector<std::uint64_t>tempw;
  int count=2;
  for (int i = 0; i < w0[0]; i++) { 
    tempw.push_back(w0[0]);
    tempw.push_back(x0[1]);
    for (int k = 0; k < w0[1]; k++) {
      tempw.push_back(w0[count]);
      count++;
    }
    auto tempw_begin=tempw.begin();
    auto tempw_end=tempw.end();

    advance(tempw_begin, 2);
    __gnu_parallel::transform(tempw_begin, tempw_end, x0_begin, tempw_begin , std::multiplies{});
      
    std::uint64_t sum=0;
    for(int j=2;j<tempw.size();j++) {
      sum+=tempw[j];
    }
    z.push_back(sum);
    tempw.clear();
  }
  return z;
}

void operationsMatrixMultiplication() {   
  if (w0.size() <= 2 || x0.size() <= 2 || w1.size() <= 2 || x1.size() <= 2) {
    std::cerr<<"Shares unavailable to perform computations.\n";
    exit(1);
  }

  auto w0_begin = w0.begin(); 
  auto w1_begin = w1.begin(); 
  auto w0_end = w0.end();
  auto w1_end = w1.end();

  //to skip rows and columns 
  advance(w0_begin, 2);
  advance(w1_begin, 2);
  auto x0_begin = x0.begin(); 
  auto x1_begin = x1.begin(); 
  auto x0_end = x0.end();
  auto x1_end = x1.end();

  //to skip rows and columns 
  advance(x0_begin, 2);
  advance(x1_begin, 2);

  //delx0=delx0+delx1
  __gnu_parallel::transform(x0_begin, x0_end, x1_begin, x0_begin , std::plus{});
  std::cout<<"x0:"<<*x0_begin<<"\n";

  //delw0=delw0+delw1
  __gnu_parallel::transform(w0_begin, w0_end, w1_begin, w0_begin , std::plus{});
  std::cout<<"w0:"<<*w0_begin<<"\n";
  
  //z=(256*784 * 784*1)= 256*1
  std::vector<std::uint64_t>z = multiplicate(w0, x0);

  std::cout<<"Computed Z=w0.x0 of size "<<z.size()<<"\n";

  std::vector<std::uint64_t>r;
  r.resize(x0.size(),0);
  r[0]=w0[0];
  r[1]=x0[1];
  
  for(int i=2;i<z.size();i++)
  { 
    std::random_device rd;
    std::mt19937 gen(rd());
    auto temp=RandomNumDistribution(gen);
    r[i]=temp;
  }
  std::cout<<"Generated Random value R of size "<<r.size()<<std::endl;
  auto r_begin = r.begin();
  advance(r_begin, 2);

  auto z_begin = z.begin();
  auto z_end = z.end();
  advance(z_begin, 2);

  //z=z-r

  __gnu_parallel::transform(z_begin, z_end, r_begin, z_begin , std::minus{});

  for (int i=0;i<z.size();i++) {
    adduint64(z[i], msg_Z); //z=z-r  server1
    adduint64(r[i], msg_R); //r  server0
  }
  operations_done_flag = true; 
}

int ResetFlags(int type) {
  operations_done_flag = false;
  server0_ready_flag = false;
  server1_ready_flag = false;
  X0_Priv_Share_Flag = false;
  X1_Priv_Share_Flag = false;
  Y0_Priv_Share_Flag = false;
  Y1_Priv_Share_Flag = false;
  relu_ready_flag_0 = false;
  relu_ready_flag_1 = false;
  reluABY_s0_flag = false;
  reluABY_s1_flag = false;
  SC0_flag = false;
  SC1_flag = false;
  SC_PC0_flag = false;
  SC_PC1_flag = false;
  a0_tilde.clear();
  a1_tilde.clear();
  c0.clear();
  c1.clear();
  X0_Priv.clear();
  X1_Priv.clear();
  Y0_Priv.clear();
  Y1_Priv.clear();
  MSB_len0.clear();
  MSB_len1.clear();
  MSB_c0.clear();
  MSB_c1.clear();
  server0_compute_msb_ready_flag = false;
  server1_compute_msb_ready_flag = false;
  MSB_PC_0_flag = false;
  MSB_PC_1_flag = false;
  server0_relu_ready_flag = false;
  server1_relu_ready_flag = false;

  Relu_X0_Priv.clear();
  Relu_X1_Priv.clear();
  Relu_Y0_Priv.clear();
  Relu_Y1_Priv.clear();

    flag = 0;
    x0.clear();
    w0.clear();
    x1.clear();
    w1.clear();
    msg_Z.clear();
    msg_R.clear();
    Conv_c1 = 1;
    Conv_c2 = 1;
    Conv_c3 = 1;
    Conv_c4 = 1;
    w_rows = 0;
    w_cols = 0;
    x_rows = 0;
    x_cols = 0;
  return EXIT_SUCCESS;
}

int DerivativeRelu() {
  ShareConvert();
  ComputeMSB();
  return 0;
}

int ReLU() {
  while (!relu_ready_flag_0 || !relu_ready_flag_1) {
    boost::this_thread::sleep_for(boost::chrono::milliseconds(200));
  }

  try {
    std::vector<std::uint8_t> ack{(std::uint8_t)ReluSync};
    comm_layer->send_message(0, ack);
    comm_layer->send_message(1, ack);
  }
  catch (std::exception& e) {
    std::cerr << e.what() << std::endl;
  }

  std::cout << "Entered ReLU." << std::endl;
  DerivativeRelu();
  X0_Priv_Share_Flag = false;
  X1_Priv_Share_Flag = false;
  Y0_Priv_Share_Flag = false;
  Y1_Priv_Share_Flag = false;

  // Wait for initial message from P0 and P1 and send acknowledgement to P0 and P1
  while ((!server0_relu_ready_flag) || (!server1_relu_ready_flag)) {
    std::cout << ".";
    boost::this_thread::sleep_for(boost::chrono::milliseconds(200));
  }

  std::cout<<"Sending Relu acknowledgement message to server 0 and 1\n";
  std::vector<std::uint8_t> ack{(std::uint8_t)ReluReady};
  try {
    comm_layer->send_message(server1, ack); 
    comm_layer->send_message(server0, ack);
  }
  catch (std::runtime_error& e) {
    std::cerr << "Error occurred while sending the ack message to server 0 or 1: " << e.what() << "\n";
    return EXIT_FAILURE;
  }

  while(!X0_Priv_Share_Flag || !X1_Priv_Share_Flag || !Y0_Priv_Share_Flag || !Y1_Priv_Share_Flag)
  {
    std::cout<<".";
    boost::this_thread::sleep_for(boost::chrono::milliseconds(400)); 
  }

  std::vector<std::uint64_t> X0Y1, X1Y0;
  std::vector<std::uint64_t> OT_0, OT_1; 
  std::vector<std::uint8_t> OT0_msg, OT1_msg;

  if (HadamardMatrixMultiplication(Relu_X0_Priv, Relu_Y1_Priv, X0Y1))
      std::cout << "";
  else std::cout << "Could Not Perform Matrix multiplication \n ";

  if (HadamardMatrixMultiplication(Relu_X1_Priv, Relu_Y0_Priv, X1Y0))
      std::cout << "";
  else std::cout << "Could Not Perform Matrix multiplication \n ";

  OTGeneration(X0Y1, X1Y0, OT_0, OT_1);
  ConvertVetorIntoMessage(OT_0, OT0_msg, Relu_OT);
  ConvertVetorIntoMessage(OT_1, OT1_msg, Relu_OT);

  try{
      comm_layer->send_message(server0, OT0_msg);
    }
  catch (std::runtime_error& e) {
      std::cerr << "Error while sending OT message to server 0: " << e.what() << "\n";
      return EXIT_FAILURE;
    }
  
  try{
      comm_layer->send_message(server1, OT1_msg);
    }
  catch (std::runtime_error& e) {
      std::cerr << "Error while sending OT message to server 1: " << e.what() << "\n";
      return EXIT_FAILURE;
    }

  std::cout << "Exited ReLU." << std::endl;
  return 0;
}

int MatrixMultAddition() {
  while (!operations_done_flag) {
    std::cout << ".";
    boost::this_thread::sleep_for(boost::chrono::milliseconds(100));
  }

  std::cout << "Sending (Z-R) of size: " << msg_Z.size() << " to party 1.\n";
  try {
    msg_Z.insert(msg_Z.begin(), (std::uint8_t)MatrixMulAddOpMessage);
    comm_layer->send_message(1, msg_Z);//z-r
  }
  catch (std::runtime_error& e) {
    std::cerr << "Error occurred while sending (Z-R) to server 1: " << e.what() << "\n";
    return EXIT_FAILURE;
  }

  std::cout<<"Sending R of size "<<msg_R.size()<<" to party 0.\n";
  try {
    msg_R.insert(msg_R.begin(), (std::uint8_t)MatrixMulAddOpMessage);
    comm_layer->send_message(0,msg_R);//r
  }
  catch (std::runtime_error& e) {
    std::cerr << "Error occurred while sending (R) to server 0: " << e.what() << "\n";
    return EXIT_FAILURE;
  } 

  return EXIT_SUCCESS;
}

int MatrixMultiplicationLayer() {
  HelperNodeSyncFunction();

  MatrixMultAddition();
  std::cout << "Matrix addition and multiplication done." << std::endl;
  return EXIT_SUCCESS;
}

int MatrixMultiplicationReluLayer() {
  HelperNodeSyncFunction();

  MatrixMultAddition();

  while ((!reluABY_s0_flag) || (!reluABY_s1_flag)) {
    boost::this_thread::sleep_for(boost::chrono::milliseconds(200));
  }

  try {
    std::vector<std::uint8_t> ackReluABY{(std::uint8_t)reluABYAck};
    comm_layer->send_message(0, ackReluABY);
    comm_layer->send_message(1, ackReluABY);
  }
  catch (std::exception& e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return EXIT_FAILURE;
  }
  std::cout<<"Sent acknowledgement message to server 0 and 1\n";

  ReLU();

  ResetFlags(0);
  
  return EXIT_SUCCESS;
}

std::vector<std::uint64_t> convolution(std::vector<std::uint64_t> input, std::vector<std::uint64_t> weights, std::uint64_t kernels, std::uint64_t channels,
  std::uint64_t rows, std::uint64_t cols, std::uint64_t pads[], std::uint64_t strides[], std::uint64_t img_rows,std::uint64_t img_cols) {

std::vector<std::vector<std::uint64_t>> kernel_segments;


std::cout<<"conv_kernels: "<<conv_kernels<<"\n";
std::cout<<"conv_channels: "<<conv_channels<<"\n";
std::cout<<"conv_rows: "<<conv_rows<<"\n";
std::cout<<"conv_cols: "<<conv_cols<<"\n";
std::cout<<"pads: "<<pads[0]<<" "<<pads[1]<<" "<<pads[2]<<" "<<pads[3]<<"\n";
std::cout<<"strides: "<<strides[0]<<" "<<strides[1]<<"\n";

std::cout<<"image_channels: "<<image_channels<<"\n";
std::cout<<"image_rows: "<<image_rows<<"\n";
std::cout<<"image_cols: "<<image_cols<<"\n";

for(int i=0;i<input.size();i++)
{
// std::cout<<input[i]<<" ";
}
std::cout<<"***************\n";

for(int i=0;i<weights.size();i++)
{
// std::cout<<weights[i]<<" ";
}
std::cout<<"\n";
std::cout<<"***************\n";




int temp = 0;
for (int i = 0; i < kernels; i++) {
auto first = weights.begin() + temp;
temp += channels * rows * cols;
auto last = weights.begin() + temp;
std::cout << temp << std::endl;
std::vector<std::uint64_t> kernel(first, last);
kernel_segments.push_back(kernel);
}

std::cout << "kernel_segments size : " << kernel_segments.size() << "\n";

std::vector<std::uint64_t> image;
for (int c = 0; c < channels; c++) {
for (int i = 0; i < pads[0]; i++) {
for (int j = 0; j < img_cols + pads[1] + pads[3]; j++) image.push_back(0);
}

for (int i = 0; i < img_rows; i++) {
for (int j = 0; j < pads[1]; j++) image.push_back(0);
for (int j = 0; j < img_cols; j++)
image.push_back(input[c * img_rows * img_cols + i * img_cols + j]);
for (int j = 0; j < pads[3]; j++) image.push_back(0);
}

for (int i = 0; i < pads[2]; i++) {
for (int j = 0; j < img_cols + pads[1] + pads[3]; j++) image.push_back(0);
}
}
  img_rows += pads[0] + pads[2];
  img_cols += pads[1] + pads[3];

  uint64_t output_chnls = kernels;
  uint64_t output_rows = (img_rows - rows + strides[0]) / strides[0];
  uint64_t output_columns = (img_cols - cols + strides[1]) / strides[1];

  std::vector<std::vector<std::uint64_t>> image_segments(output_rows * output_columns,
                            std::vector<std::uint64_t>(channels * rows * cols));

  for (unsigned int i = 0; i < output_rows; i++) {
    for (unsigned int j = 0; j < output_columns; j++) {
      unsigned int row_start = i * strides[0];
      unsigned int col_start = j * strides[1];

      for (unsigned int k = 0; k < channels; k++) {
        for (unsigned int l = 0; l < rows * cols; l++) {
          unsigned int row = row_start + l / cols;
          unsigned int col = col_start + l % cols;

          image_segments[i * output_columns + j][k * rows * cols + l] =
          image[k * img_rows * img_cols + row * img_cols + col];
        }
      }
    }
  }

  // std::cout << "kernel_segments[k].size(): " << kernel_segments[0].size() << "\n";
  std::vector<std::uint64_t> output(kernels * output_rows * output_columns);
  int j = 0;
  for (int k = 0; k < kernels; k++) {
    for (int i = 0; i < output_rows * output_columns; i++) {
      output[j] = (MOTION::matrix_multiply(1, kernel_segments[k].size(), 1, kernel_segments[k],
                              image_segments[i]))[0];
      j++;
    }
  }
  return output;
}

void operationsConvolution() {   
  //delx0=delx0+delx1
  __gnu_parallel::transform(x0.begin(), x0.end(), x1.begin(), x0.begin() , std::plus{});
  
  //delw0=delw0+delw1
  __gnu_parallel::transform(w0.begin(), w0.end(), w1.begin(), w0.begin() , std::plus{});

  std::vector<std::uint64_t>z=convolution(x0,w0,conv_kernels,image_channels,conv_rows,conv_cols,pads,strides,image_rows,image_cols);

  uint64_t output_chnls = conv_kernels;
  std::uint64_t output_rows = (image_rows - conv_rows + pads[0]+pads[2]+strides[0]) / strides[0];
  std::uint64_t output_cols = (image_cols - conv_cols + pads[1]+pads[3]+strides[1]) / strides[1];

  std::vector<std::uint64_t>r;
  r.resize(output_chnls*output_rows*output_cols,0);
  
  for (int i = 0; i < z.size(); i++) { 
    std::random_device rd;
    std::mt19937 gen(rd());
    auto temp = RandomNumDistribution(gen);
    r[i] = temp;
  }

  __gnu_parallel::transform(z.begin(), z.end(), r.begin(), z.begin() , std::minus{});

  //Final output:-
   std::cout<<"Final Output :- \n";
  adduint64(output_chnls,msg_Z);
  adduint64(output_rows,msg_Z);
  adduint64(output_cols,msg_Z);

  adduint64(output_chnls,msg_R);
  adduint64(output_rows,msg_R);
  adduint64(output_cols,msg_R);


  for(int i=0;i<z.size();i++) {  
  adduint64(z[i],msg_Z); //z=z-r  server1
  adduint64(r[i],msg_R); //r  server0
  }
  std::cout<<"\n";

  operations_done_flag = true; 
}


int ConvolutionOperation() {
  while (!operations_done_flag) {
    std::cout << ".";
    boost::this_thread::sleep_for(boost::chrono::milliseconds(100));
  }

  std::cout<<"Sending (Z-R) of size "<<msg_Z.size()<<" to party 1.\n";
  try {
    msg_Z.insert(msg_Z.begin(), (std::uint8_t)ConvolutionOpMessage);
    comm_layer->send_message(1, msg_Z);//z-r
  }
  catch (std::runtime_error& e) {
    std::cerr << "Error occurred while sending (Z-R) to server 1: " << e.what() << "\n";
    return EXIT_FAILURE;
  }

  std::cout << "Sending R of size: " << msg_R.size() << " to party 0.\n";
  try {
    msg_R.insert(msg_R.begin(), (std::uint8_t)ConvolutionOpMessage);
    comm_layer->send_message(0, msg_R); //r
  }
  catch (std::runtime_error& e) {
    std::cerr << "Error occurred while sending (R) to server 0: " << e.what() << "\n";
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

int ConvolutionReluLayer() {
  HelperNodeSyncFunction();

  std::cout << "Helper node sync done." << std::endl;

  ConvolutionOperation();

  std::cout << "Convolution done." << std::endl;

  while ((!reluABY_s0_flag) || (!reluABY_s1_flag)) {
    boost::this_thread::sleep_for(boost::chrono::milliseconds(200));
  }

  try {
    std::vector<std::uint8_t> ackReluABY{(std::uint8_t)reluABYAck};
    comm_layer->send_message(0, ackReluABY);
    comm_layer->send_message(1, ackReluABY);
  }
  catch (std::exception& e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return EXIT_FAILURE;
  }
  std::cout<<"Sent acknowledgement message to server 0 and 1\n";

  ReLU();

  ResetFlags(1);

  return EXIT_SUCCESS;
}

#endif