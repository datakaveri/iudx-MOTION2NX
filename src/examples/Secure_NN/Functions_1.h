#ifndef FUNCTIONS_1_H
#define FUNCTIONS_1_H

#pragma once
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

#include "GlobalVar_1.h"

using namespace std::chrono;


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
//******** END : RandomNumberGenerators***********************
//used at P0 and P1
void GenerateRandomPermutation(std::vector<std::uint64_t>& nums, std::size_t start,  std::size_t end, std::uint64_t seed = 1234) {
    // std::cout << " Entered GenerateRandomPermutation \n ";
    std::mt19937 gen(seed); // Initialize  with the given seed
    std::shuffle(nums.begin()+start, nums.begin()+end, gen); // Shuffle the vector with the seeded generator

    // std::cout << "A random permutation of the given set is: ";
    // for (int num : nums) {
    //     std::cout << num << " ";
    // }
    // std::cout << std::endl;
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
//****************************Operations on ODD (L-1)***********************
// If a+b > (2^64 -1) return 1
inline std::uint64_t WrapAround(std::uint64_t a, std::uint64_t b) {
  return (a > LMINUS_ONE - b);} 


void WrapAround(std::vector<std::uint64_t> &a0, std::vector<std::uint64_t> &a1, std::vector<std::uint64_t> &ans, std::size_t len)
{
    for(int i = 0; i<len; i++)
    {
        ans[i] = WrapAround(a0[i], a1[i]);
    }
}
uint64_t AddModuloOdd(uint64_t a, uint64_t b)
{
	

	if (a == LMINUS_ONE and b == LMINUS_ONE)
		return 0;
	else 
		return (a + b + WrapAround(a, b)) % LMINUS_ONE;
}


uint64_t SubtractModuloOdd(uint64_t a, uint64_t b)
{
	uint64_t temp = LMINUS_ONE - b;
	return AddModuloOdd(a, temp);
}

void AddModuloOdd(std::vector<std::uint64_t> &a0, std::vector<std::uint64_t> &a1, std::vector<std::uint64_t> &ans, std::size_t len = 1)
{
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
     x0[i] = RandomNumOverOddRing(gen);
  SubtractModuloOdd(x, x0, x1, len);
}
/************************************************************/

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





// To Generate Common Random vector at P0 and P1 over L
// Input : seed and no. of values to be generated
//output : Random vector of length len over L 
void CommonRandVectorOverL(std::vector<uint64_t>& q, int len, int seed)
{
  std::cout << "CommonRandVectorOverL \n ";
  std::mt19937 gen_com_L(seed); 
  for (int i = 0; i < len; i++)
  {
    q[i] = RandomNumDistribution(gen_com_L);
  }
}
// To Generate Common Random bit vector at P0 and P1 
// Input : seed and no. of values to be generated
//output : Random bit vector of length len 
void CommonRandBitVector(std::vector<uint64_t>& q, int len, int seed)
{
  std::cout << "CommonRandBitVector  \n";
  std::mt19937 gen_com_L(seed); 
  for (int i = 0; i < len; i++)
  {
    q[i] = RandomNumDistribution(gen_com_L) & 1;
  }
}
// c = a + b (addition over L)
void AddVectorsOverL(std::vector<uint64_t>& a, std::vector<uint64_t>& b, std::vector<uint64_t>& ans, std::size_t len )
{
  std:: cout << " AddVectorsOverL \n ";
  for(int i = 0;i < len; i++) ans[i] = a[i] + b[i];
    
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
    file2 << "3PC ReLU at Party 1: \n";
    file2 << "RSS - " << rss << " kB\n";
    file2 << "Shared Memory - " << shared_mem << " kB\n";
    file2 << "Private Memory - " << rss - shared_mem << "kB\n";
    file2.close();
  }
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
  std::cout << "Exiting HadamardMatrixMultiplication.\n" << std::endl;
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
int SendVetor(std::vector<std::uint64_t> &v, int party_id, std::uint8_t msg_type)
{
  std::vector<uint8_t> v_msg;
  ConvertVetorIntoMessage(v, v_msg, msg_type);
  try{
      comm_layer->send_message(party_id, v_msg);
  }
  catch (std::runtime_error& e) {
      std::cerr << "Error occurred while sending: " << msg_type <<e.what() << "\n";
      return EXIT_FAILURE;
  }
return 0;
}
// Input :  Arithmetic shares of X and Y 
// Output : Arithmatic shares of Z = XY
int ArithMatrixMultiplication(std::vector<uint64_t> &X_arith, std::vector<uint64_t> &Y_arith, std::vector<uint64_t> &Z_arith)
{
  std::vector<std::uint64_t> X_ABY_public, X_ABY_private;
  std::vector<std::uint64_t> Y_ABY_public, Y_ABY_private;
  std::vector<std::uint64_t>  Z_ABY_public, Z_ABY_private;
  std::vector<std::uint8_t> X_msg_ABY_shares, Y_msg_ABY_shares;
  std::vector<std::uint8_t> X_msg_Private_Shares, Y_msg_Private_Shares;

  GeneratePrivateShares_ABY(X_arith, X_ABY_private);
  X_ABY_public.resize(X_arith.size());
  ParallelAddition(X_arith, X_ABY_private, X_ABY_public, 2);//X_ABY_public = X0 + deltaX0 (Its own arith share + private share)
  ConvertVetorIntoMessage(X_ABY_public, X_msg_ABY_shares, (std::uint8_t)XArithToABY);
  std::cout << "X-ABY Publicshare message will be seding to the other party is : " << X_msg_ABY_shares.size()  << "\n";

  GeneratePrivateShares_ABY(Y_arith, Y_ABY_private);
  Y_ABY_public.resize(Y_arith.size());
  ParallelAddition(Y_arith, Y_ABY_private, Y_ABY_public); 
  ConvertVetorIntoMessage(Y_ABY_public, Y_msg_ABY_shares, (std::uint8_t)YArithToABY);

  //Prepare private shares messages from vectors deltaX0, deltaY0 to send helper node
  ConvertVetorIntoMessage(X_ABY_private, X_msg_Private_Shares, (std::uint8_t)X_PrivateShares);
  ConvertVetorIntoMessage(Y_ABY_private, Y_msg_Private_Shares, (std::uint8_t)Y_PrivateShares);

   //%%%%% Sending X_ABY shares from my party to other party nad woaut for message from other party 
  std::cout << "Sending Public shares message to Server : " << 1-my_id << " Message size : "<< X_msg_ABY_shares.size() << "\n";
  try{
      comm_layer->send_message(1-my_id, X_msg_ABY_shares);
    }
  catch (std::runtime_error& e) {
      std::cerr << "Error occurred while sending X_msg_ABY_shares to node other part: " << e.what() << "\n";
      return EXIT_FAILURE;
    }

  std::cout << "Waiting for Public shares message from Server : " << 1-my_id << "\n";
  while(!XABY_receive_flag)
    {
      std::cout<<"X";
      boost::this_thread::sleep_for(boost::chrono::milliseconds(200));
    }
  ParallelAddition(X_ABY_public, x_temp_vec, X_ABY_public);
  //%%%%% Sending Y_ABY shares from my party to other party nad woaut for message from other party 
  std::cout << "Sending Y Public shares message to Server : " << 1-my_id << " Message size : "<< Y_msg_ABY_shares.size() << "\n";
  try{
      comm_layer->send_message(1-my_id, Y_msg_ABY_shares);
    }
  catch (std::runtime_error& e) {
      std::cerr << "Error occurred while sending X_msg_ABY_shares to node other part: " << e.what() << "\n";
      return EXIT_FAILURE;
    }

  std::cout << "Waiting for Public shares message from Server : " << 1-my_id << "\n";
  while(!YABY_receive_flag)
    {
      std::cout<<"A";
      boost::this_thread::sleep_for(boost::chrono::milliseconds(200));
    }
  ParallelAddition(Y_ABY_public, y_temp_vec, Y_ABY_public);

  // //Syncing up with helper node,
  // std::vector<std::uint8_t> started{(std::uint8_t)HelperNodeSync};
  // std::cout<<"Sending Probe message helper node.\n";
  // try{
  //     comm_layer->send_message(helpernode_id, started);
  // }
  // catch (std::runtime_error& e) {
  //     std::cerr << "Error occurred while sending the start message to helper node: " << e.what() << "\n";
  //     return EXIT_FAILURE;
  // }
  // //Waiting to receive the acknowledgement from helpernode
  // while(!helpernode_ready_flag)
  //     {
  //       std::cout<<"h";
  //       boost::this_thread::sleep_for(boost::chrono::milliseconds(200));
  //     }
  //Sending private shares to the Helper node
  try{
      comm_layer->send_message(helpernode_id, X_msg_Private_Shares);
  }
  catch (std::runtime_error& e) {
      std::cerr << "Error occurred while sending the start message to helper node: " << e.what() << "\n";
      return EXIT_FAILURE;
  }
  try{
      comm_layer->send_message(helpernode_id, Y_msg_Private_Shares);
  }
  catch (std::runtime_error& e) {
      std::cerr << "Error occurred while sending the start message to helper node: " << e.what() << "\n";
      return EXIT_FAILURE;
  }
 
  while(!OT_flag)
      {
      std::cout<<"o";
      boost::this_thread::sleep_for(boost::chrono::milliseconds(200));
      }

  std::vector<std::uint64_t> dxdy; //deltax * deltay
  HadamardMatrixMultiplication(X_ABY_private, Y_ABY_private, dxdy);
  // std::cout << "\n ** dXdY ** \n";
  // for(int i = 0; i < dxdy.size(); i++)
  //    std::cout <<  dxdy[i] << " ,  ";
  
  std::vector<std::uint64_t> DxDy; //Deltax * Deltay
  HadamardMatrixMultiplication(X_ABY_public, Y_ABY_public, DxDy);
  // std::cout << "\n ** DXDY** \n";
  // for(int i = 0; i < DxDy.size(); i++)
  //    std::cout <<  DxDy[i] << " ,  ";
  
  std::vector<std::uint64_t> Dxdy; //DeltaX * deltay
  HadamardMatrixMultiplication(X_ABY_public, Y_ABY_private, Dxdy);
  // std::cout << "\n **DXdY ** \n";
  // for(int i = 0; i < Dxdy.size(); i++)
  //    std::cout <<  Dxdy[i] << " ,  ";
  
  std::vector<std::uint64_t> dxDy; //deltax * Deltay
  HadamardMatrixMultiplication(X_ABY_private, Y_ABY_public, dxDy);
  //  std::cout << "\n ** dXDY ** \n";
  // for(int i = 0; i < dxDy.size(); i++)
  //    std::cout <<  dxDy[i] << " ,  ";

 //std::cout << "\n ** (DXDY>>1) - Dxdy + dxdy - dxDy + OT0 ** \n";
 Z_arith.resize(dxdy.size());
 Z_arith[0] = dxdy[0];
 Z_arith[1] = dxdy[1];
 
  for(int i = 2; i<Z_arith.size(); i++)
    {
     Z_arith[i] = dxdy[i] + OT_vec[i] + (DxDy[i]>>1) - Dxdy[i] -dxDy[i];
     //std :: cout << Z_arith[i] << "\n";
    }
 

 std::cout << "\n *** Truncate  **** \n";
  for(int i = 2; i < Z_arith.size(); i++)
  {
     Z_arith[i] =  MOTION::new_fixed_point::truncate(Z_arith[i], fractional_bits);
     //std::cout << Z_arith[i] << "\n";
  }
  return 0;
}

// PrivateCompare: Computes beta XOR (x > r)
// Input  : A vector bit_share_x of length ((length of a) * BIT_SIZE) storing bit shares of a at Party 0, vectors r and beta of type std::uint64_t common to Party 0 and Party 1 and vector c storing the intermediate value of PrivateCompare at Party 0
// Output : Updates vector c to store the value which is to be sent to Party 2
int PrivateCompare(std::vector<std::uint64_t> &bit_share_x, std::vector<std::uint64_t> &r, std::vector<std::uint64_t> &beta, std::vector<std::uint64_t> &c) 
{
std::cout << "Enetered PrivateCompare  \n";  
std::uint64_t value_r, value_beta;
std::uint64_t t, j, ind, w, temp_prime_sh;

std::mt19937 gen_u(1234); 
std::mt19937 gen_s(5678); 
std::size_t len = r.size();
for(int i = 0; i < len; i++)
{ 
  value_r = r[i];
  value_beta = beta[i];
  if (value_beta == 1) value_r = r[i]+1;
  w = 0;
  
  for(int k = 0; k < BIT_SIZE; k++)
  {
    ind = i*BIT_SIZE + k;
    auto r_bit = (value_r >> BIT_SIZE-1 - k) & 1; 
    c[ind] = w;
    temp_prime_sh = bit_share_x[ind];
    if (r_bit == 0)
       w = AddModuloPrime(w, temp_prime_sh);
    else 
       w = AddModuloPrime(w, SubtractModuloPrime(my_id, temp_prime_sh));
    if (value_beta == 0)
    {
     if (my_id == 1) c[ind] = AddModuloPrime(c[ind], my_id + r_bit);
     c[ind] = SubtractModuloPrime(c[ind], temp_prime_sh); 
    }
    else if ((value_beta == 1) & (r[i] != LMINUS_ONE))
         {
          
          if (my_id == 1) c[ind] = AddModuloPrime(c[ind], my_id - r_bit);
          c[ind] = AddModuloPrime(c[ind], temp_prime_sh); 
         }
    else //of value_beta == 1 and r[i] == LMINUS_ONE
         {
          c[ind] = RandomNonZeroNumOverPrime(gen_u);
          if (my_id ==  1) // k = 0 at p0, p1, c[ind], -c[ind]; k>0 c[ind], 1-c[ind]
             c[ind] = SubtractModuloPrime((k != 0), c[ind]);
         }
    //std::cout << "c at ind : "<< ind << " is " << c[ind] << "\n";
    c[ind] = MultiplyModuloPrime(c[ind], RandomNonZeroNumOverPrime(gen_s));
  }
GenerateRandomPermutation(c, i*BIT_SIZE, (i+1)*BIT_SIZE);
}
std::cout << "Exited PrivateCompare.\n" << std::endl;
return 0;
}

// ShareConvert: Converts shares of a in L world to shares of a in L-1 world
// Input  : A vector a_L of type std::uint64_t storing shares of a in L world, an empty vector of type std::uint64_t b_odd to store the L-1 shares.
// Output : Updates b_odd to store odd shares of a.
int ShareConvert(std::vector<std::uint64_t> &a_L, std::vector<std::uint64_t> &b_odd)
{

  std::cout << "Entered ShareConvert.\n";
  std::size_t len = a_L.size();
  //Common to both parties starts
  std::vector<uint64_t> r(len, 0), r0(len, 0), r1(len, 0),r_min1(len,0); //r = r0+r1
  std::vector<uint64_t> eta_dp(len, 0), alpha(len, 0);
  int r0_seed = 1234;
  int r1_seed = 5678;
  int b_seed = 1;
  
  CommonRandVectorOverL(r0, len, r0_seed);
  CommonRandVectorOverL(r1, len, r1_seed);
  CommonRandBitVector(eta_dp, len, b_seed);
  AddVectorsOverL(r0, r1, r, len);
   WrapAround(r0, r1, alpha, len);

  std::vector<uint64_t> a_tilde(len, 0), beta(len, 0);
  AddVectorsOverL(a_L, r0, a_tilde, len);
  WrapAround(a_L, r0, beta, len);

   //Syncing up with helper node,
  std::vector<std::uint8_t> started{(std::uint8_t)HelperNodeSync};
  auto start = std::chrono::high_resolution_clock::now();

  std::cout<<"Sending Probe message helper node.\n";
  try{
      comm_layer->send_message(helpernode_id, started);
  }
  catch (std::runtime_error& e) {
      std::cerr << "Error occurred while sending the start message to helper node: " << e.what() << "\n";
      return EXIT_FAILURE;
  }
  //Waiting to receive the acknowledgement from helpernode
  while(!helpernode_ready_flag)
      {
        std::cout<<"h";
        boost::this_thread::sleep_for(boost::chrono::milliseconds(200));
      }

  auto end = std::chrono::high_resolution_clock::now();

  const std::string baseDirectory = (std::string)std::getenv("BASE_DIR");
  std::string statFilePath = baseDirectory + "/build_debwithrelinfo_gcc/stats/ackStats1";
  std::ofstream statFilePathFile;
  statFilePathFile.open(statFilePath, std::ios_base::app);
  if (!statFilePathFile.is_open()) {
    std::cerr << "Error: Unable to open the file path.\n";
  }
  statFilePathFile << "Helper message acknowledgement @ S1 for ReLU: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << std::endl;
  statFilePathFile.close();

  //Sending a tilde to P2
  std::cout<<"Sending a_tilde to helper node.\n";
  SendVetor(a_tilde, helpernode_id, (uint8_t)SC);
  while(!SC_P_flag || !SC_D_flag)
    {
     std::cout<<"sc ";
     boost::this_thread::sleep_for(boost::chrono::milliseconds(200));
    }

std::cout << " Prime Bit Share (Data Length) :  " << x_pr_bit.size() << ".\n";
std::cout << " Prime Bit Share (Data Length) :  " << del_odd.size() << ".\n";
std::vector<std::uint64_t> c(len*BIT_SIZE, 0);
std::vector<std::uint8_t> c_msg;
for(int i = 0; i < len; i++) r_min1[i] =  r[i] - 1;

PrivateCompare(x_pr_bit, r_min1, eta_dp,  c);
SendVetor(c, helpernode_id, (uint8_t)SC_PC);
//waiting for etaP_odd shares to be received
while(!SC_PC_flag)
    {
     std::cout<<"sc ";
     boost::this_thread::sleep_for(boost::chrono::milliseconds(200));
    }
std::vector<uint64_t> theta_odd(len,0);
for(int i = 0; i<len; i++)
{
  //
  if (eta_dp[i] == 1)
      theta_odd[i] = SubtractModuloOdd(1-my_id, etaP_odd[i]);//theta = 1-betaP_odd 
  else theta_odd[i] = etaP_odd[i];
}
AddModuloOdd(theta_odd, etaP_odd, theta_odd, len);
AddModuloOdd(theta_odd, del_odd, theta_odd, len);
for (int i = 0; i<len; i++) alpha[i] = (1-my_id)*(alpha[i] + 1);
SubtractModuloOdd(theta_odd, alpha, theta_odd, len);
SubtractModuloOdd(a_L, theta_odd, b_odd, len );
std::cout << "Exited ShareConvert.\n" << std::endl; 
return 0;
}

// ComputeMSB: Takes shares of a in L world and returns shares of most significant bit MSB(a) in L world.
// Input  : Vector a_L_0 of type std::uint64_t storing shares of a at P0 in L world, an empty vector MSB_a_L of type std::uint64_t, dimensions of matrix (rows, cols)
// Output : Updates MSB_a_L to store MSB shares.
int ComputeMSB(std::vector<std::uint64_t>& a_L_1, std::vector<std::uint64_t>& MSB_a_L, std::size_t& aLMinusOneRows, std::size_t& aLMinusOneCols) {
  std::cout << "Entered ComputeMSB." << std::endl;

  // Common Randomness
  std::size_t len = a_L_1.size();
  std::vector<std::uint64_t> beta(len, 0);

  int beta_seed = 1234;
  CommonRandBitVector(beta, len, beta_seed);

  // Step 0
  // Send acknowledgement message to helpernode after completion of ShareConvert
  std::vector<std::uint64_t> helpernode_ComputeMSB_ready{(std::uint64_t)len};
  std::cout << "Sending compute MSB ready message to helper node." << "\n";
  SendVetor(helpernode_ComputeMSB_ready, helpernode_id, ComputeMSBReady);

  while (!helpernode_computeMSB_ready_flag) {
    std::cout << ".";
    boost::this_thread::sleep_for(boost::chrono::milliseconds(200));
  }

  std::cout << "Received acknowledgement from helper node." << std::endl;

  while ((!MSB_Odd_flag) || (!MSB_P_flag) || (!MSB_L_LSB_flag)) {
    std::cout << ".";
    boost::this_thread::sleep_for(boost::chrono::milliseconds(600));
  }

  // Compute y_L_1 and r_L_1
  std::vector<std::uint64_t> y_L_1(len, 0), r_L_1(len, 0);
  AddModuloOdd(a_L_1, a_L_1, y_L_1, len);
  AddModuloOdd(y_L_1, MSB_x_Odd, r_L_1, len);

  // Reconstruct r shares
  SendVetor(r_L_1, other_party, MSB_R);

  while (!MSB_R_flag) {
    std::cout << ".";
    boost::this_thread::sleep_for(boost::chrono::milliseconds(200));
  }

  std::vector<std::uint64_t> rVec(len, 0);
  AddModuloOdd(r_L_1, R_Shares, rVec, len);

  // Private Compare
  std::vector<std::uint64_t> c(len * BIT_SIZE, 0);
  PrivateCompare(MSB_x_P, rVec, beta, c);

  SendVetor(c, helpernode_id, MSB_PC);

  // Waiting for helpernode to receive shares of betaP
  while (!MSB_PC_flag) {
    std::cout << ".";
    boost::this_thread::sleep_for(boost::chrono::milliseconds(200));
  }

  std::vector<std::uint64_t> encodedBeta(len, 0), encodedRBit(len, 0);
  for (int i = 0; i < len; i++) {
    encodedBeta[i] = MOTION::new_fixed_point::encode<uint64_t, long double>(beta[i], fractional_bits);
    encodedRBit[i] = MOTION::new_fixed_point::encode<uint64_t, long double>(rVec[i] & 1, fractional_bits);
  }
  
  // Computing the gamma and delta shares for hadamard matrix multiplication
  std::vector<std::uint64_t> gammaL(len + 2, 0), deltaL(len + 2, 0);
  for (int i = 2; i < len + 2; i++) {
    gammaL[i] = encodedBeta[i - 2] + betaP[i - 2] - 2 * MOTION::new_fixed_point::truncate(encodedBeta[i - 2] * betaP[i - 2], fractional_bits);
    deltaL[i] = MSB_x_L_LSB[i - 2] + encodedRBit[i - 2] - 2 * MOTION::new_fixed_point::truncate(MSB_x_L_LSB[i - 2] * encodedRBit[i - 2], fractional_bits);
  }

  std::vector<std::uint64_t> thetaL(len + 2, 0);
  // Hadamard matrix multiplication using ABY2.0 shares
  // Add dimensions (rows, cols) to gamma_L, delta_L, theta_L
  gammaL[0] = aLMinusOneRows; 
  gammaL[1] = aLMinusOneCols;
  deltaL[0] = aLMinusOneRows;
  deltaL[1] = aLMinusOneCols;
  ArithMatrixMultiplication(gammaL, deltaL, thetaL);

  gammaL.erase(gammaL.begin(), gammaL.begin() + 2);
  deltaL.erase(deltaL.begin(), deltaL.begin() + 2);
  thetaL.erase(thetaL.begin(), thetaL.begin() + 2);

  // Compute final MSB shares
  for (int i = 0; i < len; i++) {
    MSB_a_L[i] = gammaL[i] + deltaL[i] - 2 * thetaL[i];
  }
  
  std::cout << "Exited ComputeMSB." << std::endl;
  return 0;
}

// DerivativeRelu: Takes shares of a in L world and returns shares of ReLU'(a) in L world.
// ReLU'(a) = 1 if MSB(a) = 0, otherwise ReLU'(a) = 0.
// Input  : Vector a_L_1 of type std::uint64_t storing shares of a at P1 in L world, an empty vector a_DRelu_1 of type std::uint64_t, dimensions of matrix (rows, cols)
// Output : Updates a_DRelu_1 to store ReLU' shares.
int DerivativeRelu(std::vector<std::uint64_t>& a_L_1, std::vector<std::uint64_t>& a_DRelu_1, std::size_t rows, std::size_t cols) {
  std::cout << "Entering DerivativeReLU. " << std::endl;
  std::size_t len = a_L_1.size();

  // Step 1-2: Convert shares of 2 * a_0 into odd shares
  std::vector<std::uint64_t> c_L_1(len, 0);
  for (int i = 0; i < len; i++) {
    c_L_1[i] = 2 * a_L_1[i];
  }

  std::vector<std::uint64_t> yOdd_1, MSBShares_1;
  yOdd_1.resize(len);
  ShareConvert(c_L_1, yOdd_1);

  // Step 3: ComputeMSB of odd shares
  MSBShares_1.resize(len);
  ComputeMSB(yOdd_1, MSBShares_1, rows, cols);

  for (int i = 0; i < len; i++) {
    a_DRelu_1[i] = MOTION::new_fixed_point::encode<uint64_t, long double>(1, fractional_bits) - MSBShares_1[i];
  }

  std::cout << "Exiting DerivativeReLU.\n " << std::endl;
  return 0;
}

// ReLU: Takes shares of a in L world and returns shares of ReLU(a) in L world
// Input  : Vector a_L_1 of type std::uint64_t in L world, empty vector a_Relu_1 in L world, dimensions of matrix (rows, cols).
// Output : Updates a_Relu_1 to store ReLU shares.
int ReLU(std::vector<std::uint64_t>& a_L_1, std::vector<std::uint64_t>& a_Relu_1, std::size_t rows, std::size_t cols) {
  std::cout << "Entering ReLU." << std::endl;
  std::size_t len = a_L_1.size();
  std::vector<std::uint64_t> a_DRelu_1(len, 0);
  
  // Step 1
  DerivativeRelu(a_L_1, a_DRelu_1, rows, cols);
  
  // Step 2
  a_L_1.insert(a_L_1.begin(), cols);
  a_L_1.insert(a_L_1.begin(), rows);

  a_DRelu_1.insert(a_DRelu_1.begin(), cols);
  a_DRelu_1.insert(a_DRelu_1.begin(), rows);

  // Step 0
  // Send acknowledgement message to helpernode after completion of DRelu
  std::vector<std::uint64_t> helpernode_Relu_ready{(std::uint64_t)0};
  std::cout << "Sending Derivative ReLU ready message to helper node." << "\n";
  SendVetor(helpernode_Relu_ready, helpernode_id, ReluReady);

  while (!helpernode_ReLU_ready_flag) {
    std::cout << ".";
    boost::this_thread::sleep_for(boost::chrono::milliseconds(200));
  }

  std::cout << "Received Derivative ReLU acknowledgement from helper node." << std::endl;

  std::vector<std::uint64_t> a_L_ABY_public, a_L_ABY_private;
  std::vector<std::uint64_t> a_DRelu_ABY_public, a_DRelu_ABY_private;
  std::vector<std::uint64_t>  Z_ABY_public, Z_ABY_private;
  std::vector<std::uint8_t> a_L_msg_ABY_shares, a_DRelu_msg_ABY_shares;
  std::vector<std::uint8_t> a_L_msg_Private_Shares, a_DRelu_msg_Private_Shares;

  GeneratePrivateShares_ABY(a_L_1, a_L_ABY_private);
  a_L_ABY_public.resize(a_L_1.size());
  ParallelAddition(a_L_1, a_L_ABY_private, a_L_ABY_public, 2);
  ConvertVetorIntoMessage(a_L_ABY_public, a_L_msg_ABY_shares, (std::uint8_t)Relu_XArithToABY);
  std::cout << "X-ABY Publicshare message will be seding to the other party is : " << a_L_msg_ABY_shares.size()  << "\n";

  GeneratePrivateShares_ABY(a_DRelu_1, a_DRelu_ABY_private);
  a_DRelu_ABY_public.resize(a_DRelu_1.size());
  ParallelAddition(a_DRelu_1, a_DRelu_ABY_private, a_DRelu_ABY_public, 2); 
  ConvertVetorIntoMessage(a_DRelu_ABY_public, a_DRelu_msg_ABY_shares, (std::uint8_t)Relu_YArithToABY);

  //Prepare private shares messages from vectors deltaX0, deltaY0 to send helper node
  ConvertVetorIntoMessage(a_L_ABY_private, a_L_msg_Private_Shares, (std::uint8_t)Relu_X_PrivateShares);
  ConvertVetorIntoMessage(a_DRelu_ABY_private, a_DRelu_msg_Private_Shares, (std::uint8_t)Relu_Y_PrivateShares);

   //%%%%% Sending X_ABY shares from my party to other party nad woaut for message from other party 
  std::cout << "Sending Public shares message to Server : " << 1-my_id << " Message size : "<< a_L_msg_ABY_shares.size() << "\n";
  try{
      comm_layer->send_message(1-my_id, a_L_msg_ABY_shares);
    }
  catch (std::runtime_error& e) {
      std::cerr << "Error occurred while sending a_L_msg_ABY_shares to node other part: " << e.what() << "\n";
      return EXIT_FAILURE;
    }

  std::cout << "Waiting for Public shares message from Server : " << 1-my_id << "\n";
  while(!XABY_receive_flag)
    {
      std::cout<<"X";
      boost::this_thread::sleep_for(boost::chrono::milliseconds(200));
    }
  ParallelAddition(a_L_ABY_public, Relu_x_temp_vec, a_L_ABY_public);

  //%%%%% Sending Y_ABY shares from my party to other party nad woaut for message from other party 
  std::cout << "Sending Y Public shares message to Server : " << 1-my_id << " Message size : "<< a_DRelu_msg_ABY_shares.size() << "\n";
  try{
      comm_layer->send_message(1-my_id, a_DRelu_msg_ABY_shares);
    }
  catch (std::runtime_error& e) {
      std::cerr << "Error occurred while sending X_msg_ABY_shares to node other part: " << e.what() << "\n";
      return EXIT_FAILURE;
    }

  std::cout << "Waiting for Public shares message from Server : " << 1-my_id << "\n";
  while(!YABY_receive_flag)
    {
      std::cout<<"A";
      boost::this_thread::sleep_for(boost::chrono::milliseconds(200));
    }
  ParallelAddition(a_DRelu_ABY_public, Relu_y_temp_vec, a_DRelu_ABY_public);

  try{
      comm_layer->send_message(helpernode_id, a_L_msg_Private_Shares);
  }
  catch (std::runtime_error& e) {
      std::cerr << "Error occurred while sending the a_L_msg_Private_Shares message to helper node: " << e.what() << "\n";
      return EXIT_FAILURE;
  }
  try{
      comm_layer->send_message(helpernode_id, a_DRelu_msg_Private_Shares);
  }
  catch (std::runtime_error& e) {
      std::cerr << "Error occurred while sending the a_DRelu_msg_Private_Shares message to helper node: " << e.what() << "\n";
      return EXIT_FAILURE;
  }
 
  while(!OT_flag)
      {
      std::cout<<"o";
      boost::this_thread::sleep_for(boost::chrono::milliseconds(200));
      }

  std::vector<std::uint64_t> dxdy; //deltax * deltay
  HadamardMatrixMultiplication(a_L_ABY_private, a_DRelu_ABY_private, dxdy);
  
  std::vector<std::uint64_t> DxDy; //Deltax * Deltay
  HadamardMatrixMultiplication(a_L_ABY_public, a_DRelu_ABY_public, DxDy);
  
  std::vector<std::uint64_t> Dxdy; //DeltaX * deltay
  HadamardMatrixMultiplication(a_L_ABY_public, a_DRelu_ABY_private, Dxdy);
  
  std::vector<std::uint64_t> dxDy; //deltax * Deltay
  HadamardMatrixMultiplication(a_L_ABY_private, a_DRelu_ABY_public, dxDy);

  a_Relu_1.resize(dxdy.size());
  a_Relu_1[0] = dxdy[0];
  a_Relu_1[1] = dxdy[1];
 
  for(int i = 2; i < a_Relu_1.size(); i++)
    {
     a_Relu_1[i] = dxdy[i] + Relu_OT_vec[i] + (DxDy[i]>>1) - Dxdy[i] - dxDy[i];
    }

  std::cout << "\n *** Truncate **** \n";
  for(int i = 2; i < a_Relu_1.size(); i++)
  {
     a_Relu_1[i] =  MOTION::new_fixed_point::truncate(a_Relu_1[i], fractional_bits);
  }

  // Delete the dimensions (rows, cols) of the matrix.
  // a_Relu_1.erase(a_Relu_1.begin(), a_Relu_1.begin() + 2);

  std::cout << "Exiting ReLU." << std::endl;
  return 0;
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


#endif
