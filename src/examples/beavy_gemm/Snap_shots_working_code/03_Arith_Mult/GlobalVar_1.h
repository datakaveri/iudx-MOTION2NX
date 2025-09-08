#ifndef GLOBALVAR_1_H
#define GLOBALVAR_1_H

bool helpernode_ready_flag = false;
bool XABY_receive_flag = false;
bool YABY_receive_flag = false;
bool OT_flag = false;

std::vector<std::uint64_t> X_arith, X_ABY_public, X_ABY_private;
std::vector<std::uint64_t> Y_arith, Y_ABY_public, Y_ABY_private;
std::vector<std::uint64_t> Z_arith, Z_ABY_public, Z_ABY_private;

std::vector<std::uint64_t> OT_vec;

std::vector<std::uint8_t> X_msg_ABY_shares, Y_msg_ABY_shares;
std::vector<std::uint8_t> X_msg_Private_Shares, Y_msg_Private_Shares;

std::vector<std::uint64_t> for_test;

std::uint64_t fractional_bits;

int my_id = 1;
int other_party = 0;

int helpernode_id = 2; 
enum MessageType {XArithToABY, YArithToABY, HelperNodeSync, X_PrivateShares, Y_PrivateShares, OT};

#endif