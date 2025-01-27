bool server0_ready_flag = false, server1_ready_flag = false;
bool sent_2_server0 = false;
bool sent_2_server1 = false;

bool sent_3_server0 = false;
bool sent_3_server1 = false;

bool sent_4_server0 = false;
bool sent_4_server1 = false;

int my_id = 2;
int server0 = 0;
int server1 = 1;

enum MessageType {XArithToABY, YArithToABY, HelperNodeSync};