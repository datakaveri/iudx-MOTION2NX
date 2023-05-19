//./bin/server2

#include <bits/stdc++.h>
#include <filesystem>
#include <fstream>
#include <regex>
#include <stdexcept>
#include <utility>
#include "communication/communication_layer.h"
#include "communication/message_handler.h"
#include "communication/tcp_transport.h"
#include "utility/logger.h"

#include <boost/algorithm/string.hpp>
#include <boost/chrono.hpp>
#include <boost/json/serialize.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/log/trivial.hpp>
#include <boost/program_options.hpp>
#include <boost/thread/thread.hpp>

#include <iostream>
#include <iterator>
#include <parallel/algorithm>
#include <vector>
#include "utility/new_fixed_point.h"

std::vector<std::uint64_t> x0, w0;
std::vector<std::uint64_t> x1, w1;
int c1 = 1, c2 = 1, c3 = 1, c4 = 1;
bool operations_done_flag = false, server0_ready_flag = false, server1_ready_flag = false;
std::uint64_t w_rows = 0, w_cols = 0, x_rows = 0, x_cols = 0;
std::vector<std::uint8_t> msg_Z, msg_R;
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
    ("party", po::value<std::vector<std::string>>()->multitoken(),
     "(party id, IP, port), e.g., --party 1,127.0.0.1,7777")
    ("helper_node", po::value<std::string>()->multitoken(),
     "(helpernode IP, port), e.g., --helper_node 127.0.0.1,7777") 
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
    std::cerr << "Error while parsing the options:" << e.what() << "\n\n";
    std::cerr << desc << "\n";
    return std::nullopt;
  }

  const auto parse_helpernode_info =
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
  
  const auto parse_party_argument =
      [](const auto& s) -> std::pair<std::size_t, MOTION::Communication::tcp_connection_config> {
    const static std::regex party_argument_re("([01]),([^,]+),(\\d{1,5})");
    std::smatch match;
    if (!std::regex_match(s, match, party_argument_re)) {
      throw std::invalid_argument("Invalid party argument");
    }
    auto id = boost::lexical_cast<std::size_t>(match[1]);
    auto host = match[2];
    auto port = boost::lexical_cast<std::uint16_t>(match[3]);
    return {id, {host, port}};
  };

  const std::vector<std::string> party_infos = vm["party"].as<std::vector<std::string>>();
  if (party_infos.size() != 2) {
    std::cerr << "Expecting two --party options (for party 0 and party 1)\n";
    return std::nullopt;
  }
  const auto [id0, conn_info0] = parse_party_argument(party_infos[0]);
  const auto [id1, conn_info1] = parse_party_argument(party_infos[1]);
  if (id0 == id1) {
    std::cerr << "Need party arguments for both party 0 and party 1\n";
    return std::nullopt;
  }
  const std::string helper_node_info = vm["helper_node"].as<std::string>();
  const auto conn_info_helpernode = parse_helpernode_info(helper_node_info);

  options.tcp_config.resize(3);
  options.tcp_config[id0] = conn_info0;
  options.tcp_config[id1] = conn_info1;
  options.tcp_config[2] = conn_info_helpernode;

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
std::uint64_t RandomNumGenerator(E &engine)
{
     std::uniform_int_distribution<unsigned long long> dis(
         std::numeric_limits<std::uint64_t>::min(),
         std::numeric_limits<std::uint64_t>::max());
     return dis(engine);
}

std::vector<std::uint64_t>multiplicate(std::vector<uint64_t>&w0,std::vector<uint64_t>&x0)
{
    //z=(256*784 * 784*1)= 256*1
    if(w0.size()<=2 || x0.size()<=2)
      {
        std::cerr<<"Shares unavailable to perform computations. Weight shares size is "<<w0.size()<<"  and input shares size is "<<x0.size()<<std::endl;
        exit(1);
      }
    
    auto x0_begin = x0.begin();
    advance(x0_begin, 2);

    std::vector<std::uint64_t>z;// Output shares.
    z.push_back(w0[0]); // Output row size = Weights row size
    z.push_back(x0[1]); // Output column size =  Input column size

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
      
      std::uint64_t sum=0;
      for(int j=2;j<tempw.size();j++)
      {
        sum+=tempw[j];
      }
      z.push_back(sum);
      tempw.clear();
    }
    return z;
    
}


void operations()
{   
  if(w0.size()<=2 || x0.size()<=2 || w1.size()<=2 || x1.size()<=2)
    {
      std::cerr<<"Shares unavailable to perform computations.\n";
      exit(1);
    }
  auto w0_begin = w0.begin(); 
  auto w1_begin = w1.begin(); 
  auto w0_end = w0.end();
  auto w1_end = w1.end();
  //to skip rows and columns 
  advance(w0_begin, 2);
  advance(w1_begin, 2);
  auto x0_begin = x0.begin(); 
  auto x1_begin = x1.begin(); 
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
  std::cout<<"Computed Z=w0.x0 of size "<<z.size()<<"\n";

  std::vector<std::uint64_t>r;
  r.resize(x0.size(),0);
  r[0]=w0[0];
  r[1]=x0[1];
  
  for(int i=2;i<z.size();i++)
  { 
    std::random_device rd;
    std::mt19937 gen(rd());
    auto temp=RandomNumGenerator(gen);
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
  adduint64(z[i],msg_Z); //z=z-r  server1
  adduint64(r[i],msg_R); //r  server0
  }
  operations_done_flag = true; 
}

class TestMessageHandler : public MOTION::Communication::MessageHandler {
  void received_message(std::size_t party_id, std::vector<std::uint8_t>&&message) {
    //layer 1 - (w0 -> 256*784, x0 ->784*1 server0),(w1 -> 256*784 , x1->784*1 server1)
    //layer 2 - (w0 -> 10*256, x0 ->256*1 server0), (w1 -> 10*256 , x1->256*1 server1)
    int size_msg=message.size()/8;
    // To set the flags after the helper node receives start message from server 0 and server 1.
    if(message.size()==1 && message[0]==(std::uint8_t)1)
      {
        if(party_id==0)
          {
            std::cout<<"Server 0 has started.\n";
            server0_ready_flag = true;
            return;
          }
        else if(party_id==1)
          {
            std::cout<<"Server 1 has started.\n";
            server1_ready_flag = true;
            return;
          }
        else
          {
            std::cerr<<"Received the message \"1\" from unknown party "<<party_id<<std::endl;
            return;
          }
      }
    while((!server0_ready_flag) || (!server1_ready_flag))
      {
        std::cout<<".";
        boost::this_thread::sleep_for(boost::chrono::milliseconds(400));
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
      {
          w1.push_back(temp);
          c2++;
      }
      else if(c3>=2 && c3<=x_rows+1 && i>1 && party_id==0)
      {
          x0.push_back(temp);
          c3++;
      }
      else if(c4>=2 && c4<=x_rows+1 && i>1 && party_id==1)
      {
          x1.push_back(temp);
          c4++;
      }
    }
    // std::cout<<"W0 size="<<w0.size()<<" W1 size="<<w1.size()<<" X0 size="<<x0.size()<<" X1 size="<<x1.size()<<std::endl;
    // std::cout<<"c1 : "<<c1<<" "<<"c2 : "<<c2<<" "<<"c3 : "<<c3<<" "<<"c4 : "<<c4<<"\n";
    // std::cout<<"w_rows="<<w_rows<<" w_cols="<<w_cols<<std::endl;
    if(c1==(w_cols*w_rows+2) && c2==(w_cols*w_rows+2) && c3==w_cols+2 && c4==w_cols+2)
    { 
      operations();
    }
  }
};


int main(int argc, char* argv[]) {
  std::cout<<"Started the helper node.\n";

  int my_id = 2;
  auto options = parse_program_options(argc, argv);
  if (!options.has_value()) {
    std::cerr<<"No options given.\n";
    return EXIT_FAILURE;
  }
  std::unique_ptr<MOTION::Communication::CommunicationLayer> comm_layer;
  std::shared_ptr<MOTION::Logger> logger;
  try{
      try{
        std::cout<<"Setting up the connections.";
        MOTION::Communication::TCPSetupHelper helper(my_id, options->tcp_config);
        comm_layer = std::make_unique<MOTION::Communication::CommunicationLayer>(
            my_id, helper.setup_connections());
      }
    catch (std::runtime_error& e) {
      std::cerr << "Error occurred during connection setup: " << e.what() << "\n";
      return EXIT_FAILURE;
    }
    // try{
    // logger = std::make_shared<MOTION::Logger>(my_id, boost::log::trivial::severity_level::trace);
    // comm_layer->set_logger(logger);
    // }
    // catch (std::runtime_error& e) {
    //   std::cerr << "Error occurred during logger setup: " << e.what() << "\n";
    //   return EXIT_FAILURE;
    // }
    std::cout<<"Starting the communication layer\n";
    try{
      comm_layer->start();
    }
    catch (std::runtime_error& e) {
      std::cerr << "Error occurred while starting the communication: " << e.what() << "\n";
      return EXIT_FAILURE;
    }
    std::cout<<"Start Receiving messages in parallel\n";
    comm_layer->register_fallback_message_handler(
        [](auto party_id) { return std::make_shared<TestMessageHandler>(); });
    
    //Waiting for server 0 and 1 to send their start messages. 
    while((!server0_ready_flag) || (!server1_ready_flag))
      {
        std::cout<<".";
        boost::this_thread::sleep_for(boost::chrono::milliseconds(400));
      }

    // Sending acknowledgement message to server 0 and 1, after receiving the start message.
    std::cout<<"Sending acknowledgement message to server 0 and 1\n";
    std::vector<std::uint8_t> ack{(std::uint8_t)1};
    try{
    comm_layer->send_message(1,ack); 
    }
    catch (std::runtime_error& e) {
      std::cerr << "Error occurred while sending the ack message to server 1: " << e.what() << "\n";
      return EXIT_FAILURE;
    }
    try{
      comm_layer->send_message(0,ack);
    }
    catch (std::runtime_error& e) {
      std::cerr << "Error occurred while sending the ack message to server 0: " << e.what() << "\n";
      return EXIT_FAILURE;
    }
    std::cout<<"Sent acknowledgement message to server 0 and 1\n";

    //Waiting for the operations to complete before sending the results to the servers.
    while(!operations_done_flag)
      {
        std::cout<<".";
        boost::this_thread::sleep_for(boost::chrono::milliseconds(100));
      }
    
    std::cout<<"Sending (Z-R) of size "<<msg_Z.size()<<" to party 1.\n";
    try{  
      comm_layer->send_message(1,msg_Z);//z-r
    }
    catch (std::runtime_error& e) {
      std::cerr << "Error occurred while sending (Z-R) to server 1: " << e.what() << "\n";
      return EXIT_FAILURE;
    }
    std::cout<<"Sending R of size "<<msg_R.size()<<" to party 0.\n";
    try{
    comm_layer->send_message(0,msg_R);//r
    }
    catch (std::runtime_error& e) {
      std::cerr << "Error occurred while sending (R) to server 0: " << e.what() << "\n";
      return EXIT_FAILURE;
    }
    comm_layer->shutdown();
  }
  catch (std::runtime_error& e) {
    std::cerr << "ERROR OCCURRED: " << e.what() << "\n";
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}