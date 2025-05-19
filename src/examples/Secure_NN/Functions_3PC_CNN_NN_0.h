#ifndef FUNCTIONS_CNN_NN_0_H
#define FUNCTIONS_CNN_NN_0_H

#pragma once
#include <unistd.h>
#include <filesystem>
#include <fstream>
#include <random>
#include <regex>
#include <stdexcept>
#include <thread>
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

#include "GlobalVar_3PC_CNN_NN_0.h"

using namespace std::chrono;

struct Options {
  std::string WB_File;
  std::string input_file;
  std::size_t layers;
  std::vector<int> layer_types;
  std::string output_share_file;
  std::size_t fractional_bits;
  std::string current_path;
  MOTION::Communication::tcp_parties_config tcp_config;
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
    //std::cout << " Entered GenerateRandomPermutation \n ";
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

// ReadSharesIntoVec: Reads ABY2.0 shares (public and private shares) from the given filepath into vectors
// The first three values of the vector are dimensions of the matrix and the remaining are the share values in the format (public " " private)
// Input  : A filepath containing ABY2.0 shares and two empty vectors of type std::uint64_t publicShares and privateShares
// Output : Updates publicShares and privateShares to store the values of the shares.
int ReadSharesIntoVec(std::string file_path, std::vector<uint64_t>& publicShares, std::vector<std::uint64_t>& privateShares) {
  std::ifstream input_file;
  std::cout << "##### Entered ReadSharesIntoVec ...##### \n";

  try {
    input_file.open(file_path);
    if (!input_file) {
      std::cerr << "Unable to open ABY2.0 share file.\n";
      throw std::ifstream::failure("Error in opening the ABY2.0 share file.");
    }
  }
  catch (std::exception& e){
    std::cerr << "Error during opening ABY2.0 share file: " << e.what() << std::endl;
    return EXIT_FAILURE;
  }

  int num_channels = 0;
  int num_r = 0; 
  int num_c = 0;
  
  // Reading the dimensions of the matrix from the input filepath
  try {
    input_file >> num_channels >> num_r >> num_c;
  }
  catch (std::ifstream::failure e) {
    std::cerr << "Error while reading rows and columns from input shares file.\n";
    exit(1);
  }

  if (input_file.eof()) {
    std::cerr << "Input shares file doesn't contain rows and columns" << std::endl;
    exit(1);
  }

  std::cout << "Total number of elements: " << num_r * num_c << "\n";
  publicShares.push_back(num_channels);
  publicShares.push_back(num_r);
  publicShares.push_back(num_c);

  privateShares.push_back(num_channels);
  privateShares.push_back(num_r);
  privateShares.push_back(num_c);

  int num_vals = num_channels * num_r * num_c;
  std::uint64_t tempPublic, tempPrivate;
  int k = 0;

  while (k < num_channels * num_r * num_c) {
    try {    
      input_file >> tempPublic >> tempPrivate;
      publicShares.push_back(tempPublic);
      privateShares.push_back(tempPrivate);
    }
    catch (std::ifstream::failure e) {
      std::cerr << "Error while reading the ABY2.0 shares.\n";
      exit(1);
    }

    if (input_file.eof()) {
      std::cerr << "ABY2.0 shares file contains less number of elements" << std::endl;
      exit(1);
    }
    k++;
  }

  std::cout << "Number of elements read into public shares vector: " << publicShares.size() << "\n";
  std::cout << "Number of elements read into private shares vector: " << privateShares.size() << "\n";

  input_file.close();
  std::cout << "##### Exiting ReadSharesIntoVec ##### \n\n";
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
int GeneratePrivateShares_ABY(std::vector<uint64_t> &vec, std::vector<uint64_t> &private_share) {
std::cout << "#####Entered GeneratePrivateShares_ABY ... ##### \n";
int num_r = vec[0];
int num_c = vec[1];
//Note that the first two values in any vector are the dimensions
private_share.push_back(vec[0]);
private_share.push_back(vec[1]);
  try {
    for (int i = 0; i < num_r * num_c; i++) {
      std::random_device rd;
      std::mt19937 gen(rd());
      std::uint64_t t = RandomNumDistribution(gen);
      private_share.push_back(t);
    }
  } catch (std::exception& e) {
    std::cerr << "Error during image share generation: " << e.what() << std::endl;
    return EXIT_FAILURE;
  }
  std::cout << "##### Exiting GeneratePrivateShares_ABY ##### \n\n";
  return 1;
}

//addition is performed parallelly ans = op1 + op2
//offset from which element the addition operation has to be performed
void ParallelAddition(std::vector<std::uint64_t> &op1, std::vector<std::uint64_t> &op2, std::vector<std::uint64_t> &ans, int offset = 2) {
  std::cout << "#### Entered ParallelAddition... ##### \n" ;
  ans.resize(op1.size());
  auto op1_begin = op1.begin();
  auto op1_end = op1.end();
  auto op2_begin = op2.begin();
  auto ans_begin = ans.begin();
  if (offset) {
    for (int i = 0; i < offset; i++)
      ans[i] = op2[i];
  }
  advance(op1_begin, offset);
  advance(op2_begin, offset);
  advance(ans_begin, offset);
  __gnu_parallel::transform(op1_begin, op1_end, op2_begin, ans_begin, std::plus{});

  std::cout << "##### Exiting ParallelAddition ##### \n\n";
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
    file2 << "3PC ReLU at Party 0: \n";
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

// ArithMatrixMultiplication: Takes arithmetic shares of input: X received from Party 0 and Y from Party 1 respectively, converts them into ABY2.0 shares and performs hadamard matrix multiplication of Z = X . Y
// First two elements of all shares are the dimensions of the matrix: rows and cols
// Input  : Vectors X_shares_0 and X_shares_1 storing shares of matrix X at P0 and P1 respectively, vectors Y_shares_0 and Y_shares_1 storing shares of matrix Y at P0 and P1 respectively and empty vectors Z_shares_0 and Z_shares_1 of type std::uint64_t
// Output : Updates Z_shares_0 and Z_shares_1 to store hadamard product shares of X.Y, first two elements are the dimensions of the matrix.
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
int PrivateCompare(std::vector<std::uint64_t> &bit_share_x, std::vector<std::uint64_t> &r, std::vector<std::uint64_t> &beta, std::vector<std::uint64_t> &c, int len, int startRange, int endRange) 
{
std::cout << "Enetered PrivateCompare. \n";  
std::uint64_t value_r, value_beta;
std::uint64_t t, j, ind, w, temp_prime_sh;

std::mt19937 gen_u(1234); 
std::mt19937 gen_s(5678); 


for(int i = startRange; i < endRange; i++) {
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

  std::cout << "Entered ShareConvert. \n";
  std::size_t len = a_L.size();
  std::cout << "ShareConvert len: " << len << std::endl;

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

int partition = len / 8;
std::thread threadPC_0_1(PrivateCompare, std::ref(x_pr_bit), std::ref(r_min1), std::ref(eta_dp), std::ref(c), len, 0, partition);
std::thread threadPC_0_2(PrivateCompare, std::ref(x_pr_bit), std::ref(r_min1), std::ref(eta_dp), std::ref(c), len, partition, 2 * partition);
std::thread threadPC_0_3(PrivateCompare, std::ref(x_pr_bit), std::ref(r_min1), std::ref(eta_dp), std::ref(c), len, 2 * partition, 3 * partition);
std::thread threadPC_0_4(PrivateCompare, std::ref(x_pr_bit), std::ref(r_min1), std::ref(eta_dp), std::ref(c), len, 3 * partition, 4 * partition);
std::thread threadPC_0_5(PrivateCompare, std::ref(x_pr_bit), std::ref(r_min1), std::ref(eta_dp), std::ref(c), len, 4 * partition, 5 * partition);
std::thread threadPC_0_6(PrivateCompare, std::ref(x_pr_bit), std::ref(r_min1), std::ref(eta_dp), std::ref(c), len, 5 * partition, 6 * partition);
std::thread threadPC_0_7(PrivateCompare, std::ref(x_pr_bit), std::ref(r_min1), std::ref(eta_dp), std::ref(c), len, 6 * partition, 7 * partition);
std::thread threadPC_0_8(PrivateCompare, std::ref(x_pr_bit), std::ref(r_min1), std::ref(eta_dp), std::ref(c), len, 7 * partition, len);

threadPC_0_1.join();
threadPC_0_2.join();
threadPC_0_3.join();
threadPC_0_4.join();
threadPC_0_5.join();
threadPC_0_6.join();
threadPC_0_7.join();
threadPC_0_8.join();

// int partition = len / 2;
// std::thread threadPC_0_1(PrivateCompare, std::ref(x_pr_bit), std::ref(r_min1), std::ref(eta_dp), std::ref(c), len, 0, partition);
// std::thread threadPC_0_2(PrivateCompare, std::ref(x_pr_bit), std::ref(r_min1), std::ref(eta_dp), std::ref(c), len, partition, len);

// threadPC_0_1.join();
// threadPC_0_2.join();

// PrivateCompare(x_pr_bit, r_min1, eta_dp,  c);
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
SubtractModuloOdd(a_L, theta_odd, b_odd, len);
std::cout << "Exited ShareConvert.\n" << std::endl; 
return 0;
}

// ComputeMSB: Takes shares of a in L world and returns shares of most significant bit MSB(a) in L world.
// Input  : Vector a_L_0 of type std::uint64_t storing shares of a at P0 in L world, an empty vector MSB_a_L of type std::uint64_t, dimensions of matrix (rows, cols)
// Output : Updates MSB_a_L to store MSB shares.
int ComputeMSB(std::vector<std::uint64_t>& a_L_0, std::vector<std::uint64_t>& MSB_a_L, std::size_t& rows, std::size_t& cols) {
  std::cout << "Entered ComputeMSB." << std::endl;

  // Common randomness
  std::size_t len = a_L_0.size();
  std::vector<std::uint64_t> beta(len, 0);

  int beta_seed = 1234;
  CommonRandBitVector(beta, len, beta_seed);

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
  std::vector<std::uint64_t> y_L_0(len, 0), r_L_0(len, 0);
  AddModuloOdd(a_L_0, a_L_0, y_L_0, len);
  AddModuloOdd(y_L_0, MSB_x_Odd, r_L_0, len);

  // Reconstruct r shares
  SendVetor(r_L_0, other_party, MSB_R);

  while (!MSB_R_flag) {
    std::cout << ".";
    boost::this_thread::sleep_for(boost::chrono::milliseconds(200));
  }

  std::vector<std::uint64_t> rVec(len, 0);
  AddModuloOdd(r_L_0, R_Shares, rVec, len);

  // Private Compare
  std::vector<std::uint64_t> c(len * BIT_SIZE, 0);

  int partition = len / 8;

  std::thread threadPC_0_1(PrivateCompare, std::ref(MSB_x_P), std::ref(rVec), std::ref(beta), std::ref(c), len, 0, partition);
  std::thread threadPC_0_2(PrivateCompare, std::ref(MSB_x_P), std::ref(rVec), std::ref(beta), std::ref(c), len, partition, 2 * partition);
  std::thread threadPC_0_3(PrivateCompare, std::ref(MSB_x_P), std::ref(rVec), std::ref(beta), std::ref(c), len, 2 * partition, 3 * partition);
  std::thread threadPC_0_4(PrivateCompare, std::ref(MSB_x_P), std::ref(rVec), std::ref(beta), std::ref(c), len, 3 * partition, 4 * partition);
  std::thread threadPC_0_5(PrivateCompare, std::ref(MSB_x_P), std::ref(rVec), std::ref(beta), std::ref(c), len, 4 * partition, 5 * partition);
  std::thread threadPC_0_6(PrivateCompare, std::ref(MSB_x_P), std::ref(rVec), std::ref(beta), std::ref(c), len, 5 * partition, 6 * partition);
  std::thread threadPC_0_7(PrivateCompare, std::ref(MSB_x_P), std::ref(rVec), std::ref(beta), std::ref(c), len, 6 * partition, 7 * partition);
  std::thread threadPC_0_8(PrivateCompare, std::ref(MSB_x_P), std::ref(rVec), std::ref(beta), std::ref(c), len, 7 * partition, len);
  
  threadPC_0_1.join();
  threadPC_0_2.join();
  threadPC_0_3.join();
  threadPC_0_4.join();
  threadPC_0_5.join();
  threadPC_0_6.join();
  threadPC_0_7.join();
  threadPC_0_8.join();

  // int partition = len / 2;

  // std::thread threadPC_0_1(PrivateCompare, std::ref(MSB_x_P), std::ref(rVec), std::ref(beta), std::ref(c), len, 0, partition);
  // std::thread threadPC_0_2(PrivateCompare, std::ref(MSB_x_P), std::ref(rVec), std::ref(beta), std::ref(c), len, partition, len);
  
  // threadPC_0_1.join();
  // threadPC_0_2.join();

  // PrivateCompare(MSB_x_P, rVec, beta, c);

  SendVetor(c, helpernode_id, (uint8_t)MSB_PC);

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
    gammaL[i] = betaP[i - 2] - 2 * MOTION::new_fixed_point::truncate(betaP[i - 2] * encodedBeta[i - 2], fractional_bits);
    deltaL[i] = MSB_x_L_LSB[i - 2] - 2 * MOTION::new_fixed_point::truncate(MSB_x_L_LSB[i - 2] * encodedRBit[i - 2], fractional_bits);
  }

  std::vector<std::uint64_t> thetaL(len + 2, 0);
  // Hadamard matrix multiplication using ABY2.0 shares
  // Add dimensions (rows, cols) to gamma_L, delta_L, theta_L
  gammaL[0] = rows; 
  gammaL[1] = cols;
  deltaL[0] = rows;
  deltaL[1] = cols;
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
// Input  : Vector a_L_0 of type std::uint64_t storing shares of a at P0 in L world, an empty vector a_DRelu_0 of type std::uint64_t, dimensions of matrix (rows, cols)
// Output : Updates a_DRelu_0 to store ReLU' shares.
int DerivativeRelu(std::vector<std::uint64_t>& a_L_0, std::vector<std::uint64_t>& a_DRelu_0, std::size_t rows, std::size_t cols) {
  std::cout << "Entering DerivativeReLU. " << std::endl;
  std::size_t len = a_L_0.size();
  std::cout << "DerivativeRelu length: " << len << std::endl;

  // Step 1-2: Convert shares of 2 * a_0 into odd shares
  std::vector<std::uint64_t> c_L_0(len, 0);
  for (int i = 0; i < len; i++) {
    c_L_0[i] = 2 * a_L_0[i];
  }

  std::vector<std::uint64_t> yOdd_0, MSBShares_0;
  yOdd_0.resize(len);
  ShareConvert(c_L_0, yOdd_0);

  // Step 3: ComputeMSB of odd shares
  MSBShares_0.resize(len);
  ComputeMSB(yOdd_0, MSBShares_0, rows, cols);

  for (int i = 0; i < len; i++) {
    a_DRelu_0[i] = -MSBShares_0[i];
  }

  std::cout << "Exiting DerivativeReLU.\n " << std::endl;
  return 0;
}

// ReLU: Takes shares of a in L world and returns shares of ReLU(a) in L world
// Input  : Vector a_L_0 of type std::uint64_t in L world, empty vector a_Relu_0 in L world, dimensions of matrix (rows, cols).
// Output : Updates a_Relu_0 to store ReLU shares.
int ReLU(std::vector<std::uint64_t>& a_L_0, std::vector<std::uint64_t>& a_Relu_0, std::size_t rows, std::size_t cols) {
  try {
    std::vector<std::uint8_t> syncMessage{(std::uint8_t)ReluSync};
    comm_layer->send_message(helpernode_id, syncMessage);
  } 
  catch (std::exception& e) {
    std::cerr << "Error detected: " << e.what() << std::endl;
    return EXIT_FAILURE;
  }

  while (!relu_ready_flag) {
    std::cout << ".";
    boost::this_thread::sleep_for(boost::chrono::milliseconds(200));
  }

  a_Relu_0.resize(a_L_0.size() + 2);

  std::cout << "Entering ReLU." << std::endl;
  std::size_t len = a_L_0.size();
  std::cout << "Relu len: " << len << std::endl;
  std::vector<std::uint64_t> a_DRelu_0(len, 0);
  
  // Step 1
  DerivativeRelu(a_L_0, a_DRelu_0, rows, cols);
  
  // Step 2
  a_L_0.insert(a_L_0.begin(), cols);
  a_L_0.insert(a_L_0.begin(), rows);

  a_DRelu_0.insert(a_DRelu_0.begin(), cols);
  a_DRelu_0.insert(a_DRelu_0.begin(), rows);

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

  // Step 2: Perform matrix multiplication of arithmetic shares
  std::vector<std::uint64_t> a_L_ABY_public, a_L_ABY_private;
  std::vector<std::uint64_t> a_DRelu_ABY_public, a_DRelu_ABY_private;
  std::vector<std::uint8_t> a_L_msg_ABY_shares, a_DRelu_msg_ABY_shares;
  std::vector<std::uint8_t> a_L_msg_Private_Shares, a_DRelu_msg_Private_Shares;

  GeneratePrivateShares_ABY(a_L_0, a_L_ABY_private);
  a_L_ABY_public.resize(a_L_0.size());

  ParallelAddition(a_L_0, a_L_ABY_private, a_L_ABY_public, 2);
  ConvertVetorIntoMessage(a_L_ABY_public, a_L_msg_ABY_shares, (std::uint8_t)Relu_XArithToABY);
  std::cout << "X-ABY Publicshare message will be seding to the other party is : " << a_L_msg_ABY_shares.size()  << "\n";

  GeneratePrivateShares_ABY(a_DRelu_0, a_DRelu_ABY_private);
  a_DRelu_ABY_public.resize(a_DRelu_0.size());
  ParallelAddition(a_DRelu_0, a_DRelu_ABY_private, a_DRelu_ABY_public, 2); 
  ConvertVetorIntoMessage(a_DRelu_ABY_public, a_DRelu_msg_ABY_shares, (std::uint8_t)Relu_YArithToABY);

  // Prepare private shares messages from vectors deltaX0, deltaY0 to send helper node
  ConvertVetorIntoMessage(a_L_ABY_private, a_L_msg_Private_Shares, (std::uint8_t)Relu_X_PrivateShares);
  ConvertVetorIntoMessage(a_DRelu_ABY_private, a_DRelu_msg_Private_Shares, (std::uint8_t)Relu_Y_PrivateShares);

   //%%%%% Sending a_L_ABY shares from my party to other party nad woaut for message from other party 
  std::cout << "Sending a_L_ABY Public shares message to Server: " << 1-my_id << " Message size : "<< a_L_msg_ABY_shares.size() << "\n";
  try{
      comm_layer->send_message(1-my_id, a_L_msg_ABY_shares);
    }
  catch (std::runtime_error& e) {
      std::cerr << "Error occurred while sending a_L_msg_ABY_shares to node other part: " << e.what() << "\n";
      return EXIT_FAILURE;
    }

  std::cout << "Waiting for a_L_ABY Public shares message from Server: " << 1-my_id << "\n";
  while(!XABY_receive_flag)
    {
      std::cout<<"X";
      boost::this_thread::sleep_for(boost::chrono::milliseconds(200));
    }
  ParallelAddition(a_L_ABY_public, Relu_x_temp_vec, a_L_ABY_public);

  //%%%%% Sending a_DRelu_ABY shares from my party to other party nad woaut for message from other party 
  std::cout << "Sending a_DRelu_ABY Public shares message to Server : " << 1-my_id << " Message size : "<< a_DRelu_msg_ABY_shares.size() << "\n";
  try{
      comm_layer->send_message(1-my_id, a_DRelu_msg_ABY_shares);
    }
  catch (std::runtime_error& e) {
      std::cerr << "Error occurred while sending a_DRelu_msg_ABY_shares to node other part: " << e.what() << "\n";
      return EXIT_FAILURE;
    }

  std::cout << "Waiting for a_DRelu_ABY Public shares message from Server : " << 1-my_id << "\n";
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

  a_Relu_0.resize(dxdy.size());
  a_Relu_0[0] = dxdy[0];
  a_Relu_0[1] = dxdy[1];
 
  for(int i = 2; i < a_Relu_0.size(); i++)
    {
     a_Relu_0[i] = dxdy[i] + Relu_OT_vec[i] + (DxDy[i]>>1) - Dxdy[i] - dxDy[i];
    }
 
 std::cout << "\n *** Truncate **** \n";
  for(int i = 2; i < a_Relu_0.size(); i++)
  {
     a_Relu_0[i] =  MOTION::new_fixed_point::truncate(a_Relu_0[i], fractional_bits);
  }

  // Delete the dimensions (rows, cols) of the matrix.
  // a_Relu_0.erase(a_Relu_0.begin(), a_Relu_0.begin() + 2);

  std::cout << "Exiting ReLU." << std::endl;
  return 0; 
}

void ABYToArithShareGenerator(std::vector<std::uint64_t>& Public_shares, std::vector<std::uint64_t>& private_shares, std::vector<std::uint64_t>& ArithShares) {
  std::cout << "Entering ABY2.0ToArithShareGenerator." << std::endl;

  std::size_t len = Public_shares.size();
  ArithShares.resize(len);

  for (int i = 0; i < len; i++) {
    ArithShares[i] = (Public_shares[i] >> 1) - private_shares[i];
  }

  std::cout << "Exiting ABY2.0ToArithShareGenerator.\n" << std::endl;
  return;
}

int ArithToABYShareGenerator(std::vector<std::uint64_t> Relu_Arith_Shares, std::vector<std::uint64_t>& Relu_Public_Shares, std::vector<std::uint64_t>& Relu_Private_Shares) {
  std::cout << "Entering ArithToABYShareGenerator." << std::endl;
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

  std::cerr << Relu_Public_Shares[0] << " " << Relu_Public_Shares[1] << std::endl;
  ParallelAddition(Relu_Public_Shares, Relu_Public_Shares_Other_Party, Relu_Public_Shares, 2);
  std::cerr << Relu_Public_Shares_Other_Party[0] << " " << Relu_Public_Shares_Other_Party[1] << std::endl;

  std::cout << "Exiting ArithToABYShareGenerator." << std::endl;
  return 0;
}

// Reads ABY2.0 shares from the given options and converts the shares into messages to be sent to Party 1 and Party 2
// Input: an integer indicating the type of file to be read (1 for reading weight shares, 2 for reading image shares), an empty vector of type std::uint8_t to store the message to be sent and Options containing the filepath
// Output: Stores ABY2.0 shares in the global variables xpublic, xsecret, bpublic, bsecret, wpublic, wsecret.
int read_shares(int choice, int layer_id, int layer_type, std::vector<uint8_t>&message, const Options& options, bool convToMatMulFlag) { 
  std::string name = options.WB_File;

  // layer_type argument indicates the path from which the shares are being read.
  // if layer_type == 0, then the shares are read for matrix multiplication and addition 
  // if layer_type == 1, then the shares are read for convolution

  // choice argument indicates the nature of shares being read.
  // choice == 1, then the weight and bias shares are being read.
  // choice == 2, then image shares are being read.

  if (choice == 1 && layer_type == 1) { 
    std::ifstream content;
    std::cout << "Reading the Weight and Bias Shares.\n";
    std::string fullpath = options.current_path;
    fullpath += "/" + name;
    content.open(fullpath);

    if (!content.is_open()) {
      std::cerr << "Error in opening the weights config file.\n";
      exit(1);
    }

    // Retrieving the weights and biases filepath from the config file.
    std::string wpath, bpath;
    try {
      for (auto i = 0; i < layer_id; i++) {
        content >> wpath;    
        content >> bpath; 
      }
    }
    catch (std::ifstream::failure e) {
      std::cerr << "Error while reading the weights and bias path from config file.\n";
      exit(1);
    }
    std::cout << "Weights path: " << wpath << "\n" << "Bias path: " << bpath << "\n";
    
    std::ifstream file(wpath);
    if (!file) {
      std::cerr << "Error in opening the weights file.\n";
      exit(1);
    }

    // Dimensions of the convolution operation
    try {
      file >> conv_kernels >> conv_channels >> conv_rows >> conv_cols;

      for (int i = 0; i < 4; i++) {
        std::uint64_t temp;
        file >> temp;
        pads[i] = temp;
      }

      for (int i = 0; i < 2; i++) {
        std::uint64_t temp;
        file >> temp;
        strides[i] = temp;
      }

      std::uint64_t message_type = 1;
      adduint64(message_type, message);
      adduint64(conv_kernels, message);
      adduint64(conv_channels, message);
      adduint64(conv_rows, message);
      adduint64(conv_cols, message);

      for (int i = 0; i < 4; i++) {
        adduint64(pads[i], message);
      }

      for (int i = 0; i < 2; i++) {
        adduint64(strides[i], message);
      }
    }
    catch (std::ifstream::failure e) {
      std::cerr << "Error while reading rows and columns from weight shares file.\n";
      exit(1);
    }

    if (file.eof()) {
      std::cerr << "Weights file does not contain the shares." << std::endl;
      exit(1);
    }

    auto k = 0;
    while (k < conv_rows * conv_cols * conv_channels * conv_kernels) {
      std::uint64_t public_share, secret_share;
      try {    
        file >> public_share;
        wpublic.push_back(public_share);
        file >> secret_share;
        wsecret.push_back(secret_share);
      }
      catch (std::ifstream::failure e) {
        std::cerr << "Error while reading the weight shares.\n";
        exit(1);
      }
      if (file.eof()) {
        std::cerr << "Weight shares file contains less number of elements." << std::endl;
        exit(1);
      }
      adduint64(secret_share, message);
      k++;
    }

    std::cout << "Number of weight shares read: " << k << "\n"; 
    if (k == conv_rows * conv_cols * conv_channels * conv_kernels) {
      std::uint64_t num;
      file >> num;
      if (!file.eof()) {
        std::cerr << "Weight shares file contains more number of elements." << std::endl;
        exit(1);
      }
    }
    file.close();

    file.open(bpath);
    if (!file) {
      std::cerr << "Error in opening bias file.\n";
      exit(1);
    }
    try {
      file >> b_rows >> b_cols;
    }
    catch (std::ifstream::failure e) {
      std::cerr << "Error while reading rows and columns from bias shares file.\n";
      exit(1);
    }
    if (file.eof()) {
      std::cerr << "Bias shares file doesn't contain rows and columns" << std::endl;
      exit(1);
    }
       
    auto j = 0;
    while (j < b_rows * b_cols) {
      std::uint64_t public_share, secret_share;
      try {
        file >> public_share;
        bpublic.push_back(public_share);
        file >> secret_share;
        bsecret.push_back(secret_share);
      }
      catch (std::ifstream::failure e) {
        std::cerr << "Error while reading bias shares.\n";
        exit(1);
      }
      if (file.eof()) {
        std::cerr << "Bias shares file contains less number of elements." << std::endl;
        exit(1);
      }
      j++;
    }
    if (j == b_rows * b_cols) {
      std::uint64_t num;
      file >> num;
      if (!file.eof()) {
        std::cerr << "Bias shares file contains more number of elements." << std::endl;
        exit(1);
      }
    }
    file.close();
  }
  else if (choice == 1 && layer_type == 0) {
    std::ifstream content;
    std::cout << "Reading the Weight and Bias shares\n";
    std::string fullpath = options.current_path;
    fullpath += "/" + name;
    content.open(fullpath);
    if (!content.is_open()) {
      std::cerr << "Error in opening the weights config file\n";
      exit(1);
    }
    std::string wpath, bpath;
    // Increment until it reaches the weights and biases corresponding to the layer_id
    try {
      for (auto i = 0; i < layer_id; i++) {
        content >> wpath;    
        content >> bpath; 
      }
    }
    catch (std::ifstream::failure e) {
      std::cerr << "Error while reading the weights and bias path from config file\n";
      exit(1);
    }

    std::cout << "Weights path: " << wpath << "\nBias path: " << bpath << "\n";
    
    std::ifstream file(wpath);
    if (!file) {
      std::cerr << "Error in opening the weights file\n";
      exit(1);
    }

    std::uint64_t rows, col;
    try {
      file >> rows >> col;
    }
    catch (std::ifstream::failure e) {
      std::cerr << "Error while reading rows and columns from weight shares file.\n";
      exit(1);
    }

    if (file.eof()) {
      std::cerr << "Weights File doesn't contain the shares" << std::endl;
      exit(1);
    }

    auto k = 0;
    adduint64(rows, message);
    adduint64(col, message);
    wpublic.push_back(rows);
    wpublic.push_back(col);
    wsecret.push_back(rows);
    wsecret.push_back(col);
    while (k < rows * col) {
      std::uint64_t public_share, secret_share;
      try {     
        file >> public_share;
        wpublic.push_back(public_share);
        file >> secret_share;
        wsecret.push_back(secret_share);
      }
      catch (std::ifstream::failure e) {
        std::cerr << "Error while reading the weight shares.\n";
        exit(1);
      }
      if (file.eof()) {
        std::cerr << "Weight shares file contains less number of elements." << std::endl;
        exit(1);
      }
      adduint64(secret_share, message);
      k++;
    }

    std::cout << "Number of weight shares read: " << k << "\n"; 
    if (k == rows * col) {
      std::uint64_t num;
      file >> num;
      if (!file.eof()) {
        std::cerr << "Weight shares file contains more number of elements." << std::endl;
        exit(1);
      }
    }
    file.close();

    file.open(bpath);
    if (!file) {
      std::cerr << "Error in opening bias file\n";
      exit(1);
    }

    try {
      file >> rows >> col;
    }
    catch (std::ifstream::failure e) {
      std::cerr << "Error while reading rows and columns from bias shares file.\n";
      exit(1);
    }
    if (file.eof()) {
      std::cerr << "Bias shares file doesn't contain rows and columns." << std::endl;
      exit(1);
    }
       
    auto j = 0;
    bpublic.push_back(rows);
    bpublic.push_back(col);
    bsecret.push_back(rows);
    bsecret.push_back(col);

    while (j < rows * col) {
      std::uint64_t public_share, secret_share;
      try {
        file >> public_share;
        bpublic.push_back(public_share);
        file >> secret_share;
        bsecret.push_back(secret_share);
      }
      catch (std::ifstream::failure e) {
        std::cerr << "Error while reading bias shares.\n";
        exit(1);
      }
      if (file.eof()) {
        std::cerr << "Bias shares file contains less number of elements." << std::endl;
        exit(1);
      }
      j++;
    }

    if (j == rows * col) {
      std::uint64_t num;
      file >> num;
      if (!file.eof()) {
        std::cerr << "Bias shares file contains more number of elements." << std::endl;
        exit(1);
      }
    }
    file.close();
  } else if (choice == 2 && layer_type == 1) {
    // Reading the image shares from the respective filepath
    std::string fullpath = options.current_path;
    if (layer_id == 1) {
      fullpath += "/server" + std::to_string(my_id) + "/Image_shares/" + options.input_file;

      std::cout << "Input share file path: " << fullpath << std::endl;
      std::cout << "Reading the input shares\n";
  
      std::ifstream file(fullpath);
  
      if (!file) {
        std::cerr << "Error in opening input file at " << fullpath << "\n";
        exit(1);
      }
  
      try {
        file >> image_channels >> image_rows >> image_cols;
      }
      catch (std::ifstream::failure e) {
        std::cerr << "Error while reading rows and columns from input shares file.\n";
        exit(1);
      }
      if (file.eof()) {
        std::cerr << "Input shares file doesn't contain rows and columns" << std::endl;
        exit(1);
      }
  
      auto k = 0;
      std::uint64_t message_type = 2;
      adduint64(message_type, message);
      adduint64(image_channels, message);
      adduint64(image_rows, message);
      adduint64(image_cols, message);
  
      while (k < image_rows * image_cols * image_channels) {
        std::uint64_t public_share, secret_share;
        try {
          file >> public_share;
          xpublic.push_back(public_share);
          file >> secret_share;
          xsecret.push_back(secret_share);
        }
        catch (std::ifstream::failure e) {
          std::cerr << "Error while reading the input shares.\n";
          exit(1);
        }
        if (file.eof()) {
          std::cerr << "Input shares file contains less number of elements." << std::endl;
          exit(1);
        }
        adduint64(secret_share, message);
        k++;
      }
      if (k == image_rows * image_cols * image_channels) {
        std::uint64_t num;
        file >> num;
        if (!file.eof()) {
          std::cerr << "File contains more number of elements." << std::endl;
          exit(1);
        }
      }
      file.close();
    } 
    else if (layer_id > 1) {
      // path to the file outputshare_0 inside server 0
      image_channels = Output_Public_Shares[0];
      image_rows = Output_Public_Shares[1];
      image_cols = Output_Public_Shares[2];

      if (Output_Public_Shares.size() != image_channels * image_rows * image_cols + 3 || Output_Private_Shares.size() != image_channels * image_rows * image_cols + 3) {
        std::cerr << "Dimensions do not match. " << std::endl;
        return EXIT_FAILURE;
      }

      auto k = 0;
      std::uint64_t message_type = 2;
      adduint64(message_type, message);
      adduint64(image_channels, message);
      adduint64(image_rows, message);
      adduint64(image_cols, message);

      Output_Public_Shares.erase(Output_Public_Shares.begin(), Output_Public_Shares.begin() + 3);
      Output_Private_Shares.erase(Output_Private_Shares.begin(), Output_Private_Shares.begin() + 3);

      while (k < image_rows * image_cols * image_channels) {
        std::uint64_t public_share, secret_share;
        try {
          public_share = Output_Public_Shares[k];
          secret_share = Output_Private_Shares[k];
          xpublic.push_back(public_share);
          xsecret.push_back(secret_share);
        }
        catch (std::ifstream::failure e) {
          std::cerr << "Error while reading the input shares.\n";
          exit(1);
        }
        // Adding only the image secretshares_i in message 
        adduint64(secret_share, message);
        k++;
      }
    }
    Output_Public_Shares.clear();
    Output_Private_Shares.clear(); 
  }
  else if (choice == 2 && layer_type == 0) {
    if (layer_id == 1) {
      std::string fullpath = options.current_path;
      fullpath += "/server" + std::to_string(my_id) + "/Image_shares/" + options.input_file;
      // fullpath += "/server" + std::to_string(my_id) + "/" + options.input_file + "_" + std::to_string(my_id);

      std::cout << "Input share file path: " << fullpath << std::endl;
      std::cout << "Reading the input shares.\n";
  
      std::ifstream file(fullpath);
      if (!file) {
        std::cerr << "Error in opening input file at " << fullpath <<"\n";
        exit(1);
      }

      std::uint64_t rows, col;
      try {
        file >> rows >> col;
      }
      catch (std::ifstream::failure e) {
        std::cerr << "Error while reading rows and columns from input shares file.\n";
        exit(1);
      }
  
      if (file.eof()) {
        std::cerr << "Input shares file doesn't contain rows and columns." << std::endl;
        exit(1);
      }

      auto k = 0;
      adduint64(rows, message);
      adduint64(col, message);
      xpublic.push_back(rows);
      xpublic.push_back(col);
      xsecret.push_back(rows);
      xsecret.push_back(col);
  
      while (k < rows * col) {
        std::uint64_t public_share, secret_share;
        try {
          file >> public_share;
          xpublic.push_back(public_share);
          file >> secret_share;
          xsecret.push_back(secret_share);
        }
        catch (std::ifstream::failure e) {
          std::cerr << "Error while reading the input shares.\n";
          exit(1);
        }
        if (file.eof()) {
          std::cerr << "Input shares file contains less number of elements." << std::endl;
          exit(1);
        }
        adduint64(secret_share, message);
        k++;
      }
  
      if (k == rows * col) {
        std::uint64_t num;
        file >> num;
        if (!file.eof()) {
          std::cerr << "File contains more number of elements." << std::endl;
          exit(1);
        }
      }
      file.close();
    } else {
      std::uint64_t rows, col;
      if (convToMatMulFlag == true) {
        output_chnls = Output_Public_Shares[0];
        output_rows = Output_Public_Shares[1];
        output_columns = Output_Public_Shares[2];
        rows = output_chnls * output_rows * output_columns;
        col = 1;

        Output_Public_Shares.erase(Output_Public_Shares.begin(), Output_Public_Shares.begin() + 3);
        Output_Private_Shares.erase(Output_Private_Shares.begin(), Output_Private_Shares.begin() + 3);

        auto k = 0;
        adduint64(rows, message);
        adduint64(col, message);
        xpublic.push_back(rows);
        xpublic.push_back(col);
        xsecret.push_back(rows);
        xsecret.push_back(col);

        if (Output_Public_Shares.size() != rows * col || Output_Private_Shares.size() != rows * col) {
          std::cerr << "Dimensions do not match." << std::endl;
          return EXIT_FAILURE;
        }

        while (k < rows * col) {
          std::uint64_t public_share, secret_share;
          xpublic.push_back(Output_Public_Shares[k]);
          xsecret.push_back(Output_Private_Shares[k]);
          adduint64(secret_share, message);
          k++;
        }

        Output_Public_Shares.clear();
        Output_Private_Shares.clear();
      }
      else {
        std::uint64_t rows, col;
        rows = Output_Public_Shares[0];
        col = Output_Public_Shares[1];

        Output_Public_Shares.erase(Output_Public_Shares.begin(), Output_Public_Shares.begin() + 2);
        Output_Private_Shares.erase(Output_Private_Shares.begin(), Output_Private_Shares.begin() + 2);

        auto k = 0;
        adduint64(rows, message);
        adduint64(col, message);
        xpublic.push_back(rows);
        xpublic.push_back(col);
        xsecret.push_back(rows);
        xsecret.push_back(col);

        if (Output_Public_Shares.size() != rows * col || Output_Private_Shares.size() != rows * col) {
          std::cerr << "Dimensions do not match." << std::endl;
          return EXIT_FAILURE;
        }

        while (k < rows * col) {
          std::uint64_t public_share, secret_share;
          xpublic.push_back(Output_Public_Shares[k]);
          xsecret.push_back(Output_Private_Shares[k]);
          adduint64(secret_share, message);
          k++;
        }

        Output_Public_Shares.clear();
        Output_Private_Shares.clear();
      }
    }
  }
  return EXIT_SUCCESS;
}

int HelperNodeSyncFunction() {
  std::vector<std::uint8_t> started{(std::uint8_t)HelperNodeSync};
  std::cout << "Sending a probe message to the helper node.\n";
  try {
    comm_layer->send_message(helpernode_id, started);
  }
  catch (std::runtime_error& e) {
    std::cerr << "Error occurred while sending the start message to helper node: " << e.what() << "\n";
    return EXIT_FAILURE;
  }
  
  while (!helpernode_ready_flag) {
    std::cout << ".";
    boost::this_thread::sleep_for(boost::chrono::milliseconds(100));
  }
  return EXIT_SUCCESS;
}

int ConvolutionOperation(std::vector<std::uint8_t> message_w, std::vector<std::uint8_t> message_i) {
  std::cout << "Sending weight shares to the helper node.\n";
  try {
    message_w.insert(message_w.begin(), (std::uint8_t)ConvolutionOpMessage);
    comm_layer->send_message(helpernode_id, message_w);
  }
  catch (std::runtime_error& e) {
    std::cerr << "Error occurred while sending the weight shares to helper node: " << e.what() << "\n";
    return EXIT_FAILURE;
  }

  std::cout << "Sending image shares to the helper node.\n";
  try {
    message_i.insert(message_i.begin(), (std::uint8_t)ConvolutionOpMessage);
    comm_layer->send_message(helpernode_id, message_i);
  }
  catch (std::runtime_error& e) {
    std::cerr << "Error occurred while sending the input shares to helper node: " << e.what() << "\n";
    return EXIT_FAILURE;
  }

  // Waiting for the operations to complete. 
  std::cout << std::endl;
  while (operations_done_flag != 2) {
    std::cout << ".";
    boost::this_thread::sleep_for(boost::chrono::milliseconds(100));
  }
    
  // Creating message with publicshare_i 
  std::vector<std::uint8_t> mes0;
  mes0.push_back((std::uint8_t)ConvolutionOpMessage);

  for(int i = 0; i < prod1.size(); i++) {
    auto temp = prod1[i];
    adduint64(temp, mes0);
  }
    
  std::cout << "Sending Del_0 to the Party 1.\n";
  try {
    comm_layer->send_message(1, mes0);
  }
  catch (std::runtime_error& e) {
    std::cerr << "Error occurred while sending the output public share to Server-1: " << e.what() << "\n";
    return EXIT_FAILURE;
  }

  while (!conv_flag_1) {
    std::cout << ".";
    boost::this_thread::sleep_for(boost::chrono::milliseconds(100));
  }
  return 0;
}

std::vector<std::uint64_t> multiplicate(std::vector<uint64_t>& a, std::vector<uint64_t>& b) {   
  if (a[1] != b[0]) {
    std::cerr << "Error during matrix multiplication. Number of columns in W is not equal to the number of rows in X.";
    exit(1);
  }
  auto b_begin = b.begin();
  advance(b_begin, 2);
  std::vector<std::uint64_t> z;
  z.push_back(a[0]); //256
  z.push_back(b[1]); //1

  std::vector<std::uint64_t> tempw;
  int count = 2;
  for (int i = 0; i < a[0]; i++) { 
    tempw.push_back(a[0]);
    tempw.push_back(b[1]);
    for (int k = 0; k < a[1]; k++) {
      tempw.push_back(a[count]);
      count++;
    }
    auto tempw_begin = tempw.begin();
    auto tempw_end = tempw.end();
    advance(tempw_begin, 2);
    __gnu_parallel::transform(tempw_begin, tempw_end, b_begin, tempw_begin, std::multiplies{});     
    
    std::uint64_t sum = 0;
    for (int j = 2; j < tempw.size(); j++) {
      sum += tempw[j];
    }
    z.push_back(sum);
    tempw.clear();
  }
  return z;
}

void operationsMatrixMultiplication() {
  std::vector<std::uint64_t> w2;
  w2.resize(wpublic.size(), 0);
  for (int i = 0; i < wpublic.size(); i++) {
    w2[i] = wpublic[i];
  }

  // prod1 = wpublic * xpublic
  prod1 = multiplicate(wpublic, xpublic);
  auto prod1_begin = prod1.begin();
  auto prod1_end = prod1.end();
  advance(prod1_begin, 2);
  std::cout << "prod1 size: " << prod1.size() << "\n";

  // prod2 = Delw * delx1
  std::vector<std::uint64_t> prod2 = multiplicate(w2, xsecret);
  auto prod2_begin = prod2.begin();
  advance(prod2_begin, 2);
  std::cout << "prod2 size: " << prod2.size() << "\n";

  // prod3 = delw1 * Delx 
  std::vector<std::uint64_t> prod3 = multiplicate(wsecret, xpublic);
  std::cout << "prod3 size: " << prod3.size() << "\n";
  auto prod3_begin = prod3.begin();
  advance(prod3_begin, 2);

  // output(prod1) = prod1 - prod2 - prod3
  __gnu_parallel::transform(prod1_begin, prod1_end, prod2_begin, prod1_begin, std::minus{});
  __gnu_parallel::transform(prod1_begin, prod1_end, prod3_begin, prod1_begin, std::minus{});

  std::vector<std::uint64_t>::iterator r_begin = R.begin();
  advance(r_begin, 2);
 
 // output = Delx * Dely - Delx * dely - Dely * delx + R (R was received from s2) 
  __gnu_parallel::transform(prod1_begin, prod1_end, r_begin, prod1_begin, std::plus{});
  for (int i = 2; i < prod1.size(); i++) {
    prod1[i] = MOTION::new_fixed_point::truncate(prod1[i], fractional_bits);
  }
  
  randomnum.resize(prod1.size(), 0);
  randomnum[0] = prod1[0];
  randomnum[1] = prod1[1];
  
  for (int i = 2; i < prod1.size(); i++) { 
    std::random_device rd;
    std::mt19937 gen(rd());
    auto temp = RandomNumDistribution(gen);
    randomnum[i] = temp;
  }

  auto random_begin = randomnum.begin();
  advance(random_begin, 2);
  
  // output = output + random(local secret share)

  __gnu_parallel::transform(prod1_begin, prod1_end, random_begin, prod1_begin , std::plus{});   
  operations_done_flag++;
}

int MatrixMultAddition(std::vector<std::uint8_t>& message_w, std::vector<std::uint8_t>& message_i) {
  std::cout << "Sending weights shares to the helper node." << std::endl;
  try {
    message_w.insert(message_w.begin(), (std::uint8_t)MatrixMulAddOpMessage);
    comm_layer->send_message(helpernode_id, message_w);
  }
  catch (std::runtime_error& e) {
    std::cout << "Error occurred while sending weights to the helper node." << e.what() << std::endl;
    return EXIT_FAILURE;
  }

  std::cout << "Sending image shares to the helper node." << std::endl;
  try {
    message_i.insert(message_i.begin(), (std::uint8_t)MatrixMulAddOpMessage);
    comm_layer->send_message(helpernode_id, message_i);
  }
  catch (std::runtime_error & e) {
    std::cout << "Error occurred while sending image shares to the helper node." << e.what() << std::endl;
    return EXIT_FAILURE;
  }

  std::cout << std::endl;
  while (operations_done_flag != 2) {
    std::cout << ".";
    boost::this_thread::sleep_for(boost::chrono::milliseconds(100));
  }

  std::vector<std::uint8_t> mes0;
  mes0.push_back((std::uint8_t)MatrixMulAddOpMessage);

  for (int i = 0; i < prod1.size(); i++) {  
    auto temp = prod1[i];
    adduint64(temp, mes0);
  }

  std::cout << "Sending DEL_C0 to Party 1." << std::endl;
  try {
    comm_layer->send_message(other_party, mes0);
  }
  catch (std::runtime_error& e) {
    std::cerr << "Error occurred while sending the public share to party 1. " << e.what() << std::endl;
    return EXIT_FAILURE;
  }

  while (!matrix_add_mul_flag_1) {
    std::cout << ".";
    boost::this_thread::sleep_for(boost::chrono::milliseconds(100));
  }

  return EXIT_SUCCESS;
}

int ResetFlags(int type) {
  helpernode_ready_flag = false;
  operations_done_flag = 0;
  wpublic.clear();
  wsecret.clear();
  bpublic.clear();
  bsecret.clear();
  xpublic.clear();
  xsecret.clear();

  SC_P_flag = false;
  SC_D_flag = false;
  SC_PC_flag = false;
  helpernode_computeMSB_ready_flag = false;
  MSB_Odd_flag = false;
  MSB_P_flag = false;
  MSB_L_LSB_flag = false;
  MSB_R_flag = false;
  MSB_PC_flag = false;
  XABY_receive_flag = false;
  YABY_receive_flag = false;
  OT_flag = false;
  OtherPartySync_Flag = false;
  ArithToABY_Flag = false;
  helpernode_ReLU_ready_flag = false;
  relu_ready_flag = false;
  reluABYAck_Flag = false;

  x_pr_bit.clear();
  del_odd.clear();
  etaP_odd.clear();
  MSB_x_Odd.clear();
  MSB_x_P.clear();
  MSB_x_L_LSB.clear();
  R_Shares.clear();
  betaP.clear();
  x_temp_vec.clear();
  y_temp_vec.clear();
  OT_vec.clear();
  Relu_x_temp_vec.clear();
  Relu_y_temp_vec.clear();
  Relu_OT_vec.clear();
  Relu_Public_Shares_Other_Party.clear();

  // type == 1, ConvolutionReluLayer
  // type == 0, MatrixMultiplicationReluLayer
    conv_flag_1 = false;
    R.clear();
    prod1.clear();
    randomnum.clear();
    A_Conv_Public_Shares.clear();
    A_Conv_Private_Shares.clear();

    matrix_add_mul_flag_1 = false;
    R.clear();
    prod1.clear();
    randomnum.clear();
    A_MatMul_Public_Shares.clear();
    A_MatMul_Private_Shares.clear();

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

  int temp = 0;
  for (int i = 0; i < kernels; i++) {
  auto first = weights.begin() + temp;
  temp += channels * rows * cols;
  auto last = weights.begin() + temp;
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

  output_chnls = kernels;
  output_rows = (img_rows - rows + strides[0]) / strides[0];
  output_columns = (img_cols - cols + strides[1]) / strides[1];

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

  std::cout << "kernel_segments[k].size(): " << kernel_segments[0].size() << "\n";
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
  std::vector<std::uint64_t> w2;
  w2.resize(wpublic.size(), 0);
  for (int i = 0; i < wpublic.size(); i++) {
    w2[i] = wpublic[i];
  }

  // prod1 = wpublic * xpublic
  prod1 = convolution(xpublic, wpublic, conv_kernels, image_channels, conv_rows, conv_cols, pads, strides, image_rows, image_cols);

  // prod2 = Delw * delx1
  std::vector<std::uint64_t> prod2 = convolution(xsecret, w2, conv_kernels, image_channels, conv_rows, conv_cols, pads, strides, image_rows, image_cols);

  // prod3 = delw1 * Delx 
  std::vector<std::uint64_t> prod3 = convolution(xpublic, wsecret, conv_kernels, image_channels, conv_rows, conv_cols, pads, strides, image_rows, image_cols);

  // output(prod1) = prod1 - prod2 - prod3
  __gnu_parallel::transform(prod1.begin(), prod1.end(), prod2.begin(), prod1.begin(), std::minus{});
  __gnu_parallel::transform(prod1.begin(), prod1.end(), prod3.begin(), prod1.begin(), std::minus{});

  // output = Delx * Dely - Delx * dely - Dely * delx + R (R received from helpernode) 
  __gnu_parallel::transform(prod1.begin(), prod1.end(), R.begin(), prod1.begin(), std::plus{});
    
  for (int i = 0; i < prod1.size(); i++) {
    prod1[i] = MOTION::new_fixed_point::truncate(prod1[i], fractional_bits);
  }

  // randomnum is server_i secret share 
  randomnum.resize(prod1.size(), 0);
  for(int i = 0; i < prod1.size(); i++) { 
    std::random_device rd;
    std::mt19937 gen(rd());
    auto temp = RandomNumDistribution(gen);
    randomnum[i] = temp;
  }

  // output = output + random local secret share
  __gnu_parallel::transform(prod1.begin(), prod1.end(), randomnum.begin(), prod1.begin(), std::plus{});   

  operations_done_flag++;
}

int ConvolutionReluLayer(std::vector<std::uint8_t>& weightSharesMessage, std::vector<std::uint8_t>& imageSharesMessage, int layerId) {
  A_Conv_Public_Shares.clear();
  A_Conv_Private_Shares.clear();

  HelperNodeSyncFunction();

  ConvolutionOperation(weightSharesMessage, imageSharesMessage);
  std::cerr << "Layer " << layerId << ": Convolution layer done." << std::endl;

  try {
    std::vector<std::uint8_t> reluABYConvAck{(std::uint8_t)reluABYAck};
    comm_layer->send_message(helpernode_id, reluABYConvAck);
  }
  catch (std::exception& e) {
    std::cerr << "Error occurred while sending the message: " << e.what() << std::endl;
    return EXIT_FAILURE;
  }

  while (!reluABYAck_Flag) {
    std::cout << ".";
    boost::this_thread::sleep_for(boost::chrono::milliseconds(100));
  }

  std::size_t reluRows = A_Conv_Public_Shares[0];
  std::size_t reluCols = A_Conv_Public_Shares[1];

  std::size_t privateReluRows = A_Conv_Private_Shares[0];
  std::size_t privateReluCols = A_Conv_Private_Shares[1];

  if ((reluRows != privateReluRows) || (reluCols != privateReluCols)) {
    std::cerr << "Dimensions of the output shares are incorrect." << std::endl;
    return EXIT_FAILURE;
  }

  A_Conv_Public_Shares.erase(A_Conv_Public_Shares.begin(), A_Conv_Public_Shares.begin() + 2);
  A_Conv_Private_Shares.erase(A_Conv_Private_Shares.begin(), A_Conv_Private_Shares.begin() + 2);

  std::vector<std::uint64_t> A_L;
  std::vector<std::uint64_t> A_Relu;

  ABYToArithShareGenerator(A_Conv_Public_Shares, A_Conv_Private_Shares, A_L);

  ReLU(A_L, A_Relu, reluRows, reluCols);

  ArithToABYShareGenerator(A_Relu, Output_Public_Shares, Output_Private_Shares);
  
  std::cerr << "Layer " << layerId << ": ReLU operation done." << std::endl;

  std::size_t rowsABYShares = Output_Public_Shares[0];
  std::size_t colsABYShares = Output_Public_Shares[1];

  // Verifying that the dimensions of the original shares and the ABY2.0 ReLU shares match
  if (rowsABYShares * colsABYShares != output_rows * output_columns * output_chnls) {
    std::cerr << "Dimensions of the arithmetic shares and the ABY shares do not match." << std::endl;
    return EXIT_FAILURE;
  }

  Output_Public_Shares.erase(Output_Public_Shares.begin(), Output_Public_Shares.begin() + 2);
  Output_Private_Shares.erase(Output_Private_Shares.begin(), Output_Private_Shares.begin() + 2);

  Output_Public_Shares.insert(Output_Public_Shares.begin(), output_columns);
  Output_Public_Shares.insert(Output_Public_Shares.begin(), output_rows);
  Output_Public_Shares.insert(Output_Public_Shares.begin(), output_chnls);

  Output_Private_Shares.insert(Output_Private_Shares.begin(), output_columns);
  Output_Private_Shares.insert(Output_Private_Shares.begin(), output_rows);
  Output_Private_Shares.insert(Output_Private_Shares.begin(), output_chnls);

  ResetFlags(1);

  return EXIT_SUCCESS;
}

int MatrixMultiplicationReluLayer(std::vector<std::uint8_t>& weightSharesMessage, std::vector<std::uint8_t>& imageSharesMessage, int layerId) {
  A_Conv_Public_Shares.clear();
  A_Conv_Private_Shares.clear();
  Output_Public_Shares.clear();
  Output_Private_Shares.clear();
  
  HelperNodeSyncFunction();

  MatrixMultAddition(weightSharesMessage, imageSharesMessage);

  std::cerr << "Layer " << layerId << ": Matrix addition and multiplication done." << std::endl;
  try {
    std::vector<std::uint8_t> reluABYConvAck{(std::uint8_t)reluABYAck};
    comm_layer->send_message(helpernode_id, reluABYConvAck);
  }
  catch (std::exception& e) {
    std::cerr << "Error occurred while sending the message: " << e.what() << std::endl;
    return EXIT_FAILURE;
  }

  while (!reluABYAck_Flag) {
    std::cout << ".";
    boost::this_thread::sleep_for(boost::chrono::milliseconds(100));
  }

  std::size_t reluRows = A_MatMul_Public_Shares[0];
  std::size_t reluCols = A_MatMul_Public_Shares[1];

  std::size_t privateReluRows = A_MatMul_Private_Shares[0];
  std::size_t privateReluCols = A_MatMul_Private_Shares[1];

  if ((reluRows != privateReluRows) || (reluCols != privateReluCols)) {
    std::cerr << "Dimensions do not match." << std::endl;
    return EXIT_FAILURE;
  }  

  A_MatMul_Public_Shares.erase(A_MatMul_Public_Shares.begin(), A_MatMul_Public_Shares.begin() + 2);
  A_MatMul_Private_Shares.erase(A_MatMul_Private_Shares.begin(), A_MatMul_Private_Shares.begin() + 2);

  std::vector<std::uint64_t> A_L, A_Relu;
  ABYToArithShareGenerator(A_MatMul_Public_Shares, A_MatMul_Private_Shares, A_L);

  ReLU(A_L, A_Relu, reluRows, reluCols);
  std::cerr << Output_Public_Shares.size() << " " << Output_Private_Shares.size() << std::endl;
  ArithToABYShareGenerator(A_Relu, Output_Public_Shares, Output_Private_Shares);

  std::size_t rowsABYShares = Output_Public_Shares[0];
  std::size_t colsABYShares = Output_Public_Shares[1];

  if (reluRows * reluCols != rowsABYShares * colsABYShares) {
    std::cerr << "Dimensions do not match." << std::endl;
    std::cerr << rowsABYShares << " " << colsABYShares << std::endl;
    std::cerr << Output_Private_Shares[0] << " " << Output_Private_Shares[1] << std::endl;
    return EXIT_FAILURE;
  }

  std::cerr << "Layer " << layerId << ": ReLU done." << std::endl;
  ResetFlags(0);

  return EXIT_SUCCESS;
}

int MatrixMultiplicationLayer(std::vector<std::uint8_t>& weightSharesMessage, std::vector<std::uint8_t>& imageSharesMessage, int layerId) {
  HelperNodeSyncFunction();

  MatrixMultAddition(weightSharesMessage, imageSharesMessage);

  std::cerr << "Layer " << layerId << ": Matrix addition and multiplication done." << std::endl;

  std::size_t reluRows = A_MatMul_Public_Shares[0];
  std::size_t reluCols = A_MatMul_Public_Shares[1];

  std::size_t privateReluRows = A_MatMul_Private_Shares[0];
  std::size_t privateReluCols = A_MatMul_Private_Shares[1];

  if ((reluRows != privateReluRows) || (reluCols != privateReluCols)) {
    std::cerr << "Dimensions of the public and the private shares do not match." << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

#endif