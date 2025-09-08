#include "Functions_3PC_CNN_NN_0.h"
#include "GlobalVar_3PC_CNN_NN_0.h"

using namespace std::chrono;
namespace po = boost::program_options;

std::optional<Options> parse_program_options(int argc, char* argv[]) {
  Options options;
  boost::program_options::options_description desc("Allowed options");
  // clang-format off
  desc.add_options()
    ("help,h", po::bool_switch()->default_value(false), "Produce help message")
    ("WB-file", po::value<std::string>()->required(), "Weights and Bias filename")
    ("input-file", po::value<std::string>()->required(), "Input filename")
    ("layers", po::value<std::size_t>()->required(), "number of layers")
    ("layer-types", po::value<std::vector<int>>()->multitoken(), "layers of the model")
    ("party", po::value<std::vector<std::string>>()->multitoken(),
      "(party id, IP, port), e.g., --party 1,127.0.0.1,7777")
    ("helper_node", po::value<std::string>()->multitoken(),
      "(helpernode IP, port), e.g., --helper_node 127.0.0.1,7777")
    ("current-path", po::value<std::string>()->required(), "currentpath")
    ("fractional-bits", po::value<std::size_t>()->default_value(13), "Number of fractional bits")
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
  }
  catch (std::exception& e) {
    std::cerr << "Error: " << e.what() << "\n\n";
    std::cerr << desc << "\n";
    return std::nullopt;
  }

  options.WB_File = vm["WB-file"].as<std::string>();
  options.input_file = vm["input-file"].as<std::string>();
  options.layers = vm["layers"].as<std::size_t>();
  if (!vm["layer-types"].empty() && (vm["layer-types"].as<std::vector<int>>()).size() == options.layers) {
    options.layer_types = vm["layer-types"].as<std::vector<int>>();
  }
  
  options.current_path = vm["current-path"].as<std::string>();
  options.fractional_bits = vm["fractional-bits"].as<std::size_t>();

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
  std::cout << party_infos[0] << "\n";
  std::cout << party_infos[1] << "\n";
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

  const std::string baseDirectory = (std::string)std::getenv("BASE_DIR");

  options.tcp_config.resize(3);
  options.tcp_config[id0] = conn_info0;
  options.tcp_config[id1] = conn_info1;
  options.tcp_config[2] = conn_info_helpernode;

  options.output_share_file = baseDirectory + "/build_debwithrelinfo_gcc/server" + std::to_string(my_id) +"/outputshare_" + std::to_string(my_id);

  return options;
}

// Method which is called with received messages of the type the handler is
// registered for and the id of the sending party.
// This method may be called concurrently with different values of party_id.
// The client is responsible for the necessary synchronization,see  message_handler.h for more info
// for this reason we have to use synchronization on our own
// virtual void received_message(std::size_t party_id, std::vector<std::uint8_t>&& message) = 0;
// Note that above is a virtual function
class TestMessageHandler : public MOTION::Communication::MessageHandler
{
  void received_message(std::size_t party_id, std::vector<std::uint8_t>&& message) override 
  {
    int k = message.size() / 8;
    auto msg_type = +message[0];
    std::cout << "MESSAGE TYPE: " << msg_type << "\n\n";
    
    switch (msg_type) 
    {
    case SC_P:
        // Received prime shares in ShareConvert
        if (party_id == helpernode_id)
          {
            std::cout << "Message received from: " << party_id << ", Message Type:  SC_P. \n ";
            ConvertMessageIntoVector(message, x_pr_bit);
            std::cout << "SC_P" << std::endl;
            SC_P_flag = true;
          }
        else
          {
            std::cerr << "Received the message: " << msg_type << " from unknown party: " << party_id << std::endl;
            return;
          }
        break;
    case SC_D: //Received delta shares
        if (party_id == helpernode_id)
           {
            std::cout << "Message received from : " << party_id << " , Message Type :  SC_D. \n ";
            ConvertMessageIntoVector(message, del_odd);
            std::cout << "SC_D" << std::endl;
            SC_D_flag = true;
           }
        else
           {
             std::cerr<<"Received the message : " << msg_type << " from unknown party : " << party_id<<std::endl;
             return;
           }
      
        break;
    case SC_PC : //Received betaP_odd after PrivateCompare
       if (party_id == helpernode_id)
           {
            std::cout << "Message received from : " << party_id << " , Message Type :  SC_PC. \n ";
            ConvertMessageIntoVector(message, etaP_odd);
            SC_PC_flag = true;
           }
        else
           {
             std::cerr<<"Received the message : " << msg_type << " from unknown party : " << party_id<<std::endl;
             return;
           }
      
        break;
          
    case XArithToABY:
        if(party_id == other_party)
          {
            std::cout << "Message received from : " << party_id << ", message type :" << +message[0]<<".\n";
            //message contains part of public share from the other party, convert message into vec 
            ConvertMessageIntoVector(message, x_temp_vec);
            XABY_receive_flag = true;
            return;
          }
        else
          {
            std::cerr<<"Received the message : " << msg_type << " from unknown party : " << party_id<<std::endl;
            return;
          }
      break;
    case YArithToABY:
        if(party_id == other_party)
          {
            std::cout << "In messages Handler: received from : " << party_id << ", message type :" << +message[0]<<"\n";
            //message contains part of public share from the other party, convert message into vec 
            ConvertMessageIntoVector(message, y_temp_vec);
            YABY_receive_flag = true;
            return;
          }
        else
          {
            std::cerr<<"Received the message : " << msg_type << "from unknown party : " << party_id<<std::endl;
            return;
          }
      
      std::cout<<"\n";
      break;

    case ReluSync:
        if(party_id == helpernode_id)
        {
          std::cout<<"\nHelper node acknowledged message server " <<  my_id << ".\n";
          relu_ready_flag = true;
          return;
        }
      else
        {
          std::cerr<<"Received the message " << ReluSync <<  " from an unknown party id "<<party_id<<std::endl;
          return;
        }
      break;
    case HelperNodeSync:
        if (party_id == helpernode_id) {
          std::cout << "\nHelper node acknowledged the message received from server: " <<  my_id << ".\n";
          helpernode_ready_flag = true;
          return;
        }
        else {
          std::cerr << "Received the message: " << HelperNodeSync <<  " from an unknown party id: " << party_id << std::endl;
          return;
        }
        break;
    case OT:
        if (party_id == helpernode_id) {
          std::cout << "\nReceived OT from the helper node. \n" ;
          ConvertMessageIntoVector(message, OT_vec);
          OT_flag = true;
        }
        else {
          std::cerr << "Received the message " << OT << " from an unknown party id: " << party_id << "\n";
        }
        break;
    case ComputeMSBReady:
          if (party_id == helpernode_id) {
            std::cout << "Ready for compute MSB." << std::endl;
            helpernode_computeMSB_ready_flag = true;
          }
          else {
            std::cerr << "Received message from unknown party id" << party_id << std::endl;
          }
          break;
    case MSB_Odd:
          if (party_id == helpernode_id) {
            std::cout << "Received message from: " << party_id << ", Message Type: MSB_Odd." << std::endl;
            ConvertMessageIntoVector(message, MSB_x_Odd);
            MSB_Odd_flag = true;
            return;
          }
          else {
            std::cerr << "Received message from unknown party id" << party_id << std::endl;
          }
          break;
    case MSB_P:
          if (party_id == helpernode_id) {
            std::cout << "Received message from: " << party_id << ", Message Type: MSB_P." << std::endl;
            ConvertMessageIntoVector(message, MSB_x_P);
            MSB_P_flag = true;
            return;
          }
          else {
            std::cerr << "Received message from unknown party id" << party_id << std::endl;
          }
          break;
    case MSB_L_LSB:
          if (party_id == helpernode_id) {
            std::cout << "Received message from: " << party_id << ", Message Type: MSB_L_LSB." << std::endl;
            ConvertMessageIntoVector(message, MSB_x_L_LSB);
            MSB_L_LSB_flag = true;
            return;
          }
          else {
            std::cerr << "Received message from unknown party id" << party_id << std::endl;
          }
          break;
    case MSB_R:
          if (party_id == other_party) {
            std::cout << "Received message from: " << party_id << ", Message Type: MSB_R." << std::endl;
            ConvertMessageIntoVector(message, R_Shares);
            MSB_R_flag = true;
            return;
          }
          else {
            std::cerr << "Received message from unknown party id" << party_id << std::endl;
          }
          break;
    case MSB_PC:
          if (party_id == helpernode_id) {
            std::cout << "Received message from: " << party_id << ", Message Type: MSB_PC." << std::endl;
            ConvertMessageIntoVector(message, betaP);
            MSB_PC_flag = true;
            return;
          }
          else {
            std::cerr << "Received message from unknown party id: " << party_id << std::endl;
          }
          break;
    case ReluReady:
          XABY_receive_flag = false;
          YABY_receive_flag = false;
          OT_flag = false;
          // Since Hadamard Matrix Multiplication is to be executed again during the execution of ReLU,
          // XABY_receive_flag, YABY_receive_flag and OT_flag are reset to false to be used again.
          if (party_id == helpernode_id) {
            std::cout << "Received message from: " << party_id << ", Message type: ReluReady." << std::endl;
            helpernode_ReLU_ready_flag = true;
            return;
          } else {
            std::cerr << "Received message from unknown party id: " << party_id << std::endl;
          }
          break;
    case Relu_XArithToABY: // Received part public shares from other_party 
        if(party_id == other_party)
          {
            std::cout << "Message received from : " << party_id << ", Message type: " << +message[0]<<".\n";
            //message contains part of public share from the other party, convert message into vec 
            ConvertMessageIntoVector(message, Relu_x_temp_vec);
            XABY_receive_flag = true;
            return;
          }
        else
          {
            std::cerr<<"Received the message : " << msg_type << " from unknown party : " << party_id<<std::endl;
            return;
          }
      break;
    case Relu_YArithToABY:  // Received part public shares from other_party 
        if(party_id == other_party)
          {
            std::cout << "In messages Handler: received from : " << party_id << ", message type :" << +message[0]<<"\n";
            //message contains part of public share from the other party, convert message into vec 
            ConvertMessageIntoVector(message, Relu_y_temp_vec);
            YABY_receive_flag = true;
            return;
          }
        else
          {
            std::cerr<<"Received the message : " << msg_type << "from unknown party : " << party_id<<std::endl;
            return;
          }
        break;

      case Relu_OT: // Received OT shares from helper node in ReLU.
        if (party_id == helpernode_id) {
          std::cout << "Message received from: " << party_id << ", Message type: " << +message[0] << std::endl; 
          ConvertMessageIntoVector(message, Relu_OT_vec);
          OT_flag = true;
          return;
        } else {
          std::cerr << "Received the message: " << msg_type << " from unknown party: " << party_id << std::endl;
        }
        break;
      case OtherPartySync: // Received acknowledgement from other party
        if (party_id == (1 - my_id))
        {
          std::cout << "Message received from other party: " << 1 - my_id << ", Message type: OtherPartySync." << std::endl;
          OtherPartySync_Flag = true;
        }
        else {
          std::cout << "Message received from unknown party: " << party_id << ", Message type: OtherPartySync." << std::endl;
          return;
        }
        break;
      case ArithToABY: // Received public shares from other_party
        if (party_id == (1 - my_id))
        {
          std::cout << "Message received from other party: " << 1 - my_id << ", Message type: ArithToABY." << std::endl;
          ConvertMessageIntoVector(message, Relu_Public_Shares_Other_Party);
          ArithToABY_Flag = true;
        }
        else {
          std::cout << "Message received from unknown party: " << party_id << ", Message type: ArithToABY." << std::endl;
          return;
        }
        break;
      case reluABYAck: 
        if (party_id == helpernode_id)
        {
          std::cout << "Message received from other party: " << helpernode_id << ", Message type: reluABYAck." << std::endl;
          reluABYAck_Flag = true;
        }
        else {
          std::cout << "Message received from unknown party: " << party_id << ", Message type: ArithToABY." << std::endl;
          return;
        }
        break;
      case ConvolutionOpMessage:
        std::cout << "Message received from party: " << party_id << std::endl;
        message.erase(message.begin());

        if (party_id == helpernode_id) {
          std::cout << "R-message size before converting to 64-bit units: " << message.size() << std::endl;

          if (message.size() <= 0) {
            std::cerr << "Empty message received from party: " << party_id << std::endl;
            exit(1);
          }

          std::cout << "R-message: \n";
          for (auto index = 3; index < k; index++) {
            auto temp = getuint64(message, index);
            R.push_back(temp);
          }

          std::cout << "\n";
          if (R.size() == k - 3) {
            operationsConvolution();
            operations_done_flag++;
          }
        }     
        else if (party_id == 1 - my_id) { //wait till operation is not done 
          while (operations_done_flag != 2) {
            std::cout << ".";
            boost::this_thread::sleep_for(boost::chrono::milliseconds(100));
          }
          
          std::cout << "\nReceived message from Party 1 of size: " << message.size() << "\n";
          std::vector<std::uint64_t> Final_public;      
          
          for (int i = 0; i < k; i++) { 
            auto temp = getuint64(message, i);
            Final_public.push_back(temp);
            int kernel_index = (i * conv_kernels) / k;
            Final_public[i] += bpublic[kernel_index];
            randomnum[i] += bsecret[kernel_index];
          }
     
          //finalpublicshare = publicshare_0 + publicshare_1
          __gnu_parallel::transform(Final_public.begin(), Final_public.end(), prod1.begin(), Final_public.begin(), std::plus{});   
          
          A_Conv_Public_Shares.push_back(output_chnls * output_rows * output_columns);
          A_Conv_Public_Shares.push_back(1);

          A_Conv_Private_Shares.push_back(output_chnls * output_rows * output_columns);
          A_Conv_Private_Shares.push_back(1);
          for (int i = 0; i < Final_public.size(); i++) {
            A_Conv_Public_Shares.push_back(Final_public[i]);
            A_Conv_Private_Shares.push_back(randomnum[i]);
          }
        } 
        conv_flag_1 = true;
        break;
      case MatrixMulAddOpMessage:
        std::cout << "Message received from party: " << party_id << std::endl;
        message.erase(message.begin());

        if (party_id == helpernode_id) {
          std::cout << "R-Message size before converting to 64bit uint: " << message.size() << std::endl;
          if (message.size() <= 0) {
            std::cerr << "Empty message received from party " << party_id << std::endl;
            exit(1);
          }
          for (auto i = 0; i < k; ++i) {
            auto temp = getuint64(message, i);
            R.push_back(temp);
          }
          std::cout << "k = " << k << "   R.size()=" << R.size() << std::endl;
          if (R.size() == k) {
            operationsMatrixMultiplication();
            operations_done_flag++;
          }
        } else if (party_id == 1 - my_id) {
          while (operations_done_flag != 2) {
            std::cout<<".";
            boost::this_thread::sleep_for(boost::chrono::milliseconds(100));
          }

          std::cerr <<"\nReceived message from Party 1 of size "<< message.size() << "\n"; //should be 258
      
          // to push rows and column 
          for (int i = 0; i < 2; i++) {
            auto j = getuint64(message, i);
            A_MatMul_Public_Shares.push_back(j);
            A_MatMul_Private_Shares.push_back(j);
          }
    
          for (int i = 2; i < k - 1; i = i + 2) { 
            auto temp = getuint64(message, i);
            A_MatMul_Public_Shares.push_back(temp);
            auto temp2 = getuint64(message, i + 1);
            A_MatMul_Private_Shares.push_back(temp2);
          }

          auto public_shares_begin = A_MatMul_Public_Shares.begin();
          auto public_shares_end = A_MatMul_Public_Shares.end();
          advance(public_shares_begin, 2);
          std::cout << "Public shares: " << *public_shares_begin << "\n";
    
          auto prod1_begin = prod1.begin();
          advance(prod1_begin, 2);
          
          auto private_share_begin = A_MatMul_Private_Shares.begin();
          advance(private_share_begin, 2);
          
          auto randomnum_begin = randomnum.begin();
          auto randomnum_end = randomnum.end();
          advance(randomnum_begin, 2);
    
          std::cout << "Random number:" << *randomnum_begin << "\n";
    
          auto bpublic_begin = bpublic.begin();
          advance(bpublic_begin, 2);
          
          auto bsecret_begin = bsecret.begin();
          advance(bsecret_begin, 2);
          
          __gnu_parallel::transform(public_shares_begin, public_shares_end, prod1_begin, public_shares_begin, std::plus{});   
      
          //public shares for w.x + public shares of bias b
          __gnu_parallel::transform(public_shares_begin, public_shares_end, bpublic_begin, public_shares_begin , std::plus{});  
  
          // random num = random num + secret share of bias
          // final secret share = randomnum
          __gnu_parallel::transform(randomnum_begin, randomnum_end, bsecret_begin, randomnum_begin , std::plus{});

          A_MatMul_Private_Shares.resize(randomnum.size());
          for (int index = 0; index < randomnum.size(); index++) {
            A_MatMul_Private_Shares[index] = randomnum[index];
          }
          matrix_add_mul_flag_1 = true;
        }
        break;
      }
    }
};

int main(int argc, char* argv[]) {
  auto start = high_resolution_clock::now();
  auto options = parse_program_options(argc, argv);
  int numberOfLayers = options->layers;
  std::vector<int> layerTypes = options->layer_types;
  int WriteToFiles = 1;

  if (!options.has_value()) {
    std::cerr << "No options given." << std::endl;
    return EXIT_FAILURE;
  }

  std::cout << "My party id: " << my_id << std::endl;
  std::shared_ptr<MOTION::Logger> logger;

  // ***** Setting up the communication layer for sending and receiving messages *****

  try {
    MOTION::Communication::TCPSetupHelper helper(my_id, options->tcp_config);
    comm_layer = std::make_unique<MOTION::Communication::CommunicationLayer>(
          my_id, helper.setup_connections());
  }
  catch (std::runtime_error& e) {
    std::cerr << "Error occurred during connection setup: " << e.what() << std::endl;
    return EXIT_FAILURE;
  }

  try {
    comm_layer->start();
  }
  catch (std::runtime_error& e) {
    std::cerr << "Error occurred while starting the communication: " << e.what() << std::endl;
    return EXIT_FAILURE;
  }

  comm_layer->register_fallback_message_handler([](auto party_id) { return std::make_shared<TestMessageHandler>(); });

  // ****** Completion of communication layer setup *******************************

  InitializeModuloPrimeOps();
  std::vector<std::uint8_t> weightSharesMessage, imageSharesMessage;

  // Reading the remote image and weight shares.
  for (int layer_id = 1; layer_id <= numberOfLayers; layer_id++) {
    int layerType = layerTypes[layer_id - 1];
    
    // initialization of a flag when the layer switches from convolution to matrix multiplication
    bool ConvToMatMul = false;
    if (layer_id != 1 && layerTypes[layer_id - 2] == 1 && layerType == 0) {
      ConvToMatMul = true;
    } else {
      ConvToMatMul = false;
    }

    read_shares(1, layer_id, layerType, weightSharesMessage, options.value(), ConvToMatMul);
    read_shares(2, layer_id, layerType, imageSharesMessage, options.value(), ConvToMatMul);

    std::cout << "Shares successfully read." << std::endl;
    
    if (layerType == 0) {
      if (layer_id == numberOfLayers) {
        std::cerr << layer_id << ": Executing Matrix Multiplication Layer. " << std::endl;
        MatrixMultiplicationLayer(weightSharesMessage, imageSharesMessage, layer_id);
      }
      else {
        std::cerr << layer_id << ": Executing Matrix Multiplication and ReLU layer." << std::endl;
        MatrixMultiplicationReluLayer(weightSharesMessage, imageSharesMessage, layer_id);
      }
    }
    else if (layerType == 1) {
      std::cerr << layer_id << ": Executing Convolution + ReLU layer." << std::endl;
      ConvolutionReluLayer(weightSharesMessage, imageSharesMessage, layer_id);
    }

    weightSharesMessage.clear();
    imageSharesMessage.clear();
  }
  comm_layer->shutdown();

  // ***** Write the shares to the output file *******************

  std::ofstream OutputShareFile;
  try {
    OutputShareFile.open(options->output_share_file);
    if (!OutputShareFile) {
      std::cerr << "Error occurred while opening the output share file." << std::endl;
      return EXIT_FAILURE;
    }
  }
  catch (std::exception& e) {
    std::cerr << "Error occurred while opening the output share file: " << e.what() << std::endl;
    return EXIT_FAILURE;
  }

  std::size_t outputRows = A_MatMul_Public_Shares[0];
  std::size_t outputCols = A_MatMul_Public_Shares[1];

  OutputShareFile << outputRows << " " << outputCols << std::endl;
  for (int index = 2; index < A_MatMul_Public_Shares.size(); index++) {
    OutputShareFile << A_MatMul_Public_Shares[index] << " " << A_MatMul_Private_Shares[index] << std::endl;
  }

  OutputShareFile.close();

  // ***** Write the shares to the output file *******************

  testMemoryOccupied(WriteToFiles, my_id, options->current_path);
  std::cout << std::endl;
  std::cout << "**************************************************************\n"; 

  auto end = high_resolution_clock::now();
  return EXIT_SUCCESS;
}