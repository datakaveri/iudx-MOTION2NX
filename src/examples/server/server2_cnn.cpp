/*
./bin/server2_cnn --party 0,127.0.0.1,4009 --party 1,127.0.0.1,4010 --helper_node 127.0.0.1,4011
*/
#include <bits/stdc++.h>
#include <chrono>
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
#include "utility/linear_algebra.h"
#include "utility/new_fixed_point.h"

std::vector<std::uint64_t> x0, w0;
std::vector<std::uint64_t> x1, w1;
int c1 = 0, c2 = 0, c3 = 0, c4 = 0;
bool operations_done_flag = false, server0_ready_flag = false, server1_ready_flag = false;
std::uint64_t conv_rows = 0, conv_cols = 0, image_rows = 0, image_cols = 0;
std::uint64_t conv_kernels = 0, conv_channels = 0, image_channels = 0;
std::uint64_t pads[4], strides[2];
std::vector<std::uint8_t> msg_Z, msg_R;
int flag = 0;
namespace po = boost::program_options;

struct Options {
  std::size_t my_id;
  std::uint16_t my_port;
  std::string current_path;
  MOTION::Communication::tcp_parties_config tcp_config;
};

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
  options.current_path = vm["current-path"].as<std::string>();

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

        for(int i=0;i<input.size();i++)
       {
        // std::cout<<input[i]<<" ";
       }
        std::cout<<"***************\n";

       for(int i=0;i<weights.size();i++)
       {
        // std::cout<<weights[i]<<" ";
       }
        std::cout<<"\n";
        std::cout<<"***************\n";

       


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

  uint64_t output_chnls = kernels;
  uint64_t output_rows = (img_rows - rows + strides[0]) / strides[0];
  uint64_t output_columns = (img_cols - cols + strides[1]) / strides[1];

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
  
  // std::cout << "kernel_segments[k].size(): " << kernel_segments[0].size() << "\n";
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

  
//--------------------------------------------------------------------------------------------------

  //delx0=delx0+delx1
  __gnu_parallel::transform(x0.begin(), x0.end(), x1.begin(), x0.begin() , std::plus{});
  
  //delw0=delw0+delw1
  __gnu_parallel::transform(w0.begin(), w0.end(), w1.begin(), w0.begin() , std::plus{});

  std::vector<std::uint64_t>z=convolution(x0,w0,conv_kernels,image_channels,conv_rows,conv_cols,pads,strides,image_rows,image_cols);

  //-----------------------------------------------------------------------------------------------
  uint64_t output_chnls = conv_kernels;
  std::uint64_t output_rows = (image_rows - conv_rows + pads[0]+pads[2]+strides[0]) / strides[0];
  std::uint64_t output_cols = (image_cols - conv_cols + pads[1]+pads[3]+strides[1]) / strides[1];

  std::vector<std::uint64_t>r;
  r.resize(output_chnls*output_rows*output_cols,0);
  
  for(int i=0;i<z.size();i++)
  { 
    std::random_device rd;
    std::mt19937 gen(rd());
    auto temp=RandomNumGenerator(gen);
    r[i]=temp;
    //r[i]=0;
  }

  // //--------------------------------------------------------------------------------------------------
  
  // //z=z-r

  __gnu_parallel::transform(z.begin(), z.end(), r.begin(), z.begin() , std::minus{});
   

  //     std::cout<<"r:\n";
  //  for(int i=0;i<r.size();i++)
  // { 
  //    std::cout<<r[i]<<" ";
  // }
  // std::cout<<"\n";


  //  std::cout<<"z-r:\n";
  //  for(int i=0;i<z.size();i++)
  // { 
  //    std::cout<<z[i]<<" ";
  // }



 //********************************

  //Final output:-
   std::cout<<"Final Output :- \n";
  adduint64(output_chnls,msg_Z);
  adduint64(output_rows,msg_Z);
  adduint64(output_cols,msg_Z);

  adduint64(output_chnls,msg_R);
  adduint64(output_rows,msg_R);
  adduint64(output_cols,msg_R);


  for(int i=0;i<z.size();i++)
  {  
  // std::cout<<z[i]+r[i]<<" ";
  adduint64(z[i],msg_Z); //z=z-r  server1
  adduint64(r[i],msg_R); //r  server0
  }
  std::cout<<"\n";

  operations_done_flag = true; 
}

class TestMessageHandler : public MOTION::Communication::MessageHandler {
  void received_message(std::size_t party_id, std::vector<std::uint8_t>&&message) {
    //(w0 -> channels*kernels*w_rows*w_cols, x0 ->channels*x_rows*x_cols server0)
    //(w1 -> channels*kernels*w_rows*w_cols ,x1->channels*x_rows*x_cols server1)
    int size_msg=message.size()/8;
    std::cout<<"size_msg : " << size_msg<<"\n";
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
    
    // if msg_type is 1 , then its receiving weights and if msg_type is 2 ,its receiving image 
    std::uint64_t msg_type=getuint64(message,0);
    
    if(party_id==0 && msg_type==1)
    {  
      //weights from server0
      conv_kernels=getuint64(message,1);
      conv_channels=getuint64(message,2);
      conv_rows=getuint64(message,3);
      conv_cols=getuint64(message,4);
      std::cout << conv_kernels << " " << conv_channels << " " << conv_rows << " " << conv_cols << std::endl;
      for (int i=5; i<9; i++)
       {
        pads[i-5]=getuint64(message,i);
       }
    for (int i=9; i<11; i++)
       {
        strides[i-9]=getuint64(message,i);
       }
       
       std::cout<<"******************************"<<"\n";
       std::cout<<"party_id: "<<party_id<<" message type: "<<msg_type<<"\n";
       std::cout<<"conv_kernels: "<<conv_kernels<<"\n";
       std::cout<<"conv_channels: "<<conv_channels<<"\n";
       std::cout<<"conv_rows: "<<conv_rows<<"\n";
       std::cout<<"conv_cols: "<<conv_cols<<"\n";
       std::cout<<"pads: "<<pads[0]<<" "<<pads[1]<<" "<<pads[2]<<" "<<pads[3]<<"\n";
       std::cout<<"strides: "<<strides[0]<<" "<<strides[1]<<"\n";
       std::cout<<"******************************"<<"\n";

       for(int i=11;i<(11+(conv_kernels*conv_channels*conv_rows*conv_cols));i++)
       {
        auto temp = getuint64(message,i);
        w0.push_back(temp);
       }
    }
    else if(party_id==1 && msg_type==1)
    {
      //weights from server1
      conv_kernels=getuint64(message,1);
      conv_channels=getuint64(message,2);
      conv_rows=getuint64(message,3);
      conv_cols=getuint64(message,4);
      for (int i=5; i<9; i++)
       {
        pads[i-9]=getuint64(message,i);
       }
    for (int i=9; i<11; i++)
       {
        strides[i-9]=getuint64(message,i);
       }

       std::cout<<"&&&&&&&&&&&&&&&&&&&"<<"\n";
       std::cout<<"party_id: "<<party_id<<" message type: "<<msg_type<<"\n";
       std::cout<<"conv_kernels: "<<conv_kernels<<"\n";
       std::cout<<"conv_channels: "<<conv_channels<<"\n";
       std::cout<<"conv_rows: "<<conv_rows<<"\n";
       std::cout<<"conv_cols: "<<conv_cols<<"\n";
       std::cout<<"pads: "<<pads[0]<<" "<<pads[1]<<" "<<pads[2]<<" "<<pads[3]<<"\n";
       std::cout<<"strides: "<<strides[0]<<" "<<strides[1]<<"\n";
       std::cout<<"&&&&&&&&&&&&&&&&&&&&&&"<<"\n";

       for(int i=11;i<(11+(conv_kernels*conv_channels*conv_rows*conv_cols));i++)
       {
        auto temp = getuint64(message,i);
        w1.push_back(temp);
       }

      //   for(int i=0;i<w1.size();i++)
      //  {
      //   std::cout<<w1[i]<<" ";
      //  }
      //   std::cout<<"\n";
    }
    else if(party_id==0 && msg_type==2)
    {
      //images from server0
      image_channels=getuint64(message,1);
      image_rows=getuint64(message,2);
      image_cols=getuint64(message,3);
      

       std::cout<<"^^^^^^^^^^^^^^^^^^^^^^^^^^^^^"<<"\n";
       std::cout<<"party_id: "<<party_id<<" message type: "<<msg_type<<"\n";
       std::cout<<"image_channels: "<<image_channels<<"\n";
       std::cout<<"image_rows: "<<image_rows<<"\n";
       std::cout<<"image_cols: "<<image_cols<<"\n";
       std::cout<<"^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^"<<"\n";
      

       for(int i=4;i<(4+(image_channels*image_rows*image_cols));i++)
       {
        auto temp = getuint64(message,i);
        x0.push_back(temp);
       }

      //  for(int i=0;i<x0.size();i++)
      //  {
      //   std::cout<<x0[i]<<" ";
      //  }
      //   std::cout<<"\n";
    }
      else if(party_id==1 && msg_type==2)
    {
      //images from server1
      image_channels=getuint64(message,1);
      image_rows=getuint64(message,2);
      image_cols=getuint64(message,3);

       std::cout<<"$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$"<<"\n";
       std::cout<<"party_id: "<<party_id<<" message type: "<<msg_type<<"\n";
       std::cout<<"image_channels: "<<image_channels<<"\n";
       std::cout<<"image_rows: "<<image_rows<<"\n";
       std::cout<<"image_cols: "<<image_cols<<"\n";
       std::cout<<"$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$"<<"\n";

       for(int i=4;i<(4+(image_channels*image_rows*image_cols));i++)
       {
        auto temp = getuint64(message,i);
        x1.push_back(temp);
       }

    }

    flag++;
    //if flag = 4 , received weights , images secret shares from server0 and server1
    if(flag==4)
    { 
    operations();
    }
  }
};


int main(int argc, char* argv[]) {
  std::cout<<"Started the helper node.\n";
  int WriteToFiles = 1;
  int my_id = 2;
  auto options = parse_program_options(argc, argv);
  if (!options.has_value()) {
    std::cerr<<"No options given.\n";
    return EXIT_FAILURE;
  }

  auto startComm = std::chrono::high_resolution_clock::now();
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
    auto endComm = std::chrono::high_resolution_clock::now();

    std::cout<<"Start Receiving messages in parallel\n";
    comm_layer->register_fallback_message_handler(
        [](auto party_id) { return std::make_shared<TestMessageHandler>(); });
    
    //Waiting for server 0 and 1 to send their start messages. 
    while((!server0_ready_flag) || (!server1_ready_flag))
      {
        std::cout<<"@";
        boost::this_thread::sleep_for(boost::chrono::milliseconds(400));
      }

    // Sending acknowledgement message to server 0 and 1, after receiving the start message.
    std::cout<<"Sending acknowledgement message to server 0 and 1\n";
    std::cout<<"ACK bit before sending to 1\n";
    std::vector<std::uint8_t> ack{(std::uint8_t)1};

    try{
      comm_layer->send_message(0,ack);
    }
    catch (std::runtime_error& e) {
      std::cerr << "Error occurred while sending the ack message to server 0: " << e.what() << "\n";
      return EXIT_FAILURE;
    }
    try{
    comm_layer->send_message(1,ack); 
    }
    catch (std::runtime_error& e) {
      std::cerr << "Error occurred while sending the ack message to server 1: " << e.what() << "\n";
      return EXIT_FAILURE;
    }
    std::cout<<"ACK bit after sending to 1\n";
    std::cout<<"ACK bit before sending to 0\n";

    std::cout<<"ACK bit after sending to 0\n";
    std::cout<<"Sent acknowledgement message to server 0 and 1\n";

    //Waiting for the operations to complete before sending the results to the servers.
    while(!operations_done_flag)
      {
        std::cout<<"#";
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
    testMemoryOccupied(WriteToFiles, my_id, options->current_path);
  }
  catch (std::runtime_error& e) {
    std::cerr << "ERROR OCCURRED: " << e.what() << "\n";
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}