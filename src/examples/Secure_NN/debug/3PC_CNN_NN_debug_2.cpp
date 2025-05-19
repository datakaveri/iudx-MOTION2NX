#include "Functions_3PC_CNN_NN_debug_2.h"
#include "GlobalVar_3PC_CNN_NN_debug_2.h"

using namespace std::chrono;
namespace po = boost::program_options;

std::optional<Options> parse_program_options(int argc, char* argv[]) {
  Options options;
  boost::program_options::options_description desc("Allowed options");
  // clang-format off
  desc.add_options()
    ("help,h", po::bool_switch()->default_value(false),"produce help message")
    ("layers", po::value<std::size_t>()->required(), "number of layers")
    ("layer-types", po::value<std::vector<int>>()->multitoken(), "layers of the model")    
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

  options.layers = vm["layers"].as<std::size_t>();
  if (!vm["layer-types"].empty() && (vm["layer-types"].as<std::vector<int>>()).size() == options.layers) {
    options.layer_types = vm["layer-types"].as<std::vector<int>>();
  } else {
    throw std::invalid_argument("incorrect input parameters");
  }

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
    case ConvolutionOpMessage:
        message.erase(message.begin());
        int size_msg = message.size() / 8;
        std::cout << "size_msg: " << size_msg << "\n";

        auto i=0;        
        // If msg_type == 1, it receives weights. If msg_type == 2, it receives the image. 
        std::uint64_t msg_type = getuint64(message, 0);
        if (party_id == 0 && msg_type == 1) {  
          //weights from server0
          conv_kernels=getuint64(message,1);
          conv_channels=getuint64(message,2);
          conv_rows=getuint64(message,3);
          conv_cols=getuint64(message,4);
          std::cout << conv_kernels << " " << conv_channels << " " << conv_rows << " " << conv_cols << std::endl;

          for (int i=5; i<9; i++) {
            pads[i-5]=getuint64(message,i);
          }
          for (int i=9; i<11; i++) {
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
            pads[i-5]=getuint64(message,i);
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

      }
        else if(party_id==1 && msg_type==2)
      {
        //images from server1
        image_channels = getuint64(message, 1);
        image_rows = getuint64(message, 2);
        image_cols = getuint64(message, 3);

        std::cout<<"$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$"<<"\n";
        std::cout<<"party_id: "<<party_id<<" message type: "<<msg_type<<"\n";
        std::cout<<"image_channels: "<<image_channels<<"\n";
        std::cout<<"image_rows: "<<image_rows<<"\n";
        std::cout<<"image_cols: "<<image_cols<<"\n";
        std::cout<<"$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$"<<"\n";

        for (int i = 4; i < (4 + (image_channels * image_rows * image_cols)); i++) {
          auto temp = getuint64(message, i);
          x1.push_back(temp);
        }
      }

      flag++;
      // If flag == 4, all weights and image secret shares from server 0 and server 1 have been received.
      if (flag == 4) { 
        operations();
      }

    }
  }
};

int main(int argc, char* argv[]) {
  auto start = high_resolution_clock::now();
  std::cout << "\nStarted the helper node.\n";
  auto options = parse_program_options(argc, argv);
  int numberOfLayers = options->layers;
  std::vector<int> layerTypes = options->layer_types;
  int WriteToFiles = 1;

  if (!options.has_value()) {
    std::cerr << "No options given.\n";
    return EXIT_FAILURE;
  }

  std::shared_ptr<MOTION::Logger> logger;
  try {
    std::cout<<"Setting up the connections.";
    MOTION::Communication::TCPSetupHelper helper(my_id, options->tcp_config);
    comm_layer = std::make_unique<MOTION::Communication::CommunicationLayer>(
        my_id, helper.setup_connections());
  }
  catch (std::runtime_error& e) {
    std::cerr << "Error occurred during connection setup: " << e.what() << "\n";
    return EXIT_FAILURE;
  }
    
  std::cout << "Starting the communication layer\n";
  try {
    comm_layer->start();
  }
  catch (std::runtime_error& e) {
    std::cerr << "Error occurred while starting the communication: " << e.what() << "\n";
    return EXIT_FAILURE;
  }

  comm_layer->register_fallback_message_handler(
        [](auto party_id) { return std::make_shared<TestMessageHandler>(); });

  InitializeModuloPrimeOps();

  for (int layer_id = 1; layer_id <= numberOfLayers; layer_id++) {
    int layer_type = layerTypes[layer_id - 1];
    std::cerr << "Layer type: " << layer_type << std::endl; 
    bool ConvToMatMulFlag = false;

    if (layer_type == 1 && layer_id == 1) {
      ConvolutionReluLayer(); 
      // ResetFlags(layer_type);
    }
  }

  comm_layer->shutdown();

  auto end = high_resolution_clock::now();
  return EXIT_SUCCESS;
}