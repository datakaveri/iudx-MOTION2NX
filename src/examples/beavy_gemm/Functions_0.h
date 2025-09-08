#ifndef FUNCTIONS_0_H
#define FUNCTIONS_0_H

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

#include "GlobalVar_0.h"

using namespace std::chrono;


template <typename E>
std::uint64_t RandomNumDistribution(E& engine) {
  std::uniform_int_distribution<unsigned long long> distribution(
      std::numeric_limits<std::uint64_t>::min(), std::numeric_limits<std::uint64_t>::max());
  return distribution(engine);
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
    file2 << "Helper Node Multiplication layer : \n";
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

  //Syncing up with helper node,
  std::vector<std::uint8_t> started{(std::uint8_t)HelperNodeSync};
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
  MatrixMultiplication(X_ABY_private, Y_ABY_private, dxdy);
  // std::cout << "\n ** dXdY ** \n";
  // for(int i = 0; i < dxdy.size(); i++)
  //    std::cout <<  dxdy[i] << " ,  ";
  
  std::vector<std::uint64_t> DxDy;
  MatrixMultiplication(X_ABY_public, Y_ABY_public, DxDy);
  // std::cout << "\n ** DXDY** \n";
  // for(int i = 0; i < DxDy.size(); i++)
  //    std::cout <<  DxDy[i] << " ,  ";
  
  std::vector<std::uint64_t> Dxdy; //DeltaX * deltay
  MatrixMultiplication(X_ABY_public, Y_ABY_private, Dxdy);
  // std::cout << "\n **DXdY ** \n";
  // for(int i = 0; i < Dxdy.size(); i++)
  //    std::cout <<  Dxdy[i] << " ,  ";
  
  std::vector<std::uint64_t> dxDy; //deltax * Deltay
  MatrixMultiplication(X_ABY_private, Y_ABY_public, dxDy);
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

void print_message(std::vector<std::uint8_t>& message); 
std::uint64_t getuint64(std::vector<std::uint8_t>& message, int index);
void adduint64(std::uint64_t num, std::vector<std::uint8_t>& message);
int ReadSharesIntoVec(std::string file_path, std::vector<uint64_t>&vec);
void ConvertVetorIntoMessage(std::vector<uint64_t>&vec, std::vector<uint8_t>&msg, std::uint8_t msg_type);
void ConvertMessageIntoVector(std::vector<uint8_t>&msg, std::vector<uint64_t>&vec);
int GeneratePrivateShares_ABY(std::vector<uint64_t> &vec, std::vector<uint64_t> &private_share);
void ConvertFileIntoMessage();
void ParallelAddition(std::vector<std::uint64_t> &op1, std::vector<std::uint64_t> &op2, std::vector<std::uint64_t> &ans, int offset);
int WriteABYToFile(std::vector<uint64_t>&pub, std::vector<uint64_t>&priv, std::string file_path);
void ParallelMatrixMult(std::vector<std::uint64_t> &op1, std::vector<std::uint64_t> &op2, std::vector<std::uint64_t> &ans, int offset);
int MatrixMultiplication(std::vector<uint64_t> &a,std::vector<uint64_t> &b, std::vector<uint64_t> &ans);
void OTGeneration(std::vector<uint64_t> &v1, std::vector<uint64_t> &v2, std::vector<uint64_t> &ot0, std::vector<uint64_t> &ot1);
void ParallelSubtraction(std::vector<std::uint64_t> &op1, std::vector<std::uint64_t> &op2, std::vector<std::uint64_t> &ans, int offset);


#endif
