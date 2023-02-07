#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <random>
#include <regex>
#include <stdexcept>
#include <thread>

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
  std::ofstream indata1;
  indata1.open("/home/srishti04/Desktop/IUDX/iudx-MOTION2NX/build_debwithrelinfo_gcc/finaloutput_0",
               std::ios_base::app);

  std::ifstream indata2;
  indata2.open(
      "/home/srishti04/Desktop/IUDX/iudx-MOTION2NX/build_debwithrelinfo_gcc/server0/outputshare_0");

  std::uint64_t rows = read_file(indata2);
  std::uint64_t col = read_file(indata2);
  for (int i = 0; i < rows * col; i++) {
    std::uint64_t nums1 = read_file(indata2);
    indata1 << nums1 << " ";
    std::uint64_t nums2 = read_file(indata2);
    indata1 << nums2 << "\n";
  }

  indata1.close();
  indata2.close();

  indata1.open("/home/srishti04/Desktop/IUDX/iudx-MOTION2NX/build_debwithrelinfo_gcc/finaloutput_1",
               std::ios_base::app);

  indata2.open(
      "/home/srishti04/Desktop/IUDX/iudx-MOTION2NX/build_debwithrelinfo_gcc/server1/outputshare_1");

  rows = read_file(indata2);
  col = read_file(indata2);
  for (int i = 0; i < rows * col; i++) {
    std::uint64_t nums1 = read_file(indata2);
    indata1 << nums1 << " ";
    std::uint64_t nums2 = read_file(indata2);
    indata1 << nums2 << "\n";
  }

  indata1.close();
  indata2.close();
}