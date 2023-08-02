#include <algorithm>
#include <bitset>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>

#include <utility/new_fixed_point.h>
#include "utility/linear_algebra.h"

bool is_empty(std::ifstream& file) { return file.peek() == std::ifstream::traits_type::eof(); }

void read_input(std::vector<float>& x_input, int m) {
  std::string home_dir = getenv("BASE_DIR");
  std::string path = home_dir + "/data/ImageProvider/images/";
  std::ifstream file;

  for (int i = 0; i < m; i++) {
    std::string input_path = path + "X" + std::to_string(i) + "_ml.csv";

    std::cout << input_path << "\n";
    file.open(input_path);

    std::string str;

    while (std::getline(file, str)) {
      std::stringstream obj(str);

      std::string temp;

      while (std::getline(obj, temp, ',')) {
        auto input = std::stof(temp);
        std::cout << input << " ";
        x_input.push_back(input);
      }
    }
    std::cout << "\n";

    file.close();
    input_path = "";

    // int size_single_input = x_input.size() / m;

    // for (int i = 0; i < size_single_input; i++) {
    //   std::cout << x_input[i + size_single_input] << " ";
    // }
    // std::cout << "\n";
  }
}

uint64_t sigmoid(uint64_t dot_product, int frac_bits) {
  auto encoded_threshold = MOTION::new_fixed_point::encode<std::uint64_t, float>(0.5, frac_bits);
  auto neg_encoded_threshold =
      MOTION::new_fixed_point::encode<std::uint64_t, float>(-0.5, frac_bits);

  //  std::cout << "dot_product: " << std::bitset<sizeof(dot_product) * 8>(dot_product) << "\n";
  std::uint64_t msb = (uint64_t)1 << 63;
  // std::cout << "msb: " << std::bitset<sizeof(msb) * 8>(msb) << "\n";

  if (dot_product & msb) {
    if (dot_product < neg_encoded_threshold || dot_product == neg_encoded_threshold) {
      std::cout << "0 line 58\n";
      return 0;
    } else if (dot_product > neg_encoded_threshold && dot_product < (uint64_t)0)
      std::cout << " line 61\n";
    return dot_product + encoded_threshold;
  } else {
    if ((dot_product > (uint64_t)0 || dot_product == (uint64_t)0) &&
        dot_product < encoded_threshold) {
      std::cout << "line 67\n";
      return dot_product + encoded_threshold;
    } else if (dot_product > encoded_threshold || dot_product == encoded_threshold) {
      std::cout << "1 line 70\n";
      return 1;
    }
  }
}

std::vector<float> find_weights(std::vector<float> input, int m, int frac_bits) {
  // // input->encoded_input
  std::vector<std::uint64_t> encoded_input(input.size(), 0);

  std::transform(input.begin(), input.end(), encoded_input.begin(), [frac_bits](auto j) {
    return MOTION::new_fixed_point::encode<std::uint64_t, float>(j, frac_bits);
  });

  // // dot_product_decoded = Theta * encoded_input
  int size_single_input = input.size() / m;

  std::vector<std::uint64_t> theta_current(size_single_input, (uint64_t)0);
  std::vector<std::uint64_t> dot_product(m, 0);
  std::vector<std::uint64_t> dot_product_decoded(m, 0);

  dot_product = MOTION::matrix_multiply(1, size_single_input, m, theta_current, encoded_input);

  std::transform(dot_product.begin(), dot_product.end(), dot_product_decoded.begin(),
                 [frac_bits](auto j) {
                   return MOTION::new_fixed_point::decode<std::uint64_t, float>(j, frac_bits);
                 });

  std::cout << "dot_product: ";
  for (int i = 0; i < dot_product_decoded.size(); i++) {
    std::cout << dot_product_decoded[i] << " ";
  }

  std::cout << "\n\n";

  // // h=sigmoid (dot_product_decoded)
  std::vector<std::uint64_t> sigmoid_dp(m, 0);
  std::transform(dot_product_decoded.begin(), dot_product_decoded.end(), sigmoid_dp.begin(),
                 [frac_bits](auto j) { return sigmoid(j, frac_bits); });

  std::cout << "sigmoid_dp: ";
  for (int i = 0; i < sigmoid_dp.size(); i++) {
    std::cout << sigmoid_dp[i] << " ";
  }

  std::cout << "\n\n";

  // // H : = h-Y
  std::vector<float> actual_label(m, 0);
  actual_label[0] = 2;
  actual_label[1] = 1;

  std::vector<std::uint64_t> encoded_actual_label(m, 0);
  std::transform(actual_label.begin(), actual_label.end(), encoded_actual_label.begin(),
                 [frac_bits](auto j) {
                   return MOTION::new_fixed_point::encode<std::uint64_t, float>(j, frac_bits);
                 });

  std::cout << "encoded_actual_label: ";
  for (int i = 0; i < encoded_actual_label.size(); i++) {
    std::cout << encoded_actual_label[i] << " ";
  }

  std::cout << "\n\n";

  std::vector<std::uint64_t> temp(m, 0);
  std::transform(sigmoid_dp.begin(), sigmoid_dp.end(), encoded_actual_label.begin(), temp.begin(),
                 std::minus{});
  std::cout << "temp: ";
  for (int i = 0; i < temp.size(); i++) {
    std::cout << temp[i] << " ";
  }

  std::cout << "\n\n";

  // // X.H
  std::vector<std::uint64_t> loss_function(size_single_input, 0);
  std::vector<std::uint64_t> loss_function_decoded(size_single_input, 0);
  // loss_function = MOTION::matrix_multiply(size_single_input, m, 1, encoded_input, temp);

  for (int i = 0; i < size_single_input; i++) {
    for (int j = 0; j < m; j++) {
      auto store = (1 - j) * encoded_input[i] + j * encoded_input[i + size_single_input];
      loss_function[i] += store * temp[j];
    }
  }

  std::transform(loss_function.begin(), loss_function.end(), loss_function_decoded.begin(),
                 [frac_bits](auto j) {
                   return MOTION::new_fixed_point::decode<std::uint64_t, float>(j, frac_bits);
                 });

  std::cout << "loss function: ";
  for (int i = 0; i < loss_function_decoded.size(); i++) {
    std::cout << loss_function_decoded[i] << " ";
  }

  std::cout << "\n\n";

  // // loss_function = (alpha/m)*X.H
  std::uint64_t rate = MOTION::new_fixed_point::encode<std::uint64_t, float>(1, 13);

  std::cout << "rate: " << rate << "\n";

  std::transform(loss_function_decoded.begin(), loss_function_decoded.end(),
                 loss_function_decoded.begin(), [rate, m](auto j) { return rate * j; });

  std::transform(loss_function_decoded.begin(), loss_function_decoded.end(),
                 loss_function_decoded.begin(), [frac_bits](auto j) {
                   return MOTION::new_fixed_point::decode<std::uint64_t, float>(j, frac_bits);
                 });

  std::cout << "loss function2: ";
  for (int i = 0; i < loss_function_decoded.size(); i++) {
    std::cout << loss_function_decoded[i] << " ";
  }
  std::cout << "\n\n";

  // //Theta_new=Theta_current - loss_function
  std::vector<std::uint64_t> theta_new(size_single_input, (uint64_t)0);
  std::transform(theta_current.begin(), theta_current.end(), loss_function_decoded.begin(),
                 theta_new.begin(), std::minus{});

  std::cout << "theta_new: ";
  for (int i = 0; i < theta_new.size(); i++) {
    std::cout << theta_new[i] << " ";
  }
  std::cout << "\n\n";

  std::vector<float> decoded_theta_new(size_single_input, (uint64_t)0);
  std::transform(theta_new.begin(), theta_new.end(), decoded_theta_new.begin(),
                 [frac_bits](auto j) {
                   return MOTION::new_fixed_point::decode<std::uint64_t, float>(j, frac_bits);
                 });

  for (int i = 0; i < decoded_theta_new.size(); i++) {
    std::cout << decoded_theta_new[i] << " ";
  }
  std::cout << "\n\n";

  return decoded_theta_new;
}

int main() {
  std::vector<float> input;
  int m = 2;
  read_input(input, m);

  // find_weights(input, m, 13);
  std::vector<float> decoded_theta_new = find_weights(input, m, 13);
}