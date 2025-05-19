#include <boost/archive/text_oarchive.hpp>
#include <boost/asio.hpp>
#include <boost/chrono.hpp>
#include <boost/program_options.hpp>
#include <boost/serialization/string.hpp>
#include <boost/thread.hpp>
#include <cstdint>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
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
//This file implements standalone code for PrivateCompare
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
void AddModuloOdd(std::vector<std::uint64_t> &a0, std::vector<std::uint64_t> &a1, std::vector<std::uint64_t> &ans, std::size_t len = 1)
{
for(int i = 0; i < len; i++)
 {
  if ((a0[i] == LMINUS_ONE) and (a1[i] == LMINUS_ONE))
      ans[i] = 0;
  else 
    
    {
    ans[i] = (a0[i] + a1[i] + WrapAround(a0[i], a1[i])) % LMINUS_ONE;
    std::cout << "a0 : " << a0[i] << ", a1 : " << a1[i] << ", ans : " << ans[i] <<"\n";
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
      std::cout << "input : " << a1[i] << " , Negation : " << temp[i] << ", input + negation : " << temp[i]+a1[i] << "\n";
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
    t = RandomNumOverPrime(gen);
    bit_shares_x0[i*BIT_SIZE + k] = t;
    //Note that we are starting from MSB so that in PrivateCompare, for loop starts k = 0, k++; instead k = 64 and k-- 
    auto bt = (x[i] >> BIT_SIZE-1 - k) & 1; 
    //bit_shares_x1[i*BIT_SIZE + k] = bt - bit_shares_x0[i*BIT_SIZE + k]
    bit_shares_x1[i*BIT_SIZE + k] = subModPrime[bt][t];
  }
}
}
void GenerateLSBSharesOverL(std::vector<std::uint64_t> &lsb_shares_x0, std::vector<std::uint64_t> &lsb_shares_x1, std::vector<std::uint64_t> &x, std::size_t len = 1)
{
std:: cout << " Entered GenerateLSBSharesOverL \n";
std::uint64_t t;
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
   std::cout << lsb_shares_x0[i] + lsb_shares_x1[i] << " , " << temp <<"\n";
 }
}

std::vector<std::uint64_t> PrivateCompare(std::vector<std::uint64_t>& c, std::vector<std::uint64_t> &bit_share_x0, std::vector<std::uint64_t> &r, std::vector<std::uint64_t> &beta, std::size_t len, int startRange, int endRange, int party_id = 0) 
{
std::cout << "Entered PrivateCompare: " << party_id << "\n";  
std::uint64_t value_r, value_beta;
std::uint64_t t, j, ind, w, temp_prime_sh;
// std::vector<std::uint64_t> c(len*BIT_SIZE);

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
    // std::cout << "c at ind : "<< ind << " is " << c[ind] << "\n";

    c[ind] = MultiplyModuloPrime(c[ind], RandomNonZeroNumOverPrime(gen_s));
  }
GenerateRandomPermutation(c, i*BIT_SIZE, (i+1)*BIT_SIZE);
}
return c;
}

void PrivateCompare_P2(std::vector<std::uint64_t>& c0, std::vector<std::uint64_t>& c1, std::vector<std::uint64_t>& beta_prime, std::size_t len, int startRange, int endRange)
{
std::cout << "Entered PrivateCompare_P2: " << "  \n";
std::uint64_t ind, k;
for (int i = startRange; i < endRange;  i++) {
    for (k = 0; k < BIT_SIZE; k++) {
      ind = i * BIT_SIZE + k;
      if (AddModuloPrime(c0[ind], c1[ind]) == 0) { // x > r 
        beta_prime[i] = 1;
        std::cout << c0[ind] << c1[ind] << "index: " << ind << "\n"; 
        break;
      }
    }
  }
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
  auto start = std::chrono::high_resolution_clock::now();
  std::random_device rd;
  std::mt19937 gen(rd()); 
  InitializeModuloPrimeOps();
  int len = 8;
  std::vector<std::uint64_t> x(len, 0);
  std::vector<float> data(len, 0);
  std::vector<std::uint64_t> r(len, 0), beta(len, 0);
  std::vector<std::uint64_t> x0(len, 0), x1(len, 0);
  std::vector<std::uint64_t> beta_prime(len, 0);
  std::vector<std::uint64_t> c0(len*BIT_SIZE, 0), c1(len*BIT_SIZE, 0);
  std::vector<std::uint64_t> bit_shares_x0(len*BIT_SIZE, 0), bit_shares_x1(len*BIT_SIZE, 0);
  std::vector<std::uint64_t> lsb_share_x0(len, 0), lsb_share_x1(len, 0);
  data[0] = 8;
  r[0] = LMINUS_ONE;
  beta[0] = 0;

  data[1] = 10;
  r[1] = 2;
  beta[1] = 1;

  data[2] = 10;
  r[2] = 20;
  beta[2] = 1;

  data[3] = LMINUS_ONE;
  r[3] = 800;
  beta[3] = 1;

  data[4] = -1;
  r[4] = 100;
  beta[4] = 0;

  data[5] = -6;
  r[5] = 7.5;
  beta[5] = 1;

  data[6] = -100;
  r[6] = LMINUS_ONE;
  beta[6] = 0;

  data[7] = LMINUS_ONE;
  r[7] = -10;
  beta[7] = 1;

  for (int i = 0; i < len; i++) {
    x[i] = MOTION::new_fixed_point::encode<uint64_t, long double>(data[i], 13);
    r[i] = MOTION::new_fixed_point::encode<uint64_t, long double>(r[i], 13);
    std::cout << x[i] <<  " " << r[i] << "\n";
  }

  GenerateBitSharesOverPrime(bit_shares_x0, bit_shares_x1, x, len);

  // c0 = PrivateCompare(bit_shares_x0, r, beta, len, 0);
  // c1 = PrivateCompare(bit_shares_x1, r, beta, len, 1);

  std::thread thread1(PrivateCompare, std::ref(c0), std::ref(bit_shares_x0), std::ref(r), std::ref(beta), len, 0, (int)(len / 2), 0);
  std::thread thread2(PrivateCompare, std::ref(c0), std::ref(bit_shares_x0), std::ref(r), std::ref(beta), len, (int)(len / 2), len, 0);
  std::thread thread3(PrivateCompare, std::ref(c1), std::ref(bit_shares_x1), std::ref(r), std::ref(beta), len, 0, (int)(len / 2), 1);
  std::thread thread4(PrivateCompare, std::ref(c1), std::ref(bit_shares_x1), std::ref(r), std::ref(beta), len, (int)(len / 2), len, 1);

  thread1.join();
  thread2.join();
  thread3.join();
  thread4.join();

  PrivateCompare(c0, bit_shares_x0, r, beta, len, 0, len, 0);
  PrivateCompare(c1, bit_shares_x1, r, beta, len, 0, len, 1);

  // std::thread thread1(PrivateCompare_P2, std::ref(c0), std::ref(c1), std::ref(beta_prime), len, 0, (int)(len / 4));
  // std::thread thread2(PrivateCompare_P2, std::ref(c0), std::ref(c1), std::ref(beta_prime), len, (int)(len / 4), (int)(2 * len / 4));
  // std::thread thread3(PrivateCompare_P2, std::ref(c0), std::ref(c1), std::ref(beta_prime), len, (int)(2 * len / 4), (int)(3 * len / 4));
  // std::thread thread4(PrivateCompare_P2, std::ref(c0), std::ref(c1), std::ref(beta_prime), len, (int)(3 * len / 4), len);

  // thread1.join();
  // thread2.join();
  // thread3.join();
  // thread4.join();

  PrivateCompare_P2(c0, c1, beta_prime, len, 0, len);

  for (int i = 0; i < len; i++) {
    if (beta[i] + beta_prime[i] == 1)
      std::cout << "x > r : " << " x : " << x[i] <<", r : "<< r[i] << "\n";
    else 
      std::cout << "x <= r : " << " x : " << x[i] <<", r : "<< r[i] << "\n";
  }
  auto end = std::chrono::high_resolution_clock::now();

  auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
  std::cout << "Duration for execution: " << duration.count() << " microseconds." << std::endl;

  return EXIT_SUCCESS;

}