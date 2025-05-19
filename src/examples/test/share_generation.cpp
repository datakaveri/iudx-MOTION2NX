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

template <typename E>
std::uint64_t blah(E& engine) {
  std::uniform_int_distribution<unsigned long long> dis(std::numeric_limits<std::uint64_t>::min(),
                                                        std::numeric_limits<std::uint64_t>::max());
  return dis(engine);
}

void generate_random_numbers() {
  std::random_device rd;   // Seed for the random number engine
  std::mt19937 gen(rd());  // Mersenne Twister random number generator

  int minNumber = 0;
  int maxNumber = 10;
  std::uniform_int_distribution<int> distribution(minNumber, maxNumber);
  std::ofstream file;
  std::string home_dir = getenv("BASE_DIR");
  std::string path = home_dir + "/build_debwithrelinfo_gcc/random_numbers";
  file.open(path, std::ios_base::out);
  for (int i = 0; i < 10; i++) {
    int random_number = distribution(gen);
    file << random_number << "\n";
  }
  file.close();
}

void generate_numbers() {
  std::ofstream file;
  std::string home_dir = getenv("BASE_DIR");
  std::string path = home_dir + "/build_debwithrelinfo_gcc/test_numbers";
  file.open(path, std::ios_base::out);

  for (float value = -10.0; value <= 10.0; value += 0.001) {
    file << value << "\n";
  }
  file.close();
}

void share_generation(int rows, int columns, std::vector<float> data) {
  auto p = std::filesystem::current_path();
  auto q = std::filesystem::current_path();
  p += "/motion2nx_relu/server0/outputshare_0";
  q += "/motion2nx_relu/server1/outputshare_1";
  // p += "/SharesForW1T0";
  // q += "/SharesForW1T1";
  // p += "/server0/Image_shares/Sample_shares2";
  // q += "/server1/Image_shares/Sample_shares2";
  std::cout << p << " " << q << "\n";
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

  int size_of_data = data.size();
  std::cout << size_of_data << "\n";
  ;
  std::cout << "size of data:" << size_of_data << "\n";
  // Now that we have data, need to generate the shares
  for (int i = 0; i < size_of_data; i++) {
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
}

int main() {
  // generate_random_numbers();

  // generate_numbers();
  int r = 32768;
  int c = 1;

  std::ifstream file1;
  std::string home_dir = getenv("BASE_DIR");
  // std::string path = home_dir + "/build_debwithrelinfo_gcc/random_numbers";
  // std::string path = home_dir + "/build_debwithrelinfo_gcc/test_numbers";
  std::string path = home_dir + "/build_debwithrelinfo_gcc/motion2nx_relu/test_numbers";

  file1.open(path);
  if (!file1) {
    std::cerr << "Error detected." << std::endl;
  }
  float x;
  std::vector<float> arr;

  for (int i = 0; i < r * c; i++) {
    x = 2 * i;
    file1 >> x;
    arr.push_back(x);
  }

  std::cout << "\n";
  share_generation(r, c, arr);
}