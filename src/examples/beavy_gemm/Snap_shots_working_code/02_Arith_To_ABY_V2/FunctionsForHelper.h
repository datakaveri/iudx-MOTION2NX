#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#pragma once

#include <bits/stdc++.h>
#include <filesystem>
#include <fstream>
#include <regex>
#include <stdexcept>
#include <utility>
#include "communication/communication_layer.h"
#include "communication/message_handler.h"
#include "communication/tcp_transport.h"
#include "utility/logger.h"

#include <boost/algorithm/string.hpp>
#include <boost/chrono.hpp>
#include <boost/json/serialize.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/log/trivial.hpp>
#include <boost/program_options.hpp>
#include <boost/thread/thread.hpp>

#include <iostream>
#include <iterator>
#include <parallel/algorithm>
#include <vector>
#include "utility/new_fixed_point.h"




void adduint64(std::uint64_t num, std::vector<std::uint8_t>& message) {
  for (auto i = 0; i < sizeof(num); i++) {
    std::uint8_t byte = num & 0xff;
    message.push_back(byte);
    num = num >> 8;
  }
}


std::uint64_t getuint64(std::vector<std::uint8_t>& message, int index) {
  std::uint64_t num = 0;
  for (auto i = 0; i < 8; i++) {
    num = num << 8;
    num = num | message[(index + 1) * 8 - 1 - i];
  }
  return num;
}

template <typename E>
std::uint64_t RandomNumGenerator(E &engine)
{
     std::uniform_int_distribution<unsigned long long> dis(
         std::numeric_limits<std::uint64_t>::min(),
         std::numeric_limits<std::uint64_t>::max());
     return dis(engine);
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

int MatrixMultiplication(std::vector<uint64_t> &a,std::vector<uint64_t> &b, std::vector<uint64_t> &ans);

#endif

