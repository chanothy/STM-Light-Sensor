typedef struct command {
  char * cmd_string;
  void (*cmd_function)(char * arg);
} command_t;

void prompt();
int get_commands();
int execute_command();
int parse_command();
void lof_command();
void lon_command();
void test_command();

//command_t commands[] = {
//  {"help",help_command},
//  {"lof",lof_command},
//  {"lon",lon_command},
//  {"test",test_command},
//  {0,0}
//};

