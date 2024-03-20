#define SENTINEL_MARK_BOTTOM      0xDEADBEEFA5A5A5A5
#define SENTINEL_MARK_TOP         0xFEEDC0DE5A5A5A5A
#define FLASH_END 0x0803FFF0



typedef struct sensordata {
  uint8_t watermark;               // 0x01=populated, 0xFF=unpopulated
  uint8_t status;                  // record type, 01=sensor data, 02=log data;
  uint16_t record_number;          // Which number is this particular record
  uint32_t timestamp;              // Time, bit packed into 32 bits
  uint16_t battery_voltage;        // 16 bit battery voltage
  int16_t temperature;             // STM32 Temperature sensor reading
  uint32_t sensor_period;          // Reading from the light sensor
} sensordata_t;

typedef struct log_data {
  uint8_t watermark;               // 0x01=populated, 0xFF=unpopulated
  uint8_t status;                  // record type, 01=sensor data, 02=log data;
  uint16_t record_number;          // Which number is this particular record
  uint32_t timestamp;              // Time, bit packed into 32 bits
  uint8_t msg[8];                  // String message to make the record seem less cryptic.
} logdata_t;

typedef struct raw {               // Raw structure used to write sensordata and log_data to the flash
  uint64_t data0;
  uint64_t data1;
} raw_t;

typedef struct flash_status {
  uint64_t * data_start;
  uint32_t total_records;
  uint32_t max_possible_records;
  uint64_t * next_address;
  uint32_t next_record_number;
} flash_status_t;

int flash_write_init(flash_status_t *);
int write_record(flash_status_t *, void *);
uint64_t *find_sentinel_bottom(void);
uint64_t *find_sentinel_top(void);
int write_sentinel(uint64_t *, raw_t *);
void test();
int flash_erase(void);
static uint32_t GetPage(uint32_t Addr);
static uint32_t GetBank(uint32_t Addr);

