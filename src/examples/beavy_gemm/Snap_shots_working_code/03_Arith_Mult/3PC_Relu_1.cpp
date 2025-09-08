//./bin/3PC_Relu_0 --party 0,127.0.0.1,4009 --party 1,127.0.0.1,4010 --helper_node 127.0.0.1,4011 --current-path ${BASE_DIR}/build_debwithrelinfo_gcc
#include "Functions.h"
#include "GlobalVar_1.h"
using namespace std::chrono;

/************************************************************************************************/
// Aim : Read the Arithmatic shares and convert them into ABY shares perform multiplication
//Input :  "/home/iudx/Desktop/Haritha/iudx-MOTION2NX-1/build_debwithrelinfo_gcc/3PC_Relu/X_OutputShare_0", Y_OutputShare_0
//Output : /home/iudx/Desktop/Haritha/iudx-MOTION2NX-1/build_debwithrelinfo_gcc/3PC_Relu/ABY_shares_0
//********************************************************************************************

namespace po = boost::program_options;

struct Options {
  std::string current_path;
  MOTION::Communication::tcp_parties_config tcp_config;
  std::string Xarith_shares_file;
  std::string Yarith_shares_file;
  std::string Xaby_shares_file;
  std::string Yaby_shares_file;
  std::string output_share_file;
  std::size_t fractional_bits;
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

  options.current_path = vm["current-path"].as<std::string>();
  options.fractional_bits = vm["fractional-bits"].as<std::size_t>();
  fractional_bits = options.fractional_bits;
  std::cout<<"Fractional bits: "<< options.fractional_bits<<std::endl;
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

  options.tcp_config.resize(3);
  options.tcp_config[id0] = conn_info0;
  options.tcp_config[id1] = conn_info1;
  options.tcp_config[2] = conn_info_helpernode;
  options.Xarith_shares_file = "/home/iudx/Desktop/Haritha/iudx-MOTION2NX-1/build_debwithrelinfo_gcc/3PC_Relu/X_OutputShare_"+std::to_string(my_id);
  options.Xaby_shares_file = "/home/iudx/Desktop/Haritha/iudx-MOTION2NX-1/build_debwithrelinfo_gcc/3PC_Relu/X_ABY_shares_"+std::to_string(my_id);
  options.Yarith_shares_file = "/home/iudx/Desktop/Haritha/iudx-MOTION2NX-1/build_debwithrelinfo_gcc/3PC_Relu/Y_OutputShare_"+std::to_string(my_id);
  options.Yaby_shares_file = "/home/iudx/Desktop/Haritha/iudx-MOTION2NX-1/build_debwithrelinfo_gcc/3PC_Relu/Y_ABY_shares_"+std::to_string(my_id);
  options.output_share_file = "/home/iudx/Desktop/Haritha/iudx-MOTION2NX-1/build_debwithrelinfo_gcc/3PC_Relu/Z_Arith_shares_"+std::to_string(my_id);
return options;
}


// Method which is called with received messages of the type the handler is
// registered for and the id of the sending party.
// This method may be called concurrently with different values of party_id.
// The client is responsible for the necessary synchronization,see  message_handler.h for more info
//for this reason we have to use synchronization on our own
// virtual void received_message(std::size_t party_id, std::vector<std::uint8_t>&& message) = 0;
//Note that above is a virtual function
class TestMessageHandler : public MOTION::Communication::MessageHandler
{
  void received_message(std::size_t party_id, std::vector<std::uint8_t>&& message) override 
  {
    auto msg_type = +message[0];
    std::cout << "MESSAGE TYPE : " << msg_type << "\n\n";
    
    switch (msg_type) 
    {
    case XArithToABY:
        if(party_id == other_party)
          {
            std::cout << "In messages Handler: received from : " << party_id << ",message type :" << +message[0]<<"\n";
            std::cout << "The message size : " << message.size() << "\n";
            std::vector<std::uint64_t> temp_vec;
            //message contains part of public share from the other party, convert message into vec 
            //and add it the its own part of public share
            ConvertMessageIntoVector(message, temp_vec);
            ParallelAddition(X_ABY_public, temp_vec, X_ABY_public);
            std:: cout << "\n *** X_Arith to A_ABY **** \n";
            for(int i = 0; i<X_ABY_public.size();i++)
              std::cout << X_arith[i] << ",  " << X_ABY_private[i] << ",  " << X_ABY_public[i] << "\n";
            XABY_receive_flag = true;
            return;
          }
        else
          {
            std::cerr<<"Received the message : " << msg_type << "from unknown party : " << party_id<<std::endl;
            return;
          }
      
      std::cout<<"\n";
      while(!XABY_receive_flag)
      {
        std::cout<<".";
        boost::this_thread::sleep_for(boost::chrono::milliseconds(200));
      }
    case YArithToABY:
        if(party_id == other_party)
          {
            std::cout << "In messages Handler: received from : " << party_id << ",message type :" << +message[0]<<"\n";
            std::cout << "The message size : " << message.size() << "\n";
            std::vector<std::uint64_t> temp_vec;
            //message contains part of public share from the other party, convert message into vec 
            //and add it the its own part of public share
            ConvertMessageIntoVector(message, temp_vec);
            ParallelAddition(Y_ABY_public, temp_vec, Y_ABY_public);
            std:: cout << "\n *** XYArith to A_ABY **** \n";
            for(int i = 0; i<Y_ABY_public.size();i++)
              std::cout << Y_arith[i] << ",  " << Y_ABY_private[i] << ",  " << Y_ABY_public[i] << "\n";
            YABY_receive_flag = true;
            return;
          }
        else
          {
            std::cerr<<"Received the message : " << msg_type << "from unknown party : " << party_id<<std::endl;
            return;
          }
      
      std::cout<<"\n";
      while(!YABY_receive_flag)
      {
        std::cout<<".";
        boost::this_thread::sleep_for(boost::chrono::milliseconds(200));
      }
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
      
        while(!helpernode_ready_flag)
         {
            std::cout<<"#";
            boost::this_thread::sleep_for(boost::chrono::milliseconds(200));
         }
        break;
    case OT:
        if (party_id == helpernode_id)
         {
          std::cout << "\n Received OT from Helper \n" ;
          ConvertMessageIntoVector(message, OT_vec);
          for(int i = 0; i < OT_vec.size(); i++)
             std::cout << OT_vec[i] << "\n";
          OT_flag = true;
         }
        else
         {
          std::cerr << "Received message " << OT << " from unknown party id " << party_id << "\n";
         }
        while(!OT_flag)
         {
            std::cout<<"#";
            boost::this_thread::sleep_for(boost::chrono::milliseconds(200));
         }
        break;
   }
  }
};

int main(int argc, char* argv[]) {
  auto start = high_resolution_clock::now();
  auto options = parse_program_options(argc, argv);

  if (!options.has_value()) {
    std::cerr<<"No options given.\n";
    return EXIT_FAILURE;
  }
  
  int WriteToFiles = 1;
  
  std::cout << "My party id: " << my_id << "\n";
  std::unique_ptr<MOTION::Communication::CommunicationLayer> comm_layer;
  std::shared_ptr<MOTION::Logger> logger;
  //Read_Arithmatic shares from  "..build_debwithrelinfo_gcc/3PC_Relu/OutputShare_0"
  if (ReadSharesIntoVec(options->Xarith_shares_file, X_arith))
    std::cout<< " ";
  else std::cout << "Could not X read Arithmatic shares into a vector \n";
  //@ Party 0, Covert Arithmatic(X0) shares into ABY(DeltaX, deltax_0)
  //1. Generate deltaX0, 2. X_ABY_public = X0 + deltaX0, 3. send to Party 1
  GeneratePrivateShares_ABY(X_arith, X_ABY_private);//deltaX_0 generated
  X_ABY_public.resize(X_arith.size());
  ParallelAddition(X_arith, X_ABY_private, X_ABY_public, 2);//X_ABY_public = X0 + deltaX0 (Its own arith share + private share)
  ConvertVetorIntoMessage(X_ABY_public, X_msg_ABY_shares, (std::uint8_t)XArithToABY);
  std::cout << "X-ABY Publicshare message will be seding to the other party is : " << X_msg_ABY_shares.size()  << "\n";
  //Do the above for Y shares
  if (ReadSharesIntoVec(options->Yarith_shares_file, Y_arith))
    std::cout<< " ";
  else std::cout << "Could not read Y Arithmatic shares into a vector \n";
  GeneratePrivateShares_ABY(Y_arith, Y_ABY_private);
  Y_ABY_public.resize(Y_arith.size());
  ParallelAddition(Y_arith, Y_ABY_private, Y_ABY_public); 
  ConvertVetorIntoMessage(Y_ABY_public, Y_msg_ABY_shares, (std::uint8_t)YArithToABY);

  //Prepare private shares messages from vectors deltaX0, deltaY0 to send helper node
  ConvertVetorIntoMessage(X_ABY_private, X_msg_Private_Shares, (std::uint8_t)X_PrivateShares);
  ConvertVetorIntoMessage(Y_ABY_private, Y_msg_Private_Shares, (std::uint8_t)Y_PrivateShares);

  //%%%%%%%% Setting up COMM LAYER for sending and receiving messages
  try{
      MOTION::Communication::TCPSetupHelper helper(my_id, options->tcp_config);
      comm_layer = std::make_unique<MOTION::Communication::CommunicationLayer>(
          my_id, helper.setup_connections());
    }
    catch (std::runtime_error& e) {
      std::cerr << "Error occurred during connection setup: " << e.what() << "\n";
      return EXIT_FAILURE;
    }
    try{
      comm_layer->start();
    }
    catch (std::runtime_error& e) {
      std::cerr << "Error occurred while starting the communication: " << e.what() << "\n";
      return EXIT_FAILURE;
    }
  comm_layer->register_fallback_message_handler([](auto party_id) { return std::make_shared<TestMessageHandler>(); });
   
  //%%%%% Sending X_ABY shares from my party to other party nad woaut for message from other party 
  std::cout << "Sending Public shares message to Server : " << 1-my_id << " Message size : "<< X_msg_ABY_shares.size() << "\n";
  try{
      comm_layer->send_message(1-my_id, X_msg_ABY_shares);
    }
  catch (std::runtime_error& e) {
      std::cerr << "Error occurred while sending X_msg_ABY_shares to node other part: " << e.what() << "\n";
      return EXIT_FAILURE;
    }

  std::cout << "Waiting for Public shares message from Server : " << 1-my_id << "\n";
  while(!XABY_receive_flag)
    {
      std::cout<<"A";
      boost::this_thread::sleep_for(boost::chrono::milliseconds(200));
    }
  //Writing ABY shares to file  
  if (WriteABYToFile(X_ABY_public, X_ABY_private, options->Xaby_shares_file)!=0)
      {
       std::cout << "Writing ABY Shares to file is failed \n " ;
      };

  //%%%%% Sending Y_ABY shares from my party to other party nad woaut for message from other party 
  std::cout << "Sending Y Public shares message to Server : " << 1-my_id << " Message size : "<< Y_msg_ABY_shares.size() << "\n";
  try{
      comm_layer->send_message(1-my_id, Y_msg_ABY_shares);
    }
  catch (std::runtime_error& e) {
      std::cerr << "Error occurred while sending X_msg_ABY_shares to node other part: " << e.what() << "\n";
      return EXIT_FAILURE;
    }
  // comm_layer->register_fallback_message_handler([](auto party_id) { return std::make_shared<TestMessageHandler>(); });

  std::cout << "Waiting for Public shares message from Server : " << 1-my_id << "\n";
  while(!YABY_receive_flag)
    {
      std::cout<<"A";
      boost::this_thread::sleep_for(boost::chrono::milliseconds(200));
    }
  //Writing ABY shares to file  
  if (WriteABYToFile(Y_ABY_public, Y_ABY_private, options->Yaby_shares_file)!=0)
      {
       std::cout << "Writing ABY Shares to file is failed \n " ;
      };
  //Syncing up with helper node,
  std::vector<std::uint8_t> started{(std::uint8_t)HelperNodeSync};
  std::cout<<"Sending Probe message helper node.\n";
  try{
      comm_layer->send_message(helpernode_id, started);
  }
  catch (std::runtime_error& e) {
      std::cerr << "Error occurred while sending the start message to helper node: " << e.what() << "\n";
      return EXIT_FAILURE;
  }
  //Waiting to receive the acknowledgement from helpernode
  while(!helpernode_ready_flag)
      {
        std::cout<<"h";
        boost::this_thread::sleep_for(boost::chrono::milliseconds(200));
      }
  //Sending private shares to the Helper node
  try{
      comm_layer->send_message(helpernode_id, X_msg_Private_Shares);
  }
  catch (std::runtime_error& e) {
      std::cerr << "Error occurred while sending the start message to helper node: " << e.what() << "\n";
      return EXIT_FAILURE;
  }
  try{
      comm_layer->send_message(helpernode_id, Y_msg_Private_Shares);
  }
  catch (std::runtime_error& e) {
      std::cerr << "Error occurred while sending the start message to helper node: " << e.what() << "\n";
      return EXIT_FAILURE;
  }
 
  while(!OT_flag)
      {
      std::cout<<"o";
      boost::this_thread::sleep_for(boost::chrono::milliseconds(200));
      }

  std::vector<std::uint64_t> dxdy; //deltax * deltay
  MatrixMultiplication(X_ABY_private, Y_ABY_private, dxdy);
  std::cout << "\n ** dXdY ** \n";
  for(int i = 0; i < dxdy.size(); i++)
     std::cout <<  dxdy[i] << " ,  ";
  
  std::vector<std::uint64_t> DxDy;
  MatrixMultiplication(X_ABY_public, Y_ABY_public, DxDy);
  std::cout << "\n ** DXDY** \n";
  for(int i = 0; i < DxDy.size(); i++)
     std::cout <<  DxDy[i] << " ,  ";
  
  std::vector<std::uint64_t> Dxdy; //DeltaX * deltay
  MatrixMultiplication(X_ABY_public, Y_ABY_private, Dxdy);
  std::cout << "\n **DXdY ** \n";
  for(int i = 0; i < Dxdy.size(); i++)
     std::cout <<  Dxdy[i] << " ,  ";
  
  std::vector<std::uint64_t> dxDy; //deltax * Deltay
  MatrixMultiplication(X_ABY_private, Y_ABY_public, dxDy);
   std::cout << "\n ** dXDY ** \n";
  for(int i = 0; i < dxDy.size(); i++)
     std::cout <<  dxDy[i] << " ,  ";

 std::cout << "\n ** DXDY - Dxdy + dxdy - dxDy + OT0 ** \n";
 Z_arith.resize(dxdy.size());
 Z_arith[0] = dxdy[0];
 Z_arith[1] = dxdy[1];
 if (my_id == 0)
  {
  for(int i = 2; i<Z_arith.size(); i++)
    {
     Z_arith[i] = dxdy[i] + OT_vec[i] + DxDy[i] - Dxdy[i] -dxDy[i];
     std :: cout << Z_arith[i] << "\n";
    }
  }
 if (my_id == 1)
  {
  std::cout << "I am in Party :" << my_id << "\n";
  for(int i = 2; i<Z_arith.size(); i++)
    {
      Z_arith[i] = dxdy[i] + OT_vec[i] + (DxDy[i]>>1)- Dxdy[i] -dxDy[i];
      std :: cout << Z_arith[i] << "\n";
    }
  }

 std::cout << "\n *** Truncate  **** \n";
  for(int i = 2; i < Z_arith.size(); i++)
  {
     Z_arith[i] =  MOTION::new_fixed_point::truncate(Z_arith[i], fractional_bits);
     std::cout << Z_arith[i] << "\n";
  }
  WriteArithToFile(Z_arith, options->output_share_file);

  testMemoryOccupied(WriteToFiles,0, options->current_path);
  std::cout<<std::endl;
  std::cout <<"**************************************************************\n"; 
  auto stop = high_resolution_clock::now();
  auto duration = duration_cast<milliseconds>(stop - start);
  
  std::cout<<"Duration:"<<duration.count()<<"\n";

  std::string t1 = options->current_path + "/" + "AverageTimeDetails0";
  std::string t2 = options->current_path + "/" + "MemoryDetails0";

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
  return EXIT_SUCCESS;
}