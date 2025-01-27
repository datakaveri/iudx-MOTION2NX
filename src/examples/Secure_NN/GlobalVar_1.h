#ifndef GLOBALVAR_1_H
#define GLOBALVAR_1_H
bool helpernode_ready_flag = false;
bool XABY_receive_flag = false;
bool YABY_receive_flag = false;
bool OT_flag = false;
//ShareConvert related flags
bool SC_P_flag = false; //Prime shares
bool SC_D_flag = false; // delta shares
bool SC_PC_flag = false; // c shares to compute 
std::vector<std::uint64_t> x_pr_bit, del_odd, etaP_odd;



std::vector<std::uint64_t> x_temp_vec, y_temp_vec;
std::vector<std::uint64_t> OT_vec;

std::uint64_t fractional_bits;

int my_id = 1;
int other_party = 0;

int helpernode_id = 2; 
enum MessageType {SC, SC_P, SC_D, SC_PC, XArithToABY, YArithToABY, HelperNodeSync, X_PrivateShares, Y_PrivateShares, OT};

std::unique_ptr<MOTION::Communication::CommunicationLayer> comm_layer;


const auto LMINUS_ONE = std::numeric_limits<std::uint64_t>::max();
const uint64_t PRIME_NUM = 67;
uint64_t addModPrime[PRIME_NUM][PRIME_NUM];
uint64_t multModPrime[PRIME_NUM][PRIME_NUM];
uint64_t subModPrime[PRIME_NUM][PRIME_NUM];
const auto BIT_SIZE = 64;

#endif