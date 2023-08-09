// ./bin/machine_learning --sample-file sample_file --actual-label-file actual_label
// --fractional-bits 13 --sample-size 8

#include <algorithm>
#include <bitset>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>

#include <utility/new_fixed_point.h>
#include "utility/linear_algebra.h"

#include <boost/program_options.hpp>
#include <boost/thread/thread.hpp>

namespace po = boost::program_options;

struct Options {
  std::size_t m;
  std::string actual_label_file;
  std::string sample_file;
  std::size_t frac_bits;
};

std::optional<Options> parse_program_options(int argc, char* argv[]) {
  Options options;
  boost::program_options::options_description desc("Allowed options");
  // clang-format off
  desc.add_options()
    ("help,h", po::bool_switch()->default_value(false),"produce help message")
    ("sample-file", po::value<std::string>()->required(), "csv file containing sample file names")
    ("sample-size", po::value<std::size_t>()->required(), "sample size")
    ("actual-label-file", po::value<std::string>()->required(), "csv file containing actual labels")
    ("fractional-bits", po::value<std::size_t>()->required(), "fractional bits")
;

  po::variables_map vm;
  po::store(po::parse_command_line(argc, argv, desc), vm);
  bool help = vm["help"].as<bool>();
  if (help) {
    std::cerr << desc << "\n";
    return std::nullopt;
  }
  if (vm.count("config-file")) {
    std::ifstream ifs(vm["config-file"].as<std::string>().c_str());
    po::store(po::parse_config_file(ifs, desc), vm);
  }
  try {
    po::notify(vm);
  } catch (std::exception& e) {
    std::cerr << "error:" << e.what() << "\n\n";
    std::cerr << desc << "\n";
    return std::nullopt;
  }

  options.m = vm["sample-size"].as<std::size_t>();
  options.sample_file = vm["sample-file"].as<std::string>();
  options.actual_label_file = vm["actual-label-file"].as<std::string>();
  options.frac_bits = vm["fractional-bits"].as<std::size_t>();
   
  return options;
}

bool is_empty(std::ifstream& file) { return file.peek() == std::ifstream::traits_type::eof(); }

void read_input(std::vector<float>& x_input,std::vector<float>& transpose_input,const Options& options) {
  std::string home_dir = getenv("BASE_DIR");

  std::string path_sample_files = home_dir + "/config_files/"+options.sample_file;
  std::cout<<path_sample_files<<"\n"; 

  std::ifstream file;
  file.open(path_sample_files);

  std::vector<int>samplefile;

  while(!file.eof())
  {
    int data;
    file>>data;
    std::cout<<data<<" ";
    samplefile.push_back(data);
  }

  std::cout<<"\n";
  file.close();

  std::string path=home_dir+"/data/ImageProvider/images/";

  for (int i = 0; i < options.m; i++) {
    std::string input_path = path + "X" + std::to_string(samplefile[i]) + ".csv";

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
  }
  int size_single_input = x_input.size() / options.m;
  std::cout << size_single_input << "\n";
    
    std::cout<<"Transpose input :  \n";
    for (int i = 0; i <size_single_input; ++i) {
    for (int j = 0; j < options.m; j++) {
     // std::cout << i <<" " << j << "   ";
     auto data = x_input[j*size_single_input+i];
     std::cout<<data<<"";
     transpose_input.push_back(data);

    }
  }

  std::cout << "\n";
}

uint64_t sigmoid(uint64_t dot_product, int frac_bits) {
  auto encoded_threshold = MOTION::new_fixed_point::encode<std::uint64_t, float>(0.5, frac_bits);
  auto neg_encoded_threshold =
      MOTION::new_fixed_point::encode<std::uint64_t, float>(-0.5, frac_bits);

  std::cout << "dot_product: " << std::bitset<sizeof(dot_product) * 8>(dot_product) << "\n";
  std::uint64_t msb = (uint64_t)1 << 63;
  // std::cout << "msb: " << std::bitset<sizeof(msb) * 8>(msb) << "\n";

  if (dot_product & msb) {
    std::cout << "line 66\n";
    if (dot_product < neg_encoded_threshold || dot_product == neg_encoded_threshold) {
      std::cout << "0 line 68\n";
      return 0;
    } else {
      std::cout << " line 71\n";
      return dot_product + encoded_threshold;
    }
  } else {
    if (dot_product > encoded_threshold || dot_product == encoded_threshold) {
      std::cout << "1 line 76\n";
      return MOTION::new_fixed_point::encode<std::uint64_t, float>(1, frac_bits);
    } else {
      std::cout << "line 79\n";
      return dot_product + encoded_threshold;
    }
  }
}

std::vector<float> find_weights(std::vector<float> input,std::vector<float>transpose_input,Options& options) {
  // // input->encoded_input

  int frac_bits=options.frac_bits;
  int m = options.m;

  //#############################################################################################################

  std::vector<std::uint64_t> encoded_input(input.size(), 0);

  std::transform(input.begin(), input.end(), encoded_input.begin(), [frac_bits](auto j) {
    return MOTION::new_fixed_point::encode<std::uint64_t, float>(j, frac_bits);
  });


  std::vector<std::uint64_t> encoded_transpose_input(transpose_input.size(), 0);

  std::transform(transpose_input.begin(), transpose_input.end(), encoded_transpose_input.begin(), [frac_bits](auto j) {
    return MOTION::new_fixed_point::encode<std::uint64_t, float>(j, frac_bits);
  });


  std::cout << "transpose input after decode: ";
  for (int i = 0; i < encoded_transpose_input.size(); i++) {
    std::cout << MOTION::new_fixed_point::decode<std::uint64_t, float>(encoded_transpose_input[i],
                                                                       frac_bits)
              << " ";
  }


  // // dot_product_decoded = Theta * encoded_input
  int size_single_input = input.size() / m;

  auto theta_my = MOTION::new_fixed_point::encode<std::uint64_t, float>(0.2, frac_bits);

  std::vector<std::uint64_t> theta_current(size_single_input, theta_my);

  std::cout << "theta_current: ";
  for (int i = 0; i < theta_current.size(); i++) {
    std::cout << MOTION::new_fixed_point::decode<std::uint64_t, float>(theta_current[i],
                                                                       frac_bits)
              << " ";
  }

  std::cout << "\n\n";

  std::vector<std::uint64_t> dot_product(m, 0);
  std::vector<std::uint64_t> dot_product_decoded(m, 0);

  dot_product = MOTION::matrix_multiply(1, size_single_input, m, theta_current, encoded_transpose_input);

  std::transform(dot_product.begin(), dot_product.end(), dot_product_decoded.begin(),
                 [frac_bits](auto j) {
                   return MOTION::new_fixed_point::decode<std::uint64_t, float>(j, frac_bits);
                 });

  std::cout << "dot_product: ";
  for (int i = 0; i < dot_product_decoded.size(); i++) {
    std::cout << MOTION::new_fixed_point::decode<std::uint64_t, float>(dot_product_decoded[i],
                                                                       frac_bits)
              << " ";
  }

  std::cout << "\n\n";

  //###################################################################################################################

  // h=sigmoid (dot_product_decoded)
  std::vector<std::uint64_t> sigmoid_dp(m, 0);
  std::transform(dot_product_decoded.begin(), dot_product_decoded.end(), sigmoid_dp.begin(),
                 [frac_bits](auto j) { return sigmoid(j, frac_bits); });

  std::cout << "sigmoid_dp: ";
  for (int i = 0; i < sigmoid_dp.size(); i++) {
    std::cout << MOTION::new_fixed_point::decode<std::uint64_t, float>(sigmoid_dp[i],
                                                                       frac_bits) << " ";
  }

  std::cout << "\n\n";

  //########################################################################################################################
  
  std::string home_dir=std::getenv("BASE_DIR");
  
  std::string path =  home_dir+"/config_files/"+options.actual_label_file;
  std::ifstream file;

  file.open(path);
  std::vector<float> actual_label;
  while(!file.eof())
  {
    float data;
    file>>data;
    std::cout<<data<<" ";
    actual_label.push_back(data);
  }

  file.close();
  std::cout << "\n";

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
    std::cout << MOTION::new_fixed_point::decode<std::uint64_t, float>(temp[i], frac_bits) << " ";
  }

  std::cout << "\n\n";

  //##################################################################################################################################

  // // // X.H == 1* single input size
  std::vector<std::uint64_t> loss_function(size_single_input, 0);
  std::vector<std::uint64_t> loss_function_decoded(size_single_input, 0);
  loss_function = MOTION::matrix_multiply(1, m, size_single_input, temp, encoded_input);

  std::transform(loss_function.begin(), loss_function.end(), loss_function_decoded.begin(),
                 [frac_bits](auto j) {
                   return MOTION::new_fixed_point::decode<std::uint64_t, float>(j, frac_bits);
                 });

  std::cout << "loss function: ";
  for (int i = 0; i < loss_function_decoded.size(); i++) {
    std::cout << MOTION::new_fixed_point::decode<std::uint64_t, float>(loss_function_decoded[i],
                                                                       frac_bits)
              << " ";
  }

  std::cout << "\n\n";

  //#####################################################################################################################################

  // // loss_function = (alpha/m)*X.H
  std::uint64_t rate = MOTION::new_fixed_point::encode<std::uint64_t, float>(0.005, 13);

  std::cout << "rate: " << rate << "\n";

  std::transform(loss_function_decoded.begin(), loss_function_decoded.end(),
                 loss_function_decoded.begin(), [rate, m](auto j) { return rate * j; });

  std::transform(loss_function_decoded.begin(), loss_function_decoded.end(),
                 loss_function_decoded.begin(), [frac_bits](auto j) {
                   return MOTION::new_fixed_point::decode<std::uint64_t, float>(j, frac_bits);
                 });

  std::cout << "loss function2: ";
  for (int i = 0; i < loss_function_decoded.size(); i++) {
    std::cout << MOTION::new_fixed_point::decode<std::uint64_t, float>(loss_function_decoded[i],
                                                                       frac_bits)
              << " ";
  }
  std::cout << "\n\n";

  //#######################################################################################################################################

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
                 [frac_bits](auto& j) {
                   return MOTION::new_fixed_point::decode<std::uint64_t, float>(j, frac_bits);
                 });

  for (int i = 0; i < decoded_theta_new.size(); i++) {
    std::cout << decoded_theta_new[i] << ",";
  }
  std::cout << "\n\n";
  
  return decoded_theta_new;
}

int main(int argc, char* argv[]) {
  
  auto options = parse_program_options(argc, argv);
  std::vector<float> input, transpose_input;

  read_input(input,transpose_input,*options);
  std::vector<float> decoded_theta_new = find_weights(input,transpose_input,*options);
}