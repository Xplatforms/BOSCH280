#include <ets_sys.h>
#include <osapi.h>

void ICACHE_FLASH_ATTR BITINFO(char * text, uint8 inpins)
{
    os_printf("%s BITMASK for d%d x%02X b", text, inpins, inpins);
    uint8 tmp = inpins;
    uint8 i = 8;
    char data[9] = {0};
    while (i > 0)
    {
        i--;
        if (tmp & 1)data[i] = '1';
        else data[i] = '0';
        tmp >>= 1;
    }
    os_printf("%s\r\n", data);
}

void ICACHE_FLASH_ATTR BITINFOu16(char * text, uint16 inpins)
{
    os_printf("%s BITMASK for d%d x%02X b", text, inpins, inpins);
    uint16 tmp = inpins;
    uint8 i = 16;
    char data[18] = {0};
    while (i > 0)
    {
        i--;
        if (tmp & 1)data[i] = '1';
        else data[i] = '0';
        tmp >>= 1;
    }
    os_printf("%s\r\n", data);
}

void ICACHE_FLASH_ATTR BITINFOu32(char * text, uint32 inpins)
{
    os_printf("%s BITMASK for d%d x%02X b", text, inpins, inpins);
    uint32 tmp = inpins;
    uint8 i = 32;
    char data[36] = {0};
    while (i > 0)
    {
        i--;
        if (tmp & 1)data[i] = '1';
        else data[i] = '0';
        tmp >>= 1;
    }
    os_printf("%s\r\n", data);
} 

char * ICACHE_FLASH_ATTR printFloat(real64_t val, char *buff)
{
    char smallBuff[16] = {0};
    int val1 = (int) val;
    unsigned int val2;
    if (val < 0) {
        val2 = (int) (-100.0 * val) % 100;
    } else {
        val2 = (int) (100.0 * val) % 100;
    }
    os_sprintf(smallBuff, "%i.%02u", val1, val2);
    strcpy(buff, smallBuff);
    return buff;
}
