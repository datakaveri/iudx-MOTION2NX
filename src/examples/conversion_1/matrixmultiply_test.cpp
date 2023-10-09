#include <utility/new_fixed_point.h>
#include <bitset>
#include <iostream>
#include <vector>

#include "utility/linear_algebra.h"

int main() {
  std::vector<std::uint64_t> arr{8192, 16384, 24576, 32768, 8192, 8192, 8192, 8192};

  std::vector<std::uint64_t> theta{8192, 8192, 8192, 8192};

  std::vector<std::uint64_t> dot_product(2, 0);

  dot_product = MOTION::matrix_multiply(1, 4, 2, theta, arr);

  float x = -8;
  auto y = MOTION::new_fixed_point::encode<std::uint64_t, float>(x, 13);

  std::cout << MOTION::new_fixed_point::decode<std::uint64_t, float>(y, 13) << " ";

  std::cout << std::bitset<sizeof(y) * 8>(y) << "\n";

  for (int i = 0; i < dot_product.size(); i++) {
    auto temp = MOTION::new_fixed_point::truncate(dot_product[i], 13);
    std::cout << MOTION::new_fixed_point::decode<std::uint64_t, float>(temp, 13) << " ";
  }
}