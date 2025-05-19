/*
./bin/server1_cnn --WB_file file_config_model1 --input_file remote_image_shares  --party
0,127.0.0.1,4009 --party 1,127.0.0.1,4010 --helper_node 127.0.0.1,4011 --current-path
${BASE_DIR}/build_debwithrelinfo_gcc --layer-id 1 --fractional-bits 13
*/
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
#include "utility/linear_algebra.h"
#include "utility/new_fixed_point.h"

using namespace std::chrono;

std::vector<std::uint64_t> Z;
std::vector<std::uint64_t> wpublic, xpublic, wsecret, xsecret, bpublic, bsecret;
std::vector<std::uint64_t> randomnum, prod1;
int operations_done_flag = 0;
bool helpernode_ready_flag = false;
std::uint64_t fractional_bits;
std::uint64_t conv_kernels, conv_channels, conv_rows, conv_cols;
std::uint64_t pads[4], strides[2];
std::uint64_t image_rows, image_cols, image_channels;
std::uint64_t output_rows, output_columns, output_chnls;
std::uint64_t b_rows, b_cols;

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
    file2 << "Convolution layer at party 1: \n";
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

std::vector<std::uint64_t> convolution(std::vector<std::uint64_t> input, std::vector<std::uint64_t> weights, std::uint64_t kernels, std::uint64_t channels,
                 std::uint64_t rows, std::uint64_t cols, std::uint64_t pads[], std::uint64_t strides[], std::uint64_t img_rows,std::uint64_t img_cols) {
  
  std::vector<std::vector<std::uint64_t>> kernel_segments;


       std::cout<<"conv_kernels: "<<conv_kernels<<"\n";
       std::cout<<"conv_channels: "<<conv_channels<<"\n";
       std::cout<<"conv_rows: "<<conv_rows<<"\n";
       std::cout<<"conv_cols: "<<conv_cols<<"\n";
       std::cout<<"pads: "<<pads[0]<<" "<<pads[1]<<" "<<pads[2]<<" "<<pads[3]<<"\n";
       std::cout<<"strides: "<<strides[0]<<" "<<strides[1]<<"\n";

       std::cout<<"image_channels: "<<image_channels<<"\n";
       std::cout<<"image_rows: "<<image_rows<<"\n";
       std::cout<<"image_cols: "<<image_cols<<"\n";

      //   for(int i=0;i<input.size();i++)
      //  {
      //   std::cout<<input[i]<<" ";
      //  }
      //   std::cout<<"***************\n";

      //  for(int i=0;i<weights.size();i++)
      //  {
      //   std::cout<<weights[i]<<" ";
      //  }
      //   std::cout<<"\n";
      //   std::cout<<"***************\n";

       


  int temp = 0;
  for (int i = 0; i < kernels; i++) {
    auto first = weights.begin() + temp;
    temp += channels * rows * cols;
    auto last = weights.begin() + temp;
    std::vector<std::uint64_t> kernel(first, last);
    kernel_segments.push_back(kernel);
  }

  std::cout << "kernel_segments size : " << kernel_segments.size() << "\n";

  std::vector<std::uint64_t> image;
  for (int c = 0; c < channels; c++) {
    for (int i = 0; i < pads[0]; i++) {
      for (int j = 0; j < img_cols + pads[1] + pads[3]; j++) image.push_back(0);
    }

    for (int i = 0; i < img_rows; i++) {
      for (int j = 0; j < pads[1]; j++) image.push_back(0);
      for (int j = 0; j < img_cols; j++)
        image.push_back(input[c * img_rows * img_cols + i * img_cols + j]);
      for (int j = 0; j < pads[3]; j++) image.push_back(0);
    }

    for (int i = 0; i < pads[2]; i++) {
      for (int j = 0; j < img_cols + pads[1] + pads[3]; j++) image.push_back(0);
    }
  }
  img_rows += pads[0] + pads[2];
  img_cols += pads[1] + pads[3];

  output_chnls = kernels;
  output_rows = (img_rows - rows + strides[0]) / strides[0];
  output_columns = (img_cols - cols + strides[1]) / strides[1];

  std::vector<std::vector<std::uint64_t>> image_segments(output_rows * output_columns,
                                          std::vector<std::uint64_t>(channels * rows * cols));

  for (unsigned int i = 0; i < output_rows; i++) {
    for (unsigned int j = 0; j < output_columns; j++) {
      unsigned int row_start = i * strides[0];
      unsigned int col_start = j * strides[1];
      for (unsigned int k = 0; k < channels; k++) {
        for (unsigned int l = 0; l < rows * cols; l++) {
          unsigned int row = row_start + l / cols;
          unsigned int col = col_start + l % cols;

          // std::cout << "image_segments[" << (i * output_columns + j) << " " << (k * rows * cols + l)
          //           << "]\n";

          // std::cout << "image[" << (k * img_rows * img_cols + row * img_cols + col) << "]\n";

          image_segments[i * output_columns + j][k * rows * cols + l] =
              image[k * img_rows * img_cols + row * img_cols + col];
        }
      }
    }
  }
  
  std::cout << "kernel_segments[k].size(): " << kernel_segments[0].size() << "\n";
  std::vector<std::uint64_t> output(kernels * output_rows * output_columns);
  int j = 0;
  for (int k = 0; k < kernels; k++) {
    for (int i = 0; i < output_rows * output_columns; i++) {
      output[j] = (MOTION::matrix_multiply(1, kernel_segments[k].size(), 1, kernel_segments[k],
                                           image_segments[i]))[0];
      // std::cout << output[j] << " ";
      j++;
    }
    // std::cout << "\n";
  }

 return output;

}


void operations()
{  

//---------------------------------------------------------------------------------------------------------  

    //prod1=Delw * delx1
    //std::vector<std::uint64_t>prod1=multiplicate(wpublic,xsecret);
    prod1=convolution(xsecret,wpublic,conv_kernels,image_channels,conv_rows,conv_cols,pads,strides,image_rows,image_cols);
    
    // std::cout<<"prod1 : "<<"\n";
    // for(int i=0;i<prod1.size();i++)
    //  {
    //    std::cout<<prod1[i]<<" ";
    //  }  
    //  std::cout<<"\n";

  //----------------------------------------------------------------------------------------------------------

     //prod2=Dely * delx1
    //std::vector<std::uint64_t>prod2=multiplicate(wsecret,xpublic);
    std::vector<std::uint64_t>prod2=convolution(xpublic,wsecret,conv_kernels,image_channels,conv_rows,conv_cols,pads,strides,image_rows,image_cols);
    
    // std::cout<<"prod2 : "<<"\n";
    // for(int i=0;i<prod2.size();i++)
    //  {
    //    std::cout<<prod2[i]<<" ";
    //  }  
    //  std::cout<<"\n";


//---------------------------------------------------------------------------------------------------------------------------

  
   //z=z-Delw*delx
   //z=z-prod1
    __gnu_parallel::transform(Z.begin(), Z.end(), prod1.begin(), Z.begin() , std::minus{});

    //z=z-Delw*delx-Delx*delw
    //z=z-prod1-prod2
     __gnu_parallel::transform(Z.begin(), Z.end(), prod2.begin(), Z.begin() , std::minus{});
    
    //     std::cout<<"Z after minusing both products : "<<"\n";
    // for(int i=0;i<Z.size();i++)
    //  {
    //    std::cout<<Z[i]<<" ";
    //  }  
    //  std::cout<<"\n";

    for(int i=0;i<Z.size();i++)
     {
       Z[i] = MOTION::new_fixed_point::truncate(Z[i], fractional_bits);
     }  

   
//--------------------------------------------------------------------------------------------------------------------
    
   //randomnum is server_i secret share 
    randomnum.resize(prod1.size(),0);
    for(int i=0;i<prod1.size();i++)
    { 
      std::random_device rd;
      std::mt19937 gen(rd());
      auto temp=RandomNumGenerator(gen);
      randomnum[i]=temp;
      //randomnum[i]=0;
    }

    //z=z+random number(local secret share)
    __gnu_parallel::transform(Z.begin(), Z.end(), randomnum.begin(), Z.begin() , std::plus{});     
    operations_done_flag++;
    //final output=Z
}

class TestMessageHandler : public MOTION::Communication::MessageHandler {
  void received_message(std::size_t party_id, std::vector<std::uint8_t>&& message) override {
    std::cout << "Message received from party " << party_id << "\n";
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
      std::cout<<"Z message : \n";
    for (auto i = 3; i < k; ++i) {
      auto temp = getuint64(message, i);
      // std::cout<<temp<<" ";
      Z.push_back(temp);
    }
    // std::cout<<"\n";
    if (Z.size() == k-3) {
      operations();
      operations_done_flag++;
    }
    }
  else if(party_id==0)
  { //wait till operation is not done 
    while(operations_done_flag!=2)
      {
        std::cout<<"@";
        boost::this_thread::sleep_for(boost::chrono::milliseconds(100));
      }

    std::cout <<"\nReceived message from Party 0 of size "<< message.size() << "\n";

    std::vector<std::uint64_t>Final_public;
    for(int i=0;i<k;i++)
    { 
      auto temp = getuint64(message, i);
      Final_public.push_back(temp);
      int kernel_index = (i*conv_kernels)/k;
       
      //std::cout<<Final_public[i]<<" "<<randomnum[i]<<"\n"; 
      Final_public[i]+=bpublic[kernel_index];
      randomnum[i]+=bsecret[kernel_index];
    }




    //finalpublicshare = publicshare_0 + publicshare_1
    __gnu_parallel::transform(Final_public.begin(),Final_public.end(), Z.begin(), Final_public.begin() , std::plus{});   

    std::string basedir = getenv("BASE_DIR");
    std::string filename = basedir + "/build_debwithrelinfo_gcc";
    std::string totalpath = filename + "/server1/" + "cnn_outputshare_1";
    std::string path_next_layer = filename + "/server1/" + "outputshare_1";

    std::ofstream indata;
    indata.open(totalpath,std::ios_base::out);
    assert(indata);

    //writing only the first row in cnn_outputshare_1
    indata<<output_chnls<<" "<<output_rows<<" "<<output_columns<<"\n";
    indata.close();
    
    //writing public and secret shares and rows , cols in outputshare_1
    indata.open(path_next_layer);
   assert(indata);
   indata<<output_chnls*output_rows*output_columns<<" "<<1<<"\n";
  
   std::cout << output_chnls * output_rows * output_columns << " Output dimensions" << std::endl;

   for(int i=0;i<Final_public.size();i++)
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
    config_file<<path_next_layer;
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

    try{
    file >> conv_kernels >>conv_channels >> conv_rows >> conv_cols;
     for (int i=0; i<4; i++)
       {
        std::uint64_t temp;
        file>>temp;
        // std::cout<<temp<<" ";
        pads[i]=temp;
       }
    for (int i=0; i<2; i++)
       {
        std::uint64_t temp;
        file>>temp;
        // std::cout<<temp<<" ";
        strides[i]=temp;
       }
    std::uint64_t message_type = 1;
    adduint64(message_type,message);
    adduint64(conv_kernels, message);
    adduint64(conv_channels,message);
    adduint64(conv_rows, message);
    adduint64(conv_cols, message);
    for(int i=0;i<4;i++)
    {
      adduint64(pads[i], message);
    }
        for(int i=0;i<2;i++)
    {
      adduint64(strides[i], message);
    }
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
    while (k < conv_rows * conv_cols*conv_channels*conv_kernels) {
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
      //adding only weights secretshare_i in message 
      adduint64(secret_share, message);
      k++;
    }
    std::cout<<"Number of weight shares read: "<<k<<"\n"; 
    if (k == conv_rows * conv_cols*conv_channels*conv_kernels) {
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
    file >> b_rows >> b_cols;
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
    while (j < b_rows * b_cols) {
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
    if (j == b_rows * b_cols) {
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

    try{
    file >>image_channels>>image_rows>>image_cols;
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

    std::uint64_t message_type = 2;
    adduint64(message_type,message);
    adduint64(image_channels, message);
    adduint64(image_rows, message);
    adduint64(image_cols,message);
    while (k < image_rows * image_cols * image_channels) {
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
      //adding only image secretshares_i in message 
      adduint64(secret_share, message);
      k++;
      }
    if (k == image_rows * image_cols*image_channels) {
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
  auto startComm = high_resolution_clock::now();
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
    auto endComm = high_resolution_clock::now();


    std::vector<std::uint8_t> message_w, message_i;
    std::vector<std::uint8_t> started{(std::uint8_t)1};
    std::cout<<"Sending the start connection message to the helper node.\n";
    auto startAckMessage = high_resolution_clock::now();
    try{
    comm_layer->send_message(helpernode_id, started);
    }
    catch (std::runtime_error& e) {
        std::cerr << "Error occurred while sending the start message to helper node: " << e.what() << "\n";
        return EXIT_FAILURE;
    }

    comm_layer->register_fallback_message_handler(
          [](auto party_id) { return std::make_shared<TestMessageHandler>(); }); 

    
    read_shares(1,1,message_w,*options); //Weight shares
    read_shares(2,1,message_i,*options); //Image shares

    std::cout << "x: " << xpublic.size() << " " << xsecret.size() << std::endl;
    std::cout << "w: " << wpublic.size() << " " << wsecret.size() << std::endl;
    std::cout << "b: " << bpublic.size() << " " << bsecret.size() << std::endl; 

    //     std::cout<<"Weight shares size: "<<message_w.size()<<"\n";
    // std::cout<<"Input shares size: "<<message_i.size()<<"\n";

    //Waiting to receive the acknowledgement from helpernode
    while(!helpernode_ready_flag)
      {
        std::cout<<"#";
        boost::this_thread::sleep_for(boost::chrono::milliseconds(200));
      }
    auto endAckMessage = high_resolution_clock::now();

    const std::string baseDirectory = (std::string)std::getenv("BASE_DIR");
    std::string statFilePath = baseDirectory + "/build_debwithrelinfo_gcc/stats/ackStats1";
    std::ofstream statFilePathFile;
    statFilePathFile.open(statFilePath, std::ios_base::app);
    if (!statFilePathFile.is_open()) {
      std::cerr << "Error: Unable to open the file path.\n";
    }
    statFilePathFile << "Starting communication layer @ S1 for Convolution Layer: " << duration_cast<milliseconds>(endComm - startComm).count() << std::endl;
    statFilePathFile << "Helper message acknowledgement @ S1 for Convolution Layer: " << duration_cast<milliseconds>(endAckMessage - startAckMessage).count() << "\n" << std::endl;
    statFilePathFile.close();

    
    std::cout<<"Sending Weights shares to the helper node\n";
    try{
      comm_layer->send_message(helpernode_id, message_w);
    }
    catch (std::runtime_error& e) {
      std::cerr << "Error occurred while sending the weight shares to helper node: " << e.what() << "\n";
      return EXIT_FAILURE;
    }
    
        std::cout<<"Sending Image shares to the helper node\n";

    try{  
      comm_layer->send_message(helpernode_id, message_i);
    }
    catch (std::runtime_error& e) {
      std::cerr << "Error occurred while sending the input shares to helper node: " << e.what() << "\n";
      return EXIT_FAILURE;
    }

      //Waiting for the operations to complete. 
      while(operations_done_flag!=2)
        {
          std::cout<<"^";
          boost::this_thread::sleep_for(boost::chrono::milliseconds(100));
        }

       
    //creating message with publicshare_i 
      std::vector<std::uint8_t>mes1;
      for(int i=0;i<Z.size();i++)
      { 
        auto temp=Z[i];
        adduint64(temp,mes1);
      }
      std::cout<<"Sending Del_1 to the Party0\n";
      try{
      comm_layer->send_message(0,mes1);
      }
      catch (std::runtime_error& e) {
        std::cerr << "Error occurred while sending the output public share to Server-1: " << e.what() << "\n";
        return EXIT_FAILURE;
      }
      comm_layer->shutdown();

      testMemoryOccupied(WriteToFiles, 1, options->current_path);
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