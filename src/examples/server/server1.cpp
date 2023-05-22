//./bin/server1 --WB_file file_config_model1
#include <unistd.h>
#include <filesystem>
#include <fstream>
#include <random>
#include <regex>
#include <stdexcept>
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

#include <boost/chrono.hpp>
#include <boost/thread/thread.hpp>
#include <chrono>
#include <iostream>
#include <iterator>
#include <parallel/algorithm>
#include <vector>
#include "utility/new_fixed_point.h"
using namespace std::chrono;

std::vector<std::uint64_t> Z;  //
std::vector<std::uint64_t> wpublic, xpublic, wsecret, xsecret, bpublic, bsecret;
std::vector<std::uint64_t> randomnum;
int operations_done_flag = 0;
bool helpernode_ready_flag = false;
std::uint64_t fractional_bits;
namespace po = boost::program_options;

void testMemoryOccupied(int WriteToFiles, int my_id, std::string path) {
  int tSize = 0, resident = 0, share = 0;
  std::ifstream buffer("/proc/self/statm");
  buffer >> tSize >> resident >> share;
  buffer.close();

  long page_size_kb =
      sysconf(_SC_PAGE_SIZE) / 1024;  // in case x86-64 is configured to use 2MB pages
  double rss = resident * page_size_kb;
  std::cout << "RSS - " << rss << " kB\n";
  double shared_mem = share * page_size_kb;
  std::cout << "Shared Memory - " << shared_mem << " kB\n";
  std::cout << "Private Memory - " << rss - shared_mem << "kB\n";
  std::cout << std::endl;
  if (WriteToFiles == 1) {
    // Generate path for the AverageMemoryDetails file and MemoryDetails file
    std::string t1 = path + "/" + "AverageMemoryDetails" + std::to_string(my_id);
    std::string t2 = path + "/" + "MemoryDetails" + std::to_string(my_id);

    // Write to the AverageMemoryDetails files
    std::ofstream file1;
    file1.open(t1, std::ios_base::app);
    file1 << rss;
    file1 << "\n";
    file1.close();

    std::ofstream file2;
    file2.open(t2, std::ios_base::app);
    file2 << "Helper Node Multiplication layer : \n";
    file2 << "RSS - " << rss << " kB\n";
    file2 << "Shared Memory - " << shared_mem << " kB\n";
    file2 << "Private Memory - " << rss - shared_mem << "kB\n";
    file2.close();
  }
}

struct Options {
  std::string WB_file;
  std::string input_file;
  std::size_t layer_id;
  std::string current_path;
  MOTION::Communication::tcp_parties_config tcp_config;
  std::size_t fractional_bits;
};

std::optional<Options> parse_program_options(int argc, char* argv[]) {
  Options options;
  boost::program_options::options_description desc("Allowed options");
  // clang-format off
  desc.add_options()
    ("help,h", po::bool_switch()->default_value(false),"produce help message")
    ("layer-id", po::value<std::size_t>()->required(), "layer id")
    ("WB_file", po::value<std::string>()->required(), "Weights and Bias Filename")  
    ("input_file", po::value<std::string>()->required(), "Input File name") 
    ("party", po::value<std::vector<std::string>>()->multitoken(),
     "(party id, IP, port), e.g., --party 1,127.0.0.1,7777")
    ("helper_node", po::value<std::string>()->multitoken(),
     "(helpernode IP, port), e.g., --helper_node 127.0.0.1,7777") 
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

  options.WB_file = vm["WB_file"].as<std::string>();
  options.current_path = vm["current-path"].as<std::string>();
  options.input_file = vm["input_file"].as<std::string>();
  options.layer_id = vm["layer-id"].as<std::size_t>();
  options.fractional_bits = vm["fractional-bits"].as<std::size_t>();
  fractional_bits = options.fractional_bits;
  // clang-format on;
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
      throw std::invalid_argument("invalid party argument");
    }
    auto id = boost::lexical_cast<std::size_t>(match[1]);
    auto host = match[2];
    auto port = boost::lexical_cast<std::uint16_t>(match[3]);
    return {id, {host, port}};
  };

  const std::vector<std::string> party_infos = vm["party"].as<std::vector<std::string>>();
  if (party_infos.size() != 2) {
    std::cerr << "expecting two --party options\n";
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
std::uint64_t RandomNumGenerator(E &engine)
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
    // std::cout<<"In multiplicate, a[0]="<<a[0]<<" a[1]="<<a[1]<<" b[0]="<<b[0]<<" b[1]="<<b[1]<<std::endl;
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
     }  

   
//--------------------------------------------------------------------------------------------------------------------
  
    randomnum.resize(prod1.size(),0);
    randomnum[0]=prod1[0]; //256
    randomnum[1]=prod1[1]; //1
    
    for(int i=2;i<prod1.size();i++)
    { 
      std::random_device rd;
      std::mt19937 gen(rd());
      auto temp=RandomNumGenerator(gen);
      randomnum[i]=temp;
    }

    auto random_begin = randomnum.begin();
    advance(random_begin,2); 

    //z=z+random number(aka secret share)
    __gnu_parallel::transform(z_begin, z_end, random_begin, z_begin , std::plus{});     
    operations_done_flag++;
    //final output=Z
}

class TestMessageHandler : public MOTION::Communication::MessageHandler {
  void received_message(std::size_t party_id, std::vector<std::uint8_t>&& message) override {
  
    if(message.size()==1 && message[0]==(std::uint8_t)1)
      {
        std::cout<<"Inside the ack message block\n";
        if(party_id==2)
          {
            std::cout<<"\nHelper node has acknowledged receiving the start connection message.\n";
            helpernode_ready_flag = true;
            return;
          }
        else
          {
            std::cerr<<"Received the message \"1\" from unknown party "<<party_id<<std::endl;
            return;
          }
      }
    while(!helpernode_ready_flag)
      {
        std::cout<<".";
        boost::this_thread::sleep_for(boost::chrono::milliseconds(200));
      }
    int k = message.size() / 8;
    if(party_id==2)
    {
    std::cout << "(Z-R) received from helper node of size "<<message.size() <<std::endl;
    if(message.size()<=0)
      {
        std::cerr<<"Empty message received from party "<<party_id<<std::endl;
        exit(1);
      }
    for (auto i = 0; i < k; ++i) {
      auto temp = getuint64(message, i);
      Z.push_back(temp);
    }
    // std::cout<<"\n\n";
    if (Z.size() == k) {
      operations();
      operations_done_flag++;
    }
    }
  else if(party_id==0)
  { 
    while(operations_done_flag!=2)
      {
        std::cout<<".";
        boost::this_thread::sleep_for(boost::chrono::milliseconds(100));
      }
    std::cout << "\nDEL_C0 received from party " << party_id << "of size \n";
    std::vector<std::uint64_t>Final_public;
    std::cout <<"After receiving from Party 0 , message size is "<< message.size() << "\n"; 
    for(int i=0;i<k;i++)
    { 
      auto temp = getuint64(message, i);
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
  

    std::ofstream indata;
    indata.open(totalpath,std::ios_base::out);
    assert(indata);

    indata<<Final_public[0]<<" "<<Final_public[1]<<"\n";
    for(int i=2;i<Final_public.size();i++)
    {
      indata<<Final_public[i]<<" "<<randomnum[i]<<"\n";
    }
    indata.close();

       //Adding the path to the config file for using in the next layer
    std::string file_config_input = filename + "/file_config_input1";
    std::ofstream config_file;
    config_file.open(file_config_input,std::ios_base::out);
    if (!config_file.is_open()) {
        std::cerr << " Error in writing the file_config_input1 file\n";
        exit(1);
      }
    config_file<<totalpath;
    config_file.close();
  }    
} 
};

void read_shares(int choice,int my_id,std::vector<uint8_t>&message,const Options& options)
{ 
  std::string name=options.WB_file;
  if(choice==1)
  {
    std::ifstream content;
    std::cout<<"Reading the Weight and Bias shares\n";
    std::string fullpath = options.current_path;
    fullpath += "/"+name;
    content.open(fullpath);
    if (!content.is_open()) {
      std::cerr << " Error in opening the weights config file\n";
      exit(1);
    }
    std::string wpath,bpath;
    // Increment until it reaches the weights and bias files corresponding to the layer_id
    try{
    for(auto i=0;i<options.layer_id;i++)
      {
        content>>wpath;    
        content>>bpath; 
      }
    }
    catch (std::ifstream::failure e) {
      std::cerr << "Error while reading the weights and bias path from config file\n";
      exit(1);
    }

    std::cout<<"Weights path: "<<wpath<<"\nBias path: "<<bpath<<"\n";

    std::ifstream file(wpath);
    if (!file) {
      std::cerr << " Error in opening the weights file\n";
      exit(1);
    }
    std::uint64_t rows, col;
    try{
    file >> rows >> col;
    }
    catch (std::ifstream::failure e) {
      std::cerr << "Error while reading rows and columns from weight shares file.\n";
      exit(1);
    }

    if (file.eof()) {
      std::cerr << "Weights File doesn't contain the shares" << std::endl;
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
      try{
      file >> public_share;
      wpublic.push_back(public_share);
      file >> secret_share;
      wsecret.push_back(secret_share);
      }
      catch (std::ifstream::failure e) {
      std::cerr << "Error while reading the weight shares.\n";
      exit(1);
      }
      if (file.eof()) {
        std::cerr << "Weight shares file contains less number of elements" << std::endl;
        exit(1);
      }
      adduint64(secret_share, message);
      k++;
    }
    std::cout<<"Number of weight shares read: "<<k<<"\n"; 
    if (k == rows * col) {
      std::uint64_t num;
      file >> num;
      if (!file.eof()) {
        std::cerr << "Weight shares file contains more number of elements" << std::endl;
        exit(1);
      }
    }
    file.close();

    file.open(bpath);
    if (!file) {
    std::cerr << " Error in opening bias file\n";
    exit(1);
    }
    try{
    file >> rows >> col;
    }
    catch (std::ifstream::failure e) {
      std::cerr << "Error while reading rows and columns from bias shares file.\n";
      exit(1);
    }
    if (file.eof()) {
      std::cerr << "Bias shares file doesn't contain rows and columns" << std::endl;
      exit(1);
    }
       
    auto j=0;
    bpublic.push_back(rows);
    bpublic.push_back(col);
    bsecret.push_back(rows);
    bsecret.push_back(col);
    while (j < rows * col) {
      std::uint64_t public_share, secret_share;
      try{
      file >> public_share;
      bpublic.push_back(public_share);
      file >> secret_share;
      bsecret.push_back(secret_share);
      }
      catch (std::ifstream::failure e) {
      std::cerr << "Error while reading bias shares.\n";
      exit(1);
      }
      if (file.eof()) {
        std::cerr << "Bias shares file contains less number of elements" << std::endl;
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
  else if(choice==2)
  {
    std::string fullpath = options.current_path;
    if (options.layer_id == 1) {
      fullpath+= "/server" + std::to_string(my_id) + "/Image_shares/" + options.input_file;
    } 
    else if (options.layer_id > 1) {
      // Path to outputshare_1 inside server 1
      fullpath+= "/server" + std::to_string(my_id) + "/" + options.input_file + "_" + std::to_string(my_id);
    }     
    std::cout<<"Input share file: "<<fullpath<<std::endl;
    std::cout<<"Reading the input shares\n";

    std::ifstream file(fullpath);

    if (!file) {
      std::cerr << "Error in opening input file at "<<fullpath<<"\n";
      exit(1);
    }
    std::uint64_t rows, col;
    try{
    file >> rows >> col;
    }
    catch (std::ifstream::failure e) {
      std::cerr << "Error while reading rows and columns from input shares file.\n";
      exit(1);
    }
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
      try{
      file >> public_share;
      xpublic.push_back(public_share);
      file >> secret_share;
      xsecret.push_back(secret_share);
      }
      catch (std::ifstream::failure e) {
      std::cerr << "Error while reading the input shares.\n";
      exit(1);
      }
      if (file.eof()) {
        std::cerr << "File contains less number of elements" << std::endl;
        exit(1);
      }
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

  auto start = high_resolution_clock::now();
  auto options = parse_program_options(argc, argv);
  if (!options.has_value()) {
    std::cerr<<"No options given.\n";
    return EXIT_FAILURE;
  }

  int my_id = 1, helpernode_id=2;
  int WriteToFiles = 1;
  std::cout << "My party id: " << my_id << "\n";
  std::unique_ptr<MOTION::Communication::CommunicationLayer> comm_layer;
  std::shared_ptr<MOTION::Logger> logger;
  try{
    try{
      MOTION::Communication::TCPSetupHelper helper(my_id, options->tcp_config);
      comm_layer = std::make_unique<MOTION::Communication::CommunicationLayer>(
          my_id, helper.setup_connections());
    }
    catch (std::runtime_error& e) {
        std::cerr << "Error occurred during connection setup: " << e.what() << "\n";
        return EXIT_FAILURE;
    }
    // try{
    //   logger = std::make_shared<MOTION::Logger>(my_id, boost::log::trivial::severity_level::trace);
    //   comm_layer->set_logger(logger);
    // }
    // catch (std::runtime_error& e) {
    //     std::cerr << "Error occurred during logger setup: " << e.what() << "\n";
    //     return EXIT_FAILURE;
    // }
    try{
        comm_layer->start();
    }
    catch (std::runtime_error& e) {
        std::cerr << "Error occurred while starting the communication: " << e.what() << "\n";
        return EXIT_FAILURE;
    }
    std::vector<std::uint8_t> message1, message2;
    std::vector<std::uint8_t> started{(std::uint8_t)1};
    std::cout<<"Sending the start connection message to the helper node.\n";
    try{
    comm_layer->send_message(helpernode_id, started);
    }
    catch (std::runtime_error& e) {
        std::cerr << "Error occurred while sending the start message to helper node: " << e.what() << "\n";
        return EXIT_FAILURE;
    }

    comm_layer->register_fallback_message_handler(
          [](auto party_id) { return std::make_shared<TestMessageHandler>(); }); 

    
    read_shares(1,1,message1,*options);
    read_shares(2,1,message2,*options);

    //Waiting to receive the acknowledgement from helpernode
    while(!helpernode_ready_flag)
      {
        std::cout<<".";
        boost::this_thread::sleep_for(boost::chrono::milliseconds(200));
      }

    std::cout<<"Weights shares size: "<<message1.size()<<"\n";
    std::cout<<"Input shares size: "<<message2.size()<<"\n";
    
    std::cout<<"Sending Weights shares to the helper node\n";
    try{
      comm_layer->send_message(helpernode_id, message1);
    }
    catch (std::runtime_error& e) {
      std::cerr << "Error occurred while sending the weight shares to helper node: " << e.what() << "\n";
      return EXIT_FAILURE;
    }

    try{  
      comm_layer->send_message(helpernode_id, message2);
    }
    catch (std::runtime_error& e) {
      std::cerr << "Error occurred while sending the input shares to helper node: " << e.what() << "\n";
      return EXIT_FAILURE;
    }


      testMemoryOccupied(WriteToFiles,1, options->current_path);
      //Waiting for the operations to complete. 
      while(operations_done_flag!=2)
        {
          std::cout<<".";
          boost::this_thread::sleep_for(boost::chrono::milliseconds(100));
        }
      std::vector<std::uint8_t>mes1;
      for(int i=0;i<Z.size();i++)
      { 
        //to push zth members first and condition checking is done to not push rows and columns twice
        auto temp=Z[i];
        adduint64(temp,mes1);
        if(i>1)
        { 
          //to send secret shares
          auto temp2=randomnum[i]; 
          adduint64(temp2,mes1);
        }
      }
      std::cout<<"Sending DEL_C1 to the Party0\n";
      try{
      comm_layer->send_message(0,mes1);
      }
      catch (std::runtime_error& e) {
        std::cerr << "Error occurred while sending the output public share to Server-1: " << e.what() << "\n";
        return EXIT_FAILURE;
      }
      comm_layer->shutdown();
      auto stop = high_resolution_clock::now();
      auto duration = duration_cast<milliseconds>(stop - start);
      std::string t1 = options->current_path + "/" + "AverageTimeDetails1";
      std::string t2 = options->current_path + "/" + "MemoryDetails1";

      std::ofstream file2;
      file2.open(t2, std::ios_base::app);
      if(!file2.is_open())
      {
        std::cerr<<"Unable to open the MemoryDetails file.\n";
      }
    else{
      file2 << "Execution time - " << duration.count() << "msec";
      file2 << "\n";
      }
      file2.close();
      std::ofstream file1;
      file1.open(t1, std::ios_base::app);
      if(!file1.is_open())
      {
        std::cerr<<"Unable to open the AverageTimeDetails file.\n";
      }
    else
      {
      file1 << duration.count();
      file1 << "\n";
      }
      file1.close();
  }
  catch (std::runtime_error& e) {
    std::cerr << "ERROR OCCURRED: " << e.what() << "\n";
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}