/*
This code reads input from CreateSharesForInput.txt and creates shares of the input
and writes them to SharesForInput0.txt and SharesForInput1.txt in ShareFiles folder. These two files have
first row as "rows columns" of input data. This code is intended to create shares of vectors. It has to be 
generalized to matrix.

How to run 
$ ./bin/Shares_input
*/
// MIT License
//
// Copyright (c) 2021 Lennart Braun
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include <iostream>  
#include <type_traits>
#include <cstdint>
#include <random> 
#include <stdexcept> 
#include <filesystem>
#include <fstream>
#include <optional>

#include <boost/asio.hpp>
#include <boost/program_options.hpp>
#include <boost/serialization/string.hpp>
#include <boost/archive/text_oarchive.hpp>
#include "utility/fixed_point.h"

using namespace boost::asio;
namespace po = boost::program_options;  
using ip::tcp;  
using std::string;  
using std::cout;  
using std::endl;  

struct Matrix {
  std::vector<uint64_t> Delta;
  std::vector<uint64_t> delta;
  int row;
  int col;
};

struct Shares {
     uint64_t Delta,delta;
};

//////////////////New functions////////////////////////////////////////
///In read_file also include file not there error and file empty alerts
std::uint64_t read_file(std::ifstream& indata) {
  std::string str;
  char num;
  while (indata >> std::noskipws >> num) {
    if (num != ' ' && num != '\n') {
      str.push_back(num);
    } else {
      break;
    }
  }
  std::string::size_type sz = 0;
  std::uint64_t ret = (uint64_t)std::stoull(str, &sz, 0);
  return ret;
}

//////////////////////////////////////////////////////////////////////
template <typename E>
std::uint64_t blah(E& engine) {
    std::uniform_int_distribution<unsigned long long> dis(
        std::numeric_limits<std::uint64_t>::min(),
        std::numeric_limits<std::uint64_t>::max()
    );
    return dis(engine);
}


void share_generation(std::ifstream & indata, int rows, int columns) {
     //get input data
     std::vector<float> data;

     int count = 0;

     float temp;
     while(indata >> temp) {
          data.push_back(temp);
          count++;
          if(count == rows * columns) {
               break;
          }
     }

     while(count < rows) {
          data.push_back(0);
     }

     for(int i=0;i<data.size();i++) {
          std::cout << data[i] << " ";
     }
     std::cout << "\n";
     auto p = std::filesystem::current_path();
     auto q = std::filesystem::current_path();
     //p += "/ShareFiles/X9_n.csv";
     p += "/SharesForInput0.txt";
     q += "/SharesForInput1.txt";
       std::ofstream file1,file2;
          file1.open(p,std::ios_base::out);
          if(!file1) {
              std::cerr << " Error in reading file 1\n";
          }
          file2.open(q,std::ios_base::out);
          if(!file2) {
          std::cerr << " Error in reading file 2\n";
          }
          if(file1.is_open())
          {
            file1<<rows;
            file1<<" ";

            file1<<columns;
            file1<<"\n";
         
          }
          if(file2.is_open())
          {
            file2<<rows;
            file2<<" ";

            file2<<columns;
            file2<<"\n";
         
          }
     //Now that we have data, need to generate the shares
     for(int i=0;i<data.size();i++){
          std::random_device rd;
          std::mt19937 gen(rd());
          std::uint64_t del0 = blah(gen);
          std::uint64_t del1 = blah(gen);

          std::uint64_t Del =  del0 + del1 + MOTION::fixed_point::encode<uint64_t,float>(data[i],13);
                    
          //////////Writing shares for server 0//////////////
          if(file1.is_open())
          {
            file1<<Del;
            file1<<" ";

            file1<<del0;
            file1<<"\n";
         
          }
          
            //////////Writing shares for server 1//////////////
          if(file2.is_open())
          {
            file2<<Del;
            file2<<" ";

            file2<<del1;
            file2<<"\n";
         
          }
         

 
     }
      
       file1.close();
       file2.close();
      
     return;

}


int main(int argc, char* argv[]) {
     std::cout << "Inside main";
     auto p = std::filesystem::current_path();
     //p += "/ShareFiles/X9_n.csv";
     p += "/CreateSharesForInput.txt";
     std::cout << p;
     std::ifstream indata;
     indata.open(p);
     if((std::ifstream(p)))
      cout << "File found";
     else
      cout << "File not found";
     int rows,columns;
     indata >> rows;
     indata >> columns;

     share_generation(indata,rows,columns); 

     indata.close();

  return EXIT_SUCCESS;
}
