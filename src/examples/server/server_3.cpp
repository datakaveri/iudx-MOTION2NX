//./bin/server --config-file1  --config-file2 --my-id
#include <bits/stdc++.h>
#include <filesystem>
#include <fstream>
#include <utility>
#include "communication/communication_layer.h"
#include "communication/message_handler.h"
#include "communication/tcp_transport.h"
#include "utility/logger.h"

#include <boost/algorithm/string.hpp>
#include <boost/json/serialize.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/log/trivial.hpp>
#include <boost/program_options.hpp>

#include <iostream>
#include <iterator>
#include <parallel/algorithm>
#include <vector>

#include "utility/new_fixed_point.h"

namespace po = boost::program_options;

struct Options {
  std::string x;
  std::string y;
  std::size_t my_id;
  // std::size_t r_id;
  // std::string filepath_frombuild
};

std::optional<Options> parse_program_options(int argc, char* argv[]) {
  Options options;
  boost::program_options::options_description desc("Allowed options");
  // clang-format off
  desc.add_options()
    ("help,h", po::bool_switch()->default_value(false),"produce help message")
    ("my-id", po::value<std::size_t>()->required(), "my party id")  
    ("fn1", po::value<std::string>()->required(), "filename1")  
    ("fn2", po::value<std::string>()->required(), "filename2") 
  ;
 
 po::variables_map vm;
   po::store(po::parse_command_line(argc, argv, desc), vm);
  bool help = vm["help"].as<bool>();
  if (help) {
    std::cerr << desc << "\n";
    return std::nullopt;
  }

   try {
    po::notify(vm);
  } catch (std::exception& e) {
    std::cerr << "error:" << e.what() << "\n\n";
    std::cerr << desc << "\n";
    return std::nullopt;
  }

  options.my_id = vm["my-id"].as<std::size_t>();

  options.x = vm["fn1"].as<std::string>();
  options.y = vm["fn2"].as<std::string>();

  if (options.my_id > 2) {
    std::cerr << "my-id must be one of 0, 1 and 2\n";
    return std::nullopt;
  }

// clang-format on;
return options;
}
void print_message(std::vector<std::uint8_t>& message) {
  for (auto i = 0; i < message.size(); i++) {
    std::cout << std::hex << (int)message[i] << " ";
  }
  return;
}

std::uint64_t getuint64(std::vector<std::uint8_t>& message, int index) {
  std::uint64_t num = 0;
  for (auto i = 0; i < 8; i++) {
    num = num << 8;
    num = num | message[(index + 1) * 8 - 1 - i];
  }
  return num;
}

class TestMessageHandler : public MOTION::Communication::MessageHandler {
  void received_message(std::size_t party_id, std::vector<std::uint8_t>&&message) {
    std::cout << "Message received from party " << party_id << ":";
    // std::cout << rows << " " << col << std::endl;
    std::cout<<message.size()<<"\n";
    int k = message.size() / 8;
    for (auto i = 0; i < k; ++i) {
      auto temp = getuint64(message, i);
      std::cout <<"recieved message is "<<temp << "\n";
    }
    std::cout << std::endl;
  }
};

void adduint64(std::uint64_t num, std::vector<std::uint8_t>& message) {
  for (auto i = 0; i < sizeof(num); i++) {
    std::uint8_t byte = num & 0xff;
    message.push_back(byte);
    num = num >> 8;
  }
}

template <typename E>
std::uint64_t blah(E &engine)
{
     std::uniform_int_distribution<unsigned long long> dis(
         std::numeric_limits<std::uint64_t>::min(),
         std::numeric_limits<std::uint64_t>::max());
     return dis(engine);
}

void shares(const Options& options)
{
  std::string temp = std::filesystem::current_path();
  std::string filename1,filename2;
  std::uint64_t rows, col;
  std::vector<std::uint64_t> Del_x;
  std::vector<std::uint64_t> Del_y;
  std::vector<std::uint64_t> del_x;
  std::vector<std::uint64_t> del_y;
  

  filename1=temp+"/server"+std::to_string(options.my_id)+"/"+options.x;
  filename2=temp+"/server"+std::to_string(options.my_id)+"/"+options.y;

  std::ifstream file1(filename1);
  std::ifstream file2(filename2);
  file1>>rows>>col;
  file2>>rows>>col;
  int i=0;
  while (i < rows * col) {
    std::uint64_t num;
    file1 >> num;
    Del_x.push_back(num);
    file2>>num;
    Del_y.push_back(num);
    file1>>num;
    del_x.push_back(num);
    file2>>num;
    del_y.push_back(num);

    for(int i=0;i<Del_x.size();i++)
    {
      std::cout<<Del_x[i]<<"\n";
      std::cout<<Del_y[i]<<"\n";
      std::cout<<del_x[i]<<"\n";
      std::cout<<del_y[i]<<"\n";
    }

    if (file1.eof()) {
      std::cerr << "File contains less number of elements" << std::endl;
      exit(1);
    }
  i++;
  }
  if (i == rows * col) {
    std::uint64_t num;
    file1 >> num;
    if (!file1.eof()) {
      std::cerr << "File contains more number of elements" << std::endl;
      exit(1);
    }
  }
  
  std::vector<std::uint64_t> m1,m2,m3,m4;
  m1.resize(Del_x.size());
  m2.resize(Del_x.size());
  m3.resize(Del_x.size());
  m4.resize(Del_x.size());
  //final output=m1
  if(options.my_id==0)
  {   
    std::vector<std::uint64_t> random;
     __gnu_parallel::transform(Del_x.begin(),Del_x.end(), Del_y.begin(),
                            m1.begin(), std::multiplies{});
      
     for(int i=0;i<m1.size();i++)
     {
       m1[i] = MOTION::new_fixed_point::truncate(m1[i], 13);
       std::cout<<"xpublic:"<<m1[i]<<"\n";
     }



     __gnu_parallel::transform(Del_x.begin(),Del_x.end(), del_y.begin(),
                            m2.begin(), std::multiplies{});

    for(int i=0;i<m2.size();i++)
     {
       m2[i] = MOTION::new_fixed_point::truncate(m2[i], 13);
       std::cout<<"x2:"<<m2[i]<<"\n";
     }



     __gnu_parallel::transform(Del_y.begin(),Del_y.end(), del_x.begin(),
                            m3.begin(), std::multiplies{});

    for(int i=0;i<m3.size();i++)
     {
       m3[i] = MOTION::new_fixed_point::truncate(m3[i], 13);
       std::cout<<"ypublic:"<<m3[i]<<"\n";
     }



     __gnu_parallel::transform(del_y.begin(),del_y.end(), del_x.begin(),
                            m4.begin(), std::multiplies{});

    for(int i=0;i<m4.size();i++)
     {
       m4[i] = MOTION::new_fixed_point::truncate(m4[i], 13);
     }



     __gnu_parallel::transform(m1.begin(),m1.end(), m4.begin(),
                            m1.begin(), std::plus{});  
     __gnu_parallel::transform(m1.begin(),m1.end(), m2.begin(),
                            m1.begin(), std::minus{});  
     __gnu_parallel::transform(m1.begin(), m1.end(),m3.begin(),
                            m1.begin(), std::minus{});  

    for(int i=0;i<m1.size();i++)
    {
      std::cout<<"Del0:"<<m1[i]<<"\n";
    }
    
    for(int i=0;i<m1.size();i++)
    {
      std::random_device rd;
      std::mt19937 gen(rd());
      auto temp=blah(gen);
      random.push_back(temp);
    }

      __gnu_parallel::transform(m1.begin(), m1.end(),random.begin(),
                            m1.begin(), std::plus{}); 

    std::ofstream file;
   file.open("S0",std::ios_base::out);
   file<<rows<<" "<<col<<"\n";
   for(int i=0;i<m1.size();i++)
   {
     file<<m1[i]<<" "<<random[i]<<"\n";
   }
 
    
  }
  // //final output=m3
  else if(options.my_id==1)
  {  
      for(int i=0;i<m1.size();i++)
    {
      std::cout<<"Del1:"<<m3[i]<<"\n";
    }
     std::vector<std::uint64_t> random;


     __gnu_parallel::transform(Del_x.begin(),Del_x.end(), del_y.begin(),
                            m2.begin(), std::multiplies{});

    for(int i=0;i<m2.size();i++)
     {
       m2[i] = MOTION::new_fixed_point::truncate(m2[i], 13);
     }         



     __gnu_parallel::transform(Del_y.begin(),Del_y.end(), del_x.begin(),
                            m3.begin(), std::multiplies{});
      
    for(int i=0;i<m3.size();i++)
     {
       m3[i] = MOTION::new_fixed_point::truncate(m3[i], 13);
     }



     __gnu_parallel::transform(del_y.begin(),del_y.end(), del_x.begin(),
                            m4.begin(), std::multiplies{});

    for(int i=0;i<m4.size();i++)
     {
       m4[i] = MOTION::new_fixed_point::truncate(m4[i], 13);
     }



     __gnu_parallel::transform(m4.begin(),m4.end(), m2.begin(),
                            m4.begin(), std::minus{});  
     __gnu_parallel::transform(m4.begin(),m4.end(), m3.begin(),
                            m3.begin(), std::minus{});   
     

    for(int i=0;i<m1.size();i++)
    {
      std::cout<<"Del1:"<<m3[i]<<"\n";
    }

    for(int i=0;i<m1.size();i++)
    {
      std::random_device rd;
      std::mt19937 gen(rd());
      auto temp=blah(gen);
      random.push_back(temp);
    }

      __gnu_parallel::transform(m3.begin(), m3.end(),random.begin(),
                            m3.begin(), std::plus{});

      std::ofstream file;
   file.open("S1",std::ios_base::out);
   file<<rows<<" "<<col<<"\n";
   for(int i=0;i<m3.size();i++)
   {
     file<<m3[i]<<" "<<random[i]<<"\n";
     
   }
  }
   
}



int main(int argc, char* argv[]) {

  auto options = parse_program_options(argc, argv);

  const auto localhost = "127.0.0.1";
  const auto num_parties = 2;
  int my_id=options->my_id;
  std::cout<<"my_id:"<<my_id<<"\n";
  
  shares(*options);

 
}
