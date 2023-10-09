/*
This code reads input from CreateSharesForInput.txt and creates shares of the input
and writes them to SharesForInput0.txt and SharesForInput1.txt in ShareFiles folder. These two files
have first row as "rows columns" of input data. This code is intended to create shares of vectors.
It has to be generalized to matrix.

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

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <random>
#include <stdexcept>
#include <type_traits>

#include <boost/archive/text_oarchive.hpp>
#include <boost/asio.hpp>
#include <boost/program_options.hpp>
#include <boost/serialization/string.hpp>
#include "utility/fixed_point.h"

using namespace boost::asio;
namespace po = boost::program_options;
using ip::tcp;
using std::cout;
using std::endl;
using std::string;

struct Matrix {
  std::vector<uint64_t> Delta;
  std::vector<uint64_t> delta;
  int row;
  int col;
};

struct Shares {
  uint64_t Delta, delta;
};

//////////////////New functions////////////////////////////////////////
/// In read_file also include file not there error and file empty alerts
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
  std::uniform_int_distribution<unsigned long long> dis(std::numeric_limits<std::uint64_t>::min(),
                                                        std::numeric_limits<std::uint64_t>::max());
  return dis(engine);
}

void share_generation(std::ifstream& indata, int rows, int columns, int index) {
  // get input data
  std::vector<float> data;
  // int index=1;

  int count = 0;

  float temp;
  while (indata >> temp) {
    data.push_back(temp);
    count++;
    if (count == rows) {
      break;
    }
  }

  while (count < rows) {
    data.push_back(0);
  }

  for (int i = 0; i < data.size(); i++) {
    std::cout << data[i] << " ";
  }
  std::cout << "\n";
  auto p = std::filesystem::current_path();
  auto q = std::filesystem::current_path();
  // p += "/ShareFiles/X9_n.csv";
  p += "/server0/Image_shares/Sample_shares" + std::to_string(index);
  q += "/server1/Image_shares/Sample_shares" + std::to_string(index);
  std::ofstream file1, file2;
  file1.open(p, std::ios_base::out);
  if (!file1) {
    std::cerr << " Error in reading file 1\n";
  }
  file2.open(q, std::ios_base::out);
  if (!file2) {
    std::cerr << " Error in reading file 2\n";
  }
  if (file1.is_open()) {
    file1 << rows;
    file1 << " ";

    file1 << columns;
    file1 << "\n";
  }
  if (file2.is_open()) {
    file2 << rows;
    file2 << " ";

    file2 << columns;
    file2 << "\n";
  }
  // Now that we have data, need to generate the shares
  for (int i = 0; i < data.size(); i++) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uint64_t del0 = blah(gen);
    std::uint64_t del1 = blah(gen);

    std::uint64_t Del = del0 + del1 + MOTION::fixed_point::encode<uint64_t, float>(data[i], 13);

    //////////Writing shares for server 0//////////////
    if (file1.is_open()) {
      file1 << Del;
      file1 << " ";

      file1 << del0;
      file1 << "\n";
    }

    //////////Writing shares for server 1//////////////
    if (file2.is_open()) {
      file2 << Del;
      file2 << " ";

      file2 << del1;
      file2 << "\n";
    }
  }

  file1.close();
  file2.close();

  return;
}

void actual_share_generation(std::ifstream& indata, int rows, int columns) {
  // get input data
  std::vector<float> data;
  // int index=1;

  int count = 0;

  float temp;
  while (indata >> temp) {
    data.push_back(temp);
    count++;
    if (count == rows) {
      break;
    }
  }

  while (count < rows) {
    data.push_back(0);
  }

  for (int i = 0; i < data.size(); i++) {
    std::cout << data[i] << " ";
  }
  std::cout << "\n";
  auto p = std::filesystem::current_path();
  auto q = std::filesystem::current_path();
  // p += "/ShareFiles/X9_n.csv";
  p += "/server0/Image_shares/Theta";
  q += "/server1/Image_shares/Theta";
  std::ofstream file1, file2;
  file1.open(p, std::ios_base::out);
  if (!file1) {
    std::cerr << " Error in reading file 1\n";
  }
  file2.open(q, std::ios_base::out);
  if (!file2) {
    std::cerr << " Error in reading file 2\n";
  }
  if (file1.is_open()) {
    file1 << rows;
    file1 << " ";

    file1 << columns;
    file1 << "\n";
  }
  if (file2.is_open()) {
    file2 << rows;
    file2 << " ";

    file2 << columns;
    file2 << "\n";
  }
  // Now that we have data, need to generate the shares
  for (int i = 0; i < data.size(); i++) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uint64_t del0 = blah(gen);
    std::uint64_t del1 = blah(gen);

    std::uint64_t Del = del0 + del1 + MOTION::fixed_point::encode<uint64_t, float>(data[i], 13);

    //////////Writing shares for server 0//////////////
    if (file1.is_open()) {
      file1 << Del;
      file1 << " ";

      file1 << del0;
      file1 << "\n";
    }

    //////////Writing shares for server 1//////////////
    if (file2.is_open()) {
      file2 << Del;
      file2 << " ";

      file2 << del1;
      file2 << "\n";
    }
  }

  file1.close();
  file2.close();

  return;
}

int main() {
  std::cout << "Inside main";
  // std::vector<int> Xindex = {2, 5, 14, 29, 31, 37, 39, 40, 3, 10, 13, 25, 28, 55, 69, 71};  //
  // this int k = 1; int index = 1; // this for (int k : Xindex) {
  //   auto p = std::filesystem::current_path();
  //    p += "/ShareFiles/X9_n.csv";
  //   std::cout << "k:" << k << "\n";
  std::string home_dir = getenv("BASE_DIR");
  // std::string p = home_dir + "/data/ImageProvider/images/X" + std::to_string(k) + ".csv";  //
  std::string p = home_dir + "/data/ImageProvider/images/filled_nonzeros.csv";
  std::cout << p;
  std::ifstream indata;
  indata.open(p);
  if ((std::ifstream(p)))
    cout << "File found";
  else
    cout << "File not found";
  // int rows = 784;  //,columns;  this
  // indata >> rows;
  // indata >> columns;
  int rows = 784;

  // share_generation(indata, rows, 1, index);  // this
  // index = index + 1;                         // this
  actual_share_generation(indata, rows, 1);

  indata.close();
  //}  // this
  return EXIT_SUCCESS;
}
