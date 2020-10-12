#ifndef __BOSCH280_SENSORTEC_H__
#define __BOSCH280_SENSORTEC_H__

#include <c_types.h>

#define BOSCH280_I2C_ADDR1 0x76
#define BOSCH280_I2C_ADDR2 0x77

typedef sint32_t BOSCH280_S32_t;
typedef uint32_t BOSCH280_U32_t;
typedef sint64_t BOSCH280_S64_t;

struct dig_T_struct
{
    uint16_t T1;
    sint16_t T2;
    sint16_t T3;
};

struct dig_P_struct
{
    uint16_t P1;
    sint16_t P2;
    sint16_t P3;
    sint16_t P4;
    sint16_t P5;
    sint16_t P6;
    sint16_t P7;
    sint16_t P8;
    sint16_t P9;
};

struct dig_H_struct
{
    uint8_t  H1;
    sint16_t H2;
    uint8_t  H3;
    sint16_t H4;
    sint16_t H5;
    sint8_t  H6;
};

union dig_T_union
{
    uint8 data[6];
    struct dig_T_struct dig;
};

union dig_P_union
{
    uint8 data[18];
    struct dig_P_struct dig;
};

struct BOSCH280_device_struct
{
    uint8 bosch280_selected_device; //i2c_dev_addr
    uint8 chip_id;
    BOSCH280_S32_t t_fine;
    union  dig_T_union   callibration_temp_data;
    union  dig_P_union   callibration_pres_data;
    struct dig_H_struct  callibration_humm_data;
};


uint8 ICACHE_FLASH_ATTR BOSCH280_first_i2c_dev();
uint8 ICACHE_FLASH_ATTR BOSCH280_chip_id(uint8 i2c_dev_addr);
struct BOSCH280_device_struct * BOSCH280_init_dev(uint8 i2c_dev_addr);


bool ICACHE_FLASH_ATTR BOSCH280_soft_reset(struct BOSCH280_device_struct *);
bool ICACHE_FLASH_ATTR BOSCH280_set_mode(struct BOSCH280_device_struct *, uint8 mode);

bool ICACHE_FLASH_ATTR BOSCH280_measure(struct BOSCH280_device_struct *);
bool ICACHE_FLASH_ATTR BOSCH280_is_measuring(struct BOSCH280_device_struct *);

bool ICACHE_FLASH_ATTR BME280_read_calibration_data(struct BOSCH280_device_struct *);
bool ICACHE_FLASH_ATTR BMP280_read_calibration_data(struct BOSCH280_device_struct *);

real64_t ICACHE_FLASH_ATTR BOSCH280_read_temp_double(struct BOSCH280_device_struct *);
real64_t ICACHE_FLASH_ATTR BOSCH280_read_pres_double(struct BOSCH280_device_struct *);
real64_t ICACHE_FLASH_ATTR BOSCH280_read_humm_double(struct BOSCH280_device_struct *);

BOSCH280_S32_t ICACHE_FLASH_ATTR BOSCH280_read_temp_longint(struct BOSCH280_device_struct *);
BOSCH280_S32_t ICACHE_FLASH_ATTR BOSCH280_read_pres_longint(struct BOSCH280_device_struct *);
BOSCH280_S32_t ICACHE_FLASH_ATTR BOSCH280_read_humm_longint(struct BOSCH280_device_struct *);

bool ICACHE_FLASH_ATTR BOSCH280_set_weather_station_config(struct BOSCH280_device_struct *);





bool ICACHE_FLASH_ATTR  BOSCH280_write_byte_to_reg(uint8 i2c_dev_addr, uint8 reg, uint8 byte);
uint8 ICACHE_FLASH_ATTR BOSCH280_read_byte_from_reg(uint8 i2c_dev_addr, uint8 reg);
bool ICACHE_FLASH_ATTR  BOSCH280_read_bytes_from_reg(uint8 i2c_dev_addr, uint8 reg_start, uint8 len, uint8 ** buf /*should be not allocated*/);

#endif
