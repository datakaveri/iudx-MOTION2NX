// ./bin/machine_learning --sample-file sample_file --actual-label-file actual_label
// --fractional-bits 13 --sample-size 20 --classes 10
//1000 - 80, 10000 - 91, 15000 - 94
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
#include <random>

namespace po = boost::program_options;
using namespace std;

struct Options {
  std::size_t m;
  std::string actual_label_file;
  std::string sample_file;
  std::size_t frac_bits;
  std::size_t classes;
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
    ("classes", po::value<std::size_t>()->required(), "classes")
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
  options.classes = vm["classes"].as<std::size_t>();
  return options;
}

void generate_random_numbers(std::vector<int>* sample_file, const Options& options){
    std::random_device rd;  // Seed for the random number engine
    std::mt19937 gen(rd()); // Mersenne Twister random number generator

    int minNumber = 1;
    int maxNumber = 5000;
    int numberOfRandomNumbers = options.m / options.classes;
    
    std::uniform_int_distribution<int> distribution(minNumber, maxNumber);

    for(int i = 0; i < numberOfRandomNumbers; ++i) {
      int randomNumber = distribution(gen);
      sample_file->push_back(randomNumber);
    }
}

bool is_empty(std::ifstream& file) { return file.peek() == std::ifstream::traits_type::eof(); }

void read_input(std::vector<float>& row_major_input,std::vector<float>& column_major_input,const Options& options) {
  std::string home_dir = getenv("BASE_DIR");
  
  std::vector<int>sample_file;
  for(int i = 0; i < options.classes; i++)
  {
    generate_random_numbers(&sample_file, options);
  }

  std::ifstream file;
  std::string path = home_dir + "/data/ImageProvider/sample_data/";
  std::string input_path;

  for (int i = 0; i < options.m; i++) {
    input_path = path + "images_folder" + std::to_string((int) (i / (options.m / options.classes))) + "/X" + std::to_string(sample_file[i]) + ".csv";

    file.open(input_path);
    std::string str;
    while (std::getline(file, str)) {
      std::stringstream obj(str);
      std::string temp;
      while (std::getline(obj, temp, ',')) {
        auto input = std::stof(temp);
        row_major_input.push_back(input);
      }
    }
    file.close();
  }

  int size_single_input = row_major_input.size() / options.m;

  for (int i = 0; i < size_single_input; ++i) {
    for (int j = 0; j < options.m; j++) {
      auto data = row_major_input[j * size_single_input + i];
    column_major_input.push_back(data);
    }
  }
}

uint64_t sigmoid(uint64_t dot_product, int frac_bits) {
  auto encoded_threshold = MOTION::new_fixed_point::encode<std::uint64_t, float>(0.5, frac_bits);
  auto neg_encoded_threshold =
      MOTION::new_fixed_point::encode<std::uint64_t, float>(-0.5, frac_bits);

  std::uint64_t msb = (uint64_t)1 << 63;

  if (dot_product & msb) {
    if (dot_product < neg_encoded_threshold || dot_product == neg_encoded_threshold) {
      return 0;
    } else {
      return dot_product + encoded_threshold;
    }
  } else {
    if (dot_product > encoded_threshold || dot_product == encoded_threshold) {
      return MOTION::new_fixed_point::encode<std::uint64_t, float>(1, frac_bits);
    } else {
      return dot_product + encoded_threshold;
    }
  }
}

std::vector<std::uint64_t> find_weights(std::vector<float> row_major_input,std::vector<float>column_major_input,std::vector<std::uint64_t>theta_current,Options& options, bool write_to_file) {
  int frac_bits=options.frac_bits;
  int m = options.m;
  int classes = options.classes;

  std::vector<std::uint64_t> encoded_input(row_major_input.size(), 0);
  std::transform(row_major_input.begin(),row_major_input.end(), encoded_input.begin(), [frac_bits](auto j) {
    return MOTION::new_fixed_point::encode<std::uint64_t, float>(j, frac_bits);
  });

  std::vector<std::uint64_t> encoded_column_major_input(column_major_input.size(), 0);
  std::transform(column_major_input.begin(), column_major_input.end(), encoded_column_major_input.begin(), [frac_bits](auto j) {
    return MOTION::new_fixed_point::encode<std::uint64_t, float>(j, frac_bits);
  });

  int size_single_input = row_major_input.size() / m;
  
  std::vector<std::uint64_t> dot_product(m * classes, 0);
  std::vector<std::uint64_t> dot_product_decoded(m * classes, 0);

  dot_product = MOTION::matrix_multiply(classes, size_single_input, m, theta_current, encoded_column_major_input);
  std::transform(dot_product.begin(), dot_product.end(), dot_product_decoded.begin(),
                 [frac_bits](auto j) {
                   return MOTION::new_fixed_point::decode<std::uint64_t, float>(j, frac_bits);
                 });

  std::vector<std::uint64_t> sigmoid_dp(m * classes, 0);
  std::transform(dot_product_decoded.begin(), dot_product_decoded.end(), sigmoid_dp.begin(),
                 [frac_bits](auto j) { return sigmoid(j, frac_bits); });

  
  std::string home_dir=std::getenv("BASE_DIR");
  
  std::string path =  home_dir+"/config_files/"+options.actual_label_file;
  std::ifstream file;

  file.open(path);
  std::vector<float> actual_label;
  while(!file.eof())
  {
    float data;
    file>>data;
    actual_label.push_back(data);
  }
  file.close();

  std::vector<float> actual_label_transpose;
  for (int i = 0; i < classes; ++i) {
    for (int j = 0; j < m; j++) {
      auto data = actual_label[j * classes + i];
    actual_label_transpose.push_back(data);
    }
  }

  std::vector<std::uint64_t> encoded_actual_label_transpose(m * classes, 0);
  std::transform(actual_label_transpose.begin(), actual_label_transpose.end(), encoded_actual_label_transpose.begin(),
                 [frac_bits](auto j) {
                   return MOTION::new_fixed_point::encode<std::uint64_t, float>(j, frac_bits);
                 });

  std::vector<std::uint64_t> temp(m * classes, 0);
  std::transform(sigmoid_dp.begin(), sigmoid_dp.end(), encoded_actual_label_transpose.begin(), temp.begin(),
                 std::minus{});

  std::vector<std::uint64_t> loss_function(size_single_input * classes, 0);
  std::vector<std::uint64_t> loss_function_decoded(size_single_input * classes, 0);
  loss_function = MOTION::matrix_multiply(classes, m, size_single_input, temp, encoded_input);

  std::transform(loss_function.begin(), loss_function.end(), loss_function_decoded.begin(),
                 [frac_bits](auto j) {
                   return MOTION::new_fixed_point::decode<std::uint64_t, float>(j, frac_bits);
                 });

  std::uint64_t rate = MOTION::new_fixed_point::encode<std::uint64_t, float>(0.005, 13);

  std::transform(loss_function_decoded.begin(), loss_function_decoded.end(),
                 loss_function_decoded.begin(), [rate, m](auto j) { return rate * j; });

  std::transform(loss_function_decoded.begin(), loss_function_decoded.end(),
                 loss_function_decoded.begin(), [frac_bits](auto j) {
                   return MOTION::new_fixed_point::decode<std::uint64_t, float>(j, frac_bits);
                 });

  std::vector<std::uint64_t> theta_new(size_single_input * classes, (uint64_t)0);
  std::transform(theta_current.begin(), theta_current.end(), loss_function_decoded.begin(),
                 theta_new.begin(), std::minus{});

  if(write_to_file) {
    std::ofstream out_file;
    path = home_dir+"/build_debwithrelinfo_gcc/Theta_new";

    out_file.open(path,std::ios_base::app);

    std::vector<float> decoded_theta_new(size_single_input * classes, (uint64_t)0);
    std::transform(theta_new.begin(), theta_new.end(), decoded_theta_new.begin(),
                  [frac_bits](auto& j) {
                    return MOTION::new_fixed_point::decode<std::uint64_t, float>(j, frac_bits);
                  });
    
    for (int i = 0; i < decoded_theta_new.size(); i++) {
      out_file<<decoded_theta_new[i]<<",";
    }

    out_file<<"\n\n";
    out_file.close(); 
  } 
  return theta_new;
}

void get_test_labels(std::vector<float>&test_labels, int test_size) {
  std::string home_dir=std::getenv("BASE_DIR");
  std::string path =  home_dir + "data/ImageProvider/images_actualanswer";
  std::ifstream file;
  file.open(path);
  for(int i = 0; i < test_size; i++) {
    float data;
    file>>data;
    test_labels.push_back(data);
  }
  file.close();
}

void read_test_data(std::vector<float>&test_input, std::vector<float>& test_input_transpose, std::vector<float>&test_labels) {
  std::string home_dir = getenv("BASE_DIR");
  std::ifstream file;
  std::string path = home_dir + "/data/ImageProvider/images_actualanswer";
  std::string input_path;
  int test_size = 100;

  for (int i = 0; i < test_size; i++) {
    input_path = path + "/X" + std::to_string(i) + ".csv";
    file.open(input_path);
    std::string str;
    int line = 0;
    while (std::getline(file, str)) {
      std::stringstream obj(str);
      std::string temp;
      while (std::getline(obj, temp, ',')) {
        if(line == 0) {
          auto input = std::stof(temp);
          test_labels.push_back(input);
        } else {
          auto input = std::stof(temp);
          test_input.push_back(input);
        }
      }
      line++;
    }
    file.close();
  }

  int size_single_input = test_input.size() / test_size;
    
  for (int i = 0; i < size_single_input; ++i) {
    for (int j = 0; j < test_size; j++) {
      auto data = test_input[j * size_single_input+i];
    test_input_transpose.push_back(data);
    }
  }
}

int test_accuracy(std::vector<std::uint64_t>theta, const Options& options) {
  std::cout << "Testing accuracy\n";
  std::vector<float> test_input, test_input_transpose, test_labels;
  read_test_data(test_input, test_input_transpose, test_labels);
  int frac_bits = options.frac_bits;
  int classes = options.classes;
  int test_size = test_labels.size();

  std::vector<std::uint64_t> encoded_test_input(test_input.size(), 0);
  std::transform(test_input.begin(), test_input.end(), encoded_test_input.begin(), [frac_bits](auto j) {
    return MOTION::new_fixed_point::encode<std::uint64_t, float>(j, frac_bits);
  });

  std::vector<std::uint64_t> encoded_test_input_transpose(test_input_transpose.size(), 0);
  std::transform(test_input_transpose.begin(), test_input_transpose.end(), encoded_test_input_transpose.begin(), [frac_bits](auto j) {
    return MOTION::new_fixed_point::encode<std::uint64_t, float>(j, frac_bits);
  });

  int size_single_input = test_input.size() / test_size;
  std::vector<std::uint64_t> dot_product(test_size * classes, 0);
  std::vector<std::uint64_t> dot_product_decoded(test_size * classes, 0);

  dot_product = MOTION::matrix_multiply(classes, size_single_input, test_size, theta, encoded_test_input_transpose);
  std::transform(dot_product.begin(), dot_product.end(), dot_product_decoded.begin(),
                 [frac_bits](auto j) {
                   return MOTION::new_fixed_point::decode<std::uint64_t, float>(j, frac_bits);
                 });

 
  std::vector<std::uint64_t> sigmoid_dp(test_size * classes, 0);
  std::transform(dot_product_decoded.begin(), dot_product_decoded.end(), sigmoid_dp.begin(),
                 [frac_bits](auto j) { return sigmoid(j, frac_bits); });
  
  std::vector<std::uint64_t> sigmoid_dp_transpose;
  for (int i = 0; i < test_size; i++) {
    for (int j = 0; j < classes; j++) {
      auto data = sigmoid_dp[j * test_size + i];
    sigmoid_dp_transpose.push_back(data);
    }
  }

  float max = -1.0;
  int argmax = 0;
  std::vector<int>predictions;
  for(int i = 0; i < sigmoid_dp_transpose.size(); i++) {
    if(i != 0 && i % 10 == 0) {
      predictions.push_back(argmax);
      max = -1;
      argmax = 0;
    }
    if(max < sigmoid_dp_transpose[i]) {
      max = sigmoid_dp_transpose[i];
      argmax = i % 10;
    }
  }
  predictions.push_back(argmax);
  
  std::cout << "Predictions: ";
  for(int i: predictions) {
    std::cout << i << " ";
  }
  std::cout << "\nActual labels: ";
  for(auto i: test_labels) {
    std::cout << i << " ";
  }

  int accuracy = 0;
  for(int i = 0; i < test_size; i++) {
    if(predictions[i] == (int)test_labels[i]) {
      accuracy++;
    }
  }
  return accuracy;
}

int main(int argc, char* argv[]) {
  
  auto options = parse_program_options(argc, argv);
  std::vector<float> row_major_input, column_major_input;

  int frac_bits=options->frac_bits;
  int classes = options -> classes;

  std::vector<std::uint64_t> theta_current;

  int iterations = 20000;
  for(int i=0;i<iterations;i++) { 
    cout << i << endl;  
    read_input(row_major_input,column_major_input,*options);
    int size_single_input=row_major_input.size()/options->m;

    if(i==0) {
      std::random_device rd;  // Seed for the random number engine
      std::mt19937 gen(rd()); // Mersenne Twister random number generator

      std::uint64_t minNumber = 0;
      std::uint64_t maxNumber = 8192;

      std::uniform_int_distribution<std::uint64_t> distribution(minNumber, maxNumber);
      theta_current.assign(size_single_input * classes, 0);

      for(int i=0; i<size_single_input * classes; i++) {   
        int x = distribution(gen);
        theta_current[i]=x;
      }
    } 

    std::vector<std::uint64_t> encoded_theta_new;
    if(i == iterations - 1) {
      encoded_theta_new = find_weights(row_major_input,column_major_input,theta_current,*options, true);
    } else {
      encoded_theta_new = find_weights(row_major_input,column_major_input,theta_current,*options, false);
    }

    for(int i=0;i<encoded_theta_new.size();i++)
    {
       theta_current[i]=encoded_theta_new[i];
    }

    row_major_input.clear();
    column_major_input.clear();
  }
  std::vector<float>theta;

  std::string home_dir=std::getenv("BASE_DIR");
  std::string path =  home_dir + "/build_debwithrelinfo_gcc/Theta_new";
  std::ifstream file;

  file.open(path);
  std::string str;
  int line = 0;

  if(std::getline(file, str)) {
    std::stringstream obj(str);
    std::string temp;
    while(std::getline(obj, temp, ',')) {
      auto input = std::stof(temp);
      theta.push_back(input);
    }
    line++;
  }
  file.close();

  std::vector<std::uint64_t> encoded_theta(theta.size());
  std::transform(theta.begin(), theta.end(), encoded_theta.begin(), [frac_bits](auto j) {
    return MOTION::new_fixed_point::encode<std::uint64_t, float>(j, frac_bits);
  });
  
  int accuracy = test_accuracy(theta_current, *options);
  std::cout << "\nAccuracy: " << accuracy << "\n";
  return EXIT_SUCCESS;
}