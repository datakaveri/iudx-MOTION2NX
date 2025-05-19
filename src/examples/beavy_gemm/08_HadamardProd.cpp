#include <boost/archive/text_oarchive.hpp>
#include <boost/asio.hpp>
#include <boost/chrono.hpp>
#include <boost/program_options.hpp>
#include <boost/serialization/string.hpp>
#include <boost/thread.hpp>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <parallel/algorithm>
#include <random>
#include <stdexcept>
#include <type_traits>
#include <vector>
#include "./fixed-point.h"

using namespace boost::asio;
namespace po = boost::program_options;
using ip::tcp;
typedef std::uint64_t myType;
const myType minus_one = (myType) - 1;
#define MAX_CONNECT_RETRIES 50

//%%%%%%%%%%%%%%%%% START GLOBAL VARIABLES %%%%%%%%%%%%%%%%%%%%%%%
const auto LMINUS_ONE = std::numeric_limits<std::uint64_t>::max();
const uint64_t PRIME_NUM = 67;
uint64_t addModPrime[PRIME_NUM][PRIME_NUM];
uint64_t multModPrime[PRIME_NUM][PRIME_NUM];
uint64_t subModPrime[PRIME_NUM][PRIME_NUM];
const auto BIT_SIZE = 64;
const auto FIXED_POINT = 13;
//%%%%%%%%%%%%%%%%% END GLOBAL VARIABLES %%%%%%%%%%%%%%%%%%%%%%%%%

//******** START : RandomNumberGenerators*********************** 
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

// Generates shares of data in L world
// Input  : a (vector of length len)
// Output : a0, a1 (vectors of length len)
// a[i]   : 64-bit unsigned integer
void GenerateSharesOverL(std::vector<uint64_t>& a0, std::vector<uint64_t>& a1, std::vector<uint64_t>& a, std::size_t len)
{
std::cout << " Entered GenerateSharesOverL \n";
std::uint64_t t;
std::random_device rd;
std::mt19937 gen(rd());
for(int i = 0; i < len; i++)
  {
    a0[i] = RandomNumDistribution(gen); // 0;// zeroed randmoness
    a1[i] = a[i] - a0[i];
  }
std::cout << " Exited GenerateSharesOverL \n";
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
      std::uint64_t t = RandomNumDistribution(gen);; // zeroed randomness, RandomNumDistribution(gen);
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

void HadamardMatrixMultiplication_P2(std::vector<std::uint64_t>& X_ABY_Private_0, std::vector<std::uint64_t>& Y_ABY_Private_0, std::vector<std::uint64_t>& X_ABY_Private_1, std::vector<std::uint64_t>& Y_ABY_Private_1, std::vector<std::uint64_t>& OT_Shares_0, std::vector<std::uint64_t>& OT_Shares_1) 
{
  std::vector<std::uint64_t> X0Y1, X1Y0;

  // Computing product
  HadamardMatrixMultiplication(X_ABY_Private_0, Y_ABY_Private_1, X0Y1);
  HadamardMatrixMultiplication(X_ABY_Private_1, Y_ABY_Private_0, X1Y0);

  // OT Generation
  GeneratePrivateShares_ABY(X0Y1, OT_Shares_0);
  OT_Shares_1.resize(OT_Shares_0.size());
  OT_Shares_1[0] = OT_Shares_0[0];
  OT_Shares_1[1] = OT_Shares_0[1];

  for (int index = 2; index < OT_Shares_0.size(); index++) {
    OT_Shares_1[index] = X0Y1[index] + X1Y0[index] - OT_Shares_0[index];
  }
}

// Takes arithmetic shares of input: X received from Party 0 and Y from Party 1 respectively, converts them into ABY2.0 shares and performs hadamard matrix multiplication of Z = X . Y
// First two elements of all shares are the dimensions of the matrix: rows and cols
// Input  :
// Output :
void ArithHadamardMatrixMultiplication(std::vector<std::uint64_t>& X_shares_0, std::vector<std::uint64_t>& X_shares_1, std::vector<std::uint64_t>& Y_shares_0, std::vector<std::uint64_t>& Y_shares_1, std::vector<std::uint64_t>& Z_shares_0, std::vector<std::uint64_t>& Z_shares_1, std::size_t rows, std::size_t cols) 
{
  std::cout << "Entering ArithHadamardMatrixMultiplication" << std::endl;

  X_shares_0.insert(X_shares_0.begin(), cols);
  X_shares_0.insert(X_shares_0.begin(), rows);

  X_shares_1.insert(X_shares_1.begin(), cols);
  X_shares_1.insert(X_shares_1.begin(), rows);

  Y_shares_0.insert(Y_shares_0.begin(), cols);
  Y_shares_0.insert(Y_shares_0.begin(), rows);

  Y_shares_1.insert(Y_shares_1.begin(), cols);
  Y_shares_1.insert(Y_shares_1.begin(), rows);

  std::vector<std::uint64_t> X_ABY_public_0, X_ABY_private_0;
  std::vector<std::uint64_t> X_ABY_public_1, X_ABY_private_1;
  std::vector<std::uint64_t> Y_ABY_public_0, Y_ABY_private_0;
  std::vector<std::uint64_t> Y_ABY_public_1, Y_ABY_private_1;
  std::vector<std::uint64_t> Z_ABY_public_0, Z_ABY_private_0;
  std::vector<std::uint64_t> Z_ABY_public_1, Z_ABY_private_1;
  std::vector<std::uint64_t> X_ABY_public, Y_ABY_public;

  std::size_t lengthWithDim = X_shares_0.size();

  // @ Party 0
  GeneratePrivateShares_ABY(X_shares_0, X_ABY_private_0);
  X_ABY_public_0.resize(lengthWithDim);
  ParallelAddition(X_shares_0, X_ABY_private_0, X_ABY_public_0, 2); // Arithmetic share + private share

  GeneratePrivateShares_ABY(Y_shares_0, Y_ABY_private_0);
  Y_ABY_public_0.resize(lengthWithDim);
  ParallelAddition(Y_shares_0, Y_ABY_private_0, Y_ABY_public_0, 2);

  // @ Party 1
  GeneratePrivateShares_ABY(X_shares_1, X_ABY_private_1);
  X_ABY_public_1.resize(lengthWithDim);
  ParallelAddition(X_shares_1, X_ABY_private_1, X_ABY_public_1, 2); // Arithmetic share + private share

  GeneratePrivateShares_ABY(Y_shares_1, Y_ABY_private_1);
  Y_ABY_public_1.resize(lengthWithDim);
  ParallelAddition(Y_shares_1, Y_ABY_private_1, Y_ABY_public_1, 2);

  // Communication between P0 and P1 to reconstruct public shares 
  // Common to @ Party 0 and @ Party 1
  X_ABY_public.resize(lengthWithDim);
  ParallelAddition(X_ABY_public_0, X_ABY_public_1, X_ABY_public, 2);

  Y_ABY_public.resize(lengthWithDim);
  ParallelAddition(Y_ABY_public_0, Y_ABY_public_1, Y_ABY_public, 2);

  // Send private shares to Party 2
  std::vector<std::uint64_t> OT_0, OT_1;
  HadamardMatrixMultiplication_P2(X_ABY_private_0, Y_ABY_private_0, X_ABY_private_1, Y_ABY_private_1, OT_0, OT_1);

  // Hadamard Computations
  // @ Party 0
  std::vector<std::uint64_t> dxdy_0, DxDy_0, Dxdy_0, dxDy_0;
  HadamardMatrixMultiplication(X_ABY_private_0, Y_ABY_private_0, dxdy_0);
  HadamardMatrixMultiplication(X_ABY_public, Y_ABY_public, DxDy_0);
  HadamardMatrixMultiplication(X_ABY_public, Y_ABY_private_0, Dxdy_0);
  HadamardMatrixMultiplication(X_ABY_private_0, Y_ABY_public, dxDy_0);

  // @ Party 1
  std::vector<std::uint64_t> dxdy_1, DxDy_1, Dxdy_1, dxDy_1;
  HadamardMatrixMultiplication(X_ABY_private_1, Y_ABY_private_1, dxdy_1);
  HadamardMatrixMultiplication(X_ABY_public, Y_ABY_public, DxDy_1);
  HadamardMatrixMultiplication(X_ABY_public, Y_ABY_private_1, Dxdy_1);
  HadamardMatrixMultiplication(X_ABY_private_1, Y_ABY_public, dxDy_1);

  // Computation of product shares
  // @ Party 0

  Z_shares_0.resize(lengthWithDim);
  Z_shares_0[0] = rows;
  Z_shares_0[1] = cols;
  for (int index = 2; index < Z_shares_0.size(); index++) {
    Z_shares_0[index] = dxdy_0[index] + OT_0[index] + (DxDy_0[index] >> 1) - Dxdy_0[index] - dxDy_0[index];
    Z_shares_0[index] = MOTION::new_fixed_point::truncate(Z_shares_0[index], FIXED_POINT);
  }

  // @ Party 1
  Z_shares_1.resize(lengthWithDim);
  Z_shares_1[0] = rows;
  Z_shares_1[1] = cols;
  for (int index = 2; index < Z_shares_1.size(); index++) {
    Z_shares_1[index] = dxdy_1[index] + OT_1[index] + (DxDy_1[index] >> 1) - Dxdy_1[index] - dxDy_1[index];
    Z_shares_1[index] = MOTION::new_fixed_point::truncate(Z_shares_1[index], FIXED_POINT);
  }

  Z_shares_0.erase(Z_shares_0.begin(), Z_shares_0.begin() + 2);
  Z_shares_1.erase(Z_shares_1.begin(), Z_shares_1.begin() + 2);

  std::cout << "Exiting ArithHadamardMatrixMultiplication" << std::endl;
}


int main(int argc, char* argv[]) {
  std::cout << MOTION::new_fixed_point::decode<uint64_t, long double>(12288, 13) << std::endl;
  std::random_device rd;
  std::mt19937 gen(rd()); 
  InitializeModuloPrimeOps();

  int rows = 3;
  int cols = 2;
  int len = rows * cols;

  // Defining the data
  std::vector<float> dataA(len, 0.0), dataB(len, 0.0);
  std::vector<std::uint64_t> a(len, 0), b(len, 0);
  std::vector<std::uint64_t> a_L_Shares_0(len, 0), a_L_Shares_1(len, 0);
  std::vector<std::uint64_t> b_L_Shares_0(len, 0), b_L_Shares_1(len, 0);
  std::vector<std::uint64_t> productShares_0(len, 0), productShares_1(len, 0);

  dataA[0] = 1;
  dataA[1] = 2;
  dataA[2] = 3;
  dataA[3] = 1.5;
  dataA[4] = -6;
  dataA[5] = 1; 

  dataB[0] = 1;
  dataB[1] = 2;
  dataB[2] = 3;
  dataB[3] = 0.4;
  dataB[4] = 3.4;
  dataB[5] = -1; 

  for (int index = 0; index < len; index++) {
    a[index] = MOTION::new_fixed_point::encode<uint64_t, long double>(dataA[index], FIXED_POINT);
    b[index] = MOTION::new_fixed_point::encode<uint64_t, long double>(dataB[index], FIXED_POINT);
    std::cout << a[index] << " " << b[index] << std::endl;
  }

  GenerateSharesOverL(a_L_Shares_0, a_L_Shares_1, a, len);
  GenerateSharesOverL(b_L_Shares_0, b_L_Shares_1, b, len);

  // Hadamard Matrix Multiplication
  ArithHadamardMatrixMultiplication(a_L_Shares_0, a_L_Shares_1, b_L_Shares_0, b_L_Shares_1, productShares_0, productShares_1, rows, cols);

  for (int index = 2; index < a_L_Shares_0.size(); index++) {
    std::uint64_t sumOfShares = productShares_0[index] + productShares_1[index];
    auto testVal = MOTION::new_fixed_point::decode<uint64_t, long double>(sumOfShares, FIXED_POINT);

    std::cout << "Error detected at: " << index - 2 << ", testVal[i]: " << testVal << ", Actual product: " << dataA[index - 2] * dataB[index - 2] << std::endl;
  }

  return EXIT_SUCCESS;
}