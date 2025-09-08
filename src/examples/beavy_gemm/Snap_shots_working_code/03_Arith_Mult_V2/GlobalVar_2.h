#ifndef GLOBALVAR_2_H
#define GLOBALVAR_2_H
bool server0_ready_flag = false, server1_ready_flag = false;
bool X0_Priv_Share_Flag = false, X1_Priv_Share_Flag = false;
bool Y0_Priv_Share_Flag = false, Y1_Priv_Share_Flag = false;

std::vector<std::uint64_t> X0_Priv, X1_Priv;
std::vector<std::uint64_t> Y0_Priv, Y1_Priv;


int my_id = 2;
int server0 = 0;
int server1 = 1;

enum MessageType {XArithToABY, YArithToABY, HelperNodeSync, X_PrivateShares, Y_PrivateShares, OT};
std::unique_ptr<MOTION::Communication::CommunicationLayer> comm_layer;
#endif