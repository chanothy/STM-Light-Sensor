#include <stm32l4xx_ll_usart.h>
#include <stdio.h>
#include <queue.h>
#include <stm32l4xx_it.h>
#include <main.h>
#include <string.h>

void prompt() {
	printf("> ");
}

void get_command(uint8_t command[]) {
   extern queue_t buf;
   //extern uint8_t command[16];
   //static uint32_t counter = 0;
   uint8_t data;

   const char *led_on = "lon";
   const char *led_off = "lof";
   const char *help = "help";
	printf("\r\nHead / Tail: %d %d\n\r",head(&buf),tail(&buf)); //print where the ptrs are
//	  data = dequeue(&buf);
//	  while (data!=0) {
//		  command[counter] = data;
//		  counter++;
//		  data = dequeue(&buf);
//	  }
	  char command_str[17]; // Assuming command has a maximum length of 16 bytes
	  snprintf(command_str, sizeof(command_str), "%s", command); //turns command into str
	  printf("Command String: %s\n\r", command_str);

	  if (strstr(command_str,led_on) != NULL) {
		  printf("led_on\n\r");
		  HAL_GPIO_WritePin(LD3_GPIO_Port, LD3_Pin, 1);
	  }
	  else if (strstr(command_str,led_off) != NULL) {
		  printf("led_off\n\r");
		  HAL_GPIO_WritePin(LD3_GPIO_Port, LD3_Pin, 0);
	  }
	  else if (strstr(command_str,help) != NULL) {
		  help_command();
	  }
	  else {
		  printf("invalid_command\n\r");
	  }
	  memset(command, 0, sizeof(command));
	  //counter = 0;
	  prompt();
}

void help_command() {
	printf("Available Commands:\n\r");
	printf("help\n\r");
	printf("lof\n\r");
	printf("lon\n\r");
	printf("test\n\r");
}

void lof_command() {

}

void lon_command() {

}

void test_command(char *arguments) {

}
