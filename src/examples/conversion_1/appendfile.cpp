#include <stdlib.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>

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

int main(int argc, char* argv[]) {
  int id = atoi(argv[1]);
  if (id % 2 == id) {
    std::ofstream indata1;

    std::string base_dir = getenv("BASE_DIR");
    std::string build_dir = base_dir + "/build_debwithrelinfo_gcc";
    std::string server_dir = build_dir + "/server" + std::to_string(id);
    
    std::string dest_file_path = build_dir + "/finaloutput_" + std::to_string(id);

    indata1.open(dest_file_path, std::ios_base::app);

    std::string source_file_path = server_dir + "/outputshare_" + std::to_string(id);
    
    std::ifstream indata2;
    indata2.open(source_file_path);

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
  } else {
    std::cout << "Wrong Argument"
              << "\n";
  }
}
