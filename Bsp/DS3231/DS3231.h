#ifndef __DS3231_H
#define __DS3231_H

#include "main.h"

struct time
{
    char year;
    char mon;
    char day;
    char hour;
    char min;
    char sec;
};//保存当前时间的结构体  

typedef struct{
	uint8_t second;
	uint8_t minute;
	uint8_t hour;
	uint8_t day;
	uint8_t data;
	uint8_t month;
	uint8_t year;
}DS3231_Data_Typedef;

/******DS3231寄存器*/
#define DS3231_ADDRESS 						0xD0//DS3213的地址
#define I2C_Write									0x00//写地址
#define I2C_Read       						0X01//读地址
/*DS3213 寄存器*/
#define DS3231_SECONDS_REG				0x00//秒
#define DS3231_MINUTES_REG				0x01//分
#define DS3231_HOUR_REG				    0x02//时
#define DS3231_DAY_REG						0x03//星期
#define DS3231_DATE_REG						0x04//日
#define DS3231_MONTH_CENTURY_REG	0x05//月/世纪
#define DS3231_YEAR_REG						0x06//年

//闹钟1
#define DS3231_ALARM1SENONDS_REG	0x07
#define DS3231_ALARM1MINUTES_REG	0x08
#define DS3231_ALARM1HOURS_REG		0x09
#define DS3231_ALARM1DAY_REG			0x0A
#define DS3231_ALARM1DATE_REG			0x0A
//闹钟2
#define DS3231_ALARM2MINUTES_REG  0x0B
#define DS3231_ALARM2HOURS_REG		0x0C
#define DS3231_ALARM2DAY_REG			0x0D
#define DS3231_ALARM2DATE_REG			0x0D

#define DS3231_CONTROL_REG				0x0E//控制寄存器
#define DS3231_CONTROL_STATE_REG	0x0F//状态寄存器
#define DS3231_AGINGOFFSET_REG		0x10//老化补偿寄存器
#define DS3231_MSB_OF_TEMP_REG		0x11//温度寄存器高字节
#define DS3231_LSB_OF_TEMP_REG    0x12//温度寄存器低字节
/******DS3231寄存器*/

// //定义IIC的硬件引脚
// #define DS3231_SCL_PROT	GPIOA
// #define DS3231_SCL_PIN  GPIO_PIN_0
// #define DS3231_SDA_PROT DEV_GPIOA
// #define DS3231_SDA_PIN  0

void get_time();
void Writetime(char add,char time);

extern struct time now;

#endif