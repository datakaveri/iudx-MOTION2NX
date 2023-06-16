#include "compute_server.h"
#include <boost/asio.hpp>
#include <iostream>

using namespace boost::asio;
using ip::tcp;
using std::cout;
using std::endl;
using std::string;

namespace COMPUTE_SERVER {

string read_(tcp::socket& socket) {
  boost::asio::streambuf buf;

  boost::asio::read_until(socket, buf, "\n");
  string data = boost::asio::buffer_cast<const char*>(buf.data());
  return data;
}

void send_(tcp::socket& socket, const string& message) {
  const string msg = message + "\n";
  boost::asio::write(socket, boost::asio::buffer(msg));
}

std::vector<std::pair<uint64_t, uint64_t>> get_provider_data(int port_number) {
  int count = 0;
  std::vector<std::pair<uint64_t, uint64_t>> ret;
  while (count < 2) {
    boost::asio::io_service io_service;

    // listen for new connection
    tcp::acceptor acceptor_(io_service, tcp::endpoint(tcp::v4(), port_number));

    // socket creation
    tcp::socket socket_(io_service);

    // waiting for the connection
    acceptor_.accept(socket_);

    // read operation
    boost::system::error_code ec;
    Shares data;
    read(socket_, buffer(&data, sizeof(data)), ec);
    // string message = read_(socket_);
    cout << data.Delta << endl;

    // write operation
    send_(socket_, "Hello From Server!");

    socket_.close();

    ret.push_back(std::make_pair(data.Delta, data.delta));

    count += 1;
    cout << count << endl;
  }
  return ret;
}

// std::vector<Shares> read_struct(tcp::socket& socket, int num_elements) {
//   cout << "Before reading of data\n";
//   std::vector<Shares> data;
//   std::cout << "num_elements:" << num_elements;
//   for (int i = 0; i < num_elements; i++) {
//     boost::system::error_code ec;
//     uint64_t arr[2];
//     read(socket, boost::asio::buffer(&arr, sizeof(arr)), ec);
//     Shares temp;
//     temp.Delta = arr[0];
//     // std::cout << "t:" << arr[0] << ",";
//     temp.delta = arr[1];
//     // std::cout << arr[1] << "\n";
//     data.push_back(temp);
//   }

//   return data;
// }

std::vector<Shares> read_struct(tcp::socket& socket, int num_elements) {
  cout << "Before reading of data\n";
  std::vector<Shares> data;
  for (int i = 0; i < num_elements; i++) {
    boost::system::error_code ec;
    uint64_t arr[2];
    read(socket, boost::asio::buffer(&arr, sizeof(arr)), ec);
    Shares temp;
    temp.Delta = arr[0];
    // std::cout << temp.Delta << " ";
    temp.delta = arr[1];
    // std::cout << temp.delta << "\n";
    data.push_back(temp);
  }

  socket.close();
  return data;
}

std::vector<Shares> read_struct_vector(tcp::socket& socket, int num_elements) {
  std::cout << "Before reading of data\n";
  std::vector<Shares> data;
  boost::system::error_code ec;
  uint64_t arr[2 * num_elements];
  read(socket, boost::asio::buffer(&arr, sizeof(arr)), ec);
  if (ec) {
    std::cout << "Error:" << ec.message() << "\n";
  } else {
    cout << "Received successfully \n";
  }
  int j = 0;
  cout << "Num element\n";
  for (int i = 0; i < num_elements; i++) {
    Shares temp;
    temp.Delta = arr[j];
    // std::cout << temp.Delta << " ";
    j++;
    temp.delta = arr[j];
    j++;
    // std::cout << temp.delta << "\n";
    data.push_back(temp);
  }

  // socket.close();
  return data;
}

std::pair<std::vector<Shares>, int> get_provider_dot_product_data(int port_number) {
  boost::asio::io_service io_service;

  // listen for new connection
  tcp::acceptor acceptor_(io_service, tcp::endpoint(tcp::v4(), port_number));

  // socket creation
  tcp::socket socket_(io_service);

  // waiting for the connection
  acceptor_.accept(socket_);

  // read operation
  boost::system::error_code ec;
  int num_elements;
  read(socket_, boost::asio::buffer(&num_elements, sizeof(num_elements)), ec);
  if (ec) {
    cout << ec << "\n";
  } else {
    cout << "No Error\n";
  }
  // string message = read_(socket_);
  cout << "The number of elements are :- " << num_elements << endl;

  // write operation
  const string msg = "Server has received the number of elements \n";
  boost::asio::write(socket_, boost::asio::buffer(msg));
  cout << "server sent message to Client!" << endl;

  // Read the data in the reuired format
  std::vector<Shares> data = read_struct(socket_, num_elements);

  socket_.close();

  for (int i = 0; i < num_elements; i++) {
    std::cout << data[i].Delta << "  " << data[i].delta << " ";
    std::cout << "\n\n";
  }
  std::cout << "Finished reading input \n\n";

  return std::make_pair(data, num_elements);
}

/////////////////New function
int get_actual_answer(int port_number) {
  boost::asio::io_service io_service;

  // listen for new connection
  tcp::acceptor acceptor_(io_service, tcp::endpoint(tcp::v4(), port_number));

  // socket creation
  tcp::socket socket_(io_service);

  // waiting for the connection
  acceptor_.accept(socket_);

  // read operation
  int actualanswer;
  boost::system::error_code ec;
  read(socket_, boost::asio::buffer(&actualanswer, sizeof(actualanswer)), ec);
  if (ec) {
    cout << ec << "\n";
  } else {
    cout << "No Error \n";
  }

  cout << "The actual answer is :- " << actualanswer << endl;

  // write operation
  const string msg = "Server has received the actual answer \n";
  boost::asio::write(socket_, boost::asio::buffer(msg));
  cout << "server sent actual answer to Client!" << endl;

  socket_.close();
  std::cout << "Finished reading input \n\n";

  return actualanswer;
}

//////////////// New function end ///////////////////////////////////

std::pair<std::size_t, std::pair<std::vector<Shares>, std::vector<int>>> get_provider_mat_mul_data(
    int port_number) {
  boost::asio::io_service io_service;

  // listen for new connection
  tcp::acceptor acceptor_(io_service, tcp::endpoint(tcp::v4(), port_number));

  // socket creation
  tcp::socket socket_(io_service);

  // waiting for the connection
  acceptor_.accept(socket_);

  // Read and write the number of fractional bits
  boost::system::error_code ec;
  size_t fractional_bits;
  read(socket_, boost::asio::buffer(&fractional_bits, sizeof(fractional_bits)), ec);
  if (ec) {
    cout << ec << "\n";
  } else {
    cout << "No Error\n";
  }

  const string msg_init = "Server has received the number of fractional bits \n";
  boost::asio::write(socket_, boost::asio::buffer(msg_init));
  cout << "server sent message to Client!" << endl;

  // read operation
  int arr[2];
  read(socket_, boost::asio::buffer(&arr, sizeof(arr)), ec);
  std::cout << arr[0] << "\t" << arr[1] << std::endl;
  if (ec) {
    cout << ec << "\n";
  } else {
    cout << "No Error\n";
  }
  std::vector<int> dims;
  dims.push_back(arr[0]);
  dims.push_back(arr[1]);
  int num_elements = arr[0] * arr[1];

  cout << "The number of elements are :- " << num_elements << endl;

  // write operation
  const string msg = "Server has received the number of elements \n";
  boost::asio::write(socket_, boost::asio::buffer(msg));
  cout << "server sent message to Client!" << endl;

  // Read the data in the reuired format
  std::vector<Shares> data = read_struct(socket_, num_elements);

  std::cout << "Finished reading input \n\n";

  return std::make_pair(fractional_bits, std::make_pair(data, dims));
}

// std::tuple<int, std::size_t, std::pair<std::vector<Shares>, std::vector<int>>>
// get_provider_mat_mul_data_new(int port_number) {
//   boost::asio::io_service io_service;

//   // listen for new connection
//   tcp::acceptor acceptor_(io_service, tcp::endpoint(tcp::v4(), port_number));

//   // socket creation
//   tcp::socket socket_(io_service);

//   // waiting for the connection
//   acceptor_.accept(socket_);

//   int actualanswer;
//   boost::system::error_code ec;
//   read(socket_, boost::asio::buffer(&actualanswer, sizeof(actualanswer)), ec);
//   if (ec) {
//     cout << ec << "\n";
//   } else {
//     cout << "No Error \n";
//   }

//   cout << "compute server The actual answer is :- " << actualanswer << endl;

//   // write operation
//   const string msg1 = "Server has received the actual answer \n";
//   boost::asio::write(socket_, boost::asio::buffer(msg1));
//   cout << "server sent actual answer to Client!" << endl;

//   // Read and write the number of fractional bits
//   // boost::system::error_code ec;
//   size_t fractional_bits;
//   read(socket_, boost::asio::buffer(&fractional_bits, sizeof(fractional_bits)), ec);
//   if (ec) {
//     cout << ec << "\n";
//   } else {
//     cout << "No Error\n";
//   }

//   const string msg_init = "Server has received the number of fractional bits \n";
//   boost::asio::write(socket_, boost::asio::buffer(msg_init));
//   cout << "server sent message to Client!" << endl;
//   cout << "compute server fractional bits are :- " << int(fractional_bits) << endl;

//   // read operation
//   int arr[2];
//   read(socket_, boost::asio::buffer(&arr, sizeof(arr)), ec);
//   if (ec) {
//     cout << ec << "\n";
//   } else {
//     cout << "No Error\n";
//   }
//   std::vector<int> dims;
//   std::cout << "In compute server Rows:" << arr[0] << "Columns" << arr[1] << "\n";
//   dims.push_back(arr[0]);
//   dims.push_back(arr[1]);
//   int num_elements = arr[0] * arr[1];

//   cout << "The number of elements are :- " << num_elements << endl;

//   // write operation
//   const string msg = "Server has received the number of elements \n";
//   boost::asio::write(socket_, boost::asio::buffer(msg));
//   cout << "server sent message to Client!" << endl;

//   // Read the data in the reuired format
//   std::vector<Shares> data = read_struct(socket_, num_elements);
//   socket_.close();
//   std::cout << "Finished reading input \n\n";

//   return {actualanswer, fractional_bits, std::make_pair(data, dims)};
// }

std::tuple<int, std::size_t, std::pair<std::vector<Shares>, std::vector<int>>>
get_provider_mat_mul_data_new(int port_number) {
  boost::asio::io_service io_service;
  cout << "Inside new function \n";
  // listen for new connection
  tcp::acceptor acceptor_(io_service, tcp::endpoint(tcp::v4(), port_number));

  // socket creation
  tcp::socket socket_(io_service);

  // waiting for the connection
  acceptor_.accept(socket_);

  int actualanswer;
  boost::system::error_code ec;
  read(socket_, boost::asio::buffer(&actualanswer, sizeof(actualanswer)), ec);
  if (ec) {
    cout << ec << "\n";
  } else {
    cout << "No Error \n";
  }

  cout << "compute server The actual answer is :- " << actualanswer << endl;

  // write operation
  const string msg1 = "Server has received the Actual answer in write operation\n";
  boost::asio::write(socket_, boost::asio::buffer(msg1));
  cout << "server sent actual answer to Client!" << endl;

  // Read and write the number of fractional bits
  // boost::system::error_code ec;
  size_t fractional_bits;
  read(socket_, boost::asio::buffer(&fractional_bits, sizeof(fractional_bits)), ec);
  if (ec) {
    cout << ec << "\n";
  } else {
    cout << "No Error\n";
  }

  const string msg_init = "Server has received the number of fractional bits \n";
  boost::asio::write(socket_, boost::asio::buffer(msg_init));
  cout << "server sent message to Client!" << endl;
  cout << "compute server fractional bits are :- " << int(fractional_bits) << endl;

  // read operation
  int arr[2];
  read(socket_, boost::asio::buffer(&arr, sizeof(arr)), ec);
  if (ec) {
    cout << ec << "\n";
  } else {
    cout << "No Error\n";
  }
  std::vector<int> dims;
  std::cout << "In compute server Rows:" << arr[0] << "Columns" << arr[1] << "\n";
  dims.push_back(arr[0]);
  dims.push_back(arr[1]);
  int num_elements = arr[0] * arr[1];

  cout << "The number of elements are :- " << num_elements << endl;

  // write operation
  const string msg = "Server has received the number of elements \n";
  boost::asio::write(socket_, boost::asio::buffer(msg));
  cout << "server sent message to Client!" << endl;

  // Read the data in the reuired format
  std::vector<Shares> data = read_struct(socket_, num_elements);

  std::cout << "Finished reading input \n\n";

  return {actualanswer, fractional_bits, std::make_pair(data, dims)};
}

std::tuple<int, std::size_t,
           std::pair<std::vector<std::vector<Shares>>, std::vector<std::vector<int>>>>
get_provider_total_data(int port_number) {
  cout << "Inside new function \n";
  boost::asio::io_service io_service;

  // listen for new connection
  tcp::acceptor acceptor_(io_service, tcp::endpoint(tcp::v4(), port_number));

  // socket creation
  tcp::socket socket_(io_service);

  // waiting for the connection
  acceptor_.accept(socket_);
  boost::system::error_code ec;
  size_t fractional_bits;
  int numberOfVectors;
  const string msg_layer = "Server has received the number of vectors expected \n";
  const string msg_init = "Server has received the number of fractional bits \n";
  int arr1[2], arr2[2], arr3[2], arr4[2];
  const string msg = "Server has received the number of elements \n";
  /////////Receiving 2*number of layers or number of vectors to be expected
  read(socket_, boost::asio::buffer(&numberOfVectors, sizeof(numberOfVectors)), ec);
  if (ec) {
    cout << ec << "\n";
  } else {
    cout << "No Error\n";
  }

  boost::asio::write(socket_, boost::asio::buffer(msg_layer));
  cout << "Server received number of vectors!" << endl;

  ///////////

  // Read and write the number of fractional bits
  read(socket_, boost::asio::buffer(&fractional_bits, sizeof(fractional_bits)), ec);
  if (ec) {
    cout << ec << "\n";
  } else {
    cout << "No Error\n";
  }

  boost::asio::write(socket_, boost::asio::buffer(msg_init));
  cout << "Server sent ack message to Client!" << endl;
  std::vector<std::vector<int>> dims;
  int num_elements1;
  std::vector<std::vector<Shares>> data;
  const string msgw1 = "Server has received w1 \n";
  //////////////////////////////// Repeatedly use the following////////////////
  // read operation
  for (int i = 0; i < 4; i++) {
    read(socket_, boost::asio::buffer(&arr1, sizeof(arr1)), ec);
    if (ec) {
      cout << ec << "\n";
    } else {
      cout << "No Error\n";
    }
    std::cout << "Before dims1 push back \n";
    dims.push_back({arr1[0], arr1[1]});
    // dims1.push_back(arr1[1]);
    std::cout << "After dims1 push back \n";
    num_elements1 = arr1[0] * arr1[1];
    cout << "Rows, columns" << arr1[0] << " " << arr1[1] << endl;
    cout << "The number of elements are :- " << num_elements1 << endl;

    // write operation
    boost::asio::write(socket_, boost::asio::buffer(msg));
    cout << "Server sent message to Client!" << endl;

    // Read the data in the required format
    cout << "Before read struct vector " << endl;
    data.push_back(read_struct_vector(socket_, num_elements1));
    boost::asio::write(socket_, boost::asio::buffer(msgw1));
    // cout << "Servent sent w1 to Client!" << endl;
    std::cout << "Finished reading input W1\n\n";
  }
  socket_.close();
  std::cout << "Dims 1:" << dims[0][0] << " " << dims[0][1] << endl;
  std::cout << "Dims 2:" << dims[1][0] << " " << dims[1][1] << endl;
  std::cout << "Dims 3:" << dims[2][0] << " " << dims[2][1] << endl;
  std::cout << "Dims 4:" << dims[3][0] << " " << dims[3][1] << endl;
  return {numberOfVectors, fractional_bits, std::make_pair(data, dims)};
}

std::pair<std::size_t, std::pair<std::vector<Shares>, std::vector<int>>>
get_provider_mat_mul_const_data(int port_number) {
  boost::asio::io_service io_service;

  // listen for new connection
  tcp::acceptor acceptor_(io_service, tcp::endpoint(tcp::v4(), port_number));

  // socket creation
  tcp::socket socket_(io_service);

  // waiting for the connection
  acceptor_.accept(socket_);

  // Read and write the number of fractional bits
  boost::system::error_code ec;
  size_t fractional_bits;
  read(socket_, boost::asio::buffer(&fractional_bits, sizeof(fractional_bits)), ec);
  if (ec) {
    cout << ec << "\n";
  } else {
    cout << "No Error\n";
  }

  const string msg_init = "Server has received the number of fractional bits \n";
  boost::asio::write(socket_, boost::asio::buffer(msg_init));
  cout << "server sent message to Client!" << endl;

  // read operation
  int arr[2];
  read(socket_, boost::asio::buffer(&arr, sizeof(arr)), ec);
  if (ec) {
    cout << ec << "\n";
  } else {
    cout << "No Error\n";
  }
  std::vector<int> dims;
  dims.push_back(arr[0]);
  dims.push_back(arr[1]);
  int num_elements = arr[0] * arr[1];

  cout << "The number of elements are :- " << num_elements << endl;

  // write operation
  const string msg = "Server has received the number of elements \n";
  boost::asio::write(socket_, boost::asio::buffer(msg));
  cout << "server sent message to Client!" << endl;

  // Read the data in the reuired format
  std::vector<Shares> data = read_struct(socket_, num_elements);

  // Write Operation
  const string msg2 = "Server has received the elements \n";
  boost::asio::write(socket_, boost::asio::buffer(msg2));
  cout << "server sent message to Client!" << endl;

  // Read the constant
  int constant;
  read(socket_, boost::asio::buffer(&constant, sizeof(constant)), ec);
  if (ec) {
    cout << ec << "\n";
  } else {
    cout << "No Error\n";
  }
  dims.push_back(constant);

  socket_.close();
  std::cout << "Finished reading input \n\n";

  return std::make_pair(fractional_bits, std::make_pair(data, dims));
}

//////////////////Function to receive all vectors one shot, Ramya may 3,2023
std::tuple<int, std::size_t,
           std::pair<std::vector<std::vector<Shares>>, std::vector<std::pair<int, int>>>>
get_provider_total_data_genr(int port_number) {
  // cout << "Inside new function \n";
  boost::asio::io_service io_service;

  // listen for new connection
  tcp::acceptor acceptor_(io_service, tcp::endpoint(tcp::v4(), port_number));

  // socket creation
  tcp::socket socket_(io_service);

  // waiting for the connection
  acceptor_.accept(socket_);
  boost::system::error_code ec;
  size_t fractional_bits;
  int numberOfLayers;
  const string msg_layer = "Server has received the number of layers expected \n";
  const string msg_init = "Server has received the number of fractional bits \n";
  const string msg = "Server has received the number of elements \n";
  /////////Receiving number of layers to be expected
  read(socket_, boost::asio::buffer(&numberOfLayers, sizeof(numberOfLayers)), ec);
  if (ec) {
    cout << ec << "\n";
  } else {
    cout << "No Error. Number of layers: " << numberOfLayers << "\n";
  }

  boost::asio::write(socket_, boost::asio::buffer(msg_layer));
  cout << "Server received the number of layers!" << endl;

  ///////////

  // Read and write the number of fractional bits
  read(socket_, boost::asio::buffer(&fractional_bits, sizeof(fractional_bits)), ec);
  if (ec) {
    cout << ec << "\n";
  } else {
    cout << "No Error. Number of franctional bits: " << fractional_bits << "\n";
  }

  boost::asio::write(socket_, boost::asio::buffer(msg_init));
  cout << "Server sent ack message to Client!" << endl;
  std::vector<std::pair<int, int>> dims_of_all_shares;
  std::pair<int, int> dims;
  int num_elements1;
  std::vector<std::vector<Shares>> data;
  const string msg_data = "Server has received the data.\n";
  //////////////////////////////// Repeatedly use the following////////////////
  // read operation
  for (int i = 0; i < 2*numberOfLayers; i++) {
    read(socket_, boost::asio::buffer(&dims, sizeof(dims)), ec);
    if (ec) {
      cout << ec << "\n";
    } else {
      cout << "No Error\n";
    }
    std::cout << "Before dims push back \n";
    dims_of_all_shares.push_back(dims);
    // dims1.push_back(arr1[1]);
    std::cout << "After dims push back \n";
    num_elements1 = dims.first * dims.second;
    cout << "Rows, columns: " << dims.first << " " << dims.second << endl;
    cout << "The number of elements are :- " << num_elements1 << endl;

    // write operation
    boost::asio::write(socket_, boost::asio::buffer(msg));
    cout << "Server sent message to Client!" << endl;

    // Read the data in the required format
    cout << "Before read struct vector " << endl;
    data.push_back(read_struct_vector(socket_, num_elements1));
    boost::asio::write(socket_, boost::asio::buffer(msg_data));
    // cout << "Servent sent w1 to Client!" << endl;
    std::cout << "Finished reading input W1\n";
  }
  socket_.close();
  int i = 1;
  for(auto iter : dims_of_all_shares) {
    cout << "Dims " << i << ": " << iter.first << " " << iter.second << endl;
    i++;  
  }
  return {numberOfLayers, fractional_bits, std::make_pair(data, dims_of_all_shares)};
}

}  // namespace COMPUTE_SERVER
