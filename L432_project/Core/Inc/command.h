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
void help_command();
void ts_command();
void ds_command();
void tsl237_command();
void temp_command();
void battery_command();
void sample_command();
void log_command();
void data_command();
void total_records_command();
void manual_log_command();



