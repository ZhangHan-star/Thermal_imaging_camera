/**
 * @file coreapp.c
 * @author 星痕迷鹿 (ZhangHan8233@outlook.com)
 * @brief 核心任务c文件
 * @version 0.1
 * @date 2025-03-07
 *
 * @copyright Copyright (c) 2025
 *
 */

#include "coreapp.h"
#include "stdio.h"
#include "fatfs.h"

#include "st7789.h"
#include "MLX90640_API.h"
#include "DS3231.h"
#include "bmp.h"
#include "DS3231.h"

#include "dsp/interpolation_functions.h"

#define DEF_SEG 4    // 插值图像分割显示到屏幕的份数
#define DEF_MAG 85   // 插值放大倍数（乘以10）
#define RAW_WIDTH 32
#define RAW_HEIGHT 24
#define IMAGE_WIDTH 272  // 插值后图像宽度
#define IMAGE_HEIGHT 204 // 插值后图像高度
#define INTERP_COEFF_W (IMAGE_WIDTH-1)/(RAW_WIDTH-1)
#define INTERP_COEFF_H (IMAGE_HEIGHT-1)/(RAW_HEIGHT-1)

#if (IMAGE_WIDTH * 10 / RAW_WIDTH != DEF_MAG) || (IMAGE_HEIGHT * 10 / RAW_HEIGHT != DEF_MAG)
#error "IMAGE_WIDTH/RAW_WIDTH or IMAGE_HEIGHT/RAW_HEIGHT does not match DEF_MAG"
#endif

#if IMAGE_HEIGHT % DEF_SEG != 0
#error "IMAGE_HEIGHT must be divisible by DEF_SEG"
#endif


 // 将归一化后的数据转换为适合rgb565格式的色彩数值
void GrayToPseColor(uint8_t grayValue, uint8_t* colorR, uint8_t* colorG, uint8_t* colorB);
void set_bmp_header(FIL *file, char *FILE_NAME);
void set_bmp_write_image(FIL *file, uint16_t *image);

// MLX90640相关数据
unsigned short EE[832];
unsigned short Frame[834];
paramsMLX90640 MLXPars;
float Vdd, Ta, Tr;

//原始温度数据缓冲区
float raw_data[32 * 24];
float temp[768];
//最大最小温度
float max_temp, min_temp;

//插值后图像缓冲区
uint16_t image_data[272 * 51];
//临时存储的插值结果
float temp_interp;
//临时存储归一化数据
uint8_t temp_norma;

//共用体，用于将色彩数据转换为16位RGB565格式
union color {
  uint16_t raw;
  struct {
    uint16_t b : 5;
    uint16_t g : 6;
    uint16_t r : 5;
  };
}union_color_t;
uint8_t R, G, B;

//双线性插值结构体
arm_bilinear_interp_instance_f32 bilinearInterp;

//
uint8_t shutter_flag = 0;
FIL bmp_file;

extern struct time now;

char str[50];

void coreapp_init(void)
{
  // 初始化MLX90640
  MLX90640_SetRefreshRate(0x33, 4); //测量速率 1Hz(0~7 对应 0.5,1,2,4,8,16,32,64Hz)
  MLX90640_DumpEE(0x33, EE); //读取像素校正参数
  MLX90640_ExtractParameters(EE, &MLXPars); //解析校正参数（计算温度时需要）
  sprintf(str, "MLX90640 Init");
  ST7789_WriteString(0, 72, str, Font_11x18, BLACK, WHITE);
  // 双线性插值结构体初始化
  bilinearInterp.numCols = 32;
  bilinearInterp.numRows = 24;
  bilinearInterp.pData = temp;
  sprintf(str, "bilinearInterp Init");
  ST7789_WriteString(0, 90, str, Font_11x18, BLACK, WHITE);

  sprintf(str, "CoreApp Init");
  ST7789_WriteString(0, 126, str, Font_11x18, WHITE, BLACK);

  HAL_Delay(1000);
}


void coreapp_loop(void)
{
  int8_t status;
  // 获取数据
  while ((status = MLX90640_GetFrameData(0x33, Frame)) != 0 && status != 1)
  {
    ST7789_WriteString(0, 0, "GetFrameData Failed", Font_11x18, WHITE, BLACK);
    // 添加适当的延时，避免忙等待
    HAL_Delay(1000);
  }

  get_time();
  sprintf(str, "%02d-%02d %02d:%02d:%02d", now.mon, now.day, now.hour, now.min, now.sec);
  ST7789_WriteString(150, 204, str, Font_11x18, WHITE, BLACK);

  if (HAL_GPIO_ReadPin(SW_OK_GPIO_Port, SW_OK_Pin) == GPIO_PIN_RESET){
    shutter_flag = 1;
    char file_name[15];
    sprintf(file_name, "%2d%2d%2d.bmp", now.hour, now.min, now.sec);
    set_bmp_header(&bmp_file, file_name);
  }
  Vdd = MLX90640_GetVdd(Frame, &MLXPars); //计算 Vdd（这句可有可无）
  Ta = MLX90640_GetTa(Frame, &MLXPars); //计算实时外壳温度
  Tr = Ta - 8.0; //计算环境温度用于温度补偿
  //手册上说的环境温度可以用外壳温度-8℃
  MLX90640_CalculateTo(Frame, &MLXPars, 0.95, Tr, raw_data); //计算像素点温度

  //计算温度最大最小值
  max_temp = -40, min_temp = 300;
  for (uint16_t i = 0; i < 768; i++)
  {
    if (raw_data[i] > max_temp)max_temp = raw_data[i];
    else if (raw_data[i] < min_temp)min_temp = raw_data[i];
  }
  sprintf(str, "maxtemp:%.2f", max_temp);
  ST7789_WriteString(0, 204, str, Font_11x18, WHITE, BLACK);
  sprintf(str, "mintemp:%.2f", min_temp);
  ST7789_WriteString(0, 222, str, Font_11x18, WHITE, BLACK);


  for (uint8_t h = 0; h < 24; h++)
  {
    for (uint8_t w = 0; w < 32; w++)
    {
      temp[(23-h)*32+w] = raw_data[h*32+w];
    }
  }
  // for (uint8_t h = 0; h < 24; h++)
  // {
  //   for (uint8_t w = 0; w < 32; w++)
  //   {
  //     raw_data[h*32+w] = temp[h*32+w];
  //   }
  // }



  //进行插值处理
  for (uint8_t n = 0; n < 4; n++)
  {
    for (uint16_t h = 0; h < 51; h++)
    {
      for (uint16_t w = 0; w < 272; w++)
      {
        // temp_interp = arm_bilinear_interp_f32(&bilinearInterp, (float)(w / INTERP_COEFF_W), (float)((h + n * (IMAGE_HEIGHT / DEF_SEG)) / INTERP_COEFF_H));
        temp_interp = arm_bilinear_interp_f32(&bilinearInterp, (float)(w / 8.741935f), (float)((h + n * 51) / 8.826086));
        temp_norma = (uint8_t)((temp_interp - min_temp) / (max_temp - min_temp) * 255);
        GrayToPseColor(temp_norma, &R, &G, &B);
        union_color_t.r = R;
        union_color_t.g = G;
        union_color_t.b = B;
        image_data[h * 272 + w] = union_color_t.raw;
        image_data[h * 272 + w] = (image_data[h * 272 + w] << 8 | image_data[h * 272 + w] >> 8);
      }
    }
    ST7789_DrawImage(0, n * 51, 272, 51, image_data);
    for (uint16_t i = 0; i < 13872; i++)
    {
      image_data[i] = (image_data[i] << 8 | image_data[i] >> 8);
    }

    if(shutter_flag == 1)set_bmp_write_image(&bmp_file,image_data);
  }
  if(shutter_flag == 1){
    f_close(&bmp_file);
    sprintf(str,"Save Success!");
    ST7789_WriteString(0, 100, str, Font_11x18, WHITE, BLACK);
    HAL_Delay(1000);
    shutter_flag = 0;
  }
}



void GrayToPseColor(uint8_t grayValue, uint8_t* colorR, uint8_t* colorG, uint8_t* colorB)
{
  if (colorR == NULL || colorG == NULL || colorB == NULL)
  {
    ST7789_WriteString(0, 0, "Error:", Font_16x26, RED, BLACK);
    return;
  }

  // float grayValue;
  // grayValue = grayValue1;
  if ((grayValue >= 0) && (grayValue <= 63))
  {
    *colorR = 0;
    *colorG = 0;
    *colorB = (uint8_t)((float)grayValue / 64.0f * 31.0f);
  }
  else if ((grayValue >= 64) && (grayValue <= 127))
  {
    *colorR = 0;
    *colorG = (uint8_t)((float)(grayValue - 64) / 64.0f * 63.0f);
    *colorB = (uint8_t)((float)(127 - grayValue) / 64.0f * 31.0f);
  }
  else if ((grayValue >= 128) && (grayValue <= 191))
  {
    *colorR = (uint8_t)((float)(grayValue - 128) / 64.0f * 31.0f);
    *colorG = 255;
    *colorB = 0;
  }
  else if ((grayValue >= 192) && (grayValue <= 255))
  {
    *colorR = 127;
    *colorG = (uint8_t)((float)(255 - grayValue) / 64.0f * 63.0f);
    *colorB = 0;
  }
}

void set_bmp_header(FIL *file, char *FILE_NAME)
{
  FRESULT res;
  bmp_all_header_t bmp;
  UINT bytes_written;  // 写入的字节数
  UINT bytes_read;     // 读取的字节数
  char read_buffer[100];  // 读取缓冲区
  char str[256];

  res = f_open(file, FILE_NAME, FA_WRITE | FA_CREATE_ALWAYS);
  if (res == FR_OK) {
    sprintf(str,"File created successfully.\n");
    ST7789_WriteString(0,100,str,Font_11x18,WHITE,BLACK);
  } else {
    sprintf(str,"Failed to create file.Error:%d\n",res);
    ST7789_WriteString(0,100,str,Font_11x18,WHITE,BLACK);
    shutter_flag = 0;
    HAL_Delay(1000);
    return;
  }

  bmp.file_header.bfType = 0x4D42;
  bmp.file_header.bfSize = 54 + 272 * 204 * 2;
  bmp.file_header.bfReserved1 = 0;
  bmp.file_header.bfReserved2 = 0;
  bmp.file_header.bfOffBits = 54;

  bmp.info_header.biSize = 40;
  bmp.info_header.biWidth = 272;
  bmp.info_header.biHeight = -204;
  bmp.info_header.biPlanes = 1;
  bmp.info_header.biBitCount = 16;
  bmp.info_header.biCompression = 3;
  bmp.info_header.biSizeImage = 272 * 204 * 2;
  bmp.info_header.biXPelsPerMeter = 0;
  bmp.info_header.biYPelsPerMeter = 0;
  bmp.info_header.biClrImportant = 0;
  bmp.info_header.biClrUsed = 0;

  bmp.RGB565_mask[0].rgbBlue = 0;
  bmp.RGB565_mask[0].rgbGreen =0xF8;
  bmp.RGB565_mask[0].rgbRed = 0;
  bmp.RGB565_mask[0].rgbReserved = 0;

  bmp.RGB565_mask[1].rgbBlue = 0XE0;
  bmp.RGB565_mask[1].rgbGreen = 0X07;
  bmp.RGB565_mask[1].rgbRed = 0;
  bmp.RGB565_mask[1].rgbReserved = 0;

  bmp.RGB565_mask[2].rgbBlue = 0x1F;
  bmp.RGB565_mask[2].rgbGreen = 0;
  bmp.RGB565_mask[2].rgbRed = 0;
  bmp.RGB565_mask[2].rgbReserved = 0;

  res = f_write(file, &bmp, sizeof(bmp), &bytes_written);
  if (res == FR_OK) {
    sprintf(str,"File written successfully.");
    ST7789_WriteString(0,118,str, Font_11x18, WHITE, BLACK);
  } else {
    sprintf(str,"Failed to write file.Error:%d",res);
    ST7789_WriteString(0,118,str, Font_11x18, WHITE, BLACK);
    shutter_flag = 0;
    f_close(file);
    HAL_Delay(1000);
    return;
  }
}

void set_bmp_write_image(FIL *file, uint16_t *image)
{
  UINT bytes_written;  // 写入的字节数
  FRESULT res;
  res = f_write(file, image, 272 * 51 * 2, &bytes_written);
  if (res == FR_OK)
  {
    sprintf(str,"Write file successfully.");
    ST7789_WriteString(0,136,str, Font_11x18, WHITE, BLACK);
  } else{
    sprintf(str,"Failed to write file.Error:%d",res);
    ST7789_WriteString(0,136,str, Font_11x18, WHITE, BLACK);
    f_close(file);
    shutter_flag = 0;
    HAL_Delay(1000);
    return;
  }

}
