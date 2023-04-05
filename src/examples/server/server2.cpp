//./bin/server2

#include <bits/stdc++.h>
#include <filesystem>
#include <fstream>
#include <utility>
#include <regex>
#include <stdexcept>
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
int flag_server0=-1,flag_server1=-1;
std::uint64_t w_rows=0,w_cols=0,x_rows=0,x_cols=0;
std::vector<std::uint8_t> msg_Z;
std::vector<std::uint8_t> msg_R;
namespace po = boost::program_options;

struct Options {
  std::size_t my_id;
  std::uint16_t my_port;
  MOTION::Communication::tcp_parties_config tcp_config;
};

std::optional<Options> parse_program_options(int argc, char* argv[]) {
  Options options;
  boost::program_options::options_description desc("Allowed options");
  // clang-format off
  desc.add_options()
    ("help,h", po::bool_switch()->default_value(false),"produce help message")
    ("my_port",po::value<std::uint16_t>()->default_value(7002),"Helper node port number. Default is 7002")
    ("party_0", po::value<std::string>()->multitoken(),
     "(party0's IP, port), e.g., --party_0 127.0.0.1,7777")
    ("party_1", po::value<std::string>()->multitoken(),
     "(party1's IP, port), e.g., --party_1 127.0.0.1,7777")
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
  const auto parse_party_argument =
      [](const auto& s) -> MOTION::Communication::tcp_connection_config {
    const static std::regex party_argument_re("([^,]+),(\\d{1,5})");
    std::smatch match;
    if (!std::regex_match(s, match, party_argument_re)) {
      throw std::invalid_argument("Invalid party argument: "+s);
    }
    auto host = match[1];
    auto port = boost::lexical_cast<std::uint16_t>(match[2]);
    return {host, port};
  };

  std::uint16_t my_port = vm["my_port"].as<std::uint16_t>();
  const std::string party_0_info = vm["party_0"].as<std::string>();
  const std::string party_1_info = vm["party_1"].as<std::string>();
  options.tcp_config.resize(3);
  const auto conn_info0 = parse_party_argument(party_0_info);
  const auto conn_info1 = parse_party_argument(party_1_info);
  options.tcp_config[0] = conn_info0;
  options.tcp_config[1] = conn_info1;
  options.tcp_config[2] = {"127.0.0.1",my_port};
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
    for(int i=0;i<w0[0];i++)
    { 
      tempw.push_back(w0[0]);
      tempw.push_back(x0[1]);
      for(int k=0;k<w0[1];k++)
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
    std::cout<<"Computed z=w0.x0 of size "<<z.size()<<"\n";

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
    std::cout<<"Generated Random value R of size "<<r.size()<<std::endl;
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
    adduint64(z[i],msg_Z); //z=z-r  server1
    adduint64(r[i],msg_R); //r  server0
    // std::cout<<"z[]= "<<z[i]<<" r[]= "<<r[i]<<std::endl;
   }
   
}

class TestMessageHandler : public MOTION::Communication::MessageHandler {
  void received_message(std::size_t party_id, std::vector<std::uint8_t>&&message) {
    // std::cout << "Message received from party " << party_id << ":\n";
    //(w0 -> 256*784, x0 ->784*1 server0) ,(w1 -> 256*784 , x1->784*1 server1)
    int size_msg=message.size()/8;
    std::cout<<"Received message of size "<<size_msg<<" from party "<<party_id<<std::endl;
    if(message.size()==1 && message[0]==(std::uint8_t)1)
      {
        if(party_id==0)
          {
            std::cout<<"Server 0 started\n";
            flag_server0 = 1;
            return;
          }
        else if(party_id==1)
          {
            std::cout<<"Server 1 started\n";
            flag_server1 = 1;
            return;
          }
        else
          {
            std::cerr<<"Received 1 from unknown party "<<party_id<<std::endl;
            return;
          }
      }
    while((flag_server0==-1) || (flag_server1==-1))
      {
        std::cout<<'.';
        sleep(2);
      }
    auto i=0;
    for(i=1;i<size_msg;i++)
    { 
      auto temp=getuint64(message,i);
      if(i==1 && c1==1 && party_id==0)
      {
          w_rows=getuint64(message,i-1);
          w_cols=getuint64(message,i);
          w0.push_back(w_rows);
          w0.push_back(w_cols);
          c1++;
      }
      else if(i==1 && c2==1 && party_id==1)
      {
          w_rows=getuint64(message,i-1);
          w_cols=getuint64(message,i);
          w1.push_back(w_rows);
          w1.push_back(w_cols);
          c2++;
      }
      else if(i==1 && c3==1 && party_id==0)
      {
          x_rows=getuint64(message,i-1);
          x_cols=getuint64(message,i);
          x0.push_back(x_rows);
          x0.push_back(x_cols);
          c3++;
      }
      else if(i==1 && c4==1 && party_id==1)
      {
          x_rows=getuint64(message,i-1);
          x_cols=getuint64(message,i);
          x1.push_back(x_rows);
          x1.push_back(x_cols);
          c4++;
      }
      else if(c1>=2 && c1<=(w_cols*w_rows+1) && i>1 && party_id==0)
      {
          w0.push_back(temp);
          c1++;
      }
      else if(c2>=2 && c2<=(w_cols*w_rows+1) && i>1 && party_id==1)
      {// std::cout<<temp<<"\n";
          w1.push_back(temp);
          c2++;
      }
      else if(c3>=2 && c3<=x_rows+1 && i>1 && party_id==0)
      {// std::cout<<temp<<"\n";
          x0.push_back(temp);
          c3++;
      }
      else if(c4>=2 && c4<=x_rows+1 && i>1 && party_id==1)
      {// std::cout<<temp<<"\n";
          x1.push_back(temp);
          c4++;
      }
    }
    // std::cout<<"W0 size="<<w0.size()<<" W1 size="<<w1.size()<<" X0 size="<<x0.size()<<" X1 size="<<x1.size()<<std::endl;
    // std::cout<<"c1 : "<<c1<<" "<<"c2 : "<<c2<<" "<<"c3 : "<<c3<<" "<<"c4 : "<<c4<<"\n";
    // std::cout<<"w_rows="<<w_rows<<" w_cols="<<w_cols<<std::endl;
    if(c1==(w_cols*w_rows+2) && c2==(w_cols*w_rows+2) && c3==w_cols+2 && c4==w_cols+2)
    { 
      // std::cout<<"operations:-\n";
      operations();
    }
  }
};


int main(int argc, char* argv[]) {
  
  int my_id = 2;
  auto options = parse_program_options(argc, argv);
  // std::cout<<"my_id:"<<my_id<<"\n";
  if (!options.has_value()) {
    return EXIT_FAILURE;
  }
  // MOTION::Communication::tcp_parties_config config;
  // config.reserve(num_parties);
  // for (std::size_t party_id = 0; party_id < num_parties; ++party_id) {
  //   config.push_back({localhost, 10000 + party_id});
  // };
  try{
    MOTION::Communication::TCPSetupHelper helper(my_id, options->tcp_config);
    auto comm_layer = std::make_unique<MOTION::Communication::CommunicationLayer>(
        my_id, helper.setup_connections());

    auto logger = std::make_shared<MOTION::Logger>(my_id, boost::log::trivial::severity_level::trace);
    comm_layer->set_logger(logger);
    comm_layer->start();

    comm_layer->register_fallback_message_handler(
        [](auto party_id) { return std::make_shared<TestMessageHandler>(); });
    while((flag_server0==-1) || (flag_server1==-1))
      {
        std::cout<<'.';
        sleep(2);
      }
    std::cout<<std::endl;
    // std::cout<<"Received Msg Z of size:"<<msg_Z.size()<<std::endl;
    // std::cout<<"Received Msg R of size:"<<msg_R.size()<<std::endl;
    sleep(20);

    std::cout<<"Sending (Z-R) of size "<<msg_Z.size()<<" to party 1.\n";
    std::cout<<"Sending R of size "<<msg_R.size()<<" to party 0.\n";

    comm_layer->send_message(1,msg_Z);//z-r
    comm_layer->send_message(0,msg_R);//r
    comm_layer->shutdown();
  }
  catch (std::runtime_error& e) {
    std::cerr << "ERROR OCCURRED: " << e.what() << "\n";
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
