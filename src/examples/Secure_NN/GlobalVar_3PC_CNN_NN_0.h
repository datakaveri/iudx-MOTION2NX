#ifndef GLOBALVAR_CNN_NN_0_H
#define GLOBALVAR_CNN_NN_0_H
bool helpernode_ready_flag = false;
bool XABY_receive_flag = false;
bool YABY_receive_flag = false;
bool OT_flag = false;
bool relu_ready_flag = false;
bool reluABYAck_Flag = false;
bool matrix_add_mul_flag_1 = false;
bool conv_flag_1 = false;

//ShareConvert related flags
bool SC_P_flag = false; //Prime shares
bool SC_D_flag = false; // delta shares
bool SC_PC_flag = false; // c shares to compute 

// ComputeMSB related flags
bool helpernode_computeMSB_ready_flag = false;  // Flag set after ShareConvert completion
bool MSB_Odd_flag = false;                  // x shares in odd world
bool MSB_P_flag = false;                    // x Prime shares
bool MSB_L_LSB_flag = false;                // x LSB shares
bool MSB_R_flag = false;                    // Shares of r
bool MSB_PC_flag = false;                   // PC Shares for ComputeMSB

// ArithToABY related flags
bool OtherPartySync_Flag = false;           // Flag to establish connection
bool ArithToABY_Flag = false;               // Transfer of shares

// ReLU related flags
bool helpernode_ReLU_ready_flag = false;
// XABY_receive_flag, YABY_receive_flag and OT_flag are also used for Hadamard Matrix Multiplication in ReLU.
// These flags are reset after their use in ComputeMSB to false.

std::vector<std::uint64_t> x_pr_bit, del_odd, etaP_odd;
std::vector<std::uint64_t> MSB_x_Odd, MSB_x_P, MSB_x_L_LSB;
std::vector<std::uint64_t> R_Shares, betaP;

// Vectors used in ComputeMSB for Hadamard Matrix Multiplication
std::vector<std::uint64_t> x_temp_vec, y_temp_vec;
std::vector<std::uint64_t> OT_vec;

// Vectors used in ReLU for Hadamard Matrix Multiplication
std::vector<std::uint64_t> Relu_x_temp_vec, Relu_y_temp_vec, Relu_OT_vec;

// Vectors used in ArithToABY 
std::vector<std::uint64_t> Relu_Public_Shares_Other_Party;

std::vector<std::uint64_t> A_MatMul_Public_Shares, A_MatMul_Private_Shares;
std::vector<std::uint64_t> A_Conv_Public_Shares, A_Conv_Private_Shares;

std::vector<std::uint64_t> Output_Public_Shares, Output_Private_Shares;

std::uint64_t fractional_bits;

// Vectors and global variables used in MatrixMultAddition
std::vector<std::uint64_t> R;
std::vector<std::uint64_t> randomnum, prod1;
std::vector<std::uint64_t> wpublic, xpublic, wsecret, xsecret, bpublic, bsecret;
int operations_done_flag = 0;

// Global variables used in ConvolutionOperation
std::uint64_t conv_kernels, conv_channels, conv_rows, conv_cols;
std::uint64_t pads[4], strides[2];
std::uint64_t image_rows, image_cols, image_channels;
std::uint64_t output_rows, output_columns, output_chnls;
std::uint64_t b_rows, b_cols;

int my_id = 0;
int other_party = 1;
int helpernode_id = 2;

enum MessageType {SC, SC_P, SC_D, SC_PC, XArithToABY, YArithToABY, HelperNodeSync, X_PrivateShares, Y_PrivateShares, OT, ComputeMSBReady, MSB_Odd, MSB_P, MSB_L_LSB, MSB_R, MSB_PC, ReluReady, Relu_XArithToABY, Relu_YArithToABY, Relu_X_PrivateShares, Relu_Y_PrivateShares, Relu_OT, OtherPartySync, ArithToABY, MatrixMulAddOpMessage, ConvolutionOpMessage, ReluSync, reluABYAck};

std::unique_ptr<MOTION::Communication::CommunicationLayer> comm_layer;

const auto LMINUS_ONE = std::numeric_limits<std::uint64_t>::max();
const uint64_t PRIME_NUM = 67;
uint64_t addModPrime[PRIME_NUM][PRIME_NUM];
uint64_t multModPrime[PRIME_NUM][PRIME_NUM];
uint64_t subModPrime[PRIME_NUM][PRIME_NUM];
const auto BIT_SIZE = 64;

#endif