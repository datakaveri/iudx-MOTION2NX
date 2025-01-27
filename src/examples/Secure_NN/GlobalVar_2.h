#ifndef GLOBALVAR_2_H
#define GLOBALVAR_2_H
bool server0_ready_flag = false, server1_ready_flag = false;
bool X0_Priv_Share_Flag = false, X1_Priv_Share_Flag = false;
bool Y0_Priv_Share_Flag = false, Y1_Priv_Share_Flag = false;



std::vector<std::uint64_t> X0_Priv, X1_Priv;
std::vector<std::uint64_t> Y0_Priv, Y1_Priv;

// Vectors used in ShareConvert (received from P0, P1 )
std::vector<std::uint64_t> a0_tilde, a1_tilde;
std::vector<std::uint64_t> c0, c1; // Input to PrivateCompare

bool SC0_flag = false,  SC1_flag = false;
bool SC_PC0_flag = false,  SC_PC1_flag = false;

int my_id = 2;
int server0 = 0;
int server1 = 1;

enum MessageType {SC, SC_P, SC_D, SC_PC, XArithToABY, YArithToABY, HelperNodeSync, X_PrivateShares, Y_PrivateShares, OT};
std::unique_ptr<MOTION::Communication::CommunicationLayer> comm_layer;


const auto LMINUS_ONE = std::numeric_limits<std::uint64_t>::max();
const uint64_t PRIME_NUM = 67;
uint64_t addModPrime[PRIME_NUM][PRIME_NUM];
uint64_t multModPrime[PRIME_NUM][PRIME_NUM];
uint64_t subModPrime[PRIME_NUM][PRIME_NUM];
const auto BIT_SIZE = 64;
#endif