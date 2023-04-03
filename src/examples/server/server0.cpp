//./bin/server0 --fn1 file_config_model0 --fn2 9

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

std::vector<std::uint64_t> R;
std::vector<std::uint64_t> wpublic, xpublic, wsecret, xsecret, bpublic, bsecret;
std::vector<std::uint64_t> randomnum;
std::vector<std::uint64_t> prod1;
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
    ("fn1", po::value<std::string>()->required(), "filename1")  
    ("fn2", po::value<std::size_t>()->required(), "filename2") 
    ("current-path", po::value<std::string>()->required(), "currentpath") 
    ("fractional-bits", po::value<std::size_t>()->required(), "Number of fractional bits") 
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
  options.path = vm["current-path"].as<std::string>();
  options.file2 = vm["fn2"].as<std::size_t>();
  options.fractional_bits = vm["fractional-bits"].as<std::size_t>();
  fractional_bits = options.fractional_bits;
  std::cout<<"Fractional bits: "<<fractional_bits<<std::endl;
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
    //a=w (256*784), b=x (784*1) 
    std::cout<<"In multiplicate, a[0]="<<a[0]<<" a[1]="<<a[1]<<" b[0]="<<b[0]<<" b[1]="<<b[1]<<std::endl;

    if(a[1]!=b[0])
      {
        std::cerr<<"Error during matrix multiplication. Number of columns in W is not equal to Number of rows in X.";
        exit(1);
      }
    auto b_begin = b.begin();
    advance(b_begin, 2);
    std::vector<std::uint64_t>z;
    z.push_back(a[0]); //256
    z.push_back(b[1]); //1

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
 
    //w2=wpublic
    std::vector<std::uint64_t>w2;
    w2.resize(wpublic.size(),0);
    for(int i=0;i<wpublic.size();i++)
    {
      w2[i]=wpublic[i];
      // std::cout<<"w2:"<<w2[i]<<"\n";
    }

    //prod1=wpublic*xpublic
    prod1=multiplicate(wpublic,xpublic);
    auto prod1_begin=prod1.begin();
    auto prod1_end=prod1.end();
    advance(prod1_begin, 2);
    std::cout<<"prod1 size: "<<prod1.size()<<"\n";


  //------------------------------------------------------------------------------------------------------------------
    
    //prod2=Delw * delx1
    std::vector<std::uint64_t>prod2= multiplicate(w2,xsecret);
    auto prod2_begin=prod2.begin();
    advance(prod2_begin, 2);
    std::cout<<"prod2 size: "<<prod2.size()<<"\n";

//--------------------------------------------------------------------------------------------------------

    //prod3=delw1 * Delx 
    std::vector<std::uint64_t>prod3= multiplicate(wsecret,xpublic);
    std::cout<<"prod3 size: "<<prod3.size()<<"\n";
    auto prod3_begin=prod3.begin();
    advance(prod3_begin, 2);
//--------------------------------------------------------------------------------------------------------------
    std::cout<<"prod1 size:"<<prod1.size()<<"  prod2 size:"<<prod2.size();
    //output(prod1)=prod1-prod2-prod3
    __gnu_parallel::transform(prod1_begin, prod1_end, prod2_begin, prod1_begin , std::minus{});
    __gnu_parallel::transform(prod1_begin, prod1_end, prod3_begin, prod1_begin , std::minus{});
    std::cout<<"After minus operations.\n";
//-------------------------------------------------------------------------------------------------------------
     

    std::vector<std::uint64_t>::iterator r_begin = R.begin();
    advance(r_begin,2);
    // for(int i=0;i<R.size();i++)
    //  {
    //    std::cout<<"R:"<<R[i]<<"\n";
    //  } 

   //output=Delx*Dely-Delx*dely-Dely*delx+R(from s2)
   //output=prod1+R(from s2)
   std::cout<<"Before plus operation\n";
    std::cout<<"prod1 size:"<<prod1.size()<<"  R size:"<<R.size()<<std::endl;
    __gnu_parallel::transform(prod1_begin, prod1_end, r_begin, prod1_begin , std::plus{});
  std::cout<<"After plus operation\n";
    for(int i=2;i<prod1.size();i++)
     {
       prod1[i] = MOTION::new_fixed_point::truncate(prod1[i], fractional_bits);
      //  std::cout<<"prod1:"<<prod1[i]<<"\n";
     }
    std::cout<<"After truncation\n";

//---------------------------------------------------------------------------------------------------------------
    
    randomnum.resize(prod1.size(),0);
    randomnum[0]=prod1[0];
    randomnum[1]=prod1[1];
    
    for(int i=2;i<prod1.size();i++)
    { 
      std::random_device rd;
      std::mt19937 gen(rd());
      auto temp=blah(gen);
      // std::cout<<"r:"<<temp<<"\n";
      randomnum[i]=temp;
    }
    std::cout<<"After generating random number.\n";

    auto random_begin = randomnum.begin();
    advance(random_begin,2 );
    

    //output=output+random(local secret share)
    __gnu_parallel::transform(prod1_begin, prod1_end, random_begin, prod1_begin , std::plus{});   
    std::cout<<"After prod1+random operation\n";
    flag++;

    //final output=prod1
     
     
}



class TestMessageHandler : public MOTION::Communication::MessageHandler {
  void received_message(std::size_t party_id, std::vector<std::uint8_t>&& message) override {
    std::cout << "Message received from party " << party_id << ":";
    int k = message.size() / 8;
    if(party_id==2)
    {
    std::cout <<"R Message size before converting to 64bit uint: "<<message.size() <<std::endl;
    if(message.size()<=0)
      {
        std::cerr<<"Empty message received from party "<<party_id<<std::endl;
        exit(1);
      }
    std::cout << "Received message after converting to 64bit uint is :\n";
    for (auto i = 0; i < k; ++i) {
      auto temp = getuint64(message, i);
      std::cout << temp << ",";
      R.push_back(temp);
    }
    std::cout<<"\n\n";
    if (R.size() == k) {
      operations();
      flag++;
      }
    }
    else if(party_id==1)
    { 
     std::vector<std::uint64_t>Final_public;
     std::vector<std::uint64_t>secretshare1;
     std::cout <<"After receiving from P1 , message size : "<< message.size() << "\n"; //should be 258
    
    //to push rows and column 
    for(int i=0;i<2;i++)
    {
      auto j=getuint64(message,i);
      Final_public.push_back(j);
      secretshare1.push_back(j);
    }

    for(int i=2;i<k-1;i=i+2)
    { 
      auto temp = getuint64(message, i);
      // std::cout <<"temp: "<< temp << " ";
      Final_public.push_back(temp);
      auto temp2=getuint64(message, i+1);
      // std::cout <<"temp2: " << temp2 << " ";
      secretshare1.push_back(temp2);
    }
     
      auto finalpublic_begin = Final_public.begin();
      auto finalpublic_end = Final_public.end();
      advance(finalpublic_begin,2);
      std::cout<<"Final public:"<<*finalpublic_begin<<"\n";


      auto prod1_begin=prod1.begin();
      advance(prod1_begin,2);

      
      auto secretshare1_begin = secretshare1.begin();
      advance(secretshare1_begin,2);

      
      auto randomnum_begin = randomnum.begin();
      auto randomnum_end = randomnum.end();
      advance(randomnum_begin,2);

      std::cout<<"Random num:"<<*randomnum_begin<<"\n";
 
      auto bpublic_begin=bpublic.begin();
      advance(bpublic_begin,2);
      
		  auto bsecret_begin=bsecret.begin();
      advance(bsecret_begin,2);
      
		//final public share=Final_public
    __gnu_parallel::transform(finalpublic_begin, finalpublic_end, prod1_begin, finalpublic_begin , std::plus{});   
    
    	//  __gnu_parallel::transform(finalpublic_begin, finalpublic_end, randomnum_begin, finalpublic_begin , std::minus{});
      //   __gnu_parallel::transform(finalpublic_begin, finalpublic_end, secretshare1_begin, finalpublic_begin , std::minus{});
    //public shares for w.x + public shares of bias b
    __gnu_parallel::transform(finalpublic_begin, finalpublic_end, bpublic_begin, finalpublic_begin , std::plus{});  
	  
    //random num = random num+secret share of bias
    //final secret share=randomnum
	   __gnu_parallel::transform(randomnum_begin, randomnum_end, bsecret_begin, randomnum_begin , std::plus{});


   
  //   std::cout<<"Final output:- "<<*finalpublic_begin<<"\n";
  //  std::cout<<"Final output :-"<<"\n";
  //  for(int i=2;i<Final_public.size();i++)
  //  {
  //    auto temp =
  //         MOTION::new_fixed_point::decode<std::uint64_t, float>(Final_public[i],13);
  //    std::cout<<i<<". "<<temp<<"\n";
  //  }

   std::string basedir = getenv("BASE_DIR");
   std::string filename = basedir + "/build_debwithrelinfo_gcc";
   std::string totalpath = filename + "/server0/" + "outputshare_0";

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
    
    //~/IUDX/iudx-MOTION2NX/build_debwithrelinfo_gcc/file_config_model0
    std::string fullpath = options.path;
    fullpath += "/"+name;
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
    exit(1);
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
  int my_id = 0;
  std::cout << "My party id: " << my_id << "\n";

  MOTION::Communication::tcp_parties_config config;
  config.reserve(num_parties);
  for (std::size_t party_id = 0; party_id < num_parties; ++party_id) {
    config.push_back({localhost, 10000 + party_id});
    int l = 10000 + party_id;
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
  
  read_shares(1,0,message1,*options); //Weights and bias shares
  read_shares(2,0,message2,*options); //Image shares

  int r_party_id=2;

  std::cout<<"Weights and Bias shares size: "<<message1.size()<<"\n";
  std::cout<<"Image shares size: "<<message2.size()<<"\n";
  comm_layer->send_message(r_party_id, message1);
  comm_layer->send_message(r_party_id, message2);
  comm_layer->register_fallback_message_handler(
      [](auto party_id) { return std::make_shared<TestMessageHandler>(); });
  sleep(30);

  if(flag==2)
  {
  std::cout<<"Message z size: "<<prod1.size()<<"\n";
  std::cout<<"Last:\n";
  std::vector<std::uint8_t>mes0;
  for(int i=0;i<prod1.size();i++)
  {  
    auto temp=prod1[i];
    adduint64(temp, mes0);
  }
  comm_layer->send_message(1,mes0);
  }
  comm_layer->shutdown();
}