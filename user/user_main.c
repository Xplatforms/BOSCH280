#include <ets_sys.h>
#include <osapi.h>
#include <mem.h>
#include <user_interface.h>
#include <../driver_lib/include/driver/uart.h>

#include <i2c_master.h>
#include <bosch280_sensortec.h>


///FORWARD DECL. DEF is in utils/utils.c
void ICACHE_FLASH_ATTR BITINFO(char * text, uint8 inpins);
void ICACHE_FLASH_ATTR BITINFOu16(char * text, uint16 inpins);
void ICACHE_FLASH_ATTR BITINFOu32(char * text, uint32 inpins);
char * ICACHE_FLASH_ATTR printFloat(real64_t val, char *buff);
///

#ifdef CONFIG_ENABLE_IRAM_MEMORY
uint32 user_iram_memory_is_enabled(void){return CONFIG_ENABLE_IRAM_MEMORY;}
#endif

static const partition_item_t at_partition_table[] = {
    { SYSTEM_PARTITION_BOOTLOADER, 						0x0, 												0x1000},
    { SYSTEM_PARTITION_OTA_1,   						0x1000, 											SYSTEM_PARTITION_OTA_SIZE},
    { SYSTEM_PARTITION_OTA_2,   						SYSTEM_PARTITION_OTA_2_ADDR, 						SYSTEM_PARTITION_OTA_SIZE},
    { SYSTEM_PARTITION_RF_CAL,  						SYSTEM_PARTITION_RF_CAL_ADDR, 						0x1000},
    { SYSTEM_PARTITION_PHY_DATA, 						SYSTEM_PARTITION_PHY_DATA_ADDR, 					0x1000},
    { SYSTEM_PARTITION_SYSTEM_PARAMETER, 				SYSTEM_PARTITION_SYSTEM_PARAMETER_ADDR, 			0x3000},
};

void user_pre_init(void)
{
    if(!system_partition_table_regist(at_partition_table, sizeof(at_partition_table)/sizeof(at_partition_table[0]),SPI_FLASH_SIZE_MAP))
    {
        os_printf("system_partition_table_regist fail\r\n");
        while(1);
    }
}

static ETSTimer bosch280_timer;

void init_gpio_esp(void)
{
    gpio_init();
    i2c_master_gpio_init();
}


void ICACHE_FLASH_ATTR publish_telemetry(void * args)
{
    struct BOSCH280_device_struct * bosch_dev = (struct BOSCH280_device_struct *)args;
    if(bosch_dev == NULL)
    {
        os_printf("No Bosch i2c sensors found. return\n");
        return;
    }

    if(!BOSCH280_measure(bosch_dev))
    {
        os_printf("Can't measure BMP/BME data or timeout\r\n");
        return;
    }

    real64_t temp_double = BOSCH280_read_temp_double(bosch_dev);
    real64_t pres_double = BOSCH280_read_pres_double(bosch_dev);
    char td[16] = {0};
    char pd[16] = {0};

    char * connectivity_data = (char*)os_zalloc(48);
    //Check if this is an BME sensor by id
    if(bosch_dev->chip_id == 0x60)
    {
        real64_t humm_double = BOSCH280_read_humm_double(bosch_dev);
        char hd[16] = {0};

        os_sprintf(connectivity_data,"{\"p\":%s,\"h\":%s,\"t\":%s}",
                   printFloat(pres_double, pd),
                   printFloat(humm_double, hd),
                   printFloat(temp_double, td));
    }
    //if it is bmp we don't have hummidity
    else
    {
        os_sprintf(connectivity_data,"{\"p\":%s,\"t\":%s}",
                   printFloat(pres_double, pd),
                   printFloat(temp_double, td));
    }

    int cdata_len = os_strlen(connectivity_data);

    os_printf("[publish_telemetry] %s data len %u\r\n", connectivity_data, cdata_len);

    os_free(connectivity_data);
}

void user_init(void)
{
    init_gpio_esp();

    //for debug output
    uart_init(BIT_RATE_115200, BIT_RATE_115200);
    os_delay_us(60000);

    struct BOSCH280_device_struct * bosch_dev = NULL;

    uint8 bosch_dev_i2c = BOSCH280_first_i2c_dev();
    if(bosch_dev_i2c != 0)
    {
        bosch_dev = BOSCH280_init_dev(bosch_dev_i2c);
        os_printf("CHIP_ID 0x%x\n", bosch_dev->chip_id);
        if(BOSCH280_set_weather_station_config(bosch_dev))
        {
            //Just a Test
            if(BOSCH280_is_measuring(bosch_dev))
            {
                os_delay_us(60000);
            }

            real64_t temp_double = BOSCH280_read_temp_double(bosch_dev);
            BOSCH280_S32_t temp_longint = BOSCH280_read_temp_longint(bosch_dev);
            char td[8] = {0};
            os_printf("\r\nREAL TEMP: %u %s\r\n", temp_longint, printFloat(temp_double, td));

            BOSCH280_S32_t pres = BOSCH280_read_pres_longint(bosch_dev);
            real64_t pres_double = BOSCH280_read_pres_double(bosch_dev);
            char pd[16] = {0};
            os_printf("\r\nREAL PRES: %u Pa %s hPa\r\n", pres, printFloat(pres_double, pd));

            BOSCH280_S32_t humm = BOSCH280_read_humm_longint(bosch_dev);
            real64_t humm_double = BOSCH280_read_humm_double(bosch_dev);
            char hd[16] = {0};
            char hd1[16] = {0};
            os_printf("\r\nREAL HUMM: %s rH %s rH\r\n", printFloat((real64_t)humm/1024.0, hd1), printFloat(humm_double, hd));
        }

        //Set looping timer every 15sec
        os_timer_disarm(&bosch280_timer);
        os_timer_setfn(&bosch280_timer, (os_timer_func_t *)publish_telemetry, (void*)bosch_dev);
        os_timer_arm(&bosch280_timer, 15000, true);//msec
    }
    else
    {
        bosch_dev = NULL;
        os_printf("\r\n BOSCH SENSORTEC device not found\r\n");
    }

}
