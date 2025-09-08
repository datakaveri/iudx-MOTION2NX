bool helpernode_ready_flag = false;
bool XABY_receive_flag = false;
bool YABY_receive_flag = false;

std::vector<std::uint64_t> X_arith, X_ABY_public, X_ABY_private;
std::vector<std::uint64_t> Y_arith, Y_ABY_public, Y_ABY_private;

std::vector<std::uint8_t> msg_ABY_shares_for_mult;
std::vector<std::uint64_t> for_test;

std::uint64_t fractional_bits;

int my_id = 0;
int other_party = 1;

int helpernode_id = 2; 
enum MessageType {XArithToABY, YArithToABY, HelperNodeSync};