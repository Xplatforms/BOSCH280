#ifndef __BME280_H__
#define __BME280_H__

#include <c_types.h>

#define BME280_I2C_ADDR 0x76

typedef sint32_t BME280_S32_t;
typedef uint32_t BME280_U32_t;
typedef sint64_t BME280_S64_t;

uint8 ICACHE_FLASH_ATTR bme280_chip_id(uint8 i2c_dev_addr);
bool  ICACHE_FLASH_ATTR bme280_soft_reset(uint8 i2c_dev_addr);


bool ICACHE_FLASH_ATTR bme280_set_mode(uint8 i2c_dev_addr, uint8 mode);
bool ICACHE_FLASH_ATTR bme280_set_weather_station_config(uint8 i2c_dev_addr);
//bool ICACHE_FLASH_ATTR bme280_enable_weather_monitor_config(uint8 i2c_dev_addr);

real64_t ICACHE_FLASH_ATTR bme280_read_temp_double(uint8 i2c_dev_addr);
BME280_S32_t ICACHE_FLASH_ATTR bme280_read_temp_longint(uint8 i2c_dev_addr);

real64_t ICACHE_FLASH_ATTR bme280_read_pres_double(uint8 i2c_dev_addr);
BME280_S32_t ICACHE_FLASH_ATTR bme280_read_pres_longint(uint8 i2c_dev_addr);

real64_t ICACHE_FLASH_ATTR bme280_read_humm_double(uint8 i2c_dev_addr);
BME280_S32_t ICACHE_FLASH_ATTR bme280_read_humm_longint(uint8 i2c_dev_addr);


bool ICACHE_FLASH_ATTR bme280_write_byte_to_reg(uint8 i2c_dev_addr, uint8 reg, uint8 byte);
uint8 ICACHE_FLASH_ATTR bme280_read_byte_from_reg(uint8 i2c_dev_addr, uint8 reg);
bool ICACHE_FLASH_ATTR bme280_read_bytes_from_reg(uint8 i2c_dev_addr, uint8 reg_start, uint8 len, uint8 ** buf /*should be not allocated*/);

bool ICACHE_FLASH_ATTR bme280_read_calibration_data(uint8 i2c_dev_addr);


#endif
