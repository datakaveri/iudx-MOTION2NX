//./bin/server2

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

std::vector<std::uint64_t> x0, w0;
std::vector<std::uint64_t> x1, w1;
int c1 = 1;
int c2 = 1;
int c3 = 1;
int c4 = 1;

std::vector<std::uint8_t> mes_z;
std::vector<std::uint8_t> mes_r;
namespace po = boost::program_options;

struct Options {
  std::size_t my_id;
};

std::optional<Options> parse_program_options(int argc, char* argv[]) {
  Options options;
  boost::program_options::options_description desc("Allowed options");
  // clang-format off
  desc.add_options()
    ("help,h", po::bool_switch()->default_value(false),"produce help message")
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

// clang-format on;
return options;
}
void print_message(std::vector<std::uint8_t>& message) {
  for (auto i = 0; i < message.size(); i++) {
    std::cout << std::hex << (int)message[i] << " ";
  }
  return;
}

void adduint64(std::uint64_t num, std::vector<std::uint8_t>& message) {
  for (auto i = 0; i < sizeof(num); i++) {
    std::uint8_t byte = num & 0xff;
    message.push_back(byte);
    num = num >> 8;
  }
}


std::uint64_t getuint64(std::vector<std::uint8_t>& message, int index) {
  std::uint64_t num = 0;
  for (auto i = 0; i < 8; i++) {
    num = num << 8;
    num = num | message[(index + 1) * 8 - 1 - i];
  }
  return num;
}

template <typename E>
std::uint64_t blah(E &engine)
{
     std::uniform_int_distribution<unsigned long long> dis(
         std::numeric_limits<std::uint64_t>::min(),
         std::numeric_limits<std::uint64_t>::max());
     return dis(engine);
}

std::vector<std::uint64_t>multiplicate(std::vector<uint64_t>&w0,std::vector<uint64_t>&x0)
{
      //z=(256*784 * 784*1)= 256*1
    
    auto x0_begin = x0.begin();
    advance(x0_begin, 2);

    std::vector<std::uint64_t>z;
    z.push_back(w0[0]); //256
    z.push_back(x0[1]); //1

    std::vector<std::uint64_t>tempw;
    int count=2;
    for(int i=0;i<256;i++)
    { 
      tempw.push_back(w0[0]);
      tempw.push_back(x0[1]);
      for(int k=2;k<786;k++)
      {
        tempw.push_back(w0[count]);
        count++;
      }
      auto tempw_begin=tempw.begin();
      auto tempw_end=tempw.end();

      advance(tempw_begin, 2);
      __gnu_parallel::transform(tempw_begin, tempw_end, x0_begin, tempw_begin , std::multiplies{});
      
      // std::cout<<"tempw size: "<<tempw.size()<<"\n";

      std::uint64_t sum=0;
      for(int j=2;j<tempw.size();j++)
      {
        sum+=tempw[j];
      }
      // std::cout<<i+1<<". "<<sum<<"\n";
      z.push_back(sum);
      tempw.clear();
    }
    std::cout<<"count:"<<count<<"\n";
    return z;
    
}


void operations()
{   
    auto w0_begin = w0.begin(); //256*784 server0
    auto w1_begin = w1.begin(); //256*784 server1
    auto w0_end = w0.end();
    auto w1_end = w1.end();
    //to skip rows and columns 
    advance(w0_begin, 2);
    advance(w1_begin, 2);


    auto x0_begin = x0.begin(); //784*!
    auto x1_begin = x1.begin(); //784*1
    auto x0_end = x0.end();
    auto x1_end = x1.end();
    //to skip rows and columns 
    advance(x0_begin, 2);
    advance(x1_begin, 2);
    
  //--------------------------------------------------------------------------------------------------
  
    //delx0=delx0+delx1
    __gnu_parallel::transform(x0_begin, x0_end, x1_begin, x0_begin , std::plus{});
    std::cout<<"x0:"<<*x0_begin<<"\n";

    
    //delw0=delw0+delw1
    __gnu_parallel::transform(w0_begin, w0_end, w1_begin, w0_begin , std::plus{});
    std::cout<<"w0:"<<*w0_begin<<"\n";
    

    //z=(256*784 * 784*1)= 256*1
    std::vector<std::uint64_t>z=multiplicate(w0,x0);

    //-----------------------------------------------------------------------------------------------
    std::cout<<"z size: "<<z.size()<<"\n";

    std::vector<std::uint64_t>r;
    r.resize(x0.size(),0);
    r[0]=w0[0];
    r[1]=x0[1];
    
    for(int i=2;i<z.size();i++)
    { 
      std::random_device rd;
      std::mt19937 gen(rd());
      auto temp=blah(gen);
      r[i]=temp;
    }
    
    auto r_begin = r.begin();
    advance(r_begin,2);


    //--------------------------------------------------------------------------------------------------
    
    auto z_begin = z.begin();
    auto z_end = z.end();
    advance(z_begin,2);

    //z=z-r

    __gnu_parallel::transform(z_begin, z_end, r_begin, z_begin , std::minus{});

  
   //Final output:-
  //  std::cout<<"Final Output -:- \n";
   for(int i=0;i<z.size();i++)
   {
    // std::cout<<i+1<<". "<<z[i]<<" "<<r[i]<<"\n"; //s1 s0
    adduint64(z[i],mes_z); //z=z-r  server1
    adduint64(r[i],mes_r); //r  server0
   }
   
}

class TestMessageHandler : public MOTION::Communication::MessageHandler {
  void received_message(std::size_t party_id, std::vector<std::uint8_t>&&message) {
    std::cout << "Message received from party " << party_id << ":\n";
    
    //(w0 -> 256*784, x0 ->784*1 server0) ,(w1 -> 256*784 , x1->784*1 server1)
    int size_mes=message.size()/8;
    std::cout<<"Size_mes : "<<size_mes<<"\n";
    
    auto i=0;
    for(i=1;i<message.size()/8;i++)
    { 
      auto temp=getuint64(message,i);
      if(i==1 && c1==1 && party_id==0)
      {
          auto rows=getuint64(message,i-1);
          auto col=getuint64(message,i);
          w0.push_back(rows);
          w0.push_back(col);
          c1++;
      }
      else if(i==1 && c2==1 && party_id==1)
      {
          auto rows=getuint64(message,i-1);
          auto col=getuint64(message,i);
          w1.push_back(rows);
          w1.push_back(col);
          c2++;
      }
      else if(i==1 && c3==1 && party_id==0)
      {
          auto rows=getuint64(message,i-1);
          auto col=getuint64(message,i);
          x0.push_back(rows);
          x0.push_back(col);
          c3++;
      }
      else if(i==1 && c4==1 && party_id==1)
      {
          auto rows=getuint64(message,i-1);
          auto col=getuint64(message,i);
          x1.push_back(rows);
          x1.push_back(col);
          c4++;
      }
      else if(c1>=2 && c1<=(784*256+1) && i>1 && party_id==0)
      {
          w0.push_back(temp);
          c1++;
      }
      else if(c2>=2 && c2<=(784*256+1) && i>1 && party_id==1)
      {// std::cout<<temp<<"\n";
          w1.push_back(temp);
          c2++;
      }
      else if(c3>=2 && c3<=785 && i>1 && party_id==0)
      {// std::cout<<temp<<"\n";
          x0.push_back(temp);
          c3++;
      }
      else if(c4>=2 && c4<=785&& i>1 && party_id==1)
      {// std::cout<<temp<<"\n";
          x1.push_back(temp);
          c4++;
      }
    }
    
    std::cout<<"c1 : "<<c1<<" "<<"c2 : "<<c2<<" "<<"c3 : "<<c3<<" "<<"c4 : "<<c4<<"\n";
    
    if(c1==(784*256+2) && c2==(784*256+2) && c3==786 && c4==786)
    { 
       std::cout<<"operations:-\n";
      operations();
    }
  
  }
};


int main(int argc, char* argv[]) {
  
  const auto localhost = "127.0.0.1";
  const auto num_parties = 3;
  int my_id = 2;
  auto options = parse_program_options(argc, argv);
  std::cout<<"my_id:"<<my_id<<"\n";

  MOTION::Communication::tcp_parties_config config;
  config.reserve(num_parties);
  for (std::size_t party_id = 0; party_id < num_parties; ++party_id) {
    config.push_back({localhost, 10000 + party_id});
  };

  MOTION::Communication::TCPSetupHelper helper(my_id, config);
  auto comm_layer = std::make_unique<MOTION::Communication::CommunicationLayer>(
      my_id, helper.setup_connections());

  auto logger = std::make_shared<MOTION::Logger>(my_id, boost::log::trivial::severity_level::trace);
  comm_layer->set_logger(logger);
  


  comm_layer->start();

 
  comm_layer->register_fallback_message_handler(
      [](auto party_id) { return std::make_shared<TestMessageHandler>(); });
  
  std::cout<<"Mesz:"<<mes_z.size();
  std::cout<<"Mesr:"<<mes_r.size();

  sleep(20);

  std::cout<<"Mesz:"<<mes_z.size();
  std::cout<<"Mesr:"<<mes_r.size();

  comm_layer->send_message(1,mes_z);//z
  comm_layer->send_message(0,mes_r);//z-r
  comm_layer->shutdown();
}
