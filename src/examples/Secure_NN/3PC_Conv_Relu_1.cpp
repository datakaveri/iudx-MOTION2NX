// ./bin/3PC_Conv_Relu1 --WB_file input_config_1 --input_file remote_image_shares --layer-id 1 --party 0,127.0.0.1,4009 --party 1,127.0.0.1,4010 --helper_node 127.0.0.1,4011 --current-path ${BASE_DIR}/build_debwithrelinfo_gcc

#include "Functions_Conv_Relu_1.h"
#include "GlobalVar_Conv_Relu_1.h"

using namespace std::chrono;

//***********************************************************************************************/
// Aim    : Read the image and weight ABY2.0 shares at each server and perform convolution and ReLU.
// Input  : 
// Image shares for layer 1       : ${BASE_DIR} + "/build_debwithrelinfo_gcc/server1/Image_shares/remote_image_shares"
// Image shares for other layers  : ${BASE_DIR} + "/build_debwithrelinfo_gcc/server1/outputshare_1"
// Weight shares for layers       : ${BASE_DIR} + "/build_debwithrelinfo_gcc/server1/W(layer_id).txt", ${BASE_DIR} + "/build_debwithrelinfo_gcc/server1/B(layer_id).txt"
// Output : ${BASE_DIR} + "/build_debwithrelinfo_gcc/server0/outputshare_1"
//***********************************************************************************************/

namespace po = boost::program_options;

std::optional<Options> parse_program_options(int argc, char* argv[]) {
  Options options;
  boost::program_options::options_description desc("Allowed options");
  // clang-format off
  desc.add_options()
    ("help,h", po::bool_switch()->default_value(false), "produce help message")
    ("WB_file", po::value<std::string>()->required(), "Weights and Bias filename")
    ("input_file", po::value<std::string>()->required(), "input filename")
    ("layer-id", po::value<std::size_t>()->required(), "layer id")
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
  } catch (std::exception& e) {
    std::cerr << "error:" << e.what() << "\n\n";
    std::cerr << desc << "\n";
    return std::nullopt;
  }

  options.WB_File = vm["WB_file"].as<std::string>();
  options.layer_id = vm["layer-id"].as<std::size_t>();
  options.input_file = vm["input_file"].as<std::string>();
  options.current_path = vm["current-path"].as<std::string>();
  options.fractional_bits = vm["fractional-bits"].as<std::size_t>();
  fractional_bits = options.fractional_bits;
  std::cout << "Fractional bits: " << options.fractional_bits << std::endl;
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

  options.output_share_file = baseDirectory + "/build_debwithrelinfo_gcc/server" + std::to_string(my_id) + "/outputshare_" + std::to_string(my_id);
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
    auto msg_type = +message[0];
    std::cout << "MESSAGE TYPE: " << msg_type << "\n\n";
    int k = message.size() / 8;
    switch (msg_type) 
    {
    case SC_P:
        // Received prime shares in ShareConvert
        if (party_id == helpernode_id)
           {
            std::cout << "Message received from : " << party_id << " , Message Type :  SC_P. \n ";
            ConvertMessageIntoVector(message, x_pr_bit);
            SC_P_flag = true;
           }
        else
           {
             std::cerr<<"Received the message : " << msg_type << " from unknown party : " << party_id<<std::endl;
             return;
           }
        break;

    case SC_D: //Received delta shares
        if (party_id == helpernode_id)
           {
            std::cout << "Message received from : " << party_id << " , Message Type :  SC_D. \n ";
            ConvertMessageIntoVector(message, del_odd);
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
    case HelperNodeSync:
        if(party_id == helpernode_id)
          {
            std::cout<<"\nHelper node acknowledged message server " <<  my_id << ".\n";
            helpernode_ready_flag = true;
            return;
          }
        else
          {
            std::cerr<<"Received the message " << HelperNodeSync <<  " from unknown party id "<<party_id<<std::endl;
            return;
          }
        break;
    case OT:
        if (party_id == helpernode_id)
         {
          std::cout << "\n Received OT from Helper \n" ;
          ConvertMessageIntoVector(message, OT_vec);
          OT_flag = true;
         }
        else
         {
          std::cerr << "Received message " << OT << " from unknown party id " << party_id << "\n";
         }
        break;
    case ComputeMSBReady:
        if (party_id == helpernode_id) {
          std::cout << "Server 0 is ready for ComputeMSB. \n";
          helpernode_computeMSB_ready_flag = true;
        }
        else {
          std::cerr << "Received message " << ComputeMSBReady << " from unknown party id: " << party_id << "\n";
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
            std::cerr << "Received message " << MSB_Odd << " from unknown party id: " << party_id << std::endl;
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
            std::cerr << "Received message " << MSB_P << " from unknown party id: " << party_id << std::endl;
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
            std::cerr << "Received message " << MSB_L_LSB <<" from unknown party id: " << party_id << std::endl;
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
            std::cerr << "Received message " << MSB_R << " from unknown party id: " << party_id << std::endl;
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
            std::cerr << "Received message " << MSB_PC << " from unknown party id: " << party_id << std::endl;
          }
          break;
    case ReluReady:
          // Since Hadamard Matrix Multiplication is to be executed again during the execution of ReLU,
          // XABY_receive_flag, YABY_receive_flag and OT_flag are reset to false to be used again.
          XABY_receive_flag = false;
          YABY_receive_flag = false;
          OT_flag = false;
          if (party_id == helpernode_id) {
            std::cout << "Received message from: " << party_id << ", Message type: ReluReady." << std::endl;
            helpernode_ReLU_ready_flag = true;
            return;
          } else {
            std::cerr << "Received message from unknown party id: " << party_id << std::endl;
          }
          break;
    case Relu_XArithToABY:  // Received part public shares from other_party 
        if(party_id == other_party)
          {
            std::cout << "Message received from : " << party_id << ", Message type: " << +message[0]<<".\n";
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
      case reluABYAck: // Received public shares from other_party
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
        std::cout << "k.size(): " << k << std::endl;
        message.erase(message.begin());

          if (party_id == helpernode_id) {
            std::cout << "DEBUG HELPERNODE" << std::endl;

            std::cout << "(Z-R) received from helper node of size: " << message.size() << std::endl;
            if (message.size() <= 0) {
              std::cerr<<"Empty message received from party "<<party_id<<std::endl;
              exit(1);
            }
            std::cout << "Z message: \n";
            int offset = 3;
            for (auto i = offset; i < k; ++i) {
              auto temp = getuint64(message, i);
              Z.push_back(temp);
            }
            if (Z.size() == k - offset) {
              operations();
              operations_done_flag++;
            }
          }
          else if (party_id == 1 - my_id) { //wait till operation is not done 
            while (operations_done_flag != 2) {
              std::cout << "@";
              boost::this_thread::sleep_for(boost::chrono::milliseconds(100));
            }
    
            std::cout <<"\nReceived message from Party 0 of size: " << message.size() << "\n";
    
            std::vector<std::uint64_t> Final_public;
            for (int i = 0; i < k; i++) { 
              auto temp = getuint64(message, i);
              Final_public.push_back(temp);
              int kernel_index = (i * conv_kernels) / k;
           
              Final_public[i] += bpublic[kernel_index];
              randomnum[i] += bsecret[kernel_index];
            }

            __gnu_parallel::transform(Final_public.begin(), Final_public.end(), Z.begin(), Final_public.begin() , std::plus{});   
        
            A_Conv_Public_Shares.push_back(output_chnls * output_rows * output_columns);
            A_Conv_Public_Shares.push_back(1);
  
            A_Conv_Private_Shares.push_back(output_chnls * output_rows * output_columns);
            A_Conv_Private_Shares.push_back(1);
            for (int i = 0; i < Final_public.size(); i++) {
              A_Conv_Public_Shares.push_back(Final_public[i]);
              A_Conv_Private_Shares.push_back(randomnum[i]);
            }
            conv_flag_0 = true;
          }    
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
            std::cerr<<"Received the message " << HelperNodeSync <<  " from unknown party id "<<party_id<<std::endl;
            return;
          }
        break;
   }
  }
};

int main(int argc, char* argv[]) {
  auto start = high_resolution_clock::now();
  auto options = parse_program_options(argc, argv);
  std::vector<std::uint64_t> A_L_Public_Shares, A_L_Private_Shares;
  std::vector<std::uint64_t> A_L;
  std::vector<std::uint64_t> A_Relu;
  std::vector<std::uint64_t> A_Relu_Public_Shares, A_Relu_Private_Shares;
  int WriteToFiles = 1;

  if (!options.has_value()) {
    std::cerr<<"No options given.\n";
    return EXIT_FAILURE;
  }
 
  std::cout << "My party id: " << my_id << "\n";
  std::shared_ptr<MOTION::Logger> logger;
  
  // ****** Setting up the communication layer for sending and receiving messages ******
  try {
    MOTION::Communication::TCPSetupHelper helper(my_id, options->tcp_config);
    comm_layer = std::make_unique<MOTION::Communication::CommunicationLayer>(
          my_id, helper.setup_connections());
  }
  catch (std::runtime_error& e) {
    std::cerr << "Error occurred during connection setup: " << e.what() << "\n";
    return EXIT_FAILURE;
  }

  try {
    comm_layer->start();
  }
  catch (std::runtime_error& e) {
    std::cerr << "Error occurred while starting the communication: " << e.what() << "\n";
    return EXIT_FAILURE;
  }

  comm_layer->register_fallback_message_handler([](auto party_id) { return std::make_shared<TestMessageHandler>(); });
  // ****** Completion of communication layer setup *******************************

  // Read ABY2.0 Weight and Image Shares from the input config file
  std::vector<std::uint8_t> weightSharesMessage, imageSharesMessage;
  read_shares(1, weightSharesMessage, options.value());
  read_shares(2, imageSharesMessage, options.value());

  std::cout << "Shares succesfully read." << std::endl;

  InitializeModuloPrimeOps();

  // Start: Convolution + ReLU Pipeline
  HelperNodeSyncFunction();

  std::cout << "Helper node sync done." << std::endl;

  ConvolutionOperation(weightSharesMessage, imageSharesMessage);

  std::cout << "Convolution done." << std::endl;

  try {
    std::vector<std::uint8_t> reluABYConvAck{(std::uint8_t)reluABYAck};
    comm_layer->send_message(helpernode_id, reluABYConvAck);
  }
  catch (std::exception& e) {
    std::cerr << "Message not sent: " << e.what() << std::endl;
    return EXIT_FAILURE;
  }

  while (!reluABYAck_Flag) {
    std::cout << ".";
    boost::this_thread::sleep_for(boost::chrono::milliseconds(100));
  }

  std::cout << "Public size: " << A_Conv_Public_Shares.size() << std::endl;
  std::cout << "Private size: " << A_Conv_Private_Shares.size() << std::endl;

  std::size_t reluRows = A_Conv_Public_Shares[0];
  std::size_t reluCols = A_Conv_Public_Shares[1];

  std::size_t privateReluRows = A_Conv_Private_Shares[0];
  std::size_t privateReluCols = A_Conv_Private_Shares[1];

  if ((reluRows != privateReluRows) || (reluCols != privateReluCols)) {
    std::cerr << "Dimensions do not match." << std::endl;
    return EXIT_FAILURE;
  }

  A_Conv_Public_Shares.erase(A_Conv_Public_Shares.begin(), A_Conv_Public_Shares.begin() + 2);
  A_Conv_Private_Shares.erase(A_Conv_Private_Shares.begin(), A_Conv_Private_Shares.begin() + 2);

  ABYToArithShareGenerator(A_Conv_Public_Shares, A_Conv_Private_Shares, A_L);

  ReLU(A_L, A_Relu, reluRows, reluCols);

  ArithToABYShareGenerator(A_Relu, A_Relu_Public_Shares, A_Relu_Private_Shares);
  // End: Convolution + ReLU Pipeline

  // Write the ReLU shares computed at Party 1 to ReLU output file.
  std::ofstream ReluOutputFile;
  try {
    ReluOutputFile.open(options->output_share_file);
    if (!ReluOutputFile) {
      std::cerr << "Error: Error opening ReLU output file." << std::endl;
    }
  } catch (std::exception& error) {
    std::cerr << "Error: Error opening ReLU output share file: " << error.what() << std::endl;
  }

  std::size_t rowsABYShares = A_Relu_Public_Shares[0];
  std::size_t colsABYShares = A_Relu_Public_Shares[1];

  // Verifying that the dimensions of the arithmetic shares and the ABY2.0 shares match
  if (rowsABYShares * colsABYShares != output_rows * output_columns * output_chnls) {
    std::cerr << "Dimensions of the arithmetic shares and the ABY shares do not match." << std::endl;
    return EXIT_FAILURE;
  }

  ReluOutputFile << output_chnls << " " << output_rows <<  " " << output_columns << std::endl;
  for (int index = 2; index < A_Relu_Public_Shares.size(); index++) {
    ReluOutputFile << A_Relu_Public_Shares[index] << " " << A_Relu_Private_Shares[index] << std::endl;
  }

  ReluOutputFile.close();

  std::string dimensionsFilepath = options->current_path + "/server1/" + "cnn_outputshare_" + std::to_string(my_id);
  ReluOutputFile.open(dimensionsFilepath);

  try {
    if (!ReluOutputFile) {
      std::cerr << "Error: Error opening ReLU output file." << std::endl;
    }
  }
  catch (std::exception& e) {
    std::cerr << "Error: Error opening ReLU output share file:" << e.what() << std::endl;
  }

  ReluOutputFile << output_chnls * output_rows * output_columns << " " << 1 << std::endl;
  ReluOutputFile.close();

  comm_layer->shutdown();
  
  testMemoryOccupied(WriteToFiles, my_id, options->current_path);
  std::cout << std::endl;
  std::cout << "**************************************************************\n"; 
  auto stop = high_resolution_clock::now();
  auto duration = duration_cast<milliseconds>(stop - start);
  
  std::cout << "Duration at Server 1: " << duration.count() << "\n";

  std::string t1 = options->current_path + "/" + "AverageTimeDetails" + std::to_string(my_id);
  std::string t2 = options->current_path + "/" + "MemoryDetails" + std::to_string(my_id);

  std::ofstream file2;
  file2.open(t2, std::ios_base::app);
  if (!file2.is_open()) {
    std::cerr << "Unable to open the MemoryDetails file.\n";
  }
  else {
    file2 << "Execution time - " << duration.count() << "msec";
    file2 << "\n";
  }
  file2.close();

  std::ofstream file1;
  file1.open(t1, std::ios_base::app);
  if (!file1.is_open()) {
    std::cerr << "Unable to open the AverageTimeDetails file.\n";
  }
  else {
    file1 << duration.count();
    file1 << "\n";
  }
  file1.close();

  return EXIT_SUCCESS;
}