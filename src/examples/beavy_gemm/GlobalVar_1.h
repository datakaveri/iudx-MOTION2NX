#ifndef GLOBALVAR_1_H
#define GLOBALVAR_1_H
bool helpernode_ready_flag = false;
bool XABY_receive_flag = false;
bool YABY_receive_flag = false;
bool OT_flag = false;

std::vector<std::uint64_t> x_temp_vec, y_temp_vec;
std::vector<std::uint64_t> OT_vec;

std::uint64_t fractional_bits;

int my_id = 1;
int other_party = 0;

int helpernode_id = 2; 
enum MessageType {XArithToABY, YArithToABY, HelperNodeSync, X_PrivateShares, Y_PrivateShares, OT};

std::unique_ptr<MOTION::Communication::CommunicationLayer> comm_layer;
#endif