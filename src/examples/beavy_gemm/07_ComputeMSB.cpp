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
#include <limits>
#include <optional>
#include <parallel/algorithm>
#include <random>
#include <stdexcept>
#include <type_traits>
#include <vector>
#include "./fixed-point.h"
void ComputeMSB_P2(std::vector<uint64_t>& x0_odd, std::vector<uint64_t>& x1_odd, std::vector<uint64_t>& x0_bit_pr, std::vector<uint64_t>& x1_bit_pr, std::vector<uint64_t>& x0_lsb, std::vector<uint64_t>& x1_lsb,std::size_t len);
void ShareConvert_P2(std::vector<uint64_t>& a_tilde0, std::vector<uint64_t>& a_tilde1, std::vector<uint64_t>& x0_bit, std::vector<uint64_t>& x1_bit, std::vector<uint64_t>& delta0, std::vector<uint64_t>& delta1,std::size_t len);
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
//Prototypes


/*******************************************************************************/
//This file implements standalone code for PrivaeCompare
//x - owner is Party2, creates L-1 shares and prime shares for each bit in x
// L shares for x[0] and communicates Party 0 and Party 1
// Assumes at Party 0 and Party1  have following data
//  L-1 shares of x, prime shares of x and L shares of x[0]
//@ Party 0 and Party 1 compute have r_i =  (x_i(L-1) + y_i(L-1))
// x_i(L-1) received from Party 2, y_i(L-1) each paty has it
// r in clear (after communication between Party 0 and party 1)
// where r = (x_0(L-1) + y_0(L-1)) + (x_1(L-1) + y_1(L-1))
//
// stand alone implemetatiom, we create x_i(L-1),x_(prime),x[0]_i(L)
// y_i(L-1) 
/*****************************************************************************/
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
//used at P0 and P1
void GenerateRandomPermutation(std::vector<std::uint64_t>& nums, std::size_t start,  std::size_t end, std::uint64_t seed = 1234) {
  std::cout << " Entered GenerateRandomPermutation \n ";
    std::mt19937 gen(seed); // Initialize  with the given seed
    std::shuffle(nums.begin()+start, nums.begin()+end, gen); // Shuffle the vector with the seeded generator

    // std::cout << "A random permutation of the given set is: ";
    // for (int num : nums) {
    //     std::cout << num << " ";
    // }
    // std::cout << std::endl;
}
/********************Start L-1 Operations************************/
inline std::uint64_t WrapAround(std::uint64_t a, std::uint64_t b) {
  return (a > LMINUS_ONE - b);} 

void WrapAround(std::vector<std::uint64_t> &a0, std::vector<std::uint64_t> &a1, std::vector<std::uint64_t> &ans, std::size_t len = 1)
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

void AddModuloOdd(std::vector<std::uint64_t> &a0, std::vector<std::uint64_t> &a1, std::vector<std::uint64_t> &ans, std::size_t len)
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
//**********************END L-1 operations************************
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

// Generates bit shares over prime ring
//Input : x (vetor oh length len )
//Output : bit_shares_x0, bit_shares_x1 : vectors of length len*64
// x[i]: 64 bit integer 
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
    t = RandomNumOverPrime(gen); // 0; zeroed randmoness
    bit_shares_x0[i*BIT_SIZE + k] = t;
    //Note that we are starting from MSB so that in PrivateCompare, for loop starts k = 0, k++; instead k = 64 and k-- 
    auto bt = (x[i] >> BIT_SIZE-1 - k) & 1; 
    //bit_shares_x1[i*BIT_SIZE + k] = bt - bit_shares_x0[i*BIT_SIZE + k]
    bit_shares_x1[i*BIT_SIZE + k] = subModPrime[bt][t];
  }
}
}
void GenerateLSBSharesOverL(std::vector<std::uint64_t> &lsb_shares_x0, std::vector<std::uint64_t> &lsb_shares_x1, std::vector<std::uint64_t> &x, std::size_t len )
{
std:: cout << " Entered GenerateLSBSharesOverL \n";
std::uint64_t t;
std::random_device rd;
std::mt19937 gen(rd());
//bit_shares_x1[i*BIT_SIZE + k] = bt - bit_shares_x0[i*BIT_SIZE + k]
for(int i = 0; i< len; i++)
 {
   lsb_shares_x0[i] = RandomNumDistribution(gen); // 0;// zeroed randmoness
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

void GenerateSharesOverL(std::vector<uint64_t>& a0, std::vector<uint64_t>& a1, std::vector<uint64_t>& a, std::size_t len)
{
std:: cout << " Entered GenerateSharesOverL \n";
std::uint64_t t;
std::random_device rd;
std::mt19937 gen(rd());
for(int i = 0; i < len; i++)
  {
    a0[i] = RandomNumDistribution(gen); // 0;// zeroed randmoness
    a1[i] = a[i] - a0[i];
  }
std:: cout << " Exited GenerateSharesOverL \n";
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
      std::uint64_t t =  RandomNumDistribution(gen); // 2; // zeroed randomness, 
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
  std::cout << "Entered Hadamard Product." << std::endl;

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
  std::cout << "Exited Hadamard Product.\n" << std::endl;
  return 1;
}

std::vector<std::uint64_t> PrivateCompare(std::vector<std::uint64_t> &bit_share_x0, std::vector<std::uint64_t> &r, std::vector<std::uint64_t> &beta, std::size_t len, int party_id) 
{
std::cout << "Eneterd PrivateCompare : " << party_id << "  \n";  
std::uint64_t value_r, value_beta;
std::uint64_t t, j, ind, w, temp_prime_sh;
std::vector<std::uint64_t> c(len*BIT_SIZE);

std::mt19937 gen_u(1234); 
std::mt19937 gen_s(5678); 
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
    temp_prime_sh = bit_share_x0[ind];
    if (r_bit == 0)
       w = AddModuloPrime(w, temp_prime_sh);
    else 
       w = AddModuloPrime(w, SubtractModuloPrime(party_id, temp_prime_sh));
    if (value_beta == 0)
    {
     if (party_id == 1) c[ind] = AddModuloPrime(c[ind], party_id + r_bit);
     c[ind] = SubtractModuloPrime(c[ind], temp_prime_sh); 
    }
    else if ((value_beta == 1) & (r[i] != LMINUS_ONE))
         {
          
          if (party_id == 1) c[ind] = AddModuloPrime(c[ind], party_id - r_bit);
          c[ind] = AddModuloPrime(c[ind], temp_prime_sh); 
         }
    else //of value_beta == 1 and r[i] == LMINUS_ONE
         {
          c[ind] = RandomNonZeroNumOverPrime(gen_u);
          if (party_id ==  1) // k = 0 at p0, p1, c[ind], -c[ind]; k>0 c[ind], 1-c[ind]
             c[ind] = SubtractModuloPrime((k != 0), c[ind]);
         }
    //std::cout << "c at ind : "<< ind << " is " << c[ind] << "\n";
    c[ind] = MultiplyModuloPrime(c[ind], RandomNonZeroNumOverPrime(gen_s));
  }
GenerateRandomPermutation(c, i*BIT_SIZE, (i+1)*BIT_SIZE);
}
return c;
}


void PrivateCompare_P2(std::vector<std::uint64_t>& c0, std::vector<std::uint64_t>& c1, std::vector<std::uint64_t>& beta_prime, std::size_t len )
{
// It has to wait for the vector d0 and d1 then compute beta_prime
std::cout << "Eneterd PrivateCompare_P2 : " << "  \n";
std::uint64_t ind, k;
for(int i = 0; i<len;  i++)
  {
    for( k = 0; k<BIT_SIZE;  k++)
    {
      ind = i*BIT_SIZE +k;
      //std::cout << c0[ind] << " , " << c1[ind] << "  , " << addModPrime[c0[ind]][c1[ind]] << "\n";
      // std::cout << addModPrime[c0[ind]][c1[ind]] << "\n";
      if (AddModuloPrime(c0[ind], c1[ind]) == 0) //x>r
        {
          beta_prime[i] = 1;
          //std :: cout << c0[ind] << c1[ind] << "index : " << ind << "\n"; 
          break;
        }
    }
 
  }
}
/*****************PrivateCompare_2 End********************************/

/***************************ShareConvert Start *****************************/
 //HELPER FUNCTIONS
void CommonRandVectorOverL(std::vector<uint64_t>& q, int len, int seed)
{
  std::cout << "CommonRandVectorOverL \n ";
  std::mt19937 gen_com_L(seed); 
  for (int i = 0; i < len; i++)
  {
    q[i] = RandomNumDistribution(gen_com_L);
  }
}
void CommonRandBitVector(std::vector<uint64_t>& q, int len, int seed)
{
  std::cout << "CommonRandBitVector  \n";
  std::mt19937 gen_com_L(seed); 
  for (int i = 0; i < len; i++)
  {
    q[i] = RandomNumDistribution(gen_com_L) & 1;
  }
}
void AddVectorsOverL(std::vector<uint64_t>& a, std::vector<uint64_t>& b, std::vector<uint64_t>& ans, std::size_t len )
{
  std:: cout << " AddVectorsOverL \n ";
  for(int i = 0;i < len; i++) ans[i] = a[i] + b[i];
    
}

// P2 Generates x ad corresponding L-1 shares and prime bit shares sends to P0 and P1
// P0 and P1 compute r using L-1 : x_L-1(from P2) and 2*(a_L-1 )
void ShareConvert_P2(std::vector<uint64_t>& a_tilde0, std::vector<uint64_t>& a_tilde1, std::vector<uint64_t>& x0_bit, std::vector<uint64_t>& x1_bit, std::vector<uint64_t>& delta0, std::vector<uint64_t>& delta1,std::size_t len)
{
  std::cout << "ShareConvert_P2 \n";
  std::vector<uint64_t> x(len, 0), delta(len, 0);
  AddVectorsOverL(a_tilde0, a_tilde1, x, len);
  WrapAround(a_tilde0, a_tilde1, delta, len);
  GenerateBitSharesOverPrime(x0_bit, x1_bit, x, len);
  GenerateModuloOddShares(delta0, delta1, delta, len);
  // send x0_bit, delta_0 to P0 and  x1_bit,delta_1 to P1
  // and call PrivateCompare_2() that is waiting receiv shares from P0 and P1
  std::cout << "******************************************* \n";
}

// ShareConvert: Converts shares of a in L world to shares of a in L-1 world
// Input  : Vectors a_L_shares0 and a_L_shares1 of type std::uint64_t storing shares of a in L world at P0 and P1 respectively, empty vectors of type std::uint64_t a_Lmiusone_shares0 and a_Lmiusone_shares1 to store the L-1 shares.
// Output : Updates a_Lmiusone_shares0 and a_Lmiusone_shares1 to store odd shares of a at P0 and P1 respectively.
void ShareConvert(std::vector<uint64_t>& a_L_shares0, std::vector<uint64_t>& a_L_shares1,std::vector<uint64_t>& a_Lmiusone_shares0, std::vector<uint64_t>& a_Lmiusone_shares1, std::size_t len, int party)
{
  std::cout << "ShareConvert \n";
  //Common to both parties starts
  std::vector<uint64_t> r(len, 0), r0(len, 0), r1(len, 0); //r = r0+r1
  std::vector<uint64_t> eta_dp(len, 0), alpha(len, 0);
  int r0_seed = 1234;
  int r1_seed = 5678;
  int b_seed = 1;
  CommonRandVectorOverL(r0, len, r0_seed);
  CommonRandVectorOverL(r1, len, r1_seed);
  CommonRandBitVector(eta_dp, len, b_seed);
  AddVectorsOverL(r0, r1, r, len);
  WrapAround(r0, r1, alpha, len);
  
  //Common to both parties ends

  //@ Party 0
  std::vector<uint64_t> a_tilde0(len, 0), beta_0(len, 0);
  AddVectorsOverL(a_L_shares0, r0, a_tilde0, len);
  WrapAround(a_L_shares0, r0, beta_0, len);
  //@ Party 1 
  std::vector<uint64_t> a_tilde1(len, 0), beta_1(len, 0);
  AddVectorsOverL(a_L_shares1, r1, a_tilde1, len);
  WrapAround(a_L_shares1, r1, beta_1, len);

  //Since its is a stand alone file we doing the steps for both P0 and P1 and verifying the results
  //In actual implementation each party (P0 an d P1) sends only its own parameters to P2 
  std::vector<uint64_t> x_bit_prime0(len*BIT_SIZE, 0), x_bit_prime1(len*BIT_SIZE, 0);
  std::vector<uint64_t> delta_LminusOne0(len, 0), delta_LminusOne1(len, 0);
  
  ShareConvert_P2(a_tilde0, a_tilde1, x_bit_prime0, x_bit_prime1, delta_LminusOne0, delta_LminusOne1, len);
  std::vector<uint64_t> c0(len*BIT_SIZE), c1(len*BIT_SIZE);
  for(int i=0; i < len; i++) r[i] = r[i]-1;

  c0 = PrivateCompare(x_bit_prime0, r, eta_dp, len, 0);
  c1 = PrivateCompare(x_bit_prime1, r, eta_dp, len, 1);
  std::vector<uint64_t> eta_p(len, 0);
  PrivateCompare_P2(c0, c1, eta_p, len);
  //L-1 shares haveto be generated for eta_p and shared with P0 and P1 respectively
  //This has to be executed at P2 in ShareConvert_P2
  std::vector<uint64_t> eta_p_odd0(len, 0), eta_p_odd1(len, 0);
  GenerateModuloOddShares(eta_p_odd0, eta_p_odd1, eta_p, len);
  std::vector<uint64_t> theta_0(len, 0), theta_1(len, 0);
  //Once shares of eta_p are received from P2, P0 and P1 have to execute the following
  //theta =  eta_p xor eta_dp
  //@P0 j =0
  for(int i = 0; i< len; i++)
  {
    if (eta_dp[i] == 1) 
        theta_0[i] = SubtractModuloOdd(1, eta_p_odd0[i]);
    else theta_0[i] = eta_p_odd0[i];
  }
  AddModuloOdd(theta_0, beta_0, theta_0, len);
  AddModuloOdd(theta_0, delta_LminusOne0, theta_0, len);
  for(int i = 0; i< len; i++) alpha[i] = alpha[i] +1;
  SubtractModuloOdd(theta_0, alpha, theta_0, len);
  SubtractModuloOdd(a_L_shares0, theta_0, a_Lmiusone_shares0, len);


  //@ P1, j=1
  for(int i = 0; i<len; i++)
  {
    if (eta_dp[i] == 1)
        theta_1[i] = SubtractModuloOdd(0, eta_p_odd1[i]);
    else 
       theta_1[i] = eta_p_odd1[i];
  }
  AddModuloOdd(theta_1, beta_1, theta_1, len);
  AddModuloOdd(theta_1, delta_LminusOne1, theta_1, len);
  SubtractModuloOdd(a_L_shares1, theta_1, a_Lmiusone_shares1, len);
}

void WriteSharesToFile(std::string& filePath, std::vector<std::uint64_t>& shares) {
  std::ofstream ComputeMSBOutputFile;
  try {
    ComputeMSBOutputFile.open(filePath);
    if (!ComputeMSBOutputFile) {
      std::cerr << "Error: Error opening ComputeMSB output file." << std::endl;
    }
  } catch (std::exception& error) {
    std::cerr << "Error: Error opening ComputeMSB output share file: " << error.what() << std::endl;
  }

  for (int index = 0; index < shares.size(); index++) {
    ComputeMSBOutputFile << shares[index] << std::endl;
  }

  if (ComputeMSBOutputFile.eof()) {
    ComputeMSBOutputFile.close();
  }
}




/************** ShareConvert End***************************/
/**************************ComputeMSB****************/
//Helper functions 
void GenerateRandVecModOdd(std::vector<uint64_t>& q, std::size_t len)
{
  std::cout << "GenerateRandVecmodOdd \n";
  std::random_device rd;
  std::mt19937 gen(rd());
  for(int i = 0; i < len; i++ )
    {
     q[i] = RandomNumOverOddRing(gen); // zeroed randmoness
    }
}
void ComputeMSB_P2(std::vector<uint64_t>& x0_odd, std::vector<uint64_t>& x1_odd, std::vector<uint64_t>& x0_bit_pr, std::vector<uint64_t>& x1_bit_pr, std::vector<uint64_t>& x0_lsb, std::vector<uint64_t>& x1_lsb,std::size_t len)
{
std::cout << "ComputeMSB_P2 \n";
std::vector<uint64_t> x(len, 0);

GenerateRandVecModOdd(x0_odd, len);
GenerateRandVecModOdd(x1_odd, len);
AddModuloOdd(x0_odd, x1_odd, x, len);
GenerateBitSharesOverPrime(x0_bit_pr, x1_bit_pr, x, len);
GenerateLSBSharesOverL(x0_lsb, x1_lsb, x, len);
}

// ComputeMSB: Takes shares of a in L world and returns shares of most significant bit MSB(a) in L world.
// Input  : Vectors a_L_0 and a_L_1 of type std::uint64_t storing shares of a at P0 and P1 respectively in L world, empty vectors a0_MSB_L and a1_MSB_L of type std::uint64_t, dimensions of matrix (rows, cols)
// Output : Updates a0_MSB_L and a1_MSB_L to store MSB shares at P0 and P1 respectively.
void ComputeMSB(std::vector<uint64_t>& a0_odd, std::vector<uint64_t>& a1_odd, std::vector<uint64_t>& a0_MSB_L, std::vector<uint64_t>& a1_MSB_L, std::size_t len, std::size_t rows, std::size_t cols)
{
std::cout << "ComputeMSB \n";
std::vector<uint64_t> x0_odd(len, 0), x1_odd(len, 0);
std::vector<uint64_t> x0_lsb(len, 0), x1_lsb(len, 0);
std::vector<uint64_t> x0_bit_pr(len*BIT_SIZE, 0), x1_bit_pr(len*BIT_SIZE, 0);
std::vector<uint64_t> y0_odd(len, 0), y1_odd(len, 0);
std::vector<uint64_t> r0_odd(len, 0), r1_odd(len, 0), r(len, 0), encodedRBit(len, 0);
std::vector<uint64_t> c0(len, 0), c1(len, 0);
ComputeMSB_P2(x0_odd,  x1_odd, x0_bit_pr,  x1_bit_pr, x0_lsb,  x1_lsb, len);

AddModuloOdd(a0_odd, a0_odd, y0_odd, len);    // At P0
AddModuloOdd(a1_odd, a1_odd, y1_odd, len);    // At P1

AddModuloOdd(y0_odd, x0_odd, r0_odd, len);    // At P0
AddModuloOdd(y1_odd, x1_odd, r1_odd, len);    // At P1
AddModuloOdd(r0_odd, r1_odd, r, len);         // At P0 and P1

for(int i = 0; i < len; i++)
   {
    encodedRBit[i] = MOTION::new_fixed_point::encode<uint64_t, long double>(r[i] & 1, FIXED_POINT);
   }

std::vector<uint64_t> beta(len, 0), encodedBeta(len, 0);//At p0 and P1
CommonRandBitVector(beta, len, 1234); //beta common random bit vetcor
for(int i = 0; i < len; i++)
   {
    encodedBeta[i] = MOTION::new_fixed_point::encode<uint64_t, long double>(beta[i], FIXED_POINT);
   }

const std::string baseDirectory = (std::string)std::getenv("BASE_DIR");

std::string ShareFile0 = baseDirectory + "/build_debwithrelinfo_gcc/3PC_Relu/MSB_PCTestFileShares_0";
std::string ShareFile1 = baseDirectory + "/build_debwithrelinfo_gcc/3PC_Relu/MSB_PCTestFileShares_1";
std::string BetaFile   = baseDirectory + "/build_debwithrelinfo_gcc/3PC_Relu/MSB_PCTestBeta";
std::string RFile      = baseDirectory + "/build_debwithrelinfo_gcc/3PC_Relu/MSB_PCTestRFile";

WriteSharesToFile(ShareFile0, x0_bit_pr);
WriteSharesToFile(ShareFile1, x1_bit_pr);
WriteSharesToFile(BetaFile, beta);
WriteSharesToFile(RFile, r);

c0 = PrivateCompare(x0_bit_pr, r, beta, len, 0);//At p0 
c1 = PrivateCompare(x1_bit_pr, r, beta, len, 1);//At P1
std::vector<uint64_t> beta_p(len, 0);//At p0 and P1
PrivateCompare_P2(c0, c1, beta_p, len);//At p0 and P1

std::cout << "Values of beta_prime:" << std::endl;
for (int i = 0; i < len; i++) {
  std::cout << beta_p[i] << std::endl;
}

for(int i = 0; i < len; i++)
   {
    beta_p[i] = MOTION::new_fixed_point::encode<uint64_t, long double>(beta_p[i], FIXED_POINT);
   }
//L-1 shares for beta_p should be generated and shared with P0 and P1 respectively
//This has to be executed at P2 in ShareConvert_P2
std::vector<uint64_t> beta0_p_L(len, 0), beta1_p_L(len, 0);

GenerateSharesOverL(beta0_p_L, beta1_p_L, beta_p, len);
std::vector<std::uint64_t> gamma0_L(len, 0), gamma1_L(len, 0);
std::vector<std::uint64_t> delta0_L(len, 0), delta1_L(len, 0);
std::vector<std::uint64_t> theta0_L(len, 0), theta1_L(len, 0);

//@ Party 0 
//gamma = beta xor beta_p; delta = x[0] xor r[0]
// theta =  gamma . delta
// lsb =  gamma xor delta  
for(int i = 0; i < len; i++)
 {
  gamma0_L[i] = beta0_p_L[i] - 2 * MOTION::new_fixed_point::truncate(beta0_p_L[i] * encodedBeta[i], FIXED_POINT);
  delta0_L[i] = x0_lsb[i] - 2 * MOTION::new_fixed_point::truncate(x0_lsb[i] * encodedRBit[i], FIXED_POINT);
 }

 //@Party 1
for(int i = 0; i < len; i++)
 {
 // gamma1_L[i] = beta1_p_L[i] * (1 - 2 * beta[i]) + beta[i];

 gamma1_L[i] = beta1_p_L[i] + encodedBeta[i] - 2 * MOTION::new_fixed_point::truncate(beta1_p_L[i] * encodedBeta[i], FIXED_POINT);
 delta1_L[i] = x1_lsb[i] + encodedRBit[i] - 2 * MOTION::new_fixed_point::truncate(x1_lsb[i] * encodedRBit[i], FIXED_POINT); 
 }
//we have to compute theta = gamma.delta (element by element multiplication hadamard product)
// we have to 1. convert arith shares into ABY2.0 shares 2. Perform hadamard product 3. convert to arith sahres
//For simplicity i am doing the following 
for (int i = 0; i < len; i++)
{
  theta0_L[i] = MOTION::new_fixed_point::truncate(gamma0_L[i] * delta0_L[i] + gamma0_L[i] * delta1_L[i], FIXED_POINT);
  theta1_L[i] = MOTION::new_fixed_point::truncate(gamma1_L[i] * delta1_L[i] + gamma1_L[i] * delta0_L[i], FIXED_POINT);
}


std::vector<std::uint64_t> gamma_ABY_public_0, gamma_ABY_private_0;
std::vector<std::uint64_t> gamma_ABY_public_1, gamma_ABY_private_1;
std::vector<std::uint64_t> delta_ABY_public_0, delta_ABY_private_0;
std::vector<std::uint64_t> delta_ABY_public_1, delta_ABY_private_1;
std::vector<std::uint64_t> gamma_ABY_public, delta_ABY_public;

gamma0_L.insert(gamma0_L.begin(), cols);
gamma0_L.insert(gamma0_L.begin(), rows);

gamma1_L.insert(gamma1_L.begin(), cols);
gamma1_L.insert(gamma1_L.begin(), rows);

delta0_L.insert(delta0_L.begin(), cols);
delta0_L.insert(delta0_L.begin(), rows);

delta1_L.insert(delta1_L.begin(), cols);
delta1_L.insert(delta1_L.begin(), rows);

std::size_t length = gamma0_L.size();

// Convert the gamma and delta shares from arithmetic shares to ABY2.0 shares
// @ Party 0
GeneratePrivateShares_ABY(gamma0_L, gamma_ABY_private_0);
gamma_ABY_public_0.resize(gamma0_L.size());
ParallelAddition(gamma0_L, gamma_ABY_private_0, gamma_ABY_public_0, 2);

GeneratePrivateShares_ABY(delta0_L, delta_ABY_private_0);
delta_ABY_public_0.resize(delta0_L.size());
ParallelAddition(delta0_L, delta_ABY_private_0, delta_ABY_public_0, 2);

// @ Party 1
GeneratePrivateShares_ABY(gamma1_L, gamma_ABY_private_1);
gamma_ABY_public_1.resize(gamma1_L.size());
ParallelAddition(gamma1_L, gamma_ABY_private_1, gamma_ABY_public_1, 2);

GeneratePrivateShares_ABY(delta1_L, delta_ABY_private_1);
delta_ABY_public_1.resize(delta1_L.size());
ParallelAddition(delta1_L, delta_ABY_private_1, delta_ABY_public_1, 2);

// Reconstruct public shares
gamma_ABY_public.resize(len);
ParallelAddition(gamma_ABY_public_0, gamma_ABY_public_1, gamma_ABY_public, 2);

delta_ABY_public.resize(len);
ParallelAddition(delta_ABY_public_0, delta_ABY_public_1, delta_ABY_public, 2);

// OT @ Party 2
std::vector<std::uint64_t> X0Y1, X1Y0;
std::vector<std::uint64_t> OT_Shares_0, OT_Shares_1;

HadamardMatrixMultiplication(gamma_ABY_private_0, delta_ABY_private_1, X0Y1);
HadamardMatrixMultiplication(gamma_ABY_private_1, delta_ABY_private_0, X1Y0);

GeneratePrivateShares_ABY(X0Y1, OT_Shares_0);
OT_Shares_1.resize(OT_Shares_0.size());
OT_Shares_1[0] = OT_Shares_0[0];
OT_Shares_1[1] = OT_Shares_1[1];
for (int i = 2; i < OT_Shares_0.size(); i++) {
  OT_Shares_1[i] = X0Y1[i] + X1Y0[i] - OT_Shares_0[i];
}

// @ Party 0
std::vector<std::uint64_t> dxdy_0, DxDy_0, Dxdy_0, dxDy_0;
HadamardMatrixMultiplication(gamma_ABY_private_0, delta_ABY_private_0, dxdy_0);
HadamardMatrixMultiplication(gamma_ABY_public, delta_ABY_public, DxDy_0);
HadamardMatrixMultiplication(gamma_ABY_public, delta_ABY_private_0, Dxdy_0);
HadamardMatrixMultiplication(gamma_ABY_private_0, delta_ABY_public, dxDy_0);

// @ Party 1
std::vector<std::uint64_t> dxdy_1, DxDy_1, Dxdy_1, dxDy_1;
HadamardMatrixMultiplication(gamma_ABY_private_1, delta_ABY_private_1, dxdy_1);
HadamardMatrixMultiplication(gamma_ABY_public, delta_ABY_public, DxDy_1);
HadamardMatrixMultiplication(gamma_ABY_public, delta_ABY_private_1, Dxdy_1);
HadamardMatrixMultiplication(gamma_ABY_private_1, delta_ABY_public, dxDy_1);

std::vector<std::uint64_t> theta0_L_hadamard(length, 0), theta1_L_hadamard(length, 0);
for (int i = 2; i < length; i++) {
  theta0_L_hadamard[i] = MOTION::new_fixed_point::truncate(dxdy_0[i] + OT_Shares_0[i] - Dxdy_0[i] - dxDy_0[i], FIXED_POINT);
  theta1_L_hadamard[i] = MOTION::new_fixed_point::truncate(dxdy_1[i] + OT_Shares_1[i] + DxDy_1[i] - Dxdy_1[i] - dxDy_1[i], FIXED_POINT);
}

gamma0_L.erase(gamma0_L.begin(), gamma0_L.begin() + 2);
gamma1_L.erase(gamma1_L.begin(), gamma1_L.begin() + 2);

delta0_L.erase(delta0_L.begin(), delta0_L.begin() + 2);
delta1_L.erase(delta1_L.begin(), delta1_L.begin() + 2);

theta0_L_hadamard.erase(theta0_L_hadamard.begin(), theta0_L_hadamard.begin() + 2);
theta1_L_hadamard.erase(theta1_L_hadamard.begin(), theta1_L_hadamard.begin() + 2);

std::cout << "Computation of MSB using theta shares computed locally without Hadamard: " << std::endl;
std::vector<std::uint64_t> a0_MSB_L_local(len, 0), a1_MSB_L_local(len, 0);
for (int i = 0; i < len; i++) {
  a0_MSB_L_local[i] = gamma0_L[i] + delta0_L[i] - 2 * theta0_L[i];
  a1_MSB_L_local[i] = gamma1_L[i] + delta1_L[i] - 2 * theta1_L[i];

  std::cout << "theta0_L_local[i]: " << theta0_L[i] << ", theta1_L_local[i]: " << theta1_L[i] << ", MSB: " << (a0_MSB_L_local[i] + a1_MSB_L_local[i]) << std::endl;
}

std::cout << "\n";

std::cout << "Computation of MSB using theta shares computed using Hadamard by converting arithmetic shares to ABY2.0 shares: " << std::endl;
for (int i = 0; i < len; i++) {
  a0_MSB_L[i] = gamma0_L[i] + delta0_L[i] - 2 * theta0_L_hadamard[i];
  a1_MSB_L[i] = gamma1_L[i] + delta1_L[i] - 2 * theta1_L_hadamard[i];

  std::cout << "theta0_L_hadamard[i]: " << theta0_L_hadamard[i] << ", theta1_L_hadamard[i]: " << theta1_L_hadamard[i] << ", MSB: " << (a0_MSB_L[i] + a1_MSB_L[i]) << std::endl;
}


std::cout << "Compute MSB completed. \n" << std::endl;
}

struct Options {
  std::string permutefile;
  std::size_t fractional_bits;
};

std::optional<Options> parse_program_options(int argc, char* argv[]) {
  Options options;
  po::options_description desc("Allowed options");
  // clang-format off
  desc.add_options()
    ("help,h", po::bool_switch()->default_value(false),"produce help message")
    ("fractional-bits", po::value<size_t>()->default_value(13), "Number of fractional bits")
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
  } catch (std::exception& e) {
    std::cerr << "Input parse error:" << e.what() << std::endl;
    return std::nullopt;
  }
  
  const std::string baseDirectory = (std::string)std::getenv("BASE_DIR");
  options.permutefile =  baseDirectory + "/build_debwithrelinfo_gcc/3PC_Relu/permute";
  options.fractional_bits = vm["fractional-bits"].as<size_t>();
  return options;
}
int main(int argc, char* argv[]) 
{
  std::default_random_engine engine;
  std::random_device rd;
  std::mt19937 gen(rd()); 
  InitializeModuloPrimeOps();

  int setLen = 10;
  // Distribution of small random values ranging from -100 to 100
  std::uniform_real_distribution<float> distribution((float)(-100.0), (float)(100.0));
  // Distribution of large integer positive numbers (0 to pow(2, 32) - 1)
  std::uniform_real_distribution<float> distributionRealRositive((float)(gen.min()), (float)(gen.max()));
  // Distribution of integer negative numbers 
  std::uniform_real_distribution<float> distributionRealNegative((float)(-std::numeric_limits<float>::max()), (float)(0.0));
  // Distribution of small random values ranging from -2.0 to 2.0
  std::uniform_real_distribution<float> distributionRealSmallValues((float)(-2.0), (float)(2.0));

  int rows = 2;
  int cols = 5;

  std::size_t len = 10;
  std::vector<float> data(len, 0.0);
  std::vector<std::uint64_t> a(len, 0),  a_L_0(len, 0), a_L_1(len, 0);
  std::vector<std::uint64_t> a_Lminusone_0(len, 0), a_Lminusone_1(len, 0);
  std::vector<std::uint64_t> MSB_0(len, 0), MSB_1(len, 0);
  data[0] = 10;
  data[1] = -1;
  data[2] = 0.5;
  data[3] = 0.1;
  data[4] = -25.7;
  data[5] = 0;
  data[6] = 12.23331;
  data[7] = -938920.56;
  data[8] = 0.00001;
  data[9] = 1000;

  for(int i = 0; i < len; i++)
  {
   a[i] = MOTION::new_fixed_point::encode<uint64_t, long double>(data[i], 13);
  }

  GenerateSharesOverL(a_L_0, a_L_1, a, len); 
  ShareConvert(a_L_0, a_L_1, a_Lminusone_0, a_Lminusone_1, len, 0);
  

  // for(int i = 0; i<len; i++)
  // {
  //   std::cout << a_L_0[i] << " , " << a_L_1[i] << " , " << a_L_0[i] + a_L_1[i] << "\n";
  //   //std::cout << a_Lmiusone_0[i] << " , " << a_Lmiusone_1[i] << " , " << a_Lmiusone_0[i] + a_Lmiusone_1[i] << " \n";
  //   std::cout << a_Lmiusone_0[i] << " , " << a_Lmiusone_1[i] << " , " << AddModuloOdd(a_Lmiusone_0[i], a_Lmiusone_1[i]) << " \n";
  //   if (AddModuloOdd(a_Lmiusone_0[i], a_Lmiusone_1[i]) != a[i])
  //      std::cout << "L-1 shares are not created correct at i : " << i << "\n";
  // }
// for (int i = 0; i < len; i++) {
//   a_Lmiusone_0[i] = a_L_0[i];
//   a_Lmiusone_1[i] = a_L_1[i];
// }

ComputeMSB(a_Lminusone_0, a_Lminusone_1, MSB_0, MSB_1, len, rows, cols);

for(int i = 0; i<len; i++)
  {
    // std::cout << a_L_0[i] << " , " << a_L_1[i] << " , " << a_L_0[i] + a_L_1[i] << "\n";
    // //std::cout << a_Lmiusone_0[i] << " , " << a_Lmiusone_1[i] << " , " << a_Lmiusone_0[i] + a_Lmiusone_1[i] << " \n";
    // std::cout << a_Lmiusone_0[i] << " , " << a_Lmiusone_1[i] << " , " << AddModuloOdd(a_Lmiusone_0[i], a_Lmiusone_1[i]) << " \n";
    // if (AddModuloOdd(a_Lmiusone_0[i], a_Lmiusone_1[i]) != a[i])
    //    std::cout << "L-1 shares are not created correct at i : " << i << "\n";

    auto MSB = MOTION::new_fixed_point::decode<std::uint64_t, long double>(MSB_0[i] + MSB_1[i], FIXED_POINT);
    std::cout << "data: " << data[i] << ", " << MSB_0[i] << ", " << MSB_1[i]  << " , " << "MSB: " << MSB << "\n";
    
    if (((data[i] >= 0) && (MSB != 0)) || ((data[i] < 0) && (MSB != 1))) {
      std::cout << "MSB is not computed correctly for data[i]: " << data[i] << " at index: " << i << std::endl;
    }
  }
return EXIT_SUCCESS;

}