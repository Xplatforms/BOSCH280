#include <bme280.h>
#include <driver/i2c_master.h>

#include <osapi.h>
#include <mem.h>

#define GLOBAL_DEBUG_ON


#if defined(GLOBAL_DEBUG_ON)
#define MQTT_DEBUG_ON
#endif

#if defined(MQTT_DEBUG_ON)
#ifndef INFO
#define INFO( format, ... ) os_printf( format, ## __VA_ARGS__ )
#endif
#else
#ifndef INFO
#define INFO( format, ... )
#endif
#endif

#define BME280_CHIP_ID_REG      0xD0
#define BME280_CONF_REG         0xF4
#define BME280_SOFT_RST_REG     0xE0
#define BME280_SOFT_RST_BYTE    0xB6

#define BME280_TEMP_MSB  0xFA
#define BME280_TEMP_LSB  0xFB
#define BME280_TEMP_XLSB 0xFC


///FORWARD DECL. DEF is in user_main
void ICACHE_FLASH_ATTR BITINFO(char * text, uint8 inpins);
///


struct dig_T_struct
{
    uint16_t T1;
    sint16_t T2;
    sint16_t T3;
};

union dig_T_union
{
    uint8 data[6];
    struct dig_T_struct dig;
};

union dig_T_union callibration_temp_data;

// Returns temperature in DegC, resolution is 0.01 DegC. Output value of “5123” equals 51.23 DegC.
// t_fine carries fine temperature as global value
BME280_S32_t t_fine;
BME280_S32_t BME280_compensate_T_int32(uint16_t dig_T1, int16_t dig_T2, int16_t dig_T3, BME280_S32_t adc_T)
{
    BME280_S32_t var1, var2, T;
    var1  = ((((adc_T>>3) -((BME280_S32_t)dig_T1<<1))) * ((BME280_S32_t)dig_T2)) >> 11;
    var2  =(((((adc_T>>4) -((BME280_S32_t)dig_T1)) * ((adc_T>>4) -((BME280_S32_t)dig_T1))) >> 12) * ((BME280_S32_t)dig_T3)) >> 14;
    t_fine = var1 + var2;
    T  = (t_fine * 5 + 128) >> 8;
    return T;
}

real64_t BME280_compensate_T_double(uint16_t dig_T1, int16_t dig_T2, int16_t dig_T3, BME280_S32_t adc_T)
{
    real64_t var1, var2, T;

    var1 = (((real64_t)adc_T)/16384.0 - ((real64_t)dig_T1)/1024.0) * ((real64_t)dig_T2);
    var2 = ((((real64_t)adc_T)/131072.0 - ((real64_t)dig_T1)/8192.0) *
            (((real64_t)adc_T)/131072.0 - ((real64_t) dig_T1)/8192.0)) * ((real64_t)dig_T3);
    t_fine = (BME280_S32_t)(var1 + var2);
    T = (var1 + var2) / 5120.0;
    return T;
}

/*
// Returns pressure in Pa as unsigned 32 bit integer in Q24.8 format (24 integer bits and 8 fractional bits).
// Output value of “24674867” represents 24674867/256 = 96386.2 Pa = 963.862 hPa
BME280_U32_t BME280_compensate_P_int64(BME280_S32_t adc_P)
{
    BME280_S64_t var1, var2, p;
    var1 = ((BME280_S64_t)t_fine) -128000;
    var2 = var1 * var1 * (BME280_S64_t)dig_P6;
    var2 = var2 + ((var1*(BME280_S64_t)dig_P5)<<17);
    var2 = var2 + (((BME280_S64_t)dig_P4)<<35);
    var1 = ((var1 * var1 * (BME280_S64_t)dig_P3)>>8) + ((var1 * (BME280_S64_t)dig_P2)<<12);
    var1 = (((((BME280_S64_t)1)<<47)+var1))*((BME280_S64_t)dig_P1)>>33;
    if(var1 == 0)
    {
        return 0;
        // avoid exception caused by division by zero
    }
    p = 1048576-adc_P;
    p = (((p<<31)-var2)*3125)/var1;
    var1 = (((BME280_S64_t)dig_P9) * (p>>13) * (p>>13)) >> 25;
    var2 =(((BME280_S64_t)dig_P8) * p) >> 19;
    p = ((p + var1 + var2) >> 8) + (((BME280_S64_t)dig_P7)<<4);
    return (BME280_U32_t)p;
}
*/
// Returns humidity in %RH as unsigned 32 bit integer in Q22.10 format (22 integer and 10 fractional bits).// Output value of “47445” represents 47445/1024 = 46.333 %RHBME280_U32_tbme280_compensate_H_int32(BME280_S32_tadc_H){BME280_S32_tv_x1_u32r;v_x1_u32r = (t_fine –((BME280_S32_t)76800));


//print_bitmask_uint((uint8)(BME280_I2C_ADDR << 1)); // print addr <- WRITE
//print_bitmask_uint((uint8)(0x76 << 1) | 1); // print addr <- READ

bool ICACHE_FLASH_ATTR bme280_select_i2c_dev(uint8 i2c_dev_addr)
{
    i2c_master_start();
    i2c_master_writeByte((uint8)(i2c_dev_addr << 1));
    if (!i2c_master_checkAck()) {
        INFO("%d i2c bme280_select_i2c_dev checkAck error \r\n", __LINE__);
        i2c_master_stop();
        return false;
    }
    return true;
}

bool ICACHE_FLASH_ATTR bme280_readNextBytes(uint8 i2c_dev_addr, uint8 *data, uint16 len)
{
    uint8 loop;
    // signal i2c start
    i2c_master_start();
    // write i2c address & direction
    i2c_master_writeByte((uint8)((i2c_dev_addr << 1) | 1));
    if (!i2c_master_checkAck()) {
        INFO("i2c at24c_readNextBytes checkAck error \r\n");
        i2c_master_stop();
        return false;
    }

    // read bytes
    for (loop = 0; loop < len; loop++) {
        data[loop] = i2c_master_readByte();
        // send ack (except after last byte, then we send nack)
        if (loop < (len - 1)) i2c_master_send_ack(); else i2c_master_send_nack();
    }

    // signal i2c stop
    i2c_master_stop();

    return true;
}

bool ICACHE_FLASH_ATTR bme280_write_byte_to_reg(uint8 i2c_dev_addr, uint8 reg, uint8 byte)
{
    if(bme280_select_i2c_dev(i2c_dev_addr) == false)
    {
        INFO("[%d] BME280 i2c device can not be selected/accessed\r\n", __LINE__);
        return false;
    }

    i2c_master_writeByte((uint8)(reg));
    if (!i2c_master_checkAck()) {
        INFO("%d i2c bme280_read_byte_from_reg checkAck error \r\n", __LINE__);
        i2c_master_stop();
        return false;
    }

    i2c_master_writeByte((uint8)(byte));
    if (!i2c_master_checkAck()) {
        INFO("%d i2c bme280_read_byte_from_reg checkAck error \r\n", __LINE__);
        i2c_master_stop();
        return false;
    }

    i2c_master_stop();
    return true;
}

uint8 ICACHE_FLASH_ATTR bme280_read_byte_from_reg(uint8 i2c_dev_addr, uint8 reg)
{
    if(bme280_select_i2c_dev(i2c_dev_addr) == false)
    {
        INFO("[%d] BME280 i2c device can not be selected/accessed\r\n", __LINE__);
        return 0;
    }

    i2c_master_writeByte((uint8)(reg));
    if (!i2c_master_checkAck()) {
        INFO("%d i2c bme280_read_byte_from_reg checkAck error \r\n", __LINE__);
        i2c_master_stop();
        return 0;
    }

    i2c_master_start();
    i2c_master_writeByte((uint8)(i2c_dev_addr << 1) | 1);
    if (!i2c_master_checkAck()) {
        INFO("%d i2c bme280_read_byte_from_reg checkAck error \r\n", __LINE__);
        i2c_master_stop();
        return 0;
    }

    uint8 ret = i2c_master_readByte();
    i2c_master_send_nack();
    i2c_master_stop();
    return ret;
}

bool ICACHE_FLASH_ATTR bme280_read_bytes_from_reg(uint8 i2c_dev_addr, uint8 reg_start, uint8 len, uint8 ** buf /*should be not allocated*/)
{
  if(bme280_select_i2c_dev(i2c_dev_addr) == false)
  {
      INFO("[%d] BME280 i2c device can not be selected/accessed\r\n", __LINE__);
      return false;
  }

  i2c_master_writeByte(reg_start);
  if (!i2c_master_checkAck())
  {
      INFO("%d i2c bme280_temp checkAck error \r\n", __LINE__);
      i2c_master_stop();
      return false;
  }

  *buf = (uint8*)os_zalloc(len);
  bool ret = bme280_readNextBytes(i2c_dev_addr, *buf, len );

  //INFO("BME READ BYTES -> %s\r\n", ret?"true":"false");
  if(!ret)
  {
      os_free(*buf);
      return false;
  }

  return true;
}


uint8 ICACHE_FLASH_ATTR bme280_chip_id(uint8 i2c_dev_addr)
{
    if(bme280_select_i2c_dev(i2c_dev_addr) == false)
    {
        INFO("[%d] BME280 i2c device can not be selected/accessed\r\n", __LINE__);
        return 0;
    }

    i2c_master_writeByte((uint8)(BME280_CHIP_ID_REG));
    if (!i2c_master_checkAck()) {
        INFO("%d i2c bme280_chip_id checkAck error \r\n", __LINE__);
        i2c_master_stop();
        return 0;
    }

    i2c_master_start();
    i2c_master_writeByte((uint8)(i2c_dev_addr << 1) | 1);
    if (!i2c_master_checkAck()) {
        INFO("%d i2c bme280_chip_id checkAck error \r\n", __LINE__);
        i2c_master_stop();
        return 0;
    }

    uint8 ret = i2c_master_readByte();
    i2c_master_send_nack();
    i2c_master_stop();
    return ret;
}

bool ICACHE_FLASH_ATTR bme280_soft_reset(uint8 i2c_dev_addr)
{
    if(bme280_select_i2c_dev(i2c_dev_addr) == false)
    {
        INFO("[%d] BME280 i2c device can not be selected/accessed\r\n", __LINE__);
        return false;
    }

    i2c_master_writeByte((uint8)(BME280_SOFT_RST_REG));
    if (!i2c_master_checkAck()) {
        INFO("[%d] i2c bme280_chip_id checkAck error \r\n", __LINE__);
        i2c_master_stop();
        return false;
    }

    i2c_master_writeByte((uint8)(BME280_SOFT_RST_BYTE));
    if (!i2c_master_checkAck()) {
        INFO("[%d] i2c bme280_chip_id checkAck error \r\n", __LINE__);
        i2c_master_stop();
        return false;
    }

    i2c_master_stop();
    return true;
}

bool ICACHE_FLASH_ATTR bme280_read_calibration_data(uint8 i2c_dev_addr)
{
  uint8 * calib00 = NULL;
  bool read = bme280_read_bytes_from_reg(i2c_dev_addr, 0x88, 25, &calib00);
  if(!read)
  {
      INFO("CAN't read callibration data from 0x88..0xA1\r\n");
      return false;
  }

  uint8 * calib26 = NULL;
  read = bme280_read_bytes_from_reg(i2c_dev_addr, 0xE1, 15, &calib26);
  if(!read)
  {
      INFO("CAN't read callibration data from 0x88..0xA1\r\n");
      return false;
  }

  os_memcpy((void *)callibration_temp_data.data, calib00, 6);

  os_free(calib00);
  os_free(calib26);

  return true;
}

BME280_S32_t ICACHE_FLASH_ATTR bme280_read_temp_longint(uint8 i2c_dev_addr)
{
    uint8 * temp_data = NULL;
    bool read = bme280_read_bytes_from_reg(i2c_dev_addr, 0xFA, 3, &temp_data);
    if(!read)
    {
        INFO("CAN't read bme280_read_temp_longint\r\n");
        return 0;
    }

    uint32_t adc_t = 0;
    adc_t  = (uint32_t)temp_data[2] >> 4;
    adc_t |= (uint32_t)temp_data[1] << 4;
    adc_t |= (uint32_t)temp_data[0] << 12;

    BME280_S32_t temp = BME280_compensate_T_int32(callibration_temp_data.dig.T1, callibration_temp_data.dig.T2, callibration_temp_data.dig.T3, adc_t);
    os_free(temp_data);
    return temp;
}

real64_t ICACHE_FLASH_ATTR bme280_read_temp_double(uint8 i2c_dev_addr)
{
    uint8 * temp_data = NULL;
    bool read = bme280_read_bytes_from_reg(i2c_dev_addr, 0xFA, 3, &temp_data);
    if(!read)
    {
        INFO("CAN't read bme280_read_temp_double\r\n");
        return 0.0;
    }

    uint32_t adc_t = 0;
    adc_t  = (uint32_t)temp_data[2] >> 4;
    adc_t |= (uint32_t)temp_data[1] << 4;
    adc_t |= (uint32_t)temp_data[0] << 12;

    real64_t temp_double = BME280_compensate_T_double(callibration_temp_data.dig.T1, callibration_temp_data.dig.T2, callibration_temp_data.dig.T3, adc_t);
    os_free(temp_data);
    return temp_double;
}

bool ICACHE_FLASH_ATTR bme280_set_mode(uint8 i2c_dev_addr, uint8 mode)
{
    uint8 cur = bme280_read_byte_from_reg(i2c_dev_addr, 0xF4);
    BITINFO("CURMEAS", cur);

    cur = cur & 0b11111100; // mask out the bits we care about
    cur = cur | mode; // Set the magic bits

    BITINFO("NEWMEAS", cur);

    bme280_write_byte_to_reg(i2c_dev_addr, 0xF4, cur);
}


bool ICACHE_FLASH_ATTR bme280_set_weather_station_config(uint8 i2c_dev_addr)
{
    // weather monitoring. sens mode forced, oversampling 1,1,1, filter off
    // 0xF4 -> 8...0 osrs_t 111. osrs_p 111, mode 11
    // -> 001 001 01
    uint8 ctrl_meas = 0x25;//b 001 001 01;
    // 0xF2 ->ctrl_hum 00000 001
    uint8 ctrl_hum = 0x01;
    // 0xF5 -> config
    // 001 011 00 //500ms
    uint8 ctrl_config = 0x2C;//0x80;

    if(!bme280_write_byte_to_reg(i2c_dev_addr, 0xF2, ctrl_hum))
    {
        INFO("Can't write ctrl_hum\r\n");
        return false;
    }
    else
    {
        if(!bme280_write_byte_to_reg(i2c_dev_addr, 0xF5, ctrl_config))
        {
            INFO("Can't write ctrl_config\r\n");
            return false;
        }
        else
        {
            if(!bme280_write_byte_to_reg(i2c_dev_addr, 0xF4, ctrl_meas))
            {
                INFO("Can't write ctrl_meas\r\n");
                return false;
            }
        }
    }

    return true;
}

/*
bool ICACHE_FLASH_ATTR bme280_enable_weather_monitor_config(uint8 i2c_dev_addr)
{
    // weather monitoring. sens mode forced, oversampling 1,1,1, filter off
    // 0xF4 -> 8...0 osrs_t 111. osrs_p 111, mode 11
    // -> 001 001 10
    uint8 ctrl_meas = 0x26;//b00100110;
    // 0xF2 ->ctrl_hum 00000 001
    uint8 ctrl_hum = 0x01;
    // 0xF5 -> config
    // 101 010 00
    uint8 ctrl_config = 0x80;

    if(!bme280_write_byte_to_reg(i2c_dev_addr, 0xF2, ctrl_hum))
    {
        INFO("Can't write ctrl_hum\r\n");
        return false;
    }
    else
    {
        if(!bme280_write_byte_to_reg(i2c_dev_addr, 0xF4, ctrl_meas))
        {
            INFO("Can't write ctrl_meas\r\n");
            return false;
        }
        else
        {
            if(!bme280_write_byte_to_reg(i2c_dev_addr, 0xF5, ctrl_config))
            {
                INFO("Can't write ctrl_config\r\n");
                return false;
            }
        }
    }

    return true;
}
*/
