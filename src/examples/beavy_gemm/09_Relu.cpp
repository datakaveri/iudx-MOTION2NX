#include <boost/archive/text_oarchive.hpp>
#include <boost/asio.hpp>
#include <boost/chrono.hpp>
#include <boost/program_options.hpp>
#include <boost/serialization/string.hpp>
#include <boost/thread.hpp>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <parallel/algorithm>
#include <random>
#include <stdexcept>
#include <thread>
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


/*******************************************************************************/
// This file implements standalone code for ReLU.
// x - owner is Party2, creates L-1 shares and prime shares for each bit in x
// L shares for x[0] and communicates Party 0 and Party 1
// Assumes at Party 0 and Party1  have following data
//  L-1 shares of x, prime shares of x and L shares of x[0]
//@ Party 0 and Party 1 compute have r_i =  (x_i(L-1) + y_i(L-1))
// x_i(L-1) received from Party 2, y_i(L-1) each paty has it
// r in clear (after communication between Party 0 and party 1)
// where r = (x_0(L-1) + y_0(L-1)) + (x_1(L-1) + y_1(L-1))
//
// Standalone implementation: we create x_i(L-1),x_(prime),x[0]_i(L)
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
    // std::cout << " Entered GenerateRandomPermutation \n ";
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

void PrivateCompare(std::vector<std::uint64_t>& c, std::vector<std::uint64_t> &bit_share_x0, std::vector<std::uint64_t> &r, std::vector<std::uint64_t> &beta, std::size_t len, int party_id, int startRange, int endRange) 
{
std::cout << "Eneterd PrivateCompare : " << party_id << "  \n";  
std::uint64_t value_r, value_beta;
std::uint64_t t, j, ind, w, temp_prime_sh;
// std::vector<std::uint64_t> c(len*BIT_SIZE);
c.resize(len * BIT_SIZE);

std::mt19937 gen_u(1234); 
std::mt19937 gen_s(5678); 
for(int i = startRange; i < endRange; i++)
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
// return c;
}


void PrivateCompare_P2(std::vector<std::uint64_t>& c0, std::vector<std::uint64_t>& c1, std::vector<std::uint64_t>& beta_prime, std::size_t len, int startRange, int endRange)
{
// It has to wait for the vector d0 and d1 then compute beta_prime
std::cout << "Eneterd PrivateCompare_P2 : " << "  \n";
std::uint64_t ind, k;
for(int i = startRange; i < endRange;  i++)
  {
    for( k = 0; k<BIT_SIZE;  k++)
    {
      ind = i*BIT_SIZE +k;
      //std::cout << c0[ind] << " , " << c1[ind] << "  , " << addModPrime[c0[ind]][c1[ind]] << "\n";
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
void ShareConvert(std::vector<uint64_t>& a_L_shares0, std::vector<uint64_t>& a_L_shares1,std::vector<uint64_t>& a_Lmiusone_shares0, std::vector<uint64_t>& a_Lmiusone_shares1, std::size_t len)
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

  // PrivateCompare(c0, x_bit_prime0, r, eta_dp, len, 0, 0, len);
  // PrivateCompare(c1, x_bit_prime1, r, eta_dp, len, 1, 0, len);

  int partition = (int)(len / 8);
  std::thread threadPC_0_1(PrivateCompare, std::ref(c0), std::ref(x_bit_prime0), std::ref(r), std::ref(eta_dp), len, 0, 0, partition);
  std::thread threadPC_0_2(PrivateCompare, std::ref(c0), std::ref(x_bit_prime0), std::ref(r), std::ref(eta_dp), len, 0, partition, 2 * partition);
  std::thread threadPC_0_3(PrivateCompare, std::ref(c0), std::ref(x_bit_prime0), std::ref(r), std::ref(eta_dp), len, 0, 2 * partition, 3 * partition);
  std::thread threadPC_0_4(PrivateCompare, std::ref(c0), std::ref(x_bit_prime0), std::ref(r), std::ref(eta_dp), len, 0, 3 * partition, 4 * partition);
  std::thread threadPC_0_5(PrivateCompare, std::ref(c0), std::ref(x_bit_prime0), std::ref(r), std::ref(eta_dp), len, 0, 4 * partition, 5 * partition);
  std::thread threadPC_0_6(PrivateCompare, std::ref(c0), std::ref(x_bit_prime0), std::ref(r), std::ref(eta_dp), len, 0, 5 * partition, 6 * partition);
  std::thread threadPC_0_7(PrivateCompare, std::ref(c0), std::ref(x_bit_prime0), std::ref(r), std::ref(eta_dp), len, 0, 6 * partition, 7 * partition);
  std::thread threadPC_0_8(PrivateCompare, std::ref(c0), std::ref(x_bit_prime0), std::ref(r), std::ref(eta_dp), len, 0, 7 * partition, len);
  


  std::thread threadPC_1_1(PrivateCompare, std::ref(c1), std::ref(x_bit_prime1), std::ref(r), std::ref(eta_dp), len, 1, 0, partition);
  std::thread threadPC_1_2(PrivateCompare, std::ref(c1), std::ref(x_bit_prime1), std::ref(r), std::ref(eta_dp), len, 1, partition, 2 * partition);
  std::thread threadPC_1_3(PrivateCompare, std::ref(c1), std::ref(x_bit_prime1), std::ref(r), std::ref(eta_dp), len, 1, 2 * partition, 3 * partition);
  std::thread threadPC_1_4(PrivateCompare, std::ref(c1), std::ref(x_bit_prime1), std::ref(r), std::ref(eta_dp), len, 1, 3 * partition, 4 * partition);
  std::thread threadPC_1_5(PrivateCompare, std::ref(c1), std::ref(x_bit_prime1), std::ref(r), std::ref(eta_dp), len, 1, 4 * partition, 5 * partition);
  std::thread threadPC_1_6(PrivateCompare, std::ref(c1), std::ref(x_bit_prime1), std::ref(r), std::ref(eta_dp), len, 1, 5 * partition, 6 * partition);
  std::thread threadPC_1_7(PrivateCompare, std::ref(c1), std::ref(x_bit_prime1), std::ref(r), std::ref(eta_dp), len, 1, 6 * partition, 7 * partition);
  std::thread threadPC_1_8(PrivateCompare, std::ref(c1), std::ref(x_bit_prime1), std::ref(r), std::ref(eta_dp), len, 1, 7 * partition, len);

  threadPC_0_1.join();
  threadPC_0_2.join();
  threadPC_0_3.join();
  threadPC_0_4.join();
  threadPC_0_5.join();
  threadPC_0_6.join();
  threadPC_0_7.join();
  threadPC_0_8.join();

  threadPC_1_1.join();
  threadPC_1_2.join();
  threadPC_1_3.join();
  threadPC_1_4.join();
  threadPC_1_5.join();
  threadPC_1_6.join();
  threadPC_1_7.join();
  threadPC_1_8.join();

  std::vector<uint64_t> eta_p(len, 0);
  PrivateCompare_P2(c0, c1, eta_p, len, 0, len);

  std::thread thread1(PrivateCompare_P2, std::ref(c0), std::ref(c1), std::ref(eta_p), len, 0, partition);
  std::thread thread2(PrivateCompare_P2, std::ref(c0), std::ref(c1), std::ref(eta_p), len, partition, 2 * partition);
  std::thread thread3(PrivateCompare_P2, std::ref(c0), std::ref(c1), std::ref(eta_p), len, 2 * partition, 3 * partition);
  std::thread thread4(PrivateCompare_P2, std::ref(c0), std::ref(c1), std::ref(eta_p), len, 3 * partition, 4 * partition);
  std::thread thread5(PrivateCompare_P2, std::ref(c0), std::ref(c1), std::ref(eta_p), len, 4 * partition, 5 * partition);
  std::thread thread6(PrivateCompare_P2, std::ref(c0), std::ref(c1), std::ref(eta_p), len, 5 * partition, 6 * partition);
  std::thread thread7(PrivateCompare_P2, std::ref(c0), std::ref(c1), std::ref(eta_p), len, 6 * partition, 7 * partition);
  std::thread thread8(PrivateCompare_P2, std::ref(c0), std::ref(c1), std::ref(eta_p), len, 7 * partition, len);

  thread1.join();
  thread2.join();
  thread3.join();
  thread4.join();
  thread5.join();
  thread6.join();
  thread7.join();
  thread8.join();

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

// ArithHadamardMatrixMultiplication: Takes arithmetic shares of input: X received from Party 0 and Y from Party 1 respectively, converts them into ABY2.0 shares and performs hadamard matrix multiplication of Z = X . Y
// First two elements of all shares are the dimensions of the matrix: rows and cols
// Input  : Vectors X_shares_0 and X_shares_1 storing shares of matrix X at P0 and P1 respectively, vectors Y_shares_0 and Y_shares_1 storing shares of matrix Y at P0 and P1 respectively and empty vectors Z_shares_0 and Z_shares_1 of type std::uint64_t
// Output : Updates Z_shares_0 and Z_shares_1 to store hadamard product shares of X.Y, first two elements are the dimensions of the matrix.
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
    Z_shares_0[index] = dxdy_0[index] + OT_0[index] - Dxdy_0[index] - dxDy_0[index];
    Z_shares_0[index] = MOTION::new_fixed_point::truncate(Z_shares_0[index], FIXED_POINT);
  }

  // @ Party 1
  Z_shares_1.resize(lengthWithDim);
  Z_shares_1[0] = rows;
  Z_shares_1[1] = cols;
  for (int index = 2; index < Z_shares_1.size(); index++) {
    Z_shares_1[index] = dxdy_1[index] + OT_1[index] + (DxDy_1[index]) - Dxdy_1[index] - dxDy_1[index];
    Z_shares_1[index] = MOTION::new_fixed_point::truncate(Z_shares_1[index], FIXED_POINT);
  }

  Z_shares_0.erase(Z_shares_0.begin(), Z_shares_0.begin() + 2);
  Z_shares_1.erase(Z_shares_1.begin(), Z_shares_1.begin() + 2);

  std::cout << "Exiting ArithHadamardMatrixMultiplication" << std::endl;
}

// ComputeMSB: Takes shares of a in L world and returns shares of most significant bit MSB(a) in L world.
// Input  : Vectors a_L_0 and a_L_1 of type std::uint64_t storing shares of a at P0 and P1 respectively in L world, empty vectors a0_MSB_L and a1_MSB_L of type std::uint64_t, dimensions of matrix (rows, cols)
// Output : Updates a0_MSB_L and a1_MSB_L to store MSB shares at P0 and P1 respectively.
void ComputeMSB(std::vector<uint64_t>& a0_odd, std::vector<uint64_t>& a1_odd, std::vector<uint64_t>& a0_MSB_L, std::vector<uint64_t>& a1_MSB_L, std::size_t len, std::size_t rows, std::size_t cols)
{
std::cout << "Entered ComputeMSB \n";
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

int partition = (int)(len / 8);
// PrivateCompare(c0, x0_bit_pr, r, beta, len, 0, 0, len);//At p0 
// PrivateCompare(c1, x1_bit_pr, r, beta, len, 1, 0, len);//At P1

std::thread threadPC_0_1(PrivateCompare, std::ref(c0), std::ref(x0_bit_pr), std::ref(r), std::ref(beta), len, 0, 0, partition);
std::thread threadPC_0_2(PrivateCompare, std::ref(c0), std::ref(x0_bit_pr), std::ref(r), std::ref(beta), len, 0, partition, 2 * partition);
std::thread threadPC_0_3(PrivateCompare, std::ref(c0), std::ref(x0_bit_pr), std::ref(r), std::ref(beta), len, 0, 2 * partition, 3 * partition);
std::thread threadPC_0_4(PrivateCompare, std::ref(c0), std::ref(x0_bit_pr), std::ref(r), std::ref(beta), len, 0, 3 * partition, 4 * partition);
std::thread threadPC_0_5(PrivateCompare, std::ref(c0), std::ref(x0_bit_pr), std::ref(r), std::ref(beta), len, 0, 4 * partition, 5 * partition);
std::thread threadPC_0_6(PrivateCompare, std::ref(c0), std::ref(x0_bit_pr), std::ref(r), std::ref(beta), len, 0, 5 * partition, 6 * partition);
std::thread threadPC_0_7(PrivateCompare, std::ref(c0), std::ref(x0_bit_pr), std::ref(r), std::ref(beta), len, 0, 6 * partition, 7 * partition);
std::thread threadPC_0_8(PrivateCompare, std::ref(c0), std::ref(x0_bit_pr), std::ref(r), std::ref(beta), len, 0, 7 * partition, len);


std::thread threadPC_1_1(PrivateCompare, std::ref(c1), std::ref(x1_bit_pr), std::ref(r), std::ref(beta), len, 1, 0, partition);
std::thread threadPC_1_2(PrivateCompare, std::ref(c1), std::ref(x1_bit_pr), std::ref(r), std::ref(beta), len, 1, partition, 2 * partition);
std::thread threadPC_1_3(PrivateCompare, std::ref(c1), std::ref(x1_bit_pr), std::ref(r), std::ref(beta), len, 1, 2 * partition, 3 * partition);
std::thread threadPC_1_4(PrivateCompare, std::ref(c1), std::ref(x1_bit_pr), std::ref(r), std::ref(beta), len, 1, 3 * partition, 4 * partition);
std::thread threadPC_1_5(PrivateCompare, std::ref(c1), std::ref(x1_bit_pr), std::ref(r), std::ref(beta), len, 1, 4 * partition, 5 * partition);
std::thread threadPC_1_6(PrivateCompare, std::ref(c1), std::ref(x1_bit_pr), std::ref(r), std::ref(beta), len, 1, 5 * partition, 2 * 6 * partition);
std::thread threadPC_1_7(PrivateCompare, std::ref(c1), std::ref(x1_bit_pr), std::ref(r), std::ref(beta), len, 1, 6 * partition, 7 * partition);
std::thread threadPC_1_8(PrivateCompare, std::ref(c1), std::ref(x1_bit_pr), std::ref(r), std::ref(beta), len, 1, 7 * partition, len);



threadPC_0_1.join();
threadPC_0_2.join();
threadPC_0_3.join();
threadPC_0_4.join();
threadPC_0_5.join();
threadPC_0_6.join();
threadPC_0_7.join();
threadPC_0_8.join();

threadPC_1_1.join();
threadPC_1_2.join();
threadPC_1_3.join();
threadPC_1_4.join();
threadPC_1_5.join();
threadPC_1_6.join();
threadPC_1_7.join();
threadPC_1_8.join();

std::vector<uint64_t> beta_p(len, 0);//At p0 and P1
// PrivateCompare_P2(c0, c1, beta_p, len, 0, len);//At p0 and P1


std::thread thread1(PrivateCompare_P2, std::ref(c0), std::ref(c1), std::ref(beta_p), len, 0, partition);
std::thread thread2(PrivateCompare_P2, std::ref(c0), std::ref(c1), std::ref(beta_p), len, partition, 2 * partition);
std::thread thread3(PrivateCompare_P2, std::ref(c0), std::ref(c1), std::ref(beta_p), len, 2 * partition, 3 * partition);
std::thread thread4(PrivateCompare_P2, std::ref(c0), std::ref(c1), std::ref(beta_p), len, 3 * partition, 4 * partition);
std::thread thread5(PrivateCompare_P2, std::ref(c0), std::ref(c1), std::ref(beta_p), len, 4 * partition, 5 * partition);
std::thread thread6(PrivateCompare_P2, std::ref(c0), std::ref(c1), std::ref(beta_p), len, 5 * partition, 6 * partition);
std::thread thread7(PrivateCompare_P2, std::ref(c0), std::ref(c1), std::ref(beta_p), len, 6 * partition, 7 * partition);
std::thread thread8(PrivateCompare_P2, std::ref(c0), std::ref(c1), std::ref(beta_p), len, 7 * partition, len);

// int lenThread = 4;
// std::vector<std::thread> threadsPC(lenThread);
// for (int index = 0; index < lenThread; index++) {
//   threadsPC[index].emplace_back(PrivateCompare_P2, c0, c1, beta_p, len, partition * index, partition * (index + 1));
// }


// for (auto& thread : threadsPC) {
//   thread.join();
// }

thread1.join();
thread2.join();
thread3.join();
thread4.join();
thread5.join();
thread6.join();
thread7.join();
thread8.join();

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
 gamma1_L[i] = beta1_p_L[i] + encodedBeta[i] - 2 * MOTION::new_fixed_point::truncate(beta1_p_L[i] * encodedBeta[i], FIXED_POINT);
 delta1_L[i] = x1_lsb[i] + encodedRBit[i] - 2 * MOTION::new_fixed_point::truncate(x1_lsb[i] * encodedRBit[i], FIXED_POINT); 
 }

// **********************
// Theta calculated without three-party Hadamard matrix multiplication stored at theta0_L and theta1_L

//we have to compute theta = gamma.delta (element by element multiplication hadamard product)
// we have to 1. convert arith shares into ABY2.0 shares 2. Perform hadamard product 3. convert to arith sahres
//For simplicity i am doing the following 
for (int i = 0; i < len; i++)
{
  theta0_L[i] = MOTION::new_fixed_point::truncate(gamma0_L[i] * delta0_L[i] + gamma0_L[i] * delta1_L[i], FIXED_POINT);
  theta1_L[i] = MOTION::new_fixed_point::truncate(gamma1_L[i] * delta1_L[i] + gamma1_L[i] * delta0_L[i], FIXED_POINT);
}
// **********************

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

std::size_t lengthWithDim = gamma0_L.size();

// Convert the gamma and delta shares from arithmetic shares to ABY2.0 shares
// @ Party 0
GeneratePrivateShares_ABY(gamma0_L, gamma_ABY_private_0);
gamma_ABY_public_0.resize(lengthWithDim);
ParallelAddition(gamma0_L, gamma_ABY_private_0, gamma_ABY_public_0, 2);

GeneratePrivateShares_ABY(delta0_L, delta_ABY_private_0);
delta_ABY_public_0.resize(lengthWithDim);
ParallelAddition(delta0_L, delta_ABY_private_0, delta_ABY_public_0, 2);

// @ Party 1
GeneratePrivateShares_ABY(gamma1_L, gamma_ABY_private_1);
gamma_ABY_public_1.resize(lengthWithDim);
ParallelAddition(gamma1_L, gamma_ABY_private_1, gamma_ABY_public_1, 2);

GeneratePrivateShares_ABY(delta1_L, delta_ABY_private_1);
delta_ABY_public_1.resize(lengthWithDim);
ParallelAddition(delta1_L, delta_ABY_private_1, delta_ABY_public_1, 2);

// Reconstruct public shares
gamma_ABY_public.resize(lengthWithDim);
ParallelAddition(gamma_ABY_public_0, gamma_ABY_public_1, gamma_ABY_public, 2);

delta_ABY_public.resize(lengthWithDim);
ParallelAddition(delta_ABY_public_0, delta_ABY_public_1, delta_ABY_public, 2);

// OT @ Party 2
std::vector<std::uint64_t> X0Y1, X1Y0;
std::vector<std::uint64_t> OT_Shares_0, OT_Shares_1;

HadamardMatrixMultiplication(gamma_ABY_private_0, delta_ABY_private_1, X0Y1);
HadamardMatrixMultiplication(gamma_ABY_private_1, delta_ABY_private_0, X1Y0);

GeneratePrivateShares_ABY(X0Y1, OT_Shares_0);
OT_Shares_1.resize(lengthWithDim);
for (int i = 0; i < OT_Shares_0.size(); i++) {
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

std::vector<std::uint64_t> theta0_L_hadamard(lengthWithDim, 0), theta1_L_hadamard(lengthWithDim, 0);
for (int i = 2; i < lengthWithDim; i++) {
  theta0_L_hadamard[i] = MOTION::new_fixed_point::truncate(dxdy_0[i] + OT_Shares_0[i] - Dxdy_0[i] - dxDy_0[i], FIXED_POINT);
  theta1_L_hadamard[i] = MOTION::new_fixed_point::truncate(dxdy_1[i] + OT_Shares_1[i] + DxDy_1[i] - Dxdy_1[i] - dxDy_1[i], FIXED_POINT);
}

gamma0_L.erase(gamma0_L.begin(), gamma0_L.begin() + 2);
gamma1_L.erase(gamma1_L.begin(), gamma1_L.begin() + 2);

delta0_L.erase(delta0_L.begin(), delta0_L.begin() + 2);
delta1_L.erase(delta1_L.begin(), delta1_L.begin() + 2);

theta0_L_hadamard.erase(theta0_L_hadamard.begin(), theta0_L_hadamard.begin() + 2);
theta1_L_hadamard.erase(theta1_L_hadamard.begin(), theta1_L_hadamard.begin() + 2);

for (int i = 0; i < len; i++) {
  a0_MSB_L[i] = gamma0_L[i] + delta0_L[i] - 2 * theta0_L_hadamard[i];
  a1_MSB_L[i] = gamma1_L[i] + delta1_L[i] - 2 * theta1_L_hadamard[i];
}

std::cout << "Compute MSB completed. \n" << std::endl;
}

// DerivativeRelu: Takes shares of a in L world and returns shares of ReLU'(a) in L world.
// ReLU'(a) = 1 if MSB(a) = 0, otherwise ReLU'(a) = 0.
// Input  : Vectors a_L_0 and a_L_1 of type std::uint64_t storing shares of a at P0 and P1 in L world respectively, empty vectors a_DRelu_0 and a_DRelu_1 of type std::uint64_t, dimensions of matrix (rows, cols)
// Output : Updates a_DRelu_0 and a_DRelu_1 to store ReLU' shares of a at Party 0 and Party 1 respectively.
void DerivativeRelu(std::vector<std::uint64_t>& a_L_0, std::vector<std::uint64_t>& a_L_1, std::vector<std::uint64_t>& a_DRelu_0, std::vector<std::uint64_t>& a_DRelu_1, std::size_t len, std::size_t rows, std::size_t cols) {
  std::cout << "Entered DerivativeRelu." << std::endl;
  std::vector<std::uint64_t> c_L_0(len, 0), c_L_1(len, 0);
  for (int i = 0; i < len; i++) {
    c_L_0[i] = 2 * a_L_0[i];  // @ Party 0
    c_L_1[i] = 2 * a_L_1[i];  // @ Party 1
  }

  std::vector<std::uint64_t> yOddShares0(len, 0), yOddShares1(len, 0);
  ShareConvert(c_L_0, c_L_1, yOddShares0, yOddShares1, len);
  
  ComputeMSB(yOddShares0, yOddShares1, a_DRelu_0, a_DRelu_1, len, rows, cols);

  // @ Party 0
  for (int i = 0; i < len; i++) {
    a_DRelu_0[i] = -a_DRelu_0[i];
  }

  for (int i = 0; i < len; i++) {
    a_DRelu_1[i] = MOTION::new_fixed_point::encode<uint64_t, long double>(1, FIXED_POINT) - a_DRelu_1[i];
  }
  std::cout << "Exited DerivativeRelu.\n" << std::endl;
  return;
}

// ReLU: Takes shares of a in L world and returns shares of ReLU(a) in L world
// Input  : Vectors a_L_0 and a_L_1 of type std::uint64_t to store shares of a in L world at Party 0 and Party 1 respectively, empty vectors a_Relu_0 and a_Relu_ 1 in L world, dimensions of matrix (rows, cols).
// Output : Updates a_Relu_0 and a_Relu_1 to store ReLU shares of a at Party 0 and Party 1 respectively.
void ReLU(std::vector<std::uint64_t>& a_L_0, std::vector<std::uint64_t>& a_L_1, std::vector<std::uint64_t>& a_Relu_0, std::vector<std::uint64_t>& a_Relu_1, std::size_t len, std::size_t rows, std::size_t cols) {
  std::cout << "Entered ReLU." << std::endl;
  std::vector<std::uint64_t> a_DRelu_0(len, 0), a_DRelu_1(len, 0);

  DerivativeRelu(a_L_0, a_L_1, a_DRelu_0, a_DRelu_1, len, rows, cols);
  
  ArithHadamardMatrixMultiplication(a_L_0, a_L_1, a_DRelu_0, a_DRelu_1, a_Relu_0, a_Relu_1, rows, cols);
  std::cout << "Exited ReLU.\n" << std::endl;
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

int main(int argc, char* argv[]) {
  std::default_random_engine engine;
  std::random_device rd;
  std::mt19937 gen(rd()); 
  InitializeModuloPrimeOps();
  
  int rows = 32768;
  int cols = 1;
  int len = rows * cols;
  std::vector<float> data(len, 0.0);
  std::vector<std::uint64_t> a(len, 0), a_L_0(len, 0), a_L_1(len, 0);
  std::vector<std::uint64_t> a_LMinusOne_0(len, 0), a_LMinusOne_1(len, 0);
  std::vector<std::uint64_t> a_MSB_0(len, 0), a_MSB_1(len, 0);
  std::vector<std::uint64_t> a_DRelu_0(len, 0), a_DRelu_1(len, 0);
  std::vector<std::uint64_t> a_Relu_0(len, 0), a_Relu_1(len, 0);

  for (int i = 0; i < len; i++) {
    data[i] = (len - 2 * i);
    a[i] = MOTION::new_fixed_point::encode<uint64_t, long double>(data[i], FIXED_POINT);
  }

  GenerateSharesOverL(a_L_0, a_L_1, a, len);
  auto start = std::chrono::high_resolution_clock::now();

  ReLU(a_L_0, a_L_1, a_Relu_0, a_Relu_1, len, rows, cols);

  auto end = std::chrono::high_resolution_clock::now();

  // for (int i = 0; i < len; i++) {
  //   auto SumOverRelu = a_Relu_0[i] + a_Relu_1[i];
  //   auto tempSumOverRelu = MOTION::new_fixed_point::decode<uint64_t, long double>(SumOverRelu, FIXED_POINT);

  //   if (data[i] >= 0) {
  //     std::cout << "x: " << data[i] << ", ReLU(x): " << tempSumOverRelu << ", Actual ReLU - ReLU(x): " << (data[i] - tempSumOverRelu) << std::endl;
  //   } else {
  //     std::cout << "x: " << data[i] << ", ReLU(x): " << tempSumOverRelu << ", Actual ReLU - ReLU(x): " << (0 - tempSumOverRelu) << std::endl;
  //   }
    
  // }

  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
  std::cout << "Execution time for ReLU: " << duration.count() << " milliseconds." << std::endl;

  return EXIT_SUCCESS;
}