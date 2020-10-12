#include <bosch280_sensortec.h>
#include <i2c_master.h>

#include <osapi.h>
#include <mem.h>

#define BOSCH280_CHIP_ID_REG      0xD0
#define BOSCH280_CONF_REG         0xF4
#define BOSCH280_SOFT_RST_REG     0xE0
#define BOSCH280_SOFT_RST_BYTE    0xB6

#define BOSCH280_TEMP_MSB  0xFA
#define BOSCH280_TEMP_LSB  0xFB
#define BOSCH280_TEMP_XLSB 0xFC


///FORWARD DECL. DEF is in utils/utils.c
//void ICACHE_FLASH_ATTR BITINFO(char * text, uint8 inpins);
//void ICACHE_FLASH_ATTR BITINFOu16(char * text, uint16 inpins);
//void ICACHE_FLASH_ATTR BITINFOu32(char * text, uint32 inpins);
///

struct BOSCH280_device_struct * current_selected_device = NULL;

// Returns temperature in DegC, resolution is 0.01 DegC. Output value of “5123” equals 51.23 DegC.
// t_fine carries fine temperature as global value
BOSCH280_S32_t ICACHE_FLASH_ATTR BOSCH280_compensate_T_int32(struct BOSCH280_device_struct * dev, BOSCH280_S32_t adc_T)
{
    BOSCH280_S32_t var1, var2, T;
    var1  = ((((adc_T>>3) -((BOSCH280_S32_t)dev->callibration_temp_data.dig.T1<<1))) * ((BOSCH280_S32_t)dev->callibration_temp_data.dig.T2)) >> 11;
    var2  =(((((adc_T>>4) -((BOSCH280_S32_t)dev->callibration_temp_data.dig.T1)) * ((adc_T>>4) -((BOSCH280_S32_t)dev->callibration_temp_data.dig.T1))) >> 12)
            * ((BOSCH280_S32_t)dev->callibration_temp_data.dig.T3)) >> 14;
    dev->t_fine = var1 + var2;
    T  = (dev->t_fine * 5 + 128) >> 8;
    return T;
}

real64_t ICACHE_FLASH_ATTR BOSCH280_compensate_T_double(struct BOSCH280_device_struct * dev, BOSCH280_S32_t adc_T)
{
    real64_t var1, var2, T;

    var1 = (((real64_t)adc_T)/16384.0 - ((real64_t)dev->callibration_temp_data.dig.T1)/1024.0) * ((real64_t)dev->callibration_temp_data.dig.T2);
    var2 = ((((real64_t)adc_T)/131072.0 - ((real64_t)dev->callibration_temp_data.dig.T1)/8192.0) *
            (((real64_t)adc_T)/131072.0 - ((real64_t)dev->callibration_temp_data.dig.T1)/8192.0)) * ((real64_t)dev->callibration_temp_data.dig.T3);
    dev->t_fine = (BOSCH280_S32_t)(var1 + var2);
    T = (var1 + var2) / 5120.0;
    return T;
}

// Returns pressure in Pa as unsigned 32 bit integer in Q24.8 format (24 integer bits and 8 fractional bits).
// Output value of “24674867” represents 24674867/256 = 96386.2 Pa = 963.862 hPa
BOSCH280_U32_t ICACHE_FLASH_ATTR BOSCH280_compensate_P_int64(struct BOSCH280_device_struct * dev, BOSCH280_S32_t adc_P)
{
    BOSCH280_S64_t var1, var2, p;
    var1 = ((BOSCH280_S64_t)dev->t_fine) -128000;
    var2 = var1 * var1 * (BOSCH280_S64_t)dev->callibration_pres_data.dig.P6;
    var2 = var2 + ((var1*(BOSCH280_S64_t)dev->callibration_pres_data.dig.P5)<<17);
    var2 = var2 + (((BOSCH280_S64_t)dev->callibration_pres_data.dig.P4)<<35);
    var1 = ((var1 * var1 * (BOSCH280_S64_t)dev->callibration_pres_data.dig.P3)>>8) + ((var1 * (BOSCH280_S64_t)dev->callibration_pres_data.dig.P2)<<12);
    var1 = (((((BOSCH280_S64_t)1)<<47)+var1))*((BOSCH280_S64_t)dev->callibration_pres_data.dig.P1)>>33;
    if(var1 == 0)
    {
        return 0;
        // avoid exception caused by division by zero
    }
    p = 1048576-adc_P;
    p = (((p<<31)-var2)*3125)/var1;
    var1 = (((BOSCH280_S64_t)dev->callibration_pres_data.dig.P9) * (p>>13) * (p>>13)) >> 25;
    var2 =(((BOSCH280_S64_t)dev->callibration_pres_data.dig.P8) * p) >> 19;
    p = ((p + var1 + var2) >> 8) + (((BOSCH280_S64_t)dev->callibration_pres_data.dig.P7)<<4);
    return (BOSCH280_U32_t)p;
}

real64_t ICACHE_FLASH_ATTR BOSCH280_compensate_P_double(struct BOSCH280_device_struct * dev, BOSCH280_S32_t adc_P)
{
    real64_t var1, var2, p;
    var1 = ((real64_t)dev->t_fine/2.0) - 64000.0;
    var2 = var1 * var1 * ((real64_t)dev->callibration_pres_data.dig.P6) / 32768.0;
    var2 = var2 + var1 * ((real64_t)dev->callibration_pres_data.dig.P5) * 2.0;
    var2 = (var2/4.0)+(((real64_t)dev->callibration_pres_data.dig.P4) * 65536.0);
    var1 = (((real64_t)dev->callibration_pres_data.dig.P3) * var1 * var1 / 524288.0 + ((real64)dev->callibration_pres_data.dig.P2) * var1) / 524288.0;
    var1 = (1.0 + var1 / 32768.0)*((real64_t)dev->callibration_pres_data.dig.P1);
    if (var1 == 0.0) {
        return 0.0; // avoid exception caused by division by zero
    }
    p = 1048576.0 - (real64_t)adc_P;
    p = (p - (var2 / 4096.0)) * 6250.0 / var1;
    var1 = ((real64_t)dev->callibration_pres_data.dig.P9) * p * p / 2147483648.0;
    var2 = p * ((real64_t)dev->callibration_pres_data.dig.P8) / 32768.0;
    p = p + (var1 + var2 + ((real64_t)dev->callibration_pres_data.dig.P7)) / 16.0;
    return p;
}

// Returns humidity in %RH as unsigned 32 bit integer in Q22.10 format (22 integer and 10 fractional bits).
// Output value of “47445” represents 47445/1024 = 46.333 %RH
BOSCH280_U32_t ICACHE_FLASH_ATTR BOSCH280_compensate_H_int32(struct BOSCH280_device_struct * dev, BOSCH280_S32_t adc_H)
{
    BOSCH280_S32_t v_x1_u32r;
    v_x1_u32r = (dev->t_fine -((BOSCH280_S32_t)76800));
    v_x1_u32r = (((((adc_H << 14) -(((BOSCH280_S32_t)dev->callibration_humm_data.H4) << 20) -(((BOSCH280_S32_t)dev->callibration_humm_data.H5) * v_x1_u32r))
                   + ((BOSCH280_S32_t)16384)) >> 15) * (((((((v_x1_u32r * ((BOSCH280_S32_t)dev->callibration_humm_data.H6)) >> 10)
                                                            * (((v_x1_u32r * ((BOSCH280_S32_t)dev->callibration_humm_data.H3)) >> 11)
                                                               + ((BOSCH280_S32_t)32768))) >> 10) + ((BOSCH280_S32_t)2097152))
                                                         * ((BOSCH280_S32_t)dev->callibration_humm_data.H2) + 8192) >> 14));
    v_x1_u32r = (v_x1_u32r -(((((v_x1_u32r >> 15) * (v_x1_u32r >> 15)) >> 7) * ((BOSCH280_S32_t)dev->callibration_humm_data.H1)) >> 4));
    v_x1_u32r = (v_x1_u32r < 0 ? 0 : v_x1_u32r);
    v_x1_u32r = (v_x1_u32r > 419430400? 419430400: v_x1_u32r);
    return (BOSCH280_U32_t)(v_x1_u32r>>12);
}

// Returns humidity in %rH as as double. Output value of “46.332” represents 46.332 %rH
real64_t ICACHE_FLASH_ATTR BOSCH280_compensate_H_double(struct BOSCH280_device_struct * dev, BOSCH280_S32_t adc_H)
{
    real64_t var_H;
    var_H = (((real64_t)dev->t_fine) - 76800.0);
    var_H = (adc_H - (((real64_t)dev->callibration_humm_data.H4) * 64.0 + ((real64_t)dev->callibration_humm_data.H5) / 16384.0 * var_H)) *
            (((real64_t)dev->callibration_humm_data.H2) / 65536.0 * (1.0 + ((real64_t)dev->callibration_humm_data.H6) / 67108864.0 * var_H *
                                                                     (1.0 + ((real64_t)dev->callibration_humm_data.H3) / 67108864.0 * var_H)));
    var_H = var_H * (1.0 - ((real64_t)dev->callibration_humm_data.H1) * var_H / 524288.0);
    if (var_H > 100.0){var_H = 100.0;}
    else if (var_H < 0.0){var_H = 0.0;}
    return var_H;
}

uint8 ICACHE_FLASH_ATTR BOSCH280_first_i2c_dev()
{
    i2c_master_start();
    i2c_master_writeByte((uint8)(BOSCH280_I2C_ADDR1 << 1));
    if (!i2c_master_checkAck())
    {
        i2c_master_stop();

        i2c_master_start();
        i2c_master_writeByte((uint8)(BOSCH280_I2C_ADDR2 << 1));
        if (!i2c_master_checkAck())
        {
            os_printf("%d i2c BOSCH280_find_i2c_dev. No Bosch device found.\r\n", __LINE__);
            i2c_master_stop();
            return 0;
        }

        i2c_master_stop();
        return BOSCH280_I2C_ADDR2;
    }

    i2c_master_stop();
    return BOSCH280_I2C_ADDR1;
}

void ICACHE_FLASH_ATTR BOSCH280_set_single_dev(uint8 i2c_dev_addr)
{
    current_selected_device = (struct BOSCH280_device_struct *)os_zalloc(sizeof(struct BOSCH280_device_struct));
    current_selected_device->chip_id = BOSCH280_chip_id(i2c_dev_addr);
    current_selected_device->bosch280_selected_device = i2c_dev_addr;
}

struct BOSCH280_device_struct * BOSCH280_init_dev(uint8 i2c_dev_addr)
{
    struct BOSCH280_device_struct * dev = (struct BOSCH280_device_struct *)os_zalloc(sizeof(struct BOSCH280_device_struct));
    dev->chip_id = BOSCH280_chip_id(i2c_dev_addr);
    dev->bosch280_selected_device = i2c_dev_addr;
    switch(dev->chip_id)
    {
    case 0x56:
    case 0x57:
    case 0x58:
        BMP280_read_calibration_data(dev);
        break;

    case 0x60:
        BME280_read_calibration_data(dev);
        break;
    }

    return dev;
}

bool ICACHE_FLASH_ATTR BOSCH280_select_i2c_dev(uint8 i2c_dev_addr)
{
    i2c_master_start();
    i2c_master_writeByte((uint8)(i2c_dev_addr << 1));
    if (!i2c_master_checkAck()) {
        os_printf("%d i2c bme280_select_i2c_dev checkAck error \r\n", __LINE__);
        i2c_master_stop();
        return false;
    }
    return true;
}

bool ICACHE_FLASH_ATTR BOSCH280_readNextBytes(uint8 i2c_dev_addr, uint8 *data, uint16 len)
{
    uint8 loop;
    // signal i2c start
    i2c_master_start();
    // write i2c address & direction
    i2c_master_writeByte((uint8)((i2c_dev_addr << 1) | 1));
    if (!i2c_master_checkAck()) {
        os_printf("i2c at24c_readNextBytes checkAck error \r\n");
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

bool ICACHE_FLASH_ATTR BOSCH280_write_byte_to_reg(uint8 i2c_dev_addr, uint8 reg, uint8 byte)
{
    if(BOSCH280_select_i2c_dev(i2c_dev_addr) == false)
    {
        os_printf("[%d] BME280 i2c device can not be selected/accessed\r\n", __LINE__);
        return false;
    }

    i2c_master_writeByte((uint8)(reg));
    if (!i2c_master_checkAck()) {
        os_printf("%d i2c bme280_read_byte_from_reg checkAck error \r\n", __LINE__);
        i2c_master_stop();
        return false;
    }

    i2c_master_writeByte((uint8)(byte));
    if (!i2c_master_checkAck()) {
        os_printf("%d i2c bme280_read_byte_from_reg checkAck error \r\n", __LINE__);
        i2c_master_stop();
        return false;
    }

    i2c_master_stop();
    return true;
}

uint8 ICACHE_FLASH_ATTR BOSCH280_read_byte_from_reg(uint8 i2c_dev_addr, uint8 reg)
{
    if(BOSCH280_select_i2c_dev(i2c_dev_addr) == false)
    {
        os_printf("[%d] BME280 i2c device can not be selected/accessed\r\n", __LINE__);
        return 0;
    }

    i2c_master_writeByte((uint8)(reg));
    if (!i2c_master_checkAck()) {
        os_printf("%d i2c bme280_read_byte_from_reg checkAck error \r\n", __LINE__);
        i2c_master_stop();
        return 0;
    }

    i2c_master_start();
    i2c_master_writeByte((uint8)(i2c_dev_addr << 1) | 1);
    if (!i2c_master_checkAck()) {
        os_printf("%d i2c bme280_read_byte_from_reg checkAck error \r\n", __LINE__);
        i2c_master_stop();
        return 0;
    }

    uint8 ret = i2c_master_readByte();
    i2c_master_send_nack();
    i2c_master_stop();
    return ret;
}

bool ICACHE_FLASH_ATTR BOSCH280_read_bytes_from_reg(uint8 i2c_dev_addr, uint8 reg_start, uint8 len, uint8 ** buf /*should be not allocated*/)
{
    if(BOSCH280_select_i2c_dev(i2c_dev_addr) == false)
    {
        os_printf("[%d] BME280 i2c device can not be selected/accessed\r\n", __LINE__);
        return false;
    }

    i2c_master_writeByte(reg_start);
    if (!i2c_master_checkAck())
    {
        os_printf("%d i2c bme280_temp checkAck error \r\n", __LINE__);
        i2c_master_stop();
        return false;
    }

    *buf = (uint8*)os_zalloc(len);
    bool ret = BOSCH280_readNextBytes(i2c_dev_addr, *buf, len );

    //INFO("BME READ BYTES -> %s\r\n", ret?"true":"false");
    if(!ret)
    {
        os_free(*buf);
        return false;
    }

    return true;
}


uint8 ICACHE_FLASH_ATTR BOSCH280_chip_id(uint8 i2c_dev_addr)
{
    if(BOSCH280_select_i2c_dev(i2c_dev_addr) == false)
    {
        os_printf("[%d] BME280 i2c device can not be selected/accessed\r\n", __LINE__);
        return 0;
    }

    i2c_master_writeByte((uint8)(BOSCH280_CHIP_ID_REG));
    if (!i2c_master_checkAck()) {
        os_printf("%d i2c bme280_chip_id checkAck error \r\n", __LINE__);
        i2c_master_stop();
        return 0;
    }

    i2c_master_start();
    i2c_master_writeByte((uint8)(i2c_dev_addr << 1) | 1);
    if (!i2c_master_checkAck()) {
        os_printf("%d i2c bme280_chip_id checkAck error \r\n", __LINE__);
        i2c_master_stop();
        return 0;
    }

    uint8 ret = i2c_master_readByte();
    i2c_master_send_nack();
    i2c_master_stop();
    return ret;
}

bool ICACHE_FLASH_ATTR BOSCH280_soft_reset(struct BOSCH280_device_struct * dev)
{
    if(BOSCH280_select_i2c_dev(dev->bosch280_selected_device) == false)
    {
        os_printf("[%d] BME280 i2c device can not be selected/accessed\r\n", __LINE__);
        return false;
    }

    i2c_master_writeByte((uint8)(BOSCH280_SOFT_RST_REG));
    if (!i2c_master_checkAck()) {
        os_printf("[%d] i2c bme280_chip_id checkAck error \r\n", __LINE__);
        i2c_master_stop();
        return false;
    }

    i2c_master_writeByte((uint8)(BOSCH280_SOFT_RST_BYTE));
    if (!i2c_master_checkAck()) {
        os_printf("[%d] i2c bme280_chip_id checkAck error \r\n", __LINE__);
        i2c_master_stop();
        return false;
    }

    i2c_master_stop();
    return true;
}

bool ICACHE_FLASH_ATTR BOSCH280_is_measuring(struct BOSCH280_device_struct * dev)
{
    uint8 status = BOSCH280_read_byte_from_reg(dev->bosch280_selected_device, 0xF3);
    if(status & (1<<3))
    {
        //os_printf("measuring...\r\n");
        return true;//os_delay_us(60000);
    }
    return false;
}

bool ICACHE_FLASH_ATTR BOSCH280_measure(struct BOSCH280_device_struct * dev)
{
    if(BOSCH280_set_mode(dev, 0x01))
    {
        os_delay_us(6000);
        sint8 i = 10;
        while(BOSCH280_is_measuring(dev) && i > 0)
        {
            i--;
            os_delay_us(6000);
        }
        return (i>0);
    }
    return false;
}


bool ICACHE_FLASH_ATTR BME280_read_calibration_data(struct BOSCH280_device_struct * dev)
{
    uint8 * calib00 = NULL;
    bool read = BOSCH280_read_bytes_from_reg(dev->bosch280_selected_device, 0x88, 26, &calib00);
    if(!read)
    {
        os_printf("CAN't read callibration data from 0x88..0xA1\r\n");
        return false;
    }

    uint8 * calib26 = NULL;
    read = BOSCH280_read_bytes_from_reg(dev->bosch280_selected_device, 0xE1, 15, &calib26);
    if(!read)
    {
        os_printf("CAN't read callibration data from 0xE1..0xE6\r\n");
        return false;
    }

    os_memcpy((void *)dev->callibration_temp_data.data, calib00, 6);
    os_memcpy((void *)dev->callibration_pres_data.data, &calib00[6], 18);

    dev->callibration_humm_data.H1 = calib00[25];//bme280_read_byte_from_reg(i2c_dev_addr, 0xA1);                     //A1
    //BITINFOu16("H1 -> ", dev->callibration_humm_data.H1);
    //BITINFO("calib26[0] ", calib26[0]);
    //BITINFO("calib26[1] ", calib26[1]);
    //BITINFO("calib26[2] ", calib26[2]);
    //os_memcpy((void*)&callibration_humm_data.data[1], calib26, 2);   //E1+E2   = H2
    // [     E2          E1     ]
    // [ 0000 0000   0000 0000  ]
    // [ calib26[1]  calib26[0] ]
    dev->callibration_humm_data.H2 = (uint16_t)calib26[1] << 8;
    dev->callibration_humm_data.H2 |= calib26[0];

    //BITINFOu16("H2 -> ", dev->callibration_humm_data.H2);

    dev->callibration_humm_data.H3 = calib26[2];                      //E3 = H3
    //BITINFOu16("H3 -> ", dev->callibration_humm_data.H3);

    dev->callibration_humm_data.H4 = (uint16_t)calib26[3] << 4;       //E4 =    H4[0000 ->0000 0000<- 0000]
    dev->callibration_humm_data.H4 |= calib26[4] & 0b00001111;        //E5[0:3] H4[0000   0000 0000 ->0000<-]

    //BITINFOu16("H4 -> ", dev->callibration_humm_data.H4);

    dev->callibration_humm_data.H5 = calib26[4] >> 4;                 //E5[7:4] H5[0000   0000 0000 ->0000<-]
    dev->callibration_humm_data.H5 |= (uint16_t)calib26[5] << 4;      //E6      H5[0000 ->0000 0000<- 0000]

    //BITINFOu16("H5 -> ", dev->callibration_humm_data.H5);

    dev->callibration_humm_data.H6 = calib26[6];                      //E7

    //BITINFOu16("H6 -> ", dev->callibration_humm_data.H6);

    os_free(calib00);
    os_free(calib26);

    return true;
}

bool ICACHE_FLASH_ATTR BMP280_read_calibration_data(struct BOSCH280_device_struct * dev)
{
    uint8 * calib00 = NULL;
    bool read = BOSCH280_read_bytes_from_reg(dev->bosch280_selected_device, 0x88, 26, &calib00);
    if(!read)
    {
        os_printf("CAN't read callibration data from 0x88..0xA1\r\n");
        return false;
    }

    os_memcpy((void *)dev->callibration_temp_data.data, calib00, 6);
    os_memcpy((void *)dev->callibration_pres_data.data, &calib00[6], 18);

    os_free(calib00);
    return true;
}

real64_t ICACHE_FLASH_ATTR BOSCH280_read_humm_double(struct BOSCH280_device_struct * dev)
{
    uint8 * temp_data = NULL;
    bool read = BOSCH280_read_bytes_from_reg(dev->bosch280_selected_device, 0xFD, 2, &temp_data);
    if(!read)
    {
        os_printf("CAN't read bme280_read_temp_longint\r\n");
        return 0;
    }

    uint32_t adc_h  = 0;
    adc_h = temp_data[1];
    adc_h |= (uint32_t)temp_data[0] << 8;
    os_free(temp_data);
    real64_t humm = BOSCH280_compensate_H_double(dev, adc_h);
    return humm;
}

BOSCH280_S32_t ICACHE_FLASH_ATTR BOSCH280_read_humm_longint(struct BOSCH280_device_struct * dev)
{
    uint8 * temp_data = NULL;
    bool read = BOSCH280_read_bytes_from_reg(dev->bosch280_selected_device, 0xFD, 2, &temp_data);
    if(!read)
    {
        os_printf("CAN't read bme280_read_temp_longint\r\n");
        return 0;
    }

    BOSCH280_S32_t adc_h  = 0;
    adc_h = temp_data[1];
    adc_h |= (uint32_t)temp_data[0] << 8;

    os_free(temp_data);

    BOSCH280_U32_t humm = BOSCH280_compensate_H_int32(dev, adc_h);
    return humm;
}

BOSCH280_S32_t ICACHE_FLASH_ATTR BOSCH280_read_pres_longint(struct BOSCH280_device_struct * dev)
{
    uint8 * temp_data = NULL;
    bool read = BOSCH280_read_bytes_from_reg(dev->bosch280_selected_device, 0xF7, 3, &temp_data);
    if(!read)
    {
        os_printf("CAN't read bme280_read_temp_longint\r\n");
        return 0;
    }

    uint32_t adc_p  = 0;
    adc_p =  (uint32_t)temp_data[2] >> 4;
    adc_p |= (uint32_t)temp_data[1] << 4;
    adc_p |= (uint32_t)temp_data[0] << 12;

    os_free(temp_data);

    BOSCH280_S32_t pres = BOSCH280_compensate_P_int64(dev, adc_p);
    return pres;
}

real64_t ICACHE_FLASH_ATTR BOSCH280_read_pres_double(struct BOSCH280_device_struct * dev)
{
    uint8 * temp_data = NULL;
    bool read = BOSCH280_read_bytes_from_reg(dev->bosch280_selected_device, 0xF7, 3, &temp_data);
    if(!read)
    {
        os_printf("CAN't read bme280_read_temp_longint\r\n");
        return 0;
    }

    uint32_t adc_p  = 0;
    adc_p =  (uint32_t)temp_data[2] >> 4;
    adc_p |= (uint32_t)temp_data[1] << 4;
    adc_p |= (uint32_t)temp_data[0] << 12;

    os_free(temp_data);

    real64_t pres = BOSCH280_compensate_P_double(dev, adc_p);
    if(pres == 0.0)return 0.0;
    return pres/100.0;
}

BOSCH280_S32_t ICACHE_FLASH_ATTR BOSCH280_read_temp_longint(struct BOSCH280_device_struct * dev)
{
    uint8 * temp_data = NULL;
    bool read = BOSCH280_read_bytes_from_reg(dev->bosch280_selected_device, 0xFA, 3, &temp_data);
    if(!read)
    {
        os_printf("CAN't read bme280_read_temp_longint\r\n");
        return 0;
    }

    uint32_t adc_t = 0;
    adc_t  = (uint32_t)temp_data[2] >> 4;
    adc_t |= (uint32_t)temp_data[1] << 4;
    adc_t |= (uint32_t)temp_data[0] << 12;

    os_free(temp_data);

    BOSCH280_S32_t temp = BOSCH280_compensate_T_int32(dev, adc_t);
    return temp;
}

real64_t ICACHE_FLASH_ATTR BOSCH280_read_temp_double(struct BOSCH280_device_struct * dev)
{
    uint8 * temp_data = NULL;
    bool read = BOSCH280_read_bytes_from_reg(dev->bosch280_selected_device, 0xFA, 3, &temp_data);
    if(!read)
    {
        os_printf("CAN't read bme280_read_temp_double\r\n");
        return 0.0;
    }

    uint32_t adc_t = 0;
    adc_t  = (uint32_t)temp_data[2] >> 4;
    adc_t |= (uint32_t)temp_data[1] << 4;
    adc_t |= (uint32_t)temp_data[0] << 12;

    real64_t temp_double = BOSCH280_compensate_T_double(dev, adc_t);
    os_free(temp_data);
    return temp_double;
}

bool ICACHE_FLASH_ATTR BOSCH280_set_mode(struct BOSCH280_device_struct * dev, uint8 mode)
{
    uint8 cur = BOSCH280_read_byte_from_reg(dev->bosch280_selected_device, 0xF4);
    //BITINFO("CURMEAS", cur);

    cur = cur & 0b11111100; // mask out the bits we care about
    cur = cur | mode; // Set the magic bits

    //BITINFO("NEWMEAS", cur);

    BOSCH280_write_byte_to_reg(dev->bosch280_selected_device, 0xF4, cur);
}

bool ICACHE_FLASH_ATTR BOSCH280_set_indoor_navigation_config(struct BOSCH280_device_struct * dev)
{
    // weather monitoring. sens mode forced, oversampling 1,1,1, filter off
    // 0xF4 -> 8...0 osrs_t 111. osrs_p 111, mode 11
    // -> 001 001 01
    uint8 ctrl_meas = 0x27;//b 001 001 01;
    // 0xF2 ->ctrl_hum 000 00 001
    uint8 ctrl_hum = 0x0;
    // 0xF5 -> config
    // 001 011 00 //500ms
    uint8 ctrl_config = 0x00;//0x80;

    if(dev->chip_id == 0x60)
    {
        if(!BOSCH280_write_byte_to_reg(dev->bosch280_selected_device, 0xF2, ctrl_hum))
        {
            os_printf("Can't write ctrl_hum\r\n");
            return false;
        }
    }

    if(!BOSCH280_write_byte_to_reg(dev->bosch280_selected_device, 0xF5, ctrl_config))
    {
        os_printf("Can't write ctrl_config\r\n");
        return false;
    }
    else
    {
        if(!BOSCH280_write_byte_to_reg(dev->bosch280_selected_device, 0xF4, ctrl_meas))
        {
            os_printf("Can't write ctrl_meas\r\n");
            return false;
        }
    }

    return true;
}

bool ICACHE_FLASH_ATTR BOSCH280_set_weather_station_config(struct BOSCH280_device_struct * dev)
{
    // weather monitoring. sens mode forced, oversampling 1,1,1, filter off
    // 0xF4 -> 8...0 osrs_t 111. osrs_p 111, mode 11
    // -> 001 001 01
    uint8 ctrl_meas = 0x25;//b 001 001 01;
    // 0xF2 ->ctrl_hum 00000 001
    uint8 ctrl_hum = 0x01;
    // 0xF5 -> config
    // 001 011 00 //500ms
    uint8 ctrl_config = 0x00;//0x80;

    if(dev->chip_id == 0x60)
    {
        if(!BOSCH280_write_byte_to_reg(dev->bosch280_selected_device, 0xF2, ctrl_hum))
        {
            os_printf("Can't write ctrl_hum\r\n");
            return false;
        }
    }

    if(!BOSCH280_write_byte_to_reg(dev->bosch280_selected_device, 0xF5, ctrl_config))
    {
        os_printf("Can't write ctrl_config\r\n");
        return false;
    }
    else
    {
        if(!BOSCH280_write_byte_to_reg(dev->bosch280_selected_device, 0xF4, ctrl_meas))
        {
            os_printf("Can't write ctrl_meas\r\n");
            return false;
        }
    }

    return true;
}


