#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <regex>
#include <stdexcept>

#include <boost/algorithm/string.hpp>
#include <boost/asio.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/program_options.hpp>

using namespace boost::asio;
using ip::tcp;
using std::cout;
using std::endl;
using std::string;
namespace po = boost::program_options;
namespace fs = std::filesystem;
struct Shares {
  bool Delta, delta;
};
struct Options {
  std::string currentpath;
};

std::optional<Options> parse_program_options(int argc, char* argv[]) {
  Options options;
  boost::program_options::options_description desc("Allowed options");
  // clang-format off
    desc.add_options()
    ("help,h", po::bool_switch()->default_value(false),"produce help message")
     ("current-path",po::value<std::string>()->required(), "current path build_debwithrelinfo")
     
    ;
  // clang-format on

  po::variables_map vm;
  po::store(po::parse_command_line(argc, argv, desc), vm);
  bool help = vm["help"].as<bool>();
  if (help) {
    std::cerr << desc << "\n";
    return std::nullopt;
  }

  options.currentpath = vm["current-path"].as<std::string>();

  return options;
}

int main(int argc, char* argv[]) {
  auto options = parse_program_options(argc, argv);
  std::string path = options->currentpath;
  try {
    // Reading contents from file
    std::string t1 = path + "/server0_shares_X";
    std::string t2 = path + "/server1_shares_X";

    std::ifstream file1, file2;

    try {
      file1.open(t1);
      file2.open(t2);
      if (file1 && file2) {
        std::cout << " ";
      }
    } catch (std::ifstream::failure e) {
      std::cerr << "Error while opening the input files.\n";
      return EXIT_FAILURE;
    }
    Shares shares_data_0[10], shares_data_1[10];
    // get input data
    // std::vector<float> data1

    std::vector<bool> final_answer;
    for (int i = 0; i < 10; i++) {
      file1 >> shares_data_0[i].Delta;
      file1 >> shares_data_0[i].delta;
      file2 >> shares_data_1[i].Delta;
      file2 >> shares_data_1[i].delta;
      try {
        if (shares_data_0[i].Delta != shares_data_1[i].Delta)
          std::cout << "Error at " << i << " index \n";
      } catch (std::ifstream::failure e) {
        std::cerr << "Incorrect output shares.\n";
        return EXIT_FAILURE;
      }
      bool temp = shares_data_0[i].Delta ^ shares_data_0[i].delta ^ shares_data_1[i].delta;
      // std::cout << temp <<" ";
      final_answer.push_back(temp);
    }
    int i = 0;
    while (final_answer[i] != 0) {
      i++;
    }
    std::cout << "\nThe image shared is detected as"
              << ":" << i << "\n";
  } catch (std::runtime_error& e) {
    std::cerr << "ERROR OCCURRED: " << e.what() << "\n";
    std::cerr << "ERROR Caught !!"
              << "\n";
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
