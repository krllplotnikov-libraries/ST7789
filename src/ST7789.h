#ifndef ST7789_H_
#define ST7789_H_

#include "stdio.h"
#include "stdint.h"

#define DISPLAY_BUFFER_SIZE 320

#define RGB565(r, g, b) (((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b>> 3))

typedef enum ST7789_Rotation ST7789_Rotation_et;
typedef enum ST7789_Inversion ST7789_Inversion_et;

typedef struct ST7789_Descriptor ST7789_Descriptor_st;
typedef struct ST7789            ST7789_st;

typedef void (*ST7789_SPI_Transmit)      (ST7789_st* st7789, uint8_t* data, size_t size);
typedef void (*ST7789_SPI_ChipSelect)    (ST7789_st* st7789, uint8_t level);
typedef void (*ST7789_SelectDataCommand) (ST7789_st* st7789, uint8_t level);

enum ST7789_Rotation{
	ST7789_ROTATION_VERTICAL,
	ST7789_ROTATION_HORISONTAL,
	ST7789_ROTATION_VERTICAL_180,
};

enum ST7789_Inversion{
	ST7789_INVERTED,
	ST7789_NOT_INVERTED,
};

struct ST7789_Descriptor{
	ST7789_SPI_Transmit         spiTransmit;
	ST7789_SPI_ChipSelect       spiChipSelect;
	ST7789_SelectDataCommand    selectDataCommand;
	uint16_t                    height;
	uint16_t                    width;
};

struct ST7789{
	ST7789_Descriptor_st descriptor;
	uint8_t xOffset;
	uint8_t yOffset;
	ST7789_Rotation_et rotation;
	ST7789_Inversion_et inversion;

	uint16_t tempBuffer[DISPLAY_BUFFER_SIZE];
};

#ifdef __cplusplus
extern "C" {
#endif

void ST7789_Init(ST7789_st* st7789, ST7789_Descriptor_st* descriptor);
void ST7789_SetRotation(ST7789_st* st7789, ST7789_Rotation_et rotation);
void ST7789_SetInversion(ST7789_st* st7789, ST7789_Inversion_et inversion);
void ST7789_SetOffsetX(ST7789_st* st7789, uint8_t offset);
void ST7789_SetOffsetY(ST7789_st* st7789, uint8_t offset);

void ST7789_SetPixel(ST7789_st* st7789, uint16_t x, uint16_t y, uint16_t color);
void ST7789_SetPixelArray(ST7789_st* st7789, uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t* color, uint8_t singleFlag);

void ST7789_FillScreen(ST7789_st* st7789, uint16_t color);


#ifdef __cplusplus
}
#endif

#endif
