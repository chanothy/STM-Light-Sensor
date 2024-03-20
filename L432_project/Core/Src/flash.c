#include "main.h"
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <flash.h>
#include "stm32l4xx_hal.h"

extern int _edata;
extern int _sdata;
extern int __fini_array_end;
extern RTC_HandleTypeDef hrtc;
extern flash_status_t fs;

extern ADC_HandleTypeDef hadc1;
extern TIM_HandleTypeDef htim2;

int flash_erase(void) {
	uint32_t FirstPage = 0, NbOfPages = 0, BankNumber = 0;
	uint32_t PAGEError = 0;
	/* __IO uint32_t data32 = 0 , MemoryProgramStatus = 0; */

	static FLASH_EraseInitTypeDef EraseInitStruct;
	uint32_t program_end = ((uint32_t) &__fini_array_end + (uint32_t) &_edata
			- (uint32_t) &_sdata);
	uint32_t bottom = (((uint32_t) program_end & ~0x7FF) + 0x800);
	uint32_t top = FLASH_END;

	HAL_FLASH_Unlock();
	/* Clear OPTVERR bit set on virgin samples */
	__HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_OPTVERR);
	/* Get the 1st page to erase */
	FirstPage = GetPage(bottom);
	/* Get the number of pages to erase from 1st page */
	NbOfPages = GetPage(top) - FirstPage + 1;
	/* Get the bank */
	BankNumber = GetBank(bottom);
	/* Fill EraseInit structure*/
	EraseInitStruct.TypeErase = FLASH_TYPEERASE_PAGES;
	EraseInitStruct.Banks = BankNumber;
	EraseInitStruct.Page = FirstPage;
	EraseInitStruct.NbPages = NbOfPages;

	/* Note: If an erase operation in Flash memory also concerns data in the data or instruction cache,
	 you have to make sure that these data are rewritten before they are accessed during code
	 execution. If this cannot be done safely, it is recommended to flush the caches by setting the
	 DCRST and ICRST bits in the FLASH_CR register. */
	if (HAL_FLASHEx_Erase(&EraseInitStruct, &PAGEError) != HAL_OK) {
		HAL_FLASH_Lock();
		return (-1);
	}
	HAL_FLASH_Lock();
	return (0);
}

static uint32_t GetPage(uint32_t Addr) {
	uint32_t page = 0;

	if (Addr < (FLASH_BASE + FLASH_BANK_SIZE)) {
		/* Bank 1 */
		page = (Addr - FLASH_BASE) / FLASH_PAGE_SIZE;
	} else {
		/* Bank 2 */
		page = (Addr - (FLASH_BASE + FLASH_BANK_SIZE)) / FLASH_PAGE_SIZE;
	}

	return page;
}

static uint32_t GetBank(uint32_t Addr) {
	return FLASH_BANK_1;
}

int write_record(flash_status_t * fs, void * record) {
  raw_t * write_data;
  HAL_StatusTypeDef status = 0;

  if ((!fs) || (!record)) {
    return (-1);
  }

  if (fs->total_records >= fs->max_possible_records) {
    printf("Flash full\n\r");
    return (-1);
  }

  write_data = (raw_t *) record;

  HAL_FLASH_Unlock();
  if ((status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD,(int) fs->next_address++, write_data->data0))) {
    HAL_FLASH_Lock();
    return (-1);
  }
  if ((status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD,(int) fs->next_address, write_data->data1))) {
    HAL_FLASH_Lock();
    return (-1);
  }
  HAL_FLASH_Lock();
  fs->next_address-=3;
  fs->next_record_number++;
  fs->total_records++;
  return (0);
}


uint64_t* find_sentinel_bottom(void) {
	uint64_t *p = (uint64_t*) &__fini_array_end
			+ ((uint64_t*) &_edata - (uint64_t*) &_sdata);
	p += 2;
	p = (uint64_t*) ((uintptr_t) p & ~(uintptr_t) 0xF);
	while (p <= ((uint64_t*) FLASH_END)) {
		if (*p == SENTINEL_MARK_BOTTOM) {
			return (p);
		}
		p += 2;
	}
	return (0);
}

uint64_t* find_sentinel_top(void) {
	uint64_t *p = (uint64_t*) FLASH_END;
	if (*p == SENTINEL_MARK_TOP) {
		return (p);
	} else {
		return (0);
	}
}

int write_sentinel(uint64_t *location, raw_t *sentinel) {
	if ((!location) || (!sentinel)) {
		return (-1);
	}
	HAL_FLASH_Unlock();
	if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, (int) location++,
			sentinel->data0)) {
		HAL_FLASH_Lock();
		return (-1);
	}
	if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, (int) location,
			sentinel->data1)) {
		HAL_FLASH_Lock();
		return (-1);
	}
	HAL_FLASH_Lock();
	return (0);
}

int flash_write_init(flash_status_t *fs) {
	sensordata_t *sd = 0;
	uint16_t record_counter = 0;
	raw_t sentinel_top = { SENTINEL_MARK_TOP, 0 };
	raw_t sentinel_bottom = { SENTINEL_MARK_BOTTOM, 0 };

	uint64_t *program_end = (uint64_t*) ((uint32_t) &__fini_array_end
			+ (uint32_t) &_edata - (uint32_t) &_sdata);

	uint64_t *top = (uint64_t*) FLASH_END;

	uint64_t *bottom = (uint64_t*) (((uint32_t) program_end & ~0x7FF) + 0x800);

	uint64_t *pt = find_sentinel_top();

	uint64_t *pb = find_sentinel_bottom();

	if ((!pt) && (!pb)) {
		if (write_sentinel(top, &sentinel_top)) {
			return (-1);
		}
		if (write_sentinel(bottom, &sentinel_bottom)) {
			return (-1);
		}
	} else if ((pt) && (!pb)) {
		if (write_sentinel(bottom, &sentinel_bottom)) {
			return (-1);
		}
	} else if ((!pt) && (pb)) {
		return (-1);
	} else if ((pt) && (pb)) {
		if (pb != bottom) {

		}
	} else {
		return (-1);
	}

	fs->data_start = top - 2;
	fs->max_possible_records = (((uint32_t) top - (uint32_t) bottom) >> 4) - 1;
	sd = (sensordata_t*) top;
	sd--;
	while ((sd->watermark != 0xFF)) {
		record_counter++;
		sd--;
	}
	fs->next_record_number = record_counter;
	fs->total_records = record_counter;
	fs->next_address = (uint64_t*) sd;

	return (0);
}

int write_sensor_data(flash_status_t *fs) {
	sensordata_t sensor_data = { .watermark = 0x01, .status = 0x01,
			.record_number = 4, .timestamp = 0x1234567, .battery_voltage = 33,
			.temperature = 25, .sensor_period = 100 };

	logdata_t log_data = { .watermark = 0x01, .status = 0x02, .record_number = 2,
			.timestamp = 0x87654321, .msg = "Hello!" };

	raw_t raw_data = { .data0 = *(uint64_t*) &sensor_data, .data1 =
			*(uint64_t*) &log_data };

	if (write_record(fs, &raw_data)) {
		printf("Write failed\n\r");
	} else {
		printf("Write success\n\r");
	}
	printf("%d\n\r",fs->total_records);
	printf("%u\n\r",fs->next_address);
}

int read_all_records(flash_status_t * fs, int type) {
  sensordata_t * p = (sensordata_t *) fs->data_start;
  RTC_TimeTypeDef time;
  RTC_DateTypeDef date;
  int value;
  uint8_t * q;
  uint8_t log_length;
  uint8_t count;
  int i;

  if (p->watermark == 0xFF) {
    printf("OK\n\r");
    return (0);
  }
  else {
    while ((p->watermark!=0xFF)&&(p->watermark!=0xa5)) {
      switch (p->status) {
      case DATA_RECORD:
        if ((type == DATA_RECORD) || (type == ALL_RECORD)) {
          unpack_time(p->timestamp,&time,&date);
          printf("D,");
          printf("%d,",p->record_number);
          printf("%02d/%02d/20%02d,",date.Month,date.Date,date.Year);
          printf("%02d:%02d:%02d,",time.Hours,time.Minutes,time.Seconds);
          printf("%d.%03d,",(int) p->battery_voltage/1000,(int) p->battery_voltage%1000);
          printf("%d,",p->temperature);
          printf("%d,",p->light_data);
          value = cal_lookup(p->light_data);
          if ((value == 1) ||
              (value == -1)) {
            printf("%d\n\r",value);
          }
          else {
            value = cal_compensate_magnitude(value);
            printf("%d.%02d\n\r",value/100,value%100);
          }
        }
        p--;
        break;
      case LOG_RECORD:
        log_length = ((logdata_t *)p)->length;
        if ((type == LOG_RECORD) || (type == ALL_RECORD)) {
          unpack_time(p->timestamp,&time,&date);
          printf("L,");
          printf("%d,",p->record_number);
          printf("%02d/%02d/20%02d,",date.Month,date.Date,date.Year);
          printf("%02d:%02d:%02d,",time.Hours,time.Minutes,time.Seconds);
          if (log_length <= 7) {
            printf("%s\n\r",((logdata_t *)p)->msg);
            p--;
          }
          else {
            // Print the first 7 characters in the log record
            q = ((logdata_t *)p)->msg;
            count = 7;
            while (count > 0) {
              printf("%c",*q);
              q++;
              count--;
            }
            // Print the remaining exension records.
            count = log_length - 7; // Account for the first 7 characters
            p--;                    // point at the next record, type logex_t
            while (count/16) {
              q = (uint8_t *) p;
              for (i=0;i<16;i++) {
                printf("%c",*q);
                q++;
                count--;
              }
              p--;
            }
            if (count%16) {
              printf("%s",((logex_t *)p)->msg);
              /* q = ((logdata_t *)p)->msg; */
              /* for (i=0;i<(count%16);i++) { */
              /*   printf("%c",*q); */
              /*   q++; */
              /* } */
              p--;
            }
            printf("\n\r");
            /* p = p - (log_length-7)/16; */
            /* // Handle the additional partial frame */
            /* if ((log_length-7)%16) { */
            /*   p--; */
            /* } */
          }
        }
        else {
          if (log_length < 8) {            // Short Record
            p--;
          }
          else {                           // Long Record
            p--;                           // Head Log Record
            p = p - (log_length-7)/16;     // Full Extension Records
            if ((log_length-7)%16) {       // Partial Extension Record
              p--;
            }
          }
        }
        break;
      default:
        p--;
        printf("NOK\n\r");
      }
    }
    printf("OK\n\r");
  }
  return(0);
}
