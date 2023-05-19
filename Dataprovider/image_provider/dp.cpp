//./bin/tensor_dot_product --my-id 0 --party 0,::1,7000 --party 1,::1,7001 --arithmetic-protocol beavy --boolean-protocol beavy --repetitions 1
//./bin/tensor_dot_product --my-id 1 --party 0,::1,7000 --party 1,::1,7001 --arithmetic-protocol beavy --boolean-protocol beavy --repetitions 1


#include <iostream>
#include <type_traits>
#include <cstdint>
#include <random>
#include <stdexcept>
#include <filesystem>
#include <fstream>
#include <optional>
#include <boost/asio.hpp>
#include <boost/program_options.hpp>
#include <boost/serialization/string.hpp>
#include <boost/archive/text_oarchive.hpp>

using namespace boost::asio;
namespace po = boost::program_options;
using ip::tcp;
using std::cout;
using std::endl;
using std::string;

struct Options
{
     int cs0_port;
     int cs1_port;
     std::filesystem::path filename;
};

std::optional<Options> parse_program_options(int argc, char *argv[])
{
     Options options;
     boost::program_options::options_description desc("Allowed options");
     // clang-format off
  desc.add_options()
    ("help,h", po::bool_switch()->default_value(false),"produce help message")
    ("compute-server0-port", po::value<int>()->required(), "Port number of compute server 0")
    ("compute-server1-port", po::value<int>()->required(), "Port number of compute server 1")
    ("dp-id", po::value<int>()->required(), "Id of the data provider")
    ;
     // clang-format on

     po::variables_map vm;
     po::store(po::parse_command_line(argc, argv, desc), vm);
     bool help = vm["help"].as<bool>();
     if (help)
     {
          std::cerr << desc << "\n";
          return std::nullopt;
     }

     options.cs0_port = vm["compute-server0-port"].as<int>();
     options.cs1_port = vm["compute-server1-port"].as<int>();

     options.filename = std::filesystem::current_path();
     if (vm["dp-id"].as<int>() == 0)
     {
          options.filename += "/input_dp0.txt";
     }
     else if (vm["dp-id"].as<int>() == 1)
     {
          options.filename += "/input_dp1.txt";
     }
     else
     {
          std::cerr << "Invalid data provider ID\n";
          return std::nullopt;
     }

     return options;
}

template <typename E>
std::uint64_t RandomNumGenerator(E &engine)
{
     std::uniform_int_distribution<unsigned long long> dis(
         std::numeric_limits<std::uint64_t>::min(),
         std::numeric_limits<std::uint64_t>::max());
     return dis(engine);
}

struct Shares
{
     uint64_t Delta, delta;
};

void share_generation(std::ifstream &indata, int num_elements, Shares *cs0_data, Shares *cs1_data)
{
     // get input data
     std::vector<uint64_t> data;

     int count = 0;

     uint64_t temp;
     while (indata >> temp)  //... 
     {
          data.push_back(temp);
          count++;
          if (count == num_elements)
          {
               break;
          }
     }

     while (count < num_elements)
     {
          data.push_back(0);
     }

     for (int i = 0; i < data.size(); i++)
     {
          cout << data[i] << " ";
     }
     cout << "\n";

     // Now that we have data, need to generate the shares
     for (int i = 0; i < data.size(); i++)
     {
          std::random_device rd;
          std::mt19937 gen(rd());
          std::uint64_t del0 = RandomNumGenerator(gen);
          std::uint64_t del1 = RandomNumGenerator(gen);

          std::uint64_t Del = del0 + del1 + data[i];

          // For each data, creating 2 shares variables - 1 for CS0 and another for CS1
          Shares cs0, cs1;

          cs0.Delta = Del;
          cs0.delta = del0;

          cs1.Delta = Del;
          cs1.delta = del1;

          cs0_data[i] = cs0;
          cs1_data[i] = cs1;
     }

     return;
}

void write_struct(tcp::socket &socket, Shares *data, int num_elements)
{
     for (int i = 0; i < num_elements; i++)
     {
          uint64_t arr[2];
          arr[0] = data[i].Delta;
          arr[1] = data[i].delta;

          boost::system::error_code error;
          boost::asio::write(socket, boost::asio::buffer(&arr, sizeof(arr)), error);

          if (!error)
          {
               continue;
          }
          else
          {
               cout << "send failed: " << error.message() << endl;
          }
     }
}

int main(int argc, char *argv[])
{
     auto options = parse_program_options(argc, argv);

     if (!options.has_value())
     {
          return EXIT_FAILURE;
     }

     // Reading contents from file
     std::ifstream indata;
     indata.open(options->filename);
     int num_elements;
     indata >> num_elements;

     cout << num_elements << "\n";

     Shares cs0_data[num_elements];
     Shares cs1_data[num_elements];

     share_generation(indata, num_elements, cs0_data, cs1_data);

     indata.close();

     for (int i = 0; i < 2; i++)
     {

          cout << "\nStart of send to compute server\n";

          boost::asio::io_service io_service;

          // socket creation
          tcp::socket socket(io_service);

          auto port = options->cs0_port;
          if (i)
          {
               port = options->cs1_port;
          }

          // connection
          socket.connect(tcp::endpoint(boost::asio::ip::address::from_string("127.0.0.1"), port));

          // Send number of elements to compute server

          boost::system::error_code error;
          boost::asio::write(socket, boost::asio::buffer(&num_elements, sizeof(num_elements)), error);
          if (!error)
          {
               cout << "Not an error" << endl;
          }
          else
          {
               cout << "send failed: " << error.message() << endl;
          }

          // getting a response from the server
          boost::asio::streambuf receive_buffer;
          boost::asio::read_until(socket, receive_buffer, "\n");
          if (error && error != boost::asio::error::eof)
          {
               cout << "receive failed: " << error.message() << endl;
          }
          else
          {
               const char *data = boost::asio::buffer_cast<const char *>(receive_buffer.data());
               cout << data << endl;
          }

          // Sending CS0 data to compute_server0
          auto data = cs0_data;
          if (i)
          {
               data = cs1_data;
          }
          for (int i = 0; i < num_elements; i++)
          {
               cout << data[i].Delta << " " << data[i].delta << "\n";
          }
          write_struct(socket, data, num_elements);

          socket.close();
     }
     return 0;
}
