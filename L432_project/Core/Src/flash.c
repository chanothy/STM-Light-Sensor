#include "main.h"
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <flash.h>

extern int _edata;
extern int _sdata;
extern int __fini_array_end;
extern RTC_HandleTypeDef hrtc;
extern flash_status_t fs;

extern ADC_HandleTypeDef hadc1;
extern TIM_HandleTypeDef htim2;

int write_record(flash_status_t * fs, void * record) {
  raw_t * write_data;
  HAL_StatusTypeDef status = 0;

  if ((!fs) || (!record)) {
    return (-1);
  }

  if (fs->total_records >= fs->max_possible_records) {
    printf("full memory\n\r");
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

uint64_t * find_sentinel_bottom(void) {
  uint64_t *p = (uint64_t *)&__fini_array_end + ((uint64_t *)&_edata - (uint64_t *)&_sdata);
  p+=2;
  p = (uint64_t *) ((uintptr_t) p & ~(uintptr_t)0xF);
  while (p<=((uint64_t*)FLASH_END)) {
    if (*p==SENTINEL_MARK_BOTTOM) {
      return(p);
    }
    p+=2;
  }
  return (0);
}

uint64_t * find_sentinel_top(void) {
  uint64_t *p = (uint64_t *) FLASH_END;
  if (*p==SENTINEL_MARK_TOP) {
    return(p);
  }
  else {
    return (0);
  }
}

int write_sentinel(uint64_t * location, raw_t * sentinel) {
  if ((!location) || (!sentinel)) {
    return(-1);
  }
  HAL_FLASH_Unlock();
  if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD,(int) location++, sentinel->data0)) {
    HAL_FLASH_Lock();
    return (-1);
  }
  if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD,(int) location, sentinel->data1)) {
    HAL_FLASH_Lock();
    return (-1);
  }
  HAL_FLASH_Lock();
  return (0);
}

int flash_write_init(flash_status_t * fs) {
  sensordata_t *sd = 0;
  uint16_t record_counter = 0;
  raw_t sentinel_top = {SENTINEL_MARK_TOP,0};
  raw_t sentinel_bottom = {SENTINEL_MARK_BOTTOM,0};

  uint64_t *program_end = (uint64_t *) ((uint32_t )&__fini_array_end
                                        + (uint32_t)&_edata
                                        - (uint32_t)&_sdata);

  uint64_t *top = (uint64_t *) FLASH_END;

  uint64_t *bottom = (uint64_t *) (((uint32_t)program_end & ~0x7FF) + 0x800);

  uint64_t *pt = find_sentinel_top();

  uint64_t *pb = find_sentinel_bottom();

  if ((!pt) && (!pb)) {
    if (write_sentinel(top,&sentinel_top)) {
      return(-1);
    }
    if (write_sentinel(bottom,&sentinel_bottom)) {
      return(-1);
    }
  }
  else if ((pt) && (!pb)) {
    if (write_sentinel(bottom,&sentinel_bottom)) {
      return(-1);
    }
  }
  else if ((!pt) && (pb)) {
    return (-1);
  }
  else if ((pt) && (pb)) {
    if (pb!=bottom) {

    }
  }
  else {
    return (-1);
  }

  fs->data_start = top - 2;
  fs->max_possible_records = (((uint32_t)top-(uint32_t)bottom)>>4)-1;
  sd = (sensordata_t *) top;
  sd--;
  while ((sd->watermark!=0xFF)) {
    record_counter++;
    sd--;
  }
  fs->next_record_number = record_counter;
  fs->total_records = record_counter;
  fs->next_address = (uint64_t *) sd;

  return (0);
}
