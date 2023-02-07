#include <algorithm>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <random>
#include <regex>
#include <stdexcept>
#include <thread>

#include <boost/algorithm/string.hpp>
#include <boost/json/serialize.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/log/trivial.hpp>
#include <boost/program_options.hpp>

#include "algorithm/circuit_loader.h"
#include "base/gate_factory.h"
#include "base/two_party_backend.h"
#include "communication/communication_layer.h"
#include "communication/tcp_transport.h"
#include "compute_server/compute_server.h"
#include "statistics/analysis.h"
#include "utility/logger.h"

#include "base/two_party_tensor_backend.h"
#include "protocols/beavy/tensor.h"
#include "tensor/tensor.h"
#include "tensor/tensor_op.h"
#include "tensor/tensor_op_factory.h"
#include "utility/new_fixed_point.h"

// class Matrix{
//   private:
//   uint64_t row,column,num_elements;
//   std::vector<float>store1;

//   public:
//           Matrix(int rows, int cols) {
//             row = rows;
//             column = cols;
//             num_elements = rows*cols;
//         }

// std::vector<float> readMatrixW1CSV() {

//             std::ifstream indata;
//             indata.open("/home/srishti04/Desktop/IUDX/iudx-MOTION2NX/src/examples/conversion_1/W1.csv");

//             std::string line;
//             while (std::getline(indata,line)) {

//                 std::stringstream lineStream(line);
//                 std::string cell;

//                 while(std::getline(lineStream,cell, ','))
//                 {
//                     store1.push_back(std::stof(cell));
//                 }

//             }

//             return store1;
// }

// std::vector<float> readMatrixW2CSV() {

//             std::ifstream indata;
//             indata.open("/home/srishti04/Desktop/IUDX/iudx-MOTION2NX/src/examples/conversion_1/W2.csv");

//             std::string line;
//             while (std::getline(indata,line)) {

//                 std::stringstream lineStream(line);
//                 std::string cell;

//                 while(std::getline(lineStream,cell, ','))
//                 {
//                     store1.push_back(std::stof(cell));
//                 }

//             }

//             return store1;
// }

// };

// std::vector<float> readX()
// {
// std::ifstream file;
//     file.open("/home/srishti04/Desktop/IUDX/iudx-MOTION2NX/src/examples/conversion_1/X5_new.csv");
//     if(!file)
//     {
//       std::cerr<<"Error in reading file\n";
//     }
//     std::vector<float> data;
//     while(file.good())
//     {

//         std::string line;
//         std::getline(file,line);

//         std::stringstream lineStream(line);
//         std::string cell;

//         while(std::getline(lineStream,cell, ','))
//           {
//                //std::cout<<cell<<" ";
//                lineStream<<std::fixed<<std::setprecision(13) << std::stof(cell);
//                float fmoney = std::stof(lineStream.str());
//                //auto t=std::stof(cell);

//                //std::cout<<fmoney<<" ";
//                data.push_back(fmoney);
//           }

//         }
//        return data;
// }

// std::vector<float> readMatrixB1CSV() {

//             std::ifstream indata;
//             indata.open("/home/srishti04/Desktop/IUDX/iudx-MOTION2NX/src/examples/conversion_1/B1.csv");
//             std::vector<float>store1;
//             std::string line;
//             while (std::getline(indata,line)) {

//                 std::stringstream lineStream(line);
//                 std::string cell;

//                 while(std::getline(lineStream,cell, ','))
//                 {
//                     store1.push_back(std::stof(cell));
//                 }

//             }

//             return store1;
// }

// std::vector<float> readMatrixB2CSV() {

//             std::ifstream indata;
//             indata.open("/home/srishti04/Desktop/IUDX/iudx-MOTION2NX/src/examples/conversion_1/B2.csv");
//             std::vector<float>store1;
//             std::string line;
//             while (std::getline(indata,line)) {

//                 std::stringstream lineStream(line);
//                 std::string cell;

//                 while(std::getline(lineStream,cell, ','))
//                 {
//                     store1.push_back(std::stof(cell));
//                 }

//             }

//             return store1;
// }

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

int main() {
  //     int x=0;
  //     std::vector<float>data=readX();
  //     std::vector<uint64_t>d;
  //     for(int i=0;i<data.size();i++)
  //     {
  //       uint64_t temp1 = MOTION::new_fixed_point::encode<uint64_t,float>(data[i],13);
  //       d.push_back(temp1);
  //     }

  // //reading weight1 matrix --------------------------------------------------------------
  //     std::vector<float>weightv1;
  //     std::vector<uint64_t>w1;
  //     Matrix weightL1 = Matrix(256, 784);
  //     weightv1=weightL1.readMatrixW1CSV();

  //       for(int i=0;i<weightv1.size();i++)
  //        {
  //         //uint64_t temp1 = encode(weightv1[i],13);
  //         std::uint64_t temp1 = MOTION::new_fixed_point::encode<uint64_t,float>(weightv1[i],13);
  //        // std::cout<<"Encoded number:"<<temp1<<std::endl;

  //         w1.push_back(temp1);

  //         }

  // //pushing into matrix for matrix multiplication --------------------------------------
  //         int r=256;
  //         int c=784;
  //         uint64_t m[r][c];
  //         int k=0;
  //        for(int i=0;i<r;i++)
  //         {
  //             for(int j=0;j<c;j++)
  //             {
  //               m[i][j]=w1[k];
  //               k++;
  //               //std::cout<<m[i][j]<<std::endl;
  //             }
  //         }

  // //multiplying w1 and x ------------------------------------------------------------
  //        std::vector<uint64_t>ans;
  //        uint64_t temp=0;

  //         for(int i=0;i<r;i++)
  //         {
  //           for(int j=0;j<c;j++)
  //           {
  //             //std::cout<<m[i][j]<<"*"<<d[j]<<std::endl;
  //              //float k1=decode(m[i][j],13);
  //              //float k2=decode(d[j],13);
  //              //k3+=k1*k2;
  //              temp+=m[i][j]*d[j];
  //              //temp=temp/std::exp2(13);

  //           }
  //           //std::cout<<"Decoded w * k : "<<k3<<",";
  //           //std::cout<<temp<<",";
  //           long double temp2 = MOTION::new_fixed_point::decode<uint64_t,float>(weightv1[i],13);
  //           //std::cout<<"Decoded mul : "<<temp2<<",";
  //           //std::cout<<std::endl;
  //           ans.push_back(temp);

  //           temp=0;

  //        }

  // //encoding b1 ---------------------------------------------------------------------
  //         std::vector<float>B1v=readMatrixB1CSV();
  //         std::vector<uint64_t>B1;
  //          for(int i=0;i<B1v.size();i++)
  //        {
  //         //uint64_t temp1 = encode(B1v[i],13);
  //         std::uint64_t temp1 = MOTION::new_fixed_point::encode<uint64_t,float>(B1v[i],13);
  //         //std::cout<<temp1<<",";
  //         //float temp2 = decodeBV(temp2,13);
  //         B1.push_back(temp1);
  //         }

  // //adding w1*x+b ---------------------------------------------------------------------
  // std::vector<float>A1;

  // for(int i=0;i<B1.size();i++)
  // {
  //   auto temp=ans[i]+B1[i];
  //   long double temp2 = MOTION::new_fixed_point::decode<uint64_t,float>(temp,26);
  //   A1.push_back(temp2);
  //   temp=0;
  // }

  // //relu operation --------------------------------------------------------------------------
  // std::vector<uint64_t>A2;
  // for(int i=0;i<A1.size();i++)
  // {
  //   if(A1[i]>0)
  //   {
  //     std::uint64_t temp1 = MOTION::new_fixed_point::encode<uint64_t,float>(A1[i],13);
  //     A2.push_back(temp1);
  //   }
  //   else
  //   {
  //     A2.push_back(0);
  //   }
  // }

  // //reading weight2 matrix
  // ------------------------------------------------------------------------------

  //     std::vector<float>weightv2;
  //     std::vector<uint64_t>w2;
  //     Matrix weightL2 = Matrix(256, 784);
  //     weightv2=weightL2.readMatrixW1CSV();

  // for(int i=0;i<weightv2.size();i++)
  //        {
  //         //uint64_t temp1 = encode(weightv1[i],13);
  //         std::uint64_t temp1 = MOTION::new_fixed_point::encode<uint64_t,float>(weightv2[i],13);
  //        // std::cout<<"Encoded number:"<<temp1<<std::endl;

  //         w2.push_back(temp1);

  //         }

  // //pushing into matrix for matrix multiplication --------------------------------------
  //         int r1=256;
  //         int c1=784;
  //         uint64_t m1[r1][c1];

  //        for(int i=0;i<r1;i++)
  //         {
  //             for(int j=0;j<c1;j++)
  //             {
  //               m1[i][j]=w2[k];
  //               k++;
  //               //std::cout<<m[i][j]<<std::endl;
  //             }
  //         }

  // //multiplying w2 and A2 ------------------------------------------------------------
  //        std::vector<uint64_t>ans1;
  //        uint64_t p=0;

  //         for(int i=0;i<r1;i++)
  //         {
  //           for(int j=0;j<c1;j++)
  //           {
  //             //std::cout<<m[i][j]<<"*"<<d[j]<<std::endl;
  //              //float k1=decode(m[i][j],13);
  //              //float k2=decode(d[j],13);
  //              //k3+=k1*k2;
  //              p+=m1[i][j]*A2[j];
  //              //temp=temp/std::exp2(13);

  //           }
  //           //std::cout<<"Decoded w * k : "<<k3<<",";
  //           //std::cout<<temp<<",";
  //           long double temp2 = MOTION::new_fixed_point::decode<uint64_t,float>(p,13);
  //           //std::cout<<"Decoded mul : "<<temp2<<",";
  //           //std::cout<<std::endl;
  //           ans1.push_back(p);

  //           p=0;

  //        }

  // //encoding b2 ---------------------------------------------------------------------
  //         std::vector<float>B2v=readMatrixB2CSV();
  //         std::vector<uint64_t>B2;
  //          for(int i=0;i<B2v.size();i++)
  //        {
  //         //uint64_t temp1 = encode(B1v[i],13);
  //         std::uint64_t temp1 = MOTION::new_fixed_point::encode<uint64_t,float>(B2v[i],13);
  //         //std::cout<<temp1<<",";
  //         //float temp2 = decodeBV(temp2,13);
  //         B2.push_back(temp1);
  //         }

  // //adding w2*(relu(W1*X+B1))+B2
  // --------------------------------------------------------------------- std::vector<long
  // double>final_ans;

  // for(int i=0;i<B2.size();i++)
  // {
  //   auto temp=ans1[i]+B2[i];
  //   long double temp2 = MOTION::new_fixed_point::decode<uint64_t,float>(temp,26);
  //   std::cout<<temp2<<",";
  //   final_ans.push_back(temp2);
  //   temp=0;
  // }

  std::cout << "This is conversion ! \n";
  std::cout << "This is file1 ! \n";
  std::ifstream indata1;
  indata1.open(
      "/home/srishti04/Desktop/IUDX/iudx-MOTION2NX/build_debwithrelinfo_gcc/finaloutput_0");
  assert(indata1);
  std::uint64_t rows = read_file(indata1);
  std::uint64_t col = read_file(indata1);
  std::cout << rows << " " << col << "\n";
  for (int i = 0; i < rows * col; i++) {
    std::uint64_t num1 = read_file(indata1);
    std::uint64_t num2 = read_file(indata1);
    std::cout << num1 << " " << num2 << "\n";
  }

  std::cout << "This is end of file1 ! \n";
  indata1.close();

  std::cout << "This is conversion ! \n";
  std::cout << "This is file2 ! \n";
  std::ifstream indata2;
  indata2.open(
      "/home/srishti04/Desktop/IUDX/iudx-MOTION2NX/build_debwithrelinfo_gcc/finaloutput_1");
  assert(indata2);
  rows = read_file(indata2);
  col = read_file(indata2);
  std::cout << rows << " " << col << "\n";
  for (int i = 0; i < rows * col; i++) {
    std::uint64_t num1 = read_file(indata2);
    std::uint64_t num2 = read_file(indata2);
    std::cout << num1 << " " << num2 << "\n";
  }

  std::cout << "This is end of file2 ! \n";
  indata2.close();
}
