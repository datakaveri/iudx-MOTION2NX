#include <iostream>  
#include <boost/asio.hpp>  
#include "compute_server.h"

using namespace boost::asio;  
using ip::tcp;  
using std::string;  
using std::cout;  
using std::endl; 

namespace COMPUTE_SERVER {


string read_(tcp::socket & socket) {  
   boost::asio::streambuf buf;

   boost::asio::read_until( socket, buf, "\n" );  
   string data = boost::asio::buffer_cast<const char*>(buf.data());  
   return data;  
}  

void send_(tcp::socket & socket, const string& message) {  
   const string msg = message + "\n";  
   boost::asio::write( socket, boost::asio::buffer(msg) );  
}  

std::vector<std::pair<uint64_t,uint64_t>> get_provider_data(int port_number) {  
    int count = 0;
    std::vector<std::pair<uint64_t,uint64_t> > ret;
    while(count < 2){
        boost::asio::io_service io_service;  

        //listen for new connection  
        tcp::acceptor acceptor_(io_service, tcp::endpoint(tcp::v4(), port_number ));  

        //socket creation  
        tcp::socket socket_(io_service);  

        //waiting for the connection  
        acceptor_.accept(socket_);  

        //read operation
        boost::system::error_code ec;
        Shares data;  
        read(socket_ , buffer(&data, sizeof(data)), ec);
        // string message = read_(socket_);  
        cout << data.Delta << endl;  

        //write operation  
        send_(socket_, "Hello From Server!");  

        socket_.close();

        ret.push_back(std::make_pair(data.Delta,data.delta));

        count += 1;
        cout << count << endl;
    }
    return ret;  

}  

std::vector<Shares> read_struct(tcp::socket & socket,int num_elements) {
    cout << "Before reading of data\n";
    std::vector<Shares> data;
    for(int i=0;i<num_elements;i++) {
        boost::system::error_code ec;
        uint64_t arr[2];
        read(socket , boost::asio::buffer(&arr,sizeof(arr)), ec); 
        Shares temp;
        temp.Delta = arr[0];
        temp.delta = arr[1];
        data.push_back(temp);
    }

    return data;
}

std::pair <std::vector<Shares>,int > get_provider_dot_product_data(int port_number) {
    boost::asio::io_service io_service;  

    //listen for new connection  
    tcp::acceptor acceptor_(io_service, tcp::endpoint(tcp::v4(), port_number ));  

    //socket creation  
    tcp::socket socket_(io_service);  

    //waiting for the connection  
    acceptor_.accept(socket_);  

    //read operation
    boost::system::error_code ec;
    int num_elements;  
    read(socket_ , boost::asio::buffer(&num_elements, sizeof(num_elements)), ec);
    if(ec) {
        cout << ec << "\n";
    } else {
        cout << "No Error\n";
    }
    // string message = read_(socket_);  
    cout << "The number of elements are :- " << num_elements << endl;  

    //write operation  
    const string msg = "Server has received the number of elements \n"; 
    boost::asio::write( socket_, boost::asio::buffer(msg) ); 
    cout << "Servent sent message to Client!" << endl;
    
    //Read the data in the reuired format
    std::vector<Shares> data = read_struct(socket_, num_elements);

    socket_.close();

    for(int i=0;i<num_elements;i++) {
        std::cout << data[i].Delta << "  " << data[i].delta << " ";
        std::cout << "\n\n";
    }
    std::cout << "Finished reading input \n\n";

    return std::make_pair(data,num_elements);

}


std::pair<std::size_t ,std::pair<std::vector<Shares>,std::vector<int> > >get_provider_mat_mul_data(int port_number) {
    boost::asio::io_service io_service;  

    //listen for new connection  
    tcp::acceptor acceptor_(io_service, tcp::endpoint(tcp::v4(), port_number ));  

    //socket creation  
    tcp::socket socket_(io_service);  

    //waiting for the connection  
    acceptor_.accept(socket_);  

    // Read and write the number of fractional bits
    boost::system::error_code ec;
    size_t fractional_bits;
    read(socket_ , boost::asio::buffer(&fractional_bits, sizeof(fractional_bits)), ec);
    if(ec) {
        cout << ec << "\n";
    } else {
        cout << "No Error\n";
    }

    const string msg_init = "Server has received the number of fractional bits \n"; 
    boost::asio::write( socket_, boost::asio::buffer(msg_init) ); 
    cout << "Servent sent message to Client!" << endl;

    //read operation
    int arr[2];  
    read(socket_ , boost::asio::buffer(&arr, sizeof(arr)), ec);
    if(ec) {
        cout << ec << "\n";
    } else {
        cout << "No Error\n";
    }
    std::vector<int> dims;
    dims.push_back(arr[0]);
    dims.push_back(arr[1]); 
    int num_elements = arr[0]*arr[1];

    cout << "The number of elements are :- " << num_elements << endl;  

    //write operation  
    const string msg = "Server has received the number of elements \n"; 
    boost::asio::write( socket_, boost::asio::buffer(msg) ); 
    cout << "Servent sent message to Client!" << endl;
    
    //Read the data in the reuired format
    std::vector<Shares> data = read_struct(socket_, num_elements);

    socket_.close();
    std::cout << "Finished reading input \n\n";

    return std::make_pair(fractional_bits,std::make_pair(data,dims));
}

}