#include "DS3231.h"
#include "main.h"
#include "i2c.h"
//struct time
//{
//    char year;
//    char mon;
//    char day;
//    char hour;
//    char min;
//    char sec;
//}now;//保存当前时间的结构体

struct time now={0};

//由于器件里的是8421码所以需要转换一下
//读出来的是8421需要转成16进制
//变量写进去要转成8421写进去
char BCD2HEX(char val)//8421转16进制
{
     char temp;
     temp=val&0x0f;
     val>>=4;
     val&=0x0f;
     val*=10;
     temp+=val;
     return temp;
}
char HEX2BCD(char val)//16进制转8421
{
     char i,j,k;
     i=val/10;
     j=val%10;
     k=j|(i<<4);
     return k;
}
uint8_t AD_ReadData(uint16_t Addr, uint8_t Reg)//硬件iic读取数据，器件地址，读取地址
{
    HAL_StatusTypeDef status = HAL_OK;
    uint8_t value = 0;
    status = HAL_I2C_Mem_Read(&hi2c1, Addr, Reg, I2C_MEMADD_SIZE_8BIT, &value, 1, 100);
    if(status != HAL_OK)
    {
    }
    return value;
}
void get_time()//循环调用，读取时间到结构体
{
    now.year=BCD2HEX(AD_ReadData(DS3231_ADDRESS|I2C_Read,DS3231_YEAR_REG));
    now.mon=BCD2HEX(AD_ReadData(DS3231_ADDRESS|I2C_Read,DS3231_MONTH_CENTURY_REG));
    now.day=BCD2HEX(AD_ReadData(DS3231_ADDRESS|I2C_Read,DS3231_DATE_REG));
    now.hour=BCD2HEX(AD_ReadData(DS3231_ADDRESS|I2C_Read,DS3231_HOUR_REG));
    now.min=BCD2HEX(AD_ReadData(DS3231_ADDRESS|I2C_Read,DS3231_MINUTES_REG));
    now.sec=BCD2HEX(AD_ReadData(DS3231_ADDRESS|I2C_Read,DS3231_SECONDS_REG));
}
void Writetime(char add,char time)//修改时间，修改地址，
{
char *pData;
time=HEX2BCD(time);
pData=&time;
HAL_I2C_Mem_Write(&hi2c1,DS3231_ADDRESS|I2C_Write,add,I2C_MEMADD_SIZE_8BIT,(uint8_t *)pData,1,100);
}

