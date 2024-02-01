#include <stm32l4xx_ll_usart.h>
#include <stdio.h>
#include <queue.h>
#include <stm32l4xx_it.h>
#include <main.h>
#include <string.h>

void help_command(char *);
void lof_command(char *);
void lon_command(char *);

void prompt() {
	printf("> ");
}

typedef struct command {
	char * cmd_string;
	void (*cmd_function)(char * arg);
} command_t;

command_t commands[] = {
	{"help",help_command},
	{"lon",lon_command},
	{"lof",lof_command},
	{0,0}
};


void __attribute__((weak)) help_command(char *arguments) {
	printf("Available Commands:\n\r");
	printf("help\n\r");
	printf("lof\n\r");
	printf("lon\n\r");
	printf("test\n\r");
}

void test_command(char *arguments) {

}

void __attribute__((weak)) lof_command(char *arguments) {
	printf("led_off\n\r");
	HAL_GPIO_WritePin(LD3_GPIO_Port, LD3_Pin, 0);
}

void __attribute__((weak)) lon_command(char *arguments) {
	printf("led_on\n\r");
	HAL_GPIO_WritePin(LD3_GPIO_Port, LD3_Pin, 1);
}

void get_command(uint8_t command[]) {
	const char *led_on = "lon";
	const char *led_off = "lof";
	const char *help = "help";

	if (strstr(command,led_on) != NULL) {
	  lon_command(0);
	}
	else if (strstr(command,led_off) != NULL) {
	  lof_command(0);
	}
	else if (strstr(command,help) != NULL) {
	  help_command(0);
	}
	else {
	  printf("invalid_command\n\r");
	}
	prompt();
}

int execute_command(uint8_t * line) {
  uint8_t *cmd;
  uint8_t *arg;
  command_t *p = commands;
  int success = 0;

  if (!line) {
    return (-1); // Passed a bad pointer
  }
  if (parse_command(line,&cmd,&arg) == -1) {
    printf("Error with parse command\n\r");
    return (-1);
  }
  while (p->cmd_string) {
    if (!strcmp(p->cmd_string,(char *) cmd)) {
      if (!p->cmd_function) {
        return (-1);
      }
      (*p->cmd_function)((char *)arg);            // Run the command with the passed arguments
      success = 1;
      break;
    }
    p++;
  }
  if (success) {
    return (0);
  }
  else {
    return (-1);
  }
}

int parse_command (uint8_t *line, uint8_t **command, uint8_t **args) {
  uint8_t *p;

  if((!line) ||
     (!command) ||
     (!args)) {
    return (-1);
  }
  *command = line;
  p = line;
  while (*p!=','){
    if (!*p) {
      *args = '\0';
      return(0);
    }
    p++;
  }
  *p++ = '\0';
  *args = p;
  return (0);
}

