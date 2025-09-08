#ifndef GLOBALVAR_MATMUL_RELU_2_H
#define GLOBALVAR_MATMUL_RELU_2_H

bool server0_ready_flag = false, server1_ready_flag = false;
bool X0_Priv_Share_Flag = false, X1_Priv_Share_Flag = false;
bool Y0_Priv_Share_Flag = false, Y1_Priv_Share_Flag = false;
bool relu_ready_flag_0 = false, relu_ready_flag_1 = false;
bool reluABY_s0_flag = false, reluABY_s1_flag = false;

std::vector<std::uint64_t> X0_Priv, X1_Priv;
std::vector<std::uint64_t> Y0_Priv, Y1_Priv;

// Vectors used in ShareConvert (received from P0, P1)
std::vector<std::uint64_t> a0_tilde, a1_tilde;
std::vector<std::uint64_t> c0, c1; // Input to PrivateCompare

// Flags used in ShareConvert
bool SC0_flag = false,  SC1_flag = false;
bool SC_PC0_flag = false,  SC_PC1_flag = false;

// Vectors used in ComputeMSB (received from P0, P1)
std::vector<std::uint64_t> MSB_len0, MSB_len1;
std::vector<std::uint64_t> MSB_c0, MSB_c1;

// Flags used in ComputeMSB
bool server0_compute_msb_ready_flag = false, server1_compute_msb_ready_flag = false;
bool MSB_PC_0_flag = false, MSB_PC_1_flag = false;

// Flags used in ReLU
bool server0_relu_ready_flag = false, server1_relu_ready_flag = false;
// X0_Priv_Share_Flag, X1_Priv_Share_Flag, Y0_Priv_Share_Flag and Y1_Priv_Share_Flag are also used for Hadamard Matrix Multiplication in ReLU.
// These flags are reset after their use in ComputeMSB to false.

// Vectors used in ReLU to store shares received from P0 and P1
std::vector<std::uint64_t> Relu_X0_Priv, Relu_X1_Priv, Relu_Y0_Priv, Relu_Y1_Priv;

// 
std::vector<std::uint64_t> x0, w0;
std::vector<std::uint64_t> x1, w1;
int Conv_c1 = 1, Conv_c2 = 1, Conv_c3 = 1, Conv_c4 = 1;
std::uint64_t w_rows = 0, w_cols = 0, x_rows = 0, x_cols = 0;
std::uint64_t pads[4], strides[2];
std::vector<std::uint8_t> msg_Z, msg_R;
int flag = 0;
int WriteToFiles = 1;
int rows, col;

bool operations_done_flag = false;

int my_id = 2;
int server0 = 0;
int server1 = 1;

enum MessageType {SC, SC_P, SC_D, SC_PC, XArithToABY, YArithToABY, HelperNodeSync, X_PrivateShares, Y_PrivateShares, OT, ComputeMSBReady, MSB_Odd, MSB_P, MSB_L_LSB, MSB_R, MSB_PC, ReluReady, Relu_XArithToABY, Relu_YArithToABY, Relu_X_PrivateShares, Relu_Y_PrivateShares, Relu_OT, OtherPartySync, ArithToABY, MatrixMulAddOpMessage, ReluSync, reluABYAck};
std::unique_ptr<MOTION::Communication::CommunicationLayer> comm_layer;


const auto LMINUS_ONE = std::numeric_limits<std::uint64_t>::max();
const uint64_t PRIME_NUM = 67;
uint64_t addModPrime[PRIME_NUM][PRIME_NUM];
uint64_t multModPrime[PRIME_NUM][PRIME_NUM];
uint64_t subModPrime[PRIME_NUM][PRIME_NUM];
const auto BIT_SIZE = 64;
const int FIXED_POINT = 13;

#endif