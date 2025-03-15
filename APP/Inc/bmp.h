/**
 * @file bmp.h
 * @author ZhangHan (ZhangHan8233@outlook.com)
 * @brief 用于存储bmp图片
 * @version 0.1
 * @date 2025-03-06
 *
 * @copyright Copyright (c) 2025
 *
 */

#ifndef BMP_H
#define BMP_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// bmp文件头结构体
typedef struct __attribute__((packed))
{
  uint16_t bfType;//文件类型
  uint32_t bfSize;//文件大小
  uint16_t bfReserved1;//保留，设置为0
  uint16_t bfReserved2;//保留，设置为0
  uint32_t bfOffBits;//数据偏移量
} bmp_file_header_t;

// bmp信息头结构体
typedef struct __attribute__((packed))
{
  uint32_t biSize;//结构大小
  uint32_t biWidth;//宽度
  uint32_t biHeight;//高度，该值还表示图像是否为倒向，正数代表图像为倒向
  uint16_t biPlanes;//平面数，一般为1
  uint16_t biBitCount;//颜色位数常用的值为，1（灰度图），4（16色图），8（256色图），24（真彩色图），32（真彩色图,增加ALPHA通道）
  uint32_t biCompression;//压缩类型
  uint32_t biSizeImage;//图像大小，字节为单位
  uint32_t biXPelsPerMeter;//水平分辨率，用像素/米表示，有符号整数
  uint32_t biYPelsPerMeter;//垂直分辨率，用像素/米表示，有符号整数
  uint32_t biClrUsed;//使用的彩色表中的颜色索引数，如果为0，则表示使用所有颜色
  uint32_t biClrImportant;//重要的颜色索引数，如果为0，则表示所有颜色都是重要的
} bmp_info_header_t;

// 调色盘，rgb565结构体
typedef struct __attribute__((packed))
{
  uint8_t rgbBlue;//蓝
  uint8_t rgbGreen;//绿
  uint8_t rgbRed;//红
  uint8_t rgbReserved;//保留
} rgb565_t;

typedef struct __attribute__((packed))
{
  bmp_file_header_t file_header;
  bmp_info_header_t info_header;
  rgb565_t RGB565_mask[4];
} bmp_all_header_t;

#endif
