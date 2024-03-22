#include <stm32l4xx_ll_usart.h>
#include <stdio.h>
#include <queue.h>
#include <stm32l4xx_it.h>
#include <main.h>
#include <string.h>
#include <interrupt.h>
#include <stdlib.h>
#include <flash.h>

void help_command(char *);
void lof_command(char *);
void lon_command(char *);
void test_command(char *);
void ts_command(char *);
void ds_command(char *);
void tsl237_command(char *);
void temp_command(char *);
void battery_command(char *);
void sample_command(char *);
void log_command(char *);
void data_command(char *);
void total_records_command(char *);
void erase_command(char *);


extern RTC_HandleTypeDef hrtc;
uint32_t format = RTC_FORMAT_BIN;
extern flash_status_t fs;

void prompt() {
	RTC_DateTypeDef date;
	RTC_TimeTypeDef time;
	HAL_RTC_GetDate(&hrtc,&date,format);
	HAL_RTC_GetTime(&hrtc,&time,format);
	printf("%02d/%02d/%02d ",date.Month,date.Date,date.Year);
	printf("%02d:%02d:%02d ",time.Hours,time.Minutes,time.Seconds);
	printf("IULS> ");
}

extern queue_t buf;
extern uint32_t period;

typedef struct command {
	char * cmd_string;
	void (*cmd_function)(char * arg);
} command_t;

command_t commands[] = {
	{"help",help_command},
	{"lon",lon_command},
	{"lof",lof_command},
	{"test",test_command},
	{"ts",ts_command},
	{"ds",ds_command},
	{"tsl237",tsl237_command},
	{"temp",temp_command},
	{"battery",battery_command},
	{"sample",sample_command},
	{"log",log_command},
	{"data",data_command},
	{"total",total_records_command},
	{"ef",erase_command},
	{0,0}
};


void __attribute__((weak)) help_command(char *arguments) {
	printf("Available Commands:\n\r");
	printf("help\n\r");
	printf("lof\n\r");
	printf("lon\n\r");
	printf("test\n\r");
	printf("ts\n\r");
	printf("ds\n\r");
	printf("tsl237\n\r");
	printf("temp\n\r");
	printf("battery\n\r");
	printf("sample\n\r");
	printf("total\n\r");
}

void __attribute__((weak)) lof_command(char *arguments) {
	HAL_GPIO_WritePin(LD3_GPIO_Port, LD3_Pin, 0);
}

void __attribute__((weak)) lon_command(char *arguments) {
	HAL_GPIO_WritePin(LD3_GPIO_Port, LD3_Pin, 1);
}

void __attribute__((weak)) test_command(char *arguments) {
	printf("test_command\n\r");
	if (arguments) {
		char *pt;
		pt = strtok (arguments,",");
		while (pt != NULL) {
//			char a = *pt;
			printf("%s\n\r", pt);
			pt = strtok (NULL, ",");
		}
	}
}

void __attribute__((weak)) ts_command(char *arguments) {
	int t[3];
	if (arguments) {
		char *pt;
		pt = strtok(arguments, ",");
		for (int i = 0; i < 3; i++) {
			if (pt != NULL) {
				t[i] = atoi(pt);
				pt = strtok(NULL, ",");
			}
		}
	}

	if (t[0] && (t[0]>-1) && (t[0]<24) && t[1] && (t[1]>-1) && (t[1]<60) && t[2] && (t[2]>-1) && (t[2]<60)) {
		RTC_TimeTypeDef time;
		time.Hours = t[0];
		time.Minutes = t[1];
		time.Seconds = t[2];
		HAL_RTC_SetTime(&hrtc,&time,format);
	}
	else {
		printf("NOK\n\r");
	}
}

void __attribute__((weak)) ds_command(char *arguments) {
	int d[3];
	if (arguments) {
		char *pt;
		pt = strtok(arguments, ",");
		for (int i = 0; i < 3; i++) {
			if (pt != NULL) {
				d[i] = atoi(pt);
				pt = strtok(NULL, ",");
			}
		}
	}
	if (d[0] && (d[0]>0) && (d[0]<13) && d[1] && d[2]) {
		RTC_DateTypeDef date;
		date.Month = d[0];
		date.Date = d[1];
		date.Year = d[2];
		HAL_RTC_SetDate(&hrtc,&date,format);
	}
	else {
		printf("NOK\n\r");
	}
}

extern float tsl237_clock_frequency;

void __attribute__((weak)) tsl237_command(char *arguments) {
  int freqInt = tsl237_clock_frequency * 1000;
  int freqFirst = freqInt / 1000;
  int freqSecond = freqInt % 1000;
  printf("%d.%d hz\n\r", freqFirst, freqSecond);
}

extern sensorval_t mySensors;

void __attribute__((weak)) temp_command(char *arguments) {
	printf("%ld C\n\r", mySensors.temperature);
}

void __attribute__((weak)) battery_command(char *arguments) {
	int battInt = mySensors.vdda_value;
	int battFirst = battInt / 1000;
	int battSecond = battInt % 1000;
  printf("%d.%d V\n\r", battFirst, battSecond);
}

void __attribute__((weak)) sample_command(char *arguments) {
	store_sensor_data(&fs,mySensors.vdda_value,mySensors.temperature,period);
	if (arguments) {
			char *pt;
			pt = strtok (arguments,",");
			store_log_data(&fs, pt);
		}
	else {
		store_log_data(&fs, "none");
	}
}

void __attribute__((weak)) erase_command(char *arguments) {
	flash_erase();
  flash_write_init(&fs);
}

void __attribute__((weak)) total_records_command(char *arguments) {
	printf("Flash Total Records: %d\n\r",fs.total_records);
}


void __attribute__((weak)) data_command(char *arguments) {
	read_all_records(&fs,1);
}

void __attribute__((weak)) log_command(char *arguments) {
	read_all_records(&fs,2);
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




