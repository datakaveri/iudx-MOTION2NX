// ./bin/3PC_MatMul_Relu_2 --party 0,127.0.0.1,4009 --party 1,127.0.0.1,4010 --helper_node 127.0.0.1,4011 --current-path ${BASE_DIR}/build_debwithrelinfo_gcc
#include "Functions_MatMul_Relu_2.h"
#include "GlobalVar_MatMul_Relu_2.h"

namespace po = boost::program_options;


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

class TestMessageHandler : public MOTION::Communication::MessageHandler {
  void received_message(std::size_t party_id, std::vector<std::uint8_t>&&message) {
    int size_msg = message.size()/8;
    int msg_type = +message[0];
    std::cout << "MESSAGE TYPE: " << msg_type << "\n\n";
    switch (msg_type)
    {
    case reluABYAck:
      if (party_id == server0)
       {
         std::cout<<"Server 0 has started.\n";
         reluABY_s0_flag = true;
         return;
       }
     else if (party_id == server1)
       {
         std::cout<<"Server 1 has started.\n";
         reluABY_s1_flag = true;
         return;
       }
     else
       {
         std::cerr<<"Received the message \"1\" from unknown party "<<party_id<<std::endl;
         return;
       }
     while((!reluABY_s0_flag) || (!reluABY_s1_flag))
     {
       std::cout<<".";
       boost::this_thread::sleep_for(boost::chrono::milliseconds(400));
     }
   break;
    case SC :
        if (party_id == server0)
            {
              std::cout << "ShareConvert: a_tilde received from " << party_id << ".\n";
              ConvertMessageIntoVector(message, a0_tilde);
              SC0_flag = true;
              return;
            }
        else if (party_id == server1)
            {
              std::cout << "ShareConvert: a_tilde received from " << party_id << ".\n";
              ConvertMessageIntoVector(message, a1_tilde);
              SC1_flag = true;
              return;
            }
        break;

    case SC_PC :
        if (party_id == server0)
            {
              std::cout << "ShareConvert: c0 received from " << party_id << ".\n";
              ConvertMessageIntoVector(message, c0);
              SC_PC0_flag = true;
              return;
            }
        else if (party_id == server1)
            {
              std::cout << "ShareConvert: c1 received from " << party_id << ".\n";
              ConvertMessageIntoVector(message, c1);
              SC_PC1_flag = true;
              return;
            }
         while((!SC_PC0_flag) || (!SC_PC1_flag))
        {
          std::cout<<".";
          boost::this_thread::sleep_for(boost::chrono::milliseconds(400));
        }
        break;
    case HelperNodeSync:
         if (party_id == server0)
          {
            std::cout<<"Server 0 has started.\n";
            server0_ready_flag = true;
            return;
          }
        else if (party_id == server1)
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
        while((!server0_ready_flag) || (!server1_ready_flag))
        {
          std::cout<<".";
          boost::this_thread::sleep_for(boost::chrono::milliseconds(400));
        }
      break;

    case X_PrivateShares:
        if (party_id == server0)
           {
            std::cout << "X Private Shares from " << party_id << " received \n";
            ConvertMessageIntoVector(message, X0_Priv);
            X0_Priv_Share_Flag = true;
            return;
           }
        else if (party_id == server1)
           {
            std::cout << "X Private Shares from " << party_id << " received \n";
             ConvertMessageIntoVector(message, X1_Priv);
            X1_Priv_Share_Flag = true;
            return;
           }
        while((!X0_Priv_Share_Flag) || (!X1_Priv_Share_Flag))
        {
          std::cout<<".";
          boost::this_thread::sleep_for(boost::chrono::milliseconds(400));
        }
      break;
    case Y_PrivateShares:
        if (party_id == server0)
           {
            std::cout << "Y Private Shares from " << party_id << " received \n";
            ConvertMessageIntoVector(message, Y0_Priv);
            Y0_Priv_Share_Flag = true;
            return;
           }
        else if (party_id == server1)
           {
            std::cout << "Y Private Shares from " << party_id << " received \n";
            ConvertMessageIntoVector(message, Y1_Priv);
            Y1_Priv_Share_Flag = true;
            return;
           }
        while((!Y0_Priv_Share_Flag) || (!Y1_Priv_Share_Flag))
        {
          std::cout<<".";
          boost::this_thread::sleep_for(boost::chrono::milliseconds(400));
        }
      break;
    case ComputeMSBReady:
      if (party_id == server0) {
        std::cout << "Server 0 ready for Compute MSB." << "\n";
        server0_compute_msb_ready_flag = true;
        ConvertMessageIntoVector(message, MSB_len0);
        return;
      }
      else if (party_id == server1) {
        std::cout << "Server 1 ready for Compute MSB." << "\n";
        server1_compute_msb_ready_flag = true;
        ConvertMessageIntoVector(message, MSB_len1);
        return;
      }
      while ((!server0_compute_msb_ready_flag) || (!server1_compute_msb_ready_flag)) {
        std::cout << ".";
        boost::this_thread::sleep_for(boost::chrono::milliseconds(400));
      }
      break;
    case MSB_PC:
      if (party_id == server0) {
        std::cout << "Received PC Shares from: " << party_id << ", Message Type: " << message[0] << std::endl;
        ConvertMessageIntoVector(message, MSB_c0);
        MSB_PC_0_flag = true;
        return;
      }
      else if (party_id == server1) {
        std::cout << "Received PC Shares from: " << party_id << ", Message Type: " << message[0] << std::endl;
        ConvertMessageIntoVector(message, MSB_c1);
        MSB_PC_1_flag = true;
        return;
      }
      break;
    // Flags X0_Priv_Share_Flag, X1_Priv_Share_Flag, Y0_Priv_Share_Flag, Y1_Priv_Share_Flag are reset to false
    // after the execution of DerivativeRelu() for reuse for Hadamard Matrix Multiplication in Relu()
    case ReluReady:      
      if (party_id == server0) {
        std::cout << "Server 0 ready for Relu." << "\n";
        server0_relu_ready_flag = true;
        return;
      }
      else if (party_id == server1) {
        std::cout << "Server 1 ready for Relu." << "\n";
        server1_relu_ready_flag = true;
        return;
      }
      while ((!server0_relu_ready_flag) || (!server1_relu_ready_flag)) {
        std::cout << ".";
        boost::this_thread::sleep_for(boost::chrono::milliseconds(400));
      }
      break;
    case ReluSync:      
      if (party_id == server0) {
        std::cout << "Server 0 ready for Relu." << "\n";
        relu_ready_flag_0 = true;
        return;
      }
      else if (party_id == server1) {
        std::cout << "Server 1 ready for Relu." << "\n";
        relu_ready_flag_1 = true;
        return;
      }
      while ((!server0_relu_ready_flag) || (!server1_relu_ready_flag)) {
        std::cout << ".";
        boost::this_thread::sleep_for(boost::chrono::milliseconds(400));
      }
      break;
    case Relu_X_PrivateShares:
        if (party_id == server0)
           {
            std::cout << "X Private Shares from " << party_id << " received \n";
            ConvertMessageIntoVector(message, Relu_X0_Priv);
            X0_Priv_Share_Flag = true;
            return;
           }
        else if (party_id == server1)
           {
            std::cout << "X Private Shares from " << party_id << " received \n";
             ConvertMessageIntoVector(message, Relu_X1_Priv);
            X1_Priv_Share_Flag = true;
            return;
           }
        while((!X0_Priv_Share_Flag) || (!X1_Priv_Share_Flag))
        {
          std::cout<<".";
          boost::this_thread::sleep_for(boost::chrono::milliseconds(400));
        }
      break;
    case Relu_Y_PrivateShares:
        if (party_id == server0)
           {
            std::cout << "Y Private Shares from " << party_id << " received \n";
            ConvertMessageIntoVector(message, Relu_Y0_Priv);
            Y0_Priv_Share_Flag = true;
            return;
           }
        else if (party_id == server1)
           {
            std::cout << "Y Private Shares from " << party_id << " received \n";
            ConvertMessageIntoVector(message, Relu_Y1_Priv);
            Y1_Priv_Share_Flag = true;
            return;
           }
        while((!Y0_Priv_Share_Flag) || (!Y1_Priv_Share_Flag))
        {
          std::cout<<".";
          boost::this_thread::sleep_for(boost::chrono::milliseconds(400));
        }
      break;
    case MatrixMulAddOpMessage:
      std::cerr << "Message size: " << message.size() << std::endl;
      message.erase(message.begin());
      auto i=0;
      for(i=1;i<size_msg;i++)
      {
        auto temp=getuint64(message,i);
        if(i==1 && Conv_c1==1 && party_id==0)
        {
            w_rows=getuint64(message,i-1);
            w_cols=getuint64(message,i);
            w0.push_back(w_rows);
            w0.push_back(w_cols);
            Conv_c1++;
        }
        else if(i==1 && Conv_c2==1 && party_id==1)
        {
            w_rows=getuint64(message,i-1);
            w_cols=getuint64(message,i);
            w1.push_back(w_rows);
            w1.push_back(w_cols);
            Conv_c2++;
        }
        else if(i==1 && Conv_c3==1 && party_id==0)
        {
            x_rows=getuint64(message,i-1);
            x_cols=getuint64(message,i);
            x0.push_back(x_rows);
            x0.push_back(x_cols);
            Conv_c3++;
        }
        else if(i==1 && Conv_c4==1 && party_id==1)
        {
            x_rows=getuint64(message,i-1);
            x_cols=getuint64(message,i);
            x1.push_back(x_rows);
            x1.push_back(x_cols);
            Conv_c4++;
        }
        else if(Conv_c1>=2 && Conv_c1<=(w_cols*w_rows+1) && i>1 && party_id==0)
        {
            w0.push_back(temp);
            Conv_c1++;
        }
        else if(Conv_c2>=2 && Conv_c2<=(w_cols*w_rows+1) && i>1 && party_id==1)
        {
            w1.push_back(temp);
            Conv_c2++;
        }
        else if(Conv_c3>=2 && Conv_c3<=x_rows+1 && i>1 && party_id==0)
        {
            x0.push_back(temp);
            Conv_c3++;
        }
        else if(Conv_c4>=2 && Conv_c4<=x_rows+1 && i>1 && party_id==1)
        {
            x1.push_back(temp);
            Conv_c4++;
        }
      }
        if(Conv_c1==(w_cols*w_rows+2) && Conv_c2==(w_cols*w_rows+2) && Conv_c3==w_cols+2 && Conv_c4==w_cols+2)
        { 
          operations();
        }
      break;
    }
  }
};

int main(int argc, char* argv[]) {
  std::cout<<"\n Started the helper node.\n";
  auto options = parse_program_options(argc, argv);
  int WriteToFiles = 1;

  if (!options.has_value()) {
    std::cerr<<"No options given.\n";
    return EXIT_FAILURE;
  }
  std::shared_ptr<MOTION::Logger> logger;
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
    
    std::cout<<"Starting the communication layer\n";
    try{
      comm_layer->start();
    }
    catch (std::runtime_error& e) {
      std::cerr << "Error occurred while starting the communication: " << e.what() << "\n";
      return EXIT_FAILURE;
    }

    comm_layer->register_fallback_message_handler(
        [](auto party_id) { return std::make_shared<TestMessageHandler>(); });
  auto start = high_resolution_clock::now();
  InitializeModuloPrimeOps();

  // Helper node to execute ReLU share communication between P0, P1 and P2.
  HelperNodeSyncFunction();

  MatrixMultAddition();

  std::cout << "Matrix addition and multiplication done." << std::endl;

  while ((!reluABY_s0_flag) || (!reluABY_s1_flag)) {
    boost::this_thread::sleep_for(boost::chrono::milliseconds(200));
  }

  try {
    std::vector<std::uint8_t> ackReluABY{(std::uint8_t)reluABYAck};
    comm_layer->send_message(0, ackReluABY);
    comm_layer->send_message(1, ackReluABY);
  }
  catch (std::exception& e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return EXIT_FAILURE;
  }
  std::cout<<"Sent acknowledgement message to server 0 and 1\n";

  ReLU();

  comm_layer->shutdown();
  testMemoryOccupied(WriteToFiles, my_id, options->current_path);
  std::cout << std::endl;
  std::cout <<"**************************************************************\n"; 
  auto stop = high_resolution_clock::now();
  auto duration = duration_cast<milliseconds>(stop - start);
  
  std::cout << "Duration at Helper Node: " << duration.count() << "\n";

  std::string t1 = options->current_path + "/" + "AverageTimeDetails" + std::to_string(my_id);
  std::string t2 = options->current_path + "/" + "MemoryDetails" + std::to_string(my_id);

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

  std::ofstream file2;
  file2.open(t2, std::ios_base::app);
  if (!file2.is_open())
    {
      std::cerr << "Unable to open the MemoryDetails file.\n";
    }
  else {
  file2 << "Execution time - " << duration.count() << "msec";
  file2 << "\n";
  }
  file2.close();
  return EXIT_SUCCESS;
};
