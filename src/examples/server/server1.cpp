//./bin/server1 --fn1 file_config_model1
#include <unistd.h>
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

std::vector<std::uint64_t> Z;  //
std::vector<std::uint64_t> wpublic, xpublic, wsecret, xsecret, bpublic, bsecret;
std::vector<std::uint64_t> randomnum;
int flag = 0;
std::uint64_t fractional_bits;
namespace po = boost::program_options;

struct Options {
  std::string file1;
  std::size_t file2;
  std::string path;
  std::size_t fractional_bits;
};

std::optional<Options> parse_program_options(int argc, char* argv[]) {
  Options options;
  boost::program_options::options_description desc("Allowed options");
  // clang-format off
  desc.add_options()
    ("help,h", po::bool_switch()->default_value(false),"produce help message")
    ("fn1", po::value<std::string>()->required(), "my party id")  
    ("fn2", po::value<std::size_t>()->required(), "receiver party id") 
    ("fractional-bits", po::value<std::size_t>()->required(), "Number of fractional bits") 
    ("current-path", po::value<std::string>()->required(), "currentpath") 
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

  options.file1 = vm["fn1"].as<std::string>();
  options.file2 = vm["fn2"].as<std::size_t>();
  options.path = vm["current-path"].as<std::string>();
  options.fractional_bits = vm["fractional-bits"].as<std::size_t>();
  fractional_bits = options.fractional_bits;
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
  //Converts 8->64
  std::uint64_t num = 0;
  for (auto i = 0; i < 8; i++) {
    num = num << 8;
    num = num | message[(index + 1) * 8 - 1 - i];
  }
  return num;
}

void adduint64(std::uint64_t num, std::vector<std::uint8_t>& message) {
  //Converts 64->8
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

std::vector<std::uint64_t>multiplicate(std::vector<uint64_t>&a,std::vector<uint64_t>&b)
{
    if(a[1]!=b[0])
      {
        std::cerr<<"Error during matrix multiplication. Number of columns in W is not equal to Number of rows in X.";
        exit(1);
      }
    std::cout<<"In multiplicate, a[0]="<<a[0]<<" a[1]="<<a[1]<<" b[0]="<<b[0]<<" b[1]="<<b[1]<<std::endl;
    auto b_begin = b.begin();
    advance(b_begin, 2);
    std::vector<std::uint64_t>z;
    z.push_back(a[0]);
    z.push_back(b[1]);

    std::vector<std::uint64_t>tempw;
    int count=2;
    for(int i=0;i<a[0];i++)
    { 
      tempw.push_back(a[0]);
      tempw.push_back(b[1]);
      for(int k=0;k<a[1];k++)
      {
        tempw.push_back(a[count]);
        count++;
      }
      auto tempw_begin=tempw.begin();
      auto tempw_end=tempw.end();
      advance(tempw_begin, 2);
      __gnu_parallel::transform(tempw_begin, tempw_end, b_begin, tempw_begin , std::multiplies{});
      
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
    return z;
}

void operations()
{  
// for(int i=0;i<wpublic.size();i++)
// {
//   std::cout<<wpublic[i]<<" ";
// }
// std::cout<<"\n";
// for(int i=0;i<xpublic.size();i++)
// {
//   std::cout<<xpublic[i]<<" ";
// }
// std::cout<<"\n";
// for(int i=0;i<wsecret.size();i++)
// {
//   std::cout<<wsecret[i]<<" ";
// }
// std::cout<<"\n";
// for(int i=0;i<xsecret.size();i++)
// {
//   std::cout<<xsecret[i]<<" ";
// }
// std::cout<<"\n";

//---------------------------------------------------------------------------------------------------------
    

    //prod1=Delw * delx1
    std::vector<std::uint64_t>prod1=multiplicate(wpublic,xsecret);
    auto prod1_begin=prod1.begin();
    auto prod1_end=prod1.end();
    advance(prod1_begin, 2);


  //----------------------------------------------------------------------------------------------------------


     //prod2=Dely * delx1
    std::vector<std::uint64_t>prod2=multiplicate(wsecret,xpublic);
    auto prod2_begin=prod2.begin();
    advance(prod2_begin, 2);

//---------------------------------------------------------------------------------------------------------------------------

    auto z_begin = Z.begin();
    auto z_end = Z.end();
    advance(z_begin,2 );
    // for(int i=0;i<Z.size();i++)
    // {
    //   std::cout<<"Z:"<<Z[i]<<"\n";
    // }     
    std::cout<<"prod1 size:"<<prod1.size()<<"  Z size:"<<Z.size()<<std::endl;
  
   //z=z-Delw*delx
   //z=z-prod1
    __gnu_parallel::transform(z_begin, z_end, prod1_begin, z_begin , std::minus{});
    std::cout<<"Z1:"<<*z_begin<<"\n";

    //z=z-Delx*delw
    //z=z-prod1-prod2
     __gnu_parallel::transform(z_begin, z_end, prod2_begin, z_begin , std::minus{});
    std::cout<<"Z1:"<<*z_begin<<"\n";
   

    for(int i=2;i<Z.size();i++)
     {
       Z[i] = MOTION::new_fixed_point::truncate(Z[i], fractional_bits);
       std::cout<<"Z:"<<Z[i]<<"\n";
     }  

   
//--------------------------------------------------------------------------------------------------------------------
  
    randomnum.resize(prod1.size(),0);
    randomnum[0]=prod1[0]; //256
    randomnum[1]=prod1[1]; //1
    
    for(int i=2;i<prod1.size();i++)
    { 
      std::random_device rd;
      std::mt19937 gen(rd());
      auto temp=blah(gen);
      // std::cout<<"r:"<<temp<<"\n";
      randomnum[i]=temp;
    }

    auto random_begin = randomnum.begin();
    advance(random_begin,2); 

    //z=z+random number(aka secret share)
    __gnu_parallel::transform(z_begin, z_end, random_begin, z_begin , std::plus{});     
     std::cout<<"Z:"<<*z_begin<<"\n";
    flag++;
    //final output=Z
}

class TestMessageHandler : public MOTION::Communication::MessageHandler {
  void received_message(std::size_t party_id, std::vector<std::uint8_t>&& message) override {
  
    std::cout << "Message received from party " << party_id << ":";
     int k = message.size() / 8;
    if(party_id==2)
    {
    std::cout <<"Message size: "<<message.size() <<std::endl;
    if(message.size()<=0)
      {
        std::cerr<<"Empty message received from party "<<party_id<<std::endl;
        exit(1);
      }
    std::cout << "Received message is :\n";
    for (auto i = 0; i < k; ++i) {
      auto temp = getuint64(message, i);
      // std::cout << temp << ",";
      Z.push_back(temp);
    }
    std::cout<<"\n\n";
    if (Z.size() == k) {
      operations();
      flag++;
    }
    }
  else if(party_id==0)
  { 
    std::vector<std::uint64_t>Final_public;
    std::cout <<"After receiving from P0 , message size : "<< message.size() << "\n"; 
    for(int i=0;i<k;i++)
    { 
      auto temp = getuint64(message, i);
      // std::cout << temp << " ";
      Final_public.push_back(temp);
    }
      auto finalpublic_begin = Final_public.begin();
      auto finalpublic_end = Final_public.end();
      advance(finalpublic_begin,2);

      auto z_begin=Z.begin();
      advance(z_begin,2);
      
      auto bpublic_begin=bpublic.begin();
      advance(bpublic_begin,2);

      auto randomnum_begin = randomnum.begin();
      auto randomnum_end = randomnum.end();
      advance(randomnum_begin,2);

		  auto bsecret_begin=bsecret.begin();
      advance(bsecret_begin,2);


		  //final public share=Final_public
      __gnu_parallel::transform(finalpublic_begin, finalpublic_end, z_begin, finalpublic_begin , std::plus{});   
      __gnu_parallel::transform(finalpublic_begin, finalpublic_end, bpublic_begin, finalpublic_begin , std::plus{});  


      //final secret share=randomnum
      __gnu_parallel::transform(randomnum_begin, randomnum_end, bsecret_begin, randomnum_begin , std::plus{});


     std::string basedir = getenv("BASE_DIR");
     std::string filename = basedir + "/build_debwithrelinfo_gcc";
     std::string totalpath = filename + "/server1/" + "outputshare_1";
  
   
	 if (std::filesystem::remove(totalpath));

   std::ofstream indata;
   indata.open(totalpath,std::ios_base::app);
   assert(indata);

   indata<<Final_public[0]<<" "<<Final_public[1]<<"\n";
   for(int i=2;i<Final_public.size();i++)
   {
    indata<<Final_public[i]<<" "<<randomnum[i]<<"\n";
   }
   
    }    
  } 
};

void read_shares(int p,int my_id,std::vector<uint8_t>&message,const Options& options)
{ 
   std::string name=options.file1;

  if(p==1)
  {
    std::ifstream content;
    
    //~/IUDX/iudx-MOTION2NX/build_debwithrelinfo_gcc/file_config_model1
    std::string fullpath = options.path;
    fullpath += "/"+name;
    
    // std::cout<<"fullpath: "<<fullpath;
    content.open(fullpath);
    std::string w1path,b1path;
    content>>w1path;
    content>>b1path;
    
    std::cout<<w1path<<" "<<b1path<<"\n";

    std::ifstream file(w1path);
    if (!file) {
      std::cerr << " Error in opening the weights file\n";
      exit(1);
    }
    std::uint64_t rows, col;
    file >> rows >> col;
    std::cout<<rows<<" "<<col;

    if (file.eof()) {
      std::cerr << "Weights File doesn't contain rows and columns" << std::endl;
      exit(1);
    }

    auto k = 0;
    adduint64(rows, message);
    adduint64(col, message);
    wpublic.push_back(rows);
    wpublic.push_back(col);
    wsecret.push_back(rows);
    wsecret.push_back(col);
    while (k < rows * col) {
      std::uint64_t public_share, secret_share;
      file >> public_share;
      wpublic.push_back(public_share);
      file >> secret_share;
      wsecret.push_back(secret_share);
      if (file.eof()) {
        std::cerr << "File contains less number of elements" << std::endl;
        exit(1);
      }
      // std::cout << num << std::endl;
      adduint64(secret_share, message);
      k++;
    }
    std::cout<<"Number of shares read: "<<k<<"\n"; 
    if (k == rows * col) {
      std::uint64_t num;
      file >> num;
      if (!file.eof()) {
        std::cerr << "File contains more number of elements" << std::endl;
        exit(1);
      }
    }
    file.close();

    file.open(b1path);
    if (!file) {
    std::cerr << " Error in opening bias file\n";
    }
    file >> rows >> col;
    std::cout<<rows<<" "<<col<<"\n";

    if (file.eof()) {
      std::cerr << "File doesn't contain rows and columns" << std::endl;
      exit(1);
    }
       
    auto j=0;
    bpublic.push_back(rows);
    bpublic.push_back(col);
    bsecret.push_back(rows);
    bsecret.push_back(col);
    while (j < rows * col) {
      std::uint64_t public_share, secret_share;
      file >> public_share;
      bpublic.push_back(public_share);
      file >> secret_share;
      bsecret.push_back(secret_share);
      // std::cout<<public_share<<" "<<secret_share<<"\n";
      if (file.eof()) {
        std::cerr << "File contains less number of elements" << std::endl;
        exit(1);
      }
      j++;
    }
    if (j == rows * col) {
      std::uint64_t num;
      file >> num;
      if (!file.eof()) {
        std::cerr << "File contains more number of elements" << std::endl;
        exit(1);
      }
    }
    file.close();
  }
  else if(p==2)
  {
    name = std::to_string(options.file2);
    std::string fullname = options.path;
    fullname += "/server" + std::to_string(my_id) + "/Image_shares/ip" + name;
    std::ifstream file(fullname);
    if (!file) {
      std::cerr << " Error in opening image file\n";
      exit(1);
    }
    std::uint64_t rows, col;
    file >> rows >> col;

    if (file.eof()) {
      std::cerr << "File doesn't contain rows and columns" << std::endl;
      exit(1);
    }
    auto k = 0;
    adduint64(rows, message);
    adduint64(col, message);
    xpublic.push_back(rows);
    xpublic.push_back(col);
    xsecret.push_back(rows);
    xsecret.push_back(col);
    while (k < rows * col) {
      std::uint64_t public_share, secret_share;
      file >> public_share;
      xpublic.push_back(public_share);
      file >> secret_share;
      xsecret.push_back(secret_share);
      if (file.eof()) {
        std::cerr << "File contains less number of elements" << std::endl;
        exit(1);
      }
        // std::cout << num << std::endl;
      adduint64(secret_share, message);
      k++;
      }
    if (k == rows * col) {
      std::uint64_t num;
      file >> num;
      if (!file.eof()) {
        std::cerr << "File contains more number of elements" << std::endl;
        exit(1);
        }
      }
    file.close();
  }
}


int main(int argc, char* argv[]) {

  auto options = parse_program_options(argc, argv);
  const auto localhost = "127.0.0.1";
  const auto num_parties = 3;
  int my_id = 1;
  std::cout << "my party id: " << my_id << "\n";

  MOTION::Communication::tcp_parties_config config;
  config.reserve(num_parties);
  for (std::size_t party_id = 0; party_id < num_parties; ++party_id) {
    config.push_back({localhost, 10000 + party_id});
  }
  MOTION::Communication::TCPSetupHelper helper(my_id, config);
  auto comm_layer = std::make_unique<MOTION::Communication::CommunicationLayer>(
      my_id, helper.setup_connections());

  auto logger = std::make_shared<MOTION::Logger>(my_id, boost::log::trivial::severity_level::trace);
  comm_layer->set_logger(logger);
  // comm_layer->register_fallback_message_handler(
  //     [](auto party_id) { return std::make_shared<TestMessageHandler>(); });
  comm_layer->start();
  std::vector<std::uint8_t> message1;
  std::vector<std::uint8_t> message2;

    // int mes_no ,int my_id ,std::vector<uint8_t>&message,const Options& options
  read_shares(1,1,message1,*options);
  read_shares(2,1,message2,*options);

  int r_party_id=2;
  
  std::cout<<"Message1 size: "<<message1.size()<<"\n";
  std::cout<<"Message2 size: "<<message2.size()<<"\n";

  comm_layer->send_message(r_party_id, message1);
  comm_layer->send_message(r_party_id, message2);

 comm_layer->register_fallback_message_handler(
      [](auto party_id) { return std::make_shared<TestMessageHandler>(); }); 
  sleep(30);

   if(flag==2)
   {std::cout<<"Message z size:"<<Z.size()<<"\n";
   std::cout<<"Last:\n";
   std::vector<std::uint8_t>mes1;
  for(int i=0;i<Z.size();i++)
  { 
     //to push zth members first and if condtion there to not push rows and columns twice
     auto temp=Z[i];
    //  std::cout<<"Temp "<<temp<<" ";
     adduint64(temp,mes1);
      if(i>1)
    { 
      //to send secret shares
      auto temp2=randomnum[i]; 
      // std::cout<<"Temp2 "<<temp2<<" ";
      adduint64(temp2,mes1);
    }
  }

  comm_layer->send_message(0,mes1);
  }
  comm_layer->shutdown();
}