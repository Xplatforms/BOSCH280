#ifndef __BME280_H__
#define __BME280_H__

#include <c_types.h>

#define BME280_I2C_ADDR 0x76

typedef sint32_t BME280_S32_t;
typedef uint32_t BME280_U32_t;
typedef sint64_t BME280_S64_t;

BME280_S32_t BME280_compensate_T_int32(uint16_t dig_T1, int16_t dig_T2, int16_t dig_T3, BME280_S32_t adc_T);
//BME280_U32_t BME280_compensate_P_int64(BME280_S32_t adc_P);
//BME280_U32_t BME280_compensate_H_int32(BME280_S32_t adc_H);

uint8 ICACHE_FLASH_ATTR bme280_chip_id(uint8 i2c_dev_addr);
bool  ICACHE_FLASH_ATTR bme280_soft_reset(uint8 i2c_dev_addr);
void ICACHE_FLASH_ATTR bme280_temp(uint8 i2c_dev_addr);


void ICACHE_FLASH_ATTR readCompensationParams(uint8 i2c_dev_addr);
void ICACHE_FLASH_ATTR bme280_temp(uint8 i2c_dev_addr);

void ICACHE_FLASH_ATTR bme280_enable_sens(uint8 i2c_dev_addr);

#endif
