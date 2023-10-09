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
#include <random>

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

void generate_random_numbers(std::vector<int>* sample_file){
    std::random_device rd;  // Seed for the random number engine
    std::mt19937 gen(rd()); // Mersenne Twister random number generator

    int minNumber = 1;
    int maxNumber = 5000;
    int numberOfRandomNumbers = 8;

    std::uniform_int_distribution<int> distribution(minNumber, maxNumber);

    for (int i = 0; i < numberOfRandomNumbers; ++i) {
        int randomNumber = distribution(gen);
        std::cout << randomNumber << " ";
        sample_file->push_back(randomNumber);
    }
}

bool is_empty(std::ifstream& file) { return file.peek() == std::ifstream::traits_type::eof(); }

void read_input(std::vector<float>& row_major_input,std::vector<float>& column_major_input,const Options& options) {
  std::string home_dir = getenv("BASE_DIR");
  
    std::vector<int>sample_file;
    for(int i=0;i<2;i++)
    {
      generate_random_numbers(&sample_file);
    }

  std::ifstream file;


  std::string path=home_dir+"/data/ImageProvider/images/";
  std::string input_path;

  for (int i = 0; i < options.m; i++) {
    
    //assumption first half of m is label 1 
    //2nd half of m is label 0 
    if(i<(options.m/2))
    {
     input_path = path + "images_folder1/X" + std::to_string(sample_file[i]) + ".csv";
    } 
    else{
     input_path = path + "images_folder0/X" + std::to_string(sample_file[i]) + ".csv";
    }

    std::cout << input_path << "\n";
    file.open(input_path);

    std::string str;

    while (std::getline(file, str)) {
      std::stringstream obj(str);

      std::string temp;

      while (std::getline(obj, temp, ',')) {
        auto input = std::stof(temp);
        // std::cout << input << " ";
       row_major_input.push_back(input);
      }
    }
    // std::cout << "\n";

    file.close();
    input_path = "";
  }
  int size_single_input = row_major_input.size() / options.m;
  std::cout << size_single_input << "\n";
    
    // std::cout<<"Transpose input :  \n";
    for (int i = 0; i <size_single_input; ++i) {
    for (int j = 0; j < options.m; j++) {
     // std::cout << i <<" " << j << "   ";
     auto data = row_major_input[j*size_single_input+i];
    //  std::cout<<data<<"";
     column_major_input.push_back(data);

    }
    }
  std::cout << "\n";
}

uint64_t sigmoid(uint64_t dot_product, int frac_bits) {
  auto encoded_threshold = MOTION::new_fixed_point::encode<std::uint64_t, float>(0.5, frac_bits);
  auto neg_encoded_threshold =
      MOTION::new_fixed_point::encode<std::uint64_t, float>(-0.5, frac_bits);

  // std::cout << "dot_product: " << std::bitset<sizeof(dot_product) * 8>(dot_product) << "\n";
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

std::vector<std::uint64_t> find_weights(std::vector<float> row_major_input,std::vector<float>column_major_input,std::vector<std::uint64_t>theta_current,Options& options) {
  // // input->encoded_input

  int frac_bits=options.frac_bits;
  int m = options.m;

  //#############################################################################################################

  std::vector<std::uint64_t> encoded_input(row_major_input.size(), 0);

  std::transform(row_major_input.begin(),row_major_input.end(), encoded_input.begin(), [frac_bits](auto j) {
    return MOTION::new_fixed_point::encode<std::uint64_t, float>(j, frac_bits);
  });


  std::vector<std::uint64_t> encoded_column_major_input(column_major_input.size(), 0);

  std::transform(column_major_input.begin(), column_major_input.end(), encoded_column_major_input.begin(), [frac_bits](auto j) {
    return MOTION::new_fixed_point::encode<std::uint64_t, float>(j, frac_bits);
  });


  std::cout << "transpose input after decode: ";
  for (int i = 0; i < encoded_column_major_input.size(); i++) {
    std::cout << MOTION::new_fixed_point::decode<std::uint64_t, float>(encoded_column_major_input[i],
                                                                       frac_bits)
              << " ";
  }

    std::cout << "\n\n";


  // // dot_product_decoded = Theta * encoded_column_major_input
  int size_single_input = row_major_input.size() / m;
  
  // // redundant now
  // auto theta_my = MOTION::new_fixed_point::encode<std::uint64_t, float>(0.2, frac_bits);

  // std::vector<std::uint64_t> theta_current(size_single_input, theta_my);

  std::cout << "theta_current: \n";
  for (int i = 0; i < theta_current.size(); i++) {
    std::cout << MOTION::new_fixed_point::decode<std::uint64_t, float>(theta_current[i],
                                                                       frac_bits)
              << ",";
  }

  std::cout << "\n\n";

  std::vector<std::uint64_t> dot_product(m, 0);
  std::vector<std::uint64_t> dot_product_decoded(m, 0);

  dot_product = MOTION::matrix_multiply(1, size_single_input, m, theta_current, encoded_column_major_input);

  std::transform(dot_product.begin(), dot_product.end(), dot_product_decoded.begin(),
                 [frac_bits](auto j) {
                   return MOTION::new_fixed_point::decode<std::uint64_t, float>(j, frac_bits);
                 });

  // std::cout << "dot_product: ";
  // for (int i = 0; i < dot_product_decoded.size(); i++) {
  //   std::cout << MOTION::new_fixed_point::decode<std::uint64_t, float>(dot_product_decoded[i],
  //                                                                      frac_bits)
  //             << " ";
  // }

  // std::cout << "\n\n";

  //###################################################################################################################

  // h=sigmoid (dot_product_decoded)
  std::vector<std::uint64_t> sigmoid_dp(m, 0);
  std::transform(dot_product_decoded.begin(), dot_product_decoded.end(), sigmoid_dp.begin(),
                 [frac_bits](auto j) { return sigmoid(j, frac_bits); });

  // std::cout << "sigmoid_dp: ";
  // for (int i = 0; i < sigmoid_dp.size(); i++) {
  //   std::cout << MOTION::new_fixed_point::decode<std::uint64_t, float>(sigmoid_dp[i],
  //                                                                      frac_bits) << " ";
  // }

  // std::cout << "\n\n";

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

  // std::cout << "encoded_actual_label: ";
  // for (int i = 0; i < encoded_actual_label.size(); i++) {
  //   std::cout << encoded_actual_label[i] << " ";
  // }

  // std::cout << "\n\n";
  std::vector<std::uint64_t> temp(m, 0);
  std::transform(sigmoid_dp.begin(), sigmoid_dp.end(), encoded_actual_label.begin(), temp.begin(),
                 std::minus{});

  // std::cout << "temp: ";
  // for (int i = 0; i < temp.size(); i++) {
  //   std::cout << MOTION::new_fixed_point::decode<std::uint64_t, float>(temp[i], frac_bits) << " ";
  // }

  // std::cout << "\n\n";

  //##################################################################################################################################

  // // // H.X == 1* single input size
  std::vector<std::uint64_t> loss_function(size_single_input, 0);
  std::vector<std::uint64_t> loss_function_decoded(size_single_input, 0);
  loss_function = MOTION::matrix_multiply(1, m, size_single_input, temp, encoded_input);

  std::transform(loss_function.begin(), loss_function.end(), loss_function_decoded.begin(),
                 [frac_bits](auto j) {
                   return MOTION::new_fixed_point::decode<std::uint64_t, float>(j, frac_bits);
                 });

  // std::cout << "loss function: ";
  // for (int i = 0; i < loss_function_decoded.size(); i++) {
  //   std::cout << MOTION::new_fixed_point::decode<std::uint64_t, float>(loss_function_decoded[i],
  //                                                                      frac_bits)
  //             << " ";
  // }

  // std::cout << "\n\n";

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

  // std::cout << "loss function2: ";
  // for (int i = 0; i < loss_function_decoded.size(); i++) {
  //   std::cout << MOTION::new_fixed_point::decode<std::uint64_t, float>(loss_function_decoded[i],
  //                                                                      frac_bits)
  //             << " ";
  // }
  // std::cout << "\n\n";

  //#######################################################################################################################################

  // //Theta_new=Theta_current - loss_function
  std::vector<std::uint64_t> theta_new(size_single_input, (uint64_t)0);
  std::transform(theta_current.begin(), theta_current.end(), loss_function_decoded.begin(),
                 theta_new.begin(), std::minus{});

  // std::cout << "theta_new: ";
  // for (int i = 0; i < theta_new.size(); i++) {
  //   std::cout << theta_new[i] << " ";
  // }
  // std::cout << "\n\n";

  std::ofstream out_file;
  path = home_dir+"/build_debwithrelinfo_gcc/Theta_new";

  out_file.open(path,std::ios_base::app);

  std::vector<float> decoded_theta_new(size_single_input, (uint64_t)0);
  std::transform(theta_new.begin(), theta_new.end(), decoded_theta_new.begin(),
                 [frac_bits](auto& j) {
                   return MOTION::new_fixed_point::decode<std::uint64_t, float>(j, frac_bits);
                 });
  
  std::cout<<"Decoded theta new : \n\n";
  for (int i = 0; i < decoded_theta_new.size(); i++) {
    //std::cout << decoded_theta_new[i] << ",";
    out_file<<decoded_theta_new[i]<<",";
  }

  out_file<<"\n\n";

  out_file.close();
  
  return theta_new;
}

int main(int argc, char* argv[]) {
  
  auto options = parse_program_options(argc, argv);
  std::vector<float> row_major_input, column_major_input , inference_input;
  int size_single_input,column_major_size;
  int frac_bits=options->frac_bits;
  
  //input is row_major_input

  std::vector<std::uint64_t>theta_current;

  for(int i=0;i<1000;i++)
  {   
    read_input(row_major_input,column_major_input,*options);
    size_single_input=row_major_input.size()/options->m;

    if(i==0)
    { 
    std::random_device rd;  // Seed for the random number engine
    std::mt19937 gen(rd()); // Mersenne Twister random number generator

    std::uint64_t minNumber = 0;
    std::uint64_t maxNumber = 8192;

    std::uniform_int_distribution<std::uint64_t> distribution(minNumber, maxNumber);
    theta_current.assign(size_single_input, 0);

    for(int i=0;i<size_single_input;i++)
    {   
      int x = distribution(gen);
      theta_current[i]=x;
    }
    } 

    std::cout<< "Size of row_major_inputs : "<< row_major_input.size()<<"\n";
    std::cout<< "Size of column major input : "<< column_major_input.size()<<"\n";
    
    // // To inference the model , Theta(1*size)  Input(size*m) --> 
    inference_input.assign(column_major_input.size(),0);
    for(int i=0;i<column_major_input.size();i++)
    {
      inference_input[i]=column_major_input[i];
    }

    std::cout<<"\n\n";
    std::vector<std::uint64_t> encoded_theta_new = find_weights(row_major_input,column_major_input,theta_current,*options);

    std::cout<<"\n\n ---- next iteration ---- \n\n ";


    //put theta_current = encoded_theta_new
    for(int i=0;i<encoded_theta_new.size();i++)
    {
       theta_current[i]=encoded_theta_new[i];
    }
    std::cout<<"\n";
    std::cout<<"Size of theta current : "<<theta_current.size()<<"\n";

    row_major_input.clear();
    column_major_input.clear();
  }

  std::cout<<"inference_input size : "<< inference_input.size() <<"\n";
  // // To inference the model , Theta(1*size) * Input(size*m) --> 1 * m 
  std::vector<std::uint64_t> encoded_inference_input(inference_input.size(), (uint64_t)0);
  std::transform(inference_input.begin(), inference_input.end(), encoded_inference_input.begin(),
                 [frac_bits](auto& j) {
                   return MOTION::new_fixed_point::encode<std::uint64_t, float>(j, frac_bits);
                 });

  auto output = MOTION::matrix_multiply(1,size_single_input,options->m,theta_current,encoded_inference_input);
  std::transform(output.begin(),output.end(), output.begin(),
                 [frac_bits](auto& j) {
                   return MOTION::new_fixed_point::decode<std::uint64_t, float>(j, frac_bits);
                 });
  
  for(int i=0;i<output.size();i++)
  {
    auto temp= MOTION::new_fixed_point::decode<std::uint64_t, float>(output[i], frac_bits);
    if(temp>0)
    std::cout<<"1 ";
    else
    std::cout<<"0 ";
  }
  std::cout<<"\n";
   
return EXIT_SUCCESS;
}