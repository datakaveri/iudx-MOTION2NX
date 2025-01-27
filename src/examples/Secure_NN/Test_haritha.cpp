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
#include <random>
#include <stdexcept>
#include <type_traits>
#include <vector>
#include "./fixed-point.h"

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
    std::shuffle(nums.begin(), nums.begin()+end, gen); // Shuffle the vector with the seeded generator

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
    a0[i] = RandomNumDistribution(gen);
    a1[i] = a[i] - a0[i];
  }
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
    //c[ind] = MultiplyModuloPrime(c[ind], RandomNonZeroNumOverPrime(gen_s));
  }
//GenerateRandomPermutation(c, i*BIT_SIZE, (i+1)*BIT_SIZE);
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


/************** ShareConvert End*/
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
  
  
  options.permutefile =  "/home/iudx/Desktop/Haritha/iudx-MOTION2NX-1/build_debwithrelinfo_gcc/3PC_Relu/permute";
  options.fractional_bits = vm["fractional-bits"].as<size_t>();
  return options;
}
int main(int argc, char* argv[]) 
{
  std::random_device rd;
  std::mt19937 gen(rd()); 
  InitializeModuloPrimeOps();
  std::size_t len = 6;
  std::vector<float> data(len, 0.0);
  std::vector<std::uint64_t> a(len, 0),  a_L_0(len, 0), a_L_1(len, 0);
  std::vector<std::uint64_t> b_odd_0(len, 0), b_odd_1(len, 0);
  data[0] = 10;
  data[1] = LMINUS_ONE -1;
  data[2] = 0.5;
  data[3] = 0.1;
  data[4] = -1;
  for(int i = 0; i < len; i++)
  {
   a[i] = MOTION::new_fixed_point::encode<uint64_t, long double>(data[i], 13);
  }
  
//   GenerateSharesOverL(a_L_0, a_L_1, a, len); 
//   ShareConvert(a_L_0, a_L_1, a_Lmiusone_0, a_Lmiusone_1, len, 0);

a_L_0[0] = 1852346952341056093;
a_L_0[1] = 16465701321750049818;
a_L_0[2] = 11086684022036251186;
a_L_0[3] = 10866355458733683645;
a_L_0[4] = 2213172897949116083;
a_L_0[5] = 5764390665001582668;

a_L_1[0] = 16594397121368495523;
a_L_1[1] = 1981042751959509990;
a_L_1[2] = 7360060051673316814;
a_L_1[3] = 7580388614975892547;
a_L_1[4] = 16233571175760468301;
a_L_1[5] = 12682353408708009908;

b_odd_0[0] = 2548752759948599083; 
b_odd_0[1] = 15762832721726715362;
b_odd_0[2] = 12984923881478175837; 
b_odd_0[3] = 4111831938452549098; 
b_odd_0[4] = 6772230162212272042; 
b_odd_0[5] = 8305267294763083617;

b_odd_1[0] = 15897991313760952533; 
b_odd_1[1] = 2683911351982844446;
b_odd_1[2] = 5461820192231392163; 
b_odd_1[3] = 14334912135257027095; 
b_odd_1[4] = 11674513911497312341; 
b_odd_1[5] = 10141476778946508958; 


  

  for(int i = 0; i<len; i++)
  {
    auto x =  a_L_0[i] + a_L_1[i];
    std::cout << "Modulo L decoded : ";
    auto temp_x = MOTION::new_fixed_point::decode<uint64_t, long double>(x, 13);
    std::cout << temp_x << "\n";
    
    auto y = AddModuloOdd(b_odd_0[i], b_odd_1[i]);
    
    auto temp_y = MOTION::new_fixed_point::decode<uint64_t, long double>(y, 13);
    std::cout <<"Modulo Odd decoded : " << temp_y << "\n";
    // if (AddModuloOdd(a_Lmiusone_0[i], a_Lmiusone_1[i]) != a[i])
    //    std::cout << "L-1 shares are not created correct at i : " << i << "\n";
  }
return EXIT_SUCCESS;

}