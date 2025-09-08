// ./bin/3PC_Relu_2 --party 0,127.0.0.1,4009 --party 1,127.0.0.1,4010 --helper_node 127.0.0.1,4011
#include "Functions.h"
#include "GlobalVar_2.h"

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


class TestMessageHandler : public MOTION::Communication::MessageHandler {
  void received_message(std::size_t party_id, std::vector<std::uint8_t>&&message) {
    int size_msg=message.size()/8;
    int msg_type = +message[0];
    std::cout <<  "MESSAGE TYPE : " << msg_type << "\n\n";
    switch (msg_type)
    {
    case HelperNodeSync:
         if(party_id==server0)
          {
            std::cout<<"Server 0 has started.\n";
            server0_ready_flag = true;
            return;
          }
        else if(party_id==server1)
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
    }
  }
};


int main(int argc, char* argv[]) {
  std::cout<<"\n Started the helper node.\n";
  auto options = parse_program_options(argc, argv);
  if (!options.has_value()) {
    std::cerr<<"No options given.\n";
    return EXIT_FAILURE;
  }
  std::unique_ptr<MOTION::Communication::CommunicationLayer> comm_layer;
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
    std::cout<<"Start Receiving messages in parallel\n";
    comm_layer->register_fallback_message_handler(
        [](auto party_id) { return std::make_shared<TestMessageHandler>(); });
    
    //Waiting for server 0 and 1 to send their start messages. 
    //****************************ACk1*******************************
    while((!server0_ready_flag) || (!server1_ready_flag))
      {
        std::cout<<".";
        boost::this_thread::sleep_for(boost::chrono::milliseconds(400));
      }

    // Sending acknowledgement message to server 0 and 1, after receiving the start message.
    std::cout<<"Sending acknowledgement message to server 0 and 1\n";
    std::vector<std::uint8_t> ack{(std::uint8_t)HelperNodeSync};
    try{
    comm_layer->send_message(server1,ack); 
    }
    catch (std::runtime_error& e) {
      std::cerr << "Error occurred while sending the ack message to server 1: " << e.what() << "\n";
      return EXIT_FAILURE;
    }
    try{
      comm_layer->send_message(server0,ack);
    }
    catch (std::runtime_error& e) {
      std::cerr << "Error occurred while sending the ack message to server 0: " << e.what() << "\n";
      return EXIT_FAILURE;
    }
    std::cout<<"Sent acknowledgement message to server 0 and 1\n";
   //waiting for all the private shares to be received 
    while(!X0_Priv_Share_Flag || !X1_Priv_Share_Flag || !Y0_Priv_Share_Flag || !Y1_Priv_Share_Flag)
    {
     std::cout<<".";
     boost::this_thread::sleep_for(boost::chrono::milliseconds(400)); 
    }
   std::cout << "X private shares \n ";
  for(int i = 0; i<X0_Priv.size(); i++)
  {
    std::cout << X0_Priv[i] << " , "<< X1_Priv[i] << "\n";
  }
  std::cout << "Y private shares \n ";
  for(int i = 0; i<Y0_Priv.size(); i++)
  {
    std::cout << Y0_Priv[i] << " , "<< Y1_Priv[i] << "\n";
  }

  if (!MatrixMultiplication(X0_Priv, Y1_Priv, X0Y1))
      std::cout << "";
  else std::cout << "Could nOt Perform Matrix multiplication \n ";
  std::cout << " \nX0Y1 \n";
  for(int i = 2; i < X0Y1.size();i++)
    std::cout << X0Y1[i] <<" , ";

  if (!MatrixMultiplication(X1_Priv, Y0_Priv, X1Y0))
      std::cout << "";
  else std::cout << "Could nOt Perform Matrix multiplication \n ";
  std::cout << " \nX1Y0 \n";
  for(int i = 2; i< X1Y0.size();i++)
    std::cout << X1Y0[i] <<" , ";

 
  OTGeneration(X0Y1, X1Y0, OT_0, OT_1);
  std :: cout << "OTs \n";
  for (int i = 0; i < X0Y1.size(); i++)
  { 
    std::cout << OT_0[i] << " , " << OT_1[i] << " \n" ;
    std:: cout << OT_0[i] + OT_1[i] << " ,  " << X0Y1[i] + X1Y0[i] << "\n";
  }
  ConvertVetorIntoMessage(OT_0, OT0_msg, OT);
  ConvertVetorIntoMessage(OT_1, OT1_msg, OT);

  try{
      comm_layer->send_message(server0, OT0_msg);
    }
  catch (std::runtime_error& e) {
      std::cerr << "Error while sending OT message to server 0: " << e.what() << "\n";
      return EXIT_FAILURE;
    }
  
  try{
      comm_layer->send_message(server1, OT1_msg);
    }
  catch (std::runtime_error& e) {
      std::cerr << "Error while sending OT to server 0: " << e.what() << "\n";
      return EXIT_FAILURE;
    }
    
    comm_layer->shutdown();
  }
  
