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

int write_record(flash_status_t *fs, void *record) {
	raw_t *write_data;
	HAL_StatusTypeDef status = 0;

	if ((!fs) || (!record)) {
		return (-1);
	}

	if (fs->total_records >= fs->max_possible_records) {
		printf("Flash full\n\r");
		return (-1);
	}

	write_data = (raw_t*) record;

	HAL_FLASH_Unlock();
	if ((status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD,
			(int) fs->next_address++, write_data->data0))) {
		HAL_FLASH_Lock();
		return (-1);
	}
	if ((status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD,
			(int) fs->next_address, write_data->data1))) {
		HAL_FLASH_Lock();
		return (-1);
	}
	HAL_FLASH_Lock();
	fs->next_address -= 3;
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

int store_sensor_data(flash_status_t *fs, uint16_t battery_voltage,
	uint16_t temperature, int light_data) {
	RTC_DateTypeDef date;
	RTC_TimeTypeDef time;
	HAL_RTC_GetDate(&hrtc, &date, RTC_FORMAT_BIN);
	HAL_RTC_GetTime(&hrtc, &time, RTC_FORMAT_BIN);
	sensordata_t p = { 0x01, 1, fs->next_record_number, pack_time(
			&time, &date), battery_voltage, temperature, light_data };
	if (write_record(fs, &p)) {
		return (-1);
	}
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
	printf("%d\n\r", fs->total_records);
	printf("%u\n\r", fs->next_address);
}

int store_log_data(flash_status_t *fs, char* msg) {
	RTC_DateTypeDef date;
	RTC_TimeTypeDef time;
	HAL_RTC_GetDate(&hrtc, &date, RTC_FORMAT_BIN);
	HAL_RTC_GetTime(&hrtc, &time, RTC_FORMAT_BIN);
	logdata_t log_data = {
		.watermark = 0x01,
		.status = 0x02,
		.record_number = fs->next_record_number,
		.timestamp = pack_time(&time, &date),
		.msg = {0,0,0,0,0,0,0}
	};
	strncpy((char *) log_data.msg, msg, 8);
	if (write_record(fs, &log_data)) {
		printf("Write failed\n\r");
	} else {
		printf("Write success\n\r");
	}
	return 0;
}
int read_all_records(flash_status_t *fs, int type) {
	sensordata_t *p = (sensordata_t*) fs->data_start;
	RTC_TimeTypeDef time;
	RTC_DateTypeDef date;
	int value;
	uint8_t *q;
	uint8_t log_length;
	uint8_t count;
	int i;

	if (p->watermark == 0xFF) {
		printf("OK\n\r");
		return (0);
	} else {
		uint8_t DATA_RECORD = 1;
		uint8_t LOG_RECORD = 2;
		while ((p->watermark != 0xFF)) {
			switch (p->status) {
			case 1:
				if ((type == DATA_RECORD)) {
					unpack_time(p->timestamp, &time, &date);
					printf("D,");
					printf("%d,", p->record_number);
					printf("%02d/%02d/20%02d,", date.Month, date.Date, date.Year);
					printf("%02d:%02d:%02d,", time.Hours, time.Minutes, time.Seconds);
					printf("%d.%03d,", (int) p->battery_voltage / 1000,
							(int) p->battery_voltage % 1000);
					printf("%d,", p->temperature);
					printf("%d,", p->sensor_period);
				}
				p--;
				break;
			case 2:
//        log_length = ((logdata_t *)p)->length;
				if ((type == LOG_RECORD)) {
//					p = ;
					unpack_time(p->timestamp, &time, &date);
					printf("L,");
					printf("%d,", p->record_number);
					printf("%02d/%02d/20%02d,", date.Month, date.Date, date.Year);
					printf("%02d:%02d:%02d,", time.Hours, time.Minutes, time.Seconds);
					printf("%s", ((logdata_t*) p)->msg);
//          if (log_length <= 7) {
//            printf("%s\n\r",((logdata_t *)p)->msg);
//            p--;
//          }
//          else {
//            // Print the first 7 characters in the log record
//            q = ((logdata_t *)p)->msg;
//            count = 7;
//            while (count > 0) {
//              printf("%c",*q);
//              q++;
//              count--;
//            }
//            // Print the remaining exension records.
//            count = log_length - 7; // Account for the first 7 characters
//            p--;                    // point at the next record, type logex_t
//            while (count/16) {
//              q = (uint8_t *) p;
//              for (i=0;i<16;i++) {
//                printf("%c",*q);
//                q++;
//                count--;
//              }
//              p--;
//            }
//            if (count%16) {
//              printf("%s",((logex_t *)p)->msg);
//              /* q = ((logdata_t *)p)->msg; */
//              /* for (i=0;i<(count%16);i++) { */
//              /*   printf("%c",*q); */
//              /*   q++; */
//              /* } */
//              p--;
//            }
//            printf("\n\r");
//            /* p = p - (log_length-7)/16; */
//            /* // Handle the additional partial frame */
//            /* if ((log_length-7)%16) { */
//            /*   p--; */
//            /* } */
//          }
				}
//        else {
//          if (log_length < 8) {            // Short Record
//            p--;
//          }
//          else {                           // Long Record
//            p--;                           // Head Log Record
//            p = p - (log_length-7)/16;     // Full Extension Records
//            if ((log_length-7)%16) {       // Partial Extension Record
//              p--;
//            }
//          }
//        }
				p--;
				break;
			default:
				p--;
				printf("NOK\n\r");
			}
		}
		printf("\n\r");
	}
	return (0);
}

uint32_t pack_time(RTC_TimeTypeDef *time, RTC_DateTypeDef *date) {
	uint32_t out = 0;
	uint32_t temp = 0;
	temp = (((uint32_t) date->Date) & 0b00011111) << 27; // Add the date
	out = temp;
	// printf("temp=0x%08x,date=%d\n\r",(unsigned int) temp, date->Date);
	temp = (((uint32_t) date->Month) & 0b00001111) << 22; // Add the month
	out |= temp;
	// printf("temp=0x%08x,date=%d\n\r",(unsigned int) temp, date->Month);
	temp = ((((uint32_t) date->Year) - 19) & 0b00000111) << 19; // Add the year
	out |= temp;
	// printf("temp=0x%08x,date=%d\n\r",(unsigned int) temp, date->Year);
	temp = (((uint32_t) time->Hours) & 0b00011111) << 13; // Add the hour
	out |= temp;
	// printf("temp=0x%08x,hours=%d\n\r",(unsigned int) temp, time->Hours);
	temp = (((uint32_t) time->Minutes) & 0b00111111) << 6; // Add the minute
	out |= temp;
	// printf("temp=0x%08x,minutes=%d\n\r",(unsigned int) temp, time->Minutes);
	temp = (((uint32_t) time->Seconds) & 0b00111111); // Add the second
	out |= temp;
	// printf("temp=0x%08x,seconds=%d\n\r",(unsigned int) temp, time->Seconds);
	// printf("Done Packing Time/Date\n\r");
	return out;
}

void unpack_time(uint32_t timeval, RTC_TimeTypeDef *time, RTC_DateTypeDef *date) {
	uint32_t temp = timeval;

	// Seconds
	temp &= 0x3F;
	time->Seconds = (uint8_t) temp;

	// Minutes
	temp = timeval;
	temp >>= 6;
	temp &= 0x3F;
	time->Minutes = (uint8_t) temp;

	// Hours
	temp = timeval;
	temp >>= 13;
	temp &= 0x1F;
	time->Hours = (uint8_t) temp;

	// Year
	temp = timeval;
	temp >>= 19;
	temp &= 0x07;
	date->Year = (uint8_t) temp + 19;

	// Year
	temp = timeval;
	temp >>= 22;
	temp &= 0x0F;
	date->Month = (uint8_t) temp;

	// Day
	temp = timeval;
	temp >>= 27;
	temp &= 0x1F;
	date->Date = (uint8_t) temp;
}
