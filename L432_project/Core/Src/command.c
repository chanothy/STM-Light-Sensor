#include <stm32l4xx_ll_usart.h>
#include <stdio.h>
#include <queue.h>
#include <stm32l4xx_it.h>
#include <main.h>
#include <string.h>
#include <interrupt.h>

void help_command(char *);
void lof_command(char *);
void lon_command(char *);
void test_command(char *);

void prompt() {
	printf("> ");
}

extern queue_t buf;

typedef struct command {
	char * cmd_string;
	void (*cmd_function)(char * arg);
} command_t;

command_t commands[] = {
	{"help",help_command},
	{"lon",lon_command},
	{"lof",lof_command},
	{"test",test_command},
	{0,0}
};


void __attribute__((weak)) help_command(char *arguments) {
	printf("Available Commands:\n\r");
	printf("help\n\r");
	printf("lof\n\r");
	printf("lon\n\r");
	printf("test\n\r");
}

void __attribute__((weak)) lof_command(char *arguments) {
	printf("led_off\n\r");
	HAL_GPIO_WritePin(LD3_GPIO_Port, LD3_Pin, 0);
}

void __attribute__((weak)) lon_command(char *arguments) {
	printf("led_on\n\r");
	HAL_GPIO_WritePin(LD3_GPIO_Port, LD3_Pin, 1);
}

void __attribute__((weak)) test_command(char *arguments) {
	printf("test_command\n\r");
	if (arguments) {
		char *pt;
		pt = strtok (arguments,",");
		while (pt != NULL) {
			char a = *pt;
			printf("%c\n\r", a);
			pt = strtok (NULL, ",");
		}
	}
}


enum {COLLECT_CHARS, COMPLETE};

int get_commands(uint8_t *command_buf) {
  static uint32_t counter=0;
  static uint32_t mode = COLLECT_CHARS;

  uint8_t ch = 0;;
  uint32_t mask;

  ch=dequeue(&buf);
  while (ch!=0) {
    if ((ch!='\n')&&(ch!='\r')) {
      if (ch==0x7f) {               // backspace functionality
        if (counter > 0) {
            printf("\b \b");
            counter--;
        }
      }
      else {
        putchar(ch);
        command_buf[counter++]=ch;
        if (counter>=(QUEUE_SIZE-2)) {
          mode=COMPLETE;
          break;
        }
      }
    }
    else {
      mode = COMPLETE;
      break;
    }
    mask = disable();
    ch=dequeue(&buf);
    restore(mask);
  }
  if (mode == COMPLETE) {
    command_buf[counter] = 0;
    printf("\n\r");
    counter = 0;
    mode = COLLECT_CHARS;
    return(1);
  }
  else {
    return(0);
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

int execute_command(uint8_t * line) { // line is buffer where command is in
  uint8_t *cmd;
  uint8_t *arg;
  command_t *p = commands;
  int success = 0;

  if (!line) {
    return (-1);
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


