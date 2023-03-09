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
  std::string basedir = getenv("BASE_DIR");
  std::string filename = basedir + "/build_debwithrelinfo_gcc";
  std::string filename1 = filename + "/finaloutput_0";
  indata1.open(filename1, std::ios_base::app);

  std::string filename2 = filename + "/server0/outputshare_0";
  std::ifstream indata2;
  indata2.open(filename2);

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

  filename1 = filename + "/finaloutput_1";
  filename2 = filename + "/server1/outputshare_1";

  indata1.open(filename1, std::ios_base::app);
  indata2.open(filename2);

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
