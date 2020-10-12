#include <bme280.h>
#include <driver/i2c_master.h>

#include <osapi.h>

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

#define BME280_CHIP_ID_REG 0xD0
#define BME280_SOFT_RST_REG 0xE0
#define BME280_SOFT_RST_BYTE 0xB6

#define BME280_TEMP_MSB  0xFA
#define BME280_TEMP_LSB  0xFB
#define BME280_TEMP_XLSB 0xFC

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

    //print_bitmask_uint((uint8)(0x76 << 1) | 1);
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

void ICACHE_FLASH_ATTR print_bitmask_u32(uint32_t inpins)
{
    INFO("UINT32_t BITMASK: ");
    while (inpins) {
        if (inpins & 1)
            INFO("1");
        else
            INFO("0");
        inpins >>= 1;
    }
    INFO("\n");
}


bool ICACHE_FLASH_ATTR bme280_readNextBytes(uint8 i2c_dev_addr, uint8 *data, uint16 len) {

    int loop;

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

void ICACHE_FLASH_ATTR readCompensationParams(uint8 i2c_dev_addr)
{
  if(bme280_select_i2c_dev(i2c_dev_addr) == false)
  {
      INFO("[%d] BME280 i2c device can not be selected/accessed\r\n", __LINE__);
      return;
  }

  i2c_master_writeByte((uint8)(0x88));
  if (!i2c_master_checkAck()) {
      INFO("%d i2c bme280_temp checkAck error \r\n", __LINE__);
      i2c_master_stop();
      return;
  }
  uint8 data[25] = {0};
  bool ret = bme280_readNextBytes(i2c_dev_addr, data, 24 );
  INFO("BME READ CALIBR -> %s\r\n", ret?"true":"false");
  if(!ret)return;

  int i = 0;
  for(i= 0; i < 24; i++)
  {
      INFO("data[%d]: ",i);
      uint8 tmp =0;
      tmp |= data[i];
      while (tmp) {
          if (tmp & 1)
              INFO("1");
          else
              INFO("0");
          tmp >>= 1;
      }
      INFO("\r\n");
  }
}

uint8 ICACHE_FLASH_ATTR bme280_read_byte_from_reg(uint8 i2c_dev_addr, uint8 reg)
{
    if(bme280_select_i2c_dev(i2c_dev_addr) == false)
    {
        INFO("[%d] BME280 i2c device can not be selected/accessed\r\n", __LINE__);
        return 0;
    }

    i2c_master_writeByte((uint8)reg);
    if (!i2c_master_checkAck()) {
        INFO("%d i2c bme280_temp checkAck error \r\n", __LINE__);
        i2c_master_stop();
        return 0;
    }


    i2c_master_start();

    i2c_master_writeByte((uint8)(i2c_dev_addr << 1) | 1);
    if (!i2c_master_checkAck()) {
        INFO("%d i2c bme280_temp checkAck error \r\n", __LINE__);
        i2c_master_stop();
        return 0;
    }

    uint8 ret = i2c_master_readByte();
    if (!i2c_master_checkAck()) {
        INFO("%d i2c bme280_temp checkAck error \r\n", __LINE__);
        i2c_master_stop();
        return 0;
    }

    i2c_master_send_nack();
    i2c_master_stop();

    return ret;
}

void ICACHE_FLASH_ATTR bme280_enable_sens(uint8 i2c_dev_addr)
{

    uint8_t ctrlMeas;
    ctrlMeas = bme280_read_byte_from_reg(i2c_dev_addr, (uint8)0xF4);

    INFO("ctrlMeas -> ");
    uint8 tmp = ctrlMeas;
    while (tmp) {
        if (tmp & 1)
            INFO("1");
        else
            INFO("0");
        tmp >>= 1;
    }
    INFO("\r\n");

    // We want to change osrs_t which is bits 7,6,5
    ctrlMeas = ctrlMeas & 0b00011111; // mask out the bits we care about
    ctrlMeas = ctrlMeas | (1 << 5); // Set the magic bits // TEMP

    INFO("ctrlMeas temp -> ");
    tmp = ctrlMeas;
    while (tmp) {
        if (tmp & 1)
            INFO("1");
        else
            INFO("0");
        tmp >>= 1;
    }
    INFO("\r\n");

    // We want to change osrs_p which is bits 4,3,2
    ctrlMeas = ctrlMeas & 0b11100011; // mask out the bits we care about
    ctrlMeas = ctrlMeas | (1 << 2); // Set the magic bits //PRES

    INFO("ctrlMeas pres -> ");
    tmp = ctrlMeas;
    while (tmp) {
        if (tmp & 1)
            INFO("1");
        else
            INFO("0");
        tmp >>= 1;
    }
    INFO("\r\n");

      //writeRegister(regCtrlMeas, ctrlMeas);

}

void ICACHE_FLASH_ATTR bme280_temp(uint8 i2c_dev_addr)
{
    if(bme280_select_i2c_dev(i2c_dev_addr) == false)
    {
        INFO("[%d] BME280 i2c device can not be selected/accessed\r\n", __LINE__);
        return;
    }

    i2c_master_writeByte((uint8)(BME280_TEMP_MSB));
    if (!i2c_master_checkAck()) {
        INFO("%d i2c bme280_temp checkAck error \r\n", __LINE__);
        i2c_master_stop();
        return;
    }

    i2c_master_start();

    i2c_master_writeByte((uint8)(i2c_dev_addr << 1) | 1);
    if (!i2c_master_checkAck()) {
        INFO("%d i2c bme280_temp checkAck error \r\n", __LINE__);
        i2c_master_stop();
        return;
    }

    uint8 msb = i2c_master_readByte();
    if (!i2c_master_checkAck()) {
        INFO("%d i2c bme280_temp checkAck error \r\n", __LINE__);
        i2c_master_stop();
        return;
    }

    uint8 lsb = i2c_master_readByte();
    if (!i2c_master_checkAck()) {
        INFO("%d i2c bme280_temp checkAck error \r\n", __LINE__);
        i2c_master_stop();
        return;
    }

    uint8 xlsb = i2c_master_readByte();
    if (!i2c_master_checkAck()) {
        INFO("%d i2c bme280_temp checkAck error \r\n", __LINE__);
        i2c_master_stop();
        return;
    }

    uint32_t temp32 = (uint32_t)xlsb | (uint32_t)msb<<4 | (uint32_t)lsb<<12;
    INFO("TEMP 32bit: %u\r\n", temp32);
    print_bitmask_u32(temp32);

    INFO("MSB -> ");print_bitmask_u32(msb);
    INFO("LSB -> ");print_bitmask_u32(lsb);
    INFO("XLSB -> ");print_bitmask_u32(xlsb);

    i2c_master_send_nack();
    i2c_master_stop();


}
