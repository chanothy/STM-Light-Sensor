#include <stm32l4xx_ll_usart.h>
#include <stdio.h>
#include <queue.h>
#include <stm32l4xx_it.h>
#include <main.h>
#include <string.h>

void prompt() {
	printf("> ");
}
void help_command() {
	printf("Available Commands:\n\r");
	printf("help\n\r");
	printf("lof\n\r");
	printf("lon\n\r");
	printf("test\n\r");
}

void test_command(char *arguments) {

}
void lof_command() {
	printf("led_off\n\r");
	HAL_GPIO_WritePin(LD3_GPIO_Port, LD3_Pin, 0);
}

void lon_command() {
	printf("led_on\n\r");
	HAL_GPIO_WritePin(LD3_GPIO_Port, LD3_Pin, 1);
}
void get_command(uint8_t command[]) {
	const char *led_on = "lon";
	const char *led_off = "lof";
	const char *help = "help";

	if (strstr(command,led_on) != NULL) {
	  lon_command();
	}
	else if (strstr(command,led_off) != NULL) {
	  lof_command();
	}
	else if (strstr(command,help) != NULL) {
	  help_command();
	}
	else {
	  printf("invalid_command\n\r");
	}
	prompt();
}


