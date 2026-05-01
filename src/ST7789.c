#include "ST7789.h"
#include "ST7789_Commands.h"
#include "string.h"

#define ST7789_COMMAND_LEVEL 0
#define ST7789_DATA_LEVEL 1

#define ST7789_COLORMODE_65K    	0x50
#define ST7789_COLORMODE_262K   	0x60
#define ST7789_COLORMODE_12BIT  	0x03
#define ST7789_COLORMODE_16BIT  	0x05
#define ST7789_COLORMODE_18BIT  	0x06
#define ST7789_COLORMODE_16M    	0x07

#define ST7789_MADCTL_MY  				0x80
#define ST7789_MADCTL_MX  				0x40
#define ST7789_MADCTL_MV  				0x20
#define ST7789_MADCTL_ML  				0x10
#define ST7789_MADCTL_RGB 				0x00
#define ST7789_MADCTL_BGR 				0x08
#define ST7789_MADCTL_MH  				0x04

static void ST7789_SendCommand(ST7789_st* st7789, uint8_t command);
static void ST7789_SendData(ST7789_st* st7789, uint8_t* data, size_t size);

static void ST7789_SendOneByteCommand(ST7789_st* st7789, ST7789_OneByteCommand_et command);
static void ST7789_SendTwoByteCommand(ST7789_st* st7789, ST7789_TwoByteCommand_et command, uint8_t value);
static void ST7789_SendThreeByteCommand(ST7789_st* st7789, ST7789_ThreeByteCommand_et command, uint8_t value1, uint8_t value2);
static void ST7789_SendFourByteCommand(ST7789_st* st7789, ST7789_FourByteCommand_et command, uint8_t value1, uint8_t value2, uint8_t value3);
static void ST7789_SendFiveByteCommand(ST7789_st* st7789, ST7789_FourByteCommand_et command, uint8_t value1, uint8_t value2, uint8_t value3, uint8_t value4);

static void ST7789_ColumnSet(ST7789_st* st7789, uint16_t columnStart, uint16_t columnEnd);
static void ST7789_RowSet(ST7789_st* st7789, uint16_t rowStart, uint16_t rowEnd);
static void ST7789_SetWindow(ST7789_st* st7789, uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);

static void ST7789_RamWrite(ST7789_st* st7789, uint16_t *pBuff, uint32_t len);

void ST7789_Init(ST7789_st* st7789, ST7789_Descriptor_st* descriptor){
	st7789->descriptor.selectDataCommand = descriptor->selectDataCommand;
	st7789->descriptor.spiChipSelect = descriptor->spiChipSelect;
	st7789->descriptor.spiTransmit = descriptor->spiTransmit;
	st7789->descriptor.height = descriptor->height;
	st7789->descriptor.width = descriptor->width;
	st7789->descriptor.dimension = descriptor->dimension;

	if(st7789->descriptor.dimension == ST7789_240X280){
		st7789->xStart = 0;
		st7789->yStart = 20;
		st7789->rotation = ST7789_MADCTL_RGB;
		st7789->inversion = ST7789_INVON;
	}

	uint8_t xStart0 = st7789->xStart >> 8;
	uint8_t xStart1 = st7789->xStart & 0xFF;
	uint8_t xEnd0 = (st7789->descriptor.width - 1) >> 8;
	uint8_t xEnd1 = (st7789->descriptor.width - 1) & 0xFF;

	uint8_t yStart0 = st7789->yStart >> 8;
	uint8_t yStart1 = st7789->yStart & 0xFF;
	uint8_t yEnd0 = (st7789->descriptor.height - 1) >> 8;
	uint8_t yEnd1 = (st7789->descriptor.height - 1) & 0xFF;

	st7789->descriptor.spiChipSelect(st7789, 0);

	ST7789_SendOneByteCommand(st7789, ST7789_SWRESET);
	ST7789_SendOneByteCommand(st7789, ST7789_SLPOUT);
	ST7789_SendTwoByteCommand(st7789, ST7789_COLMOD, ST7789_COLORMODE_65K | ST7789_COLORMODE_16BIT);
	ST7789_SendTwoByteCommand(st7789, ST7789_MADCTL, ST7789_MADCTL_RGB);
	ST7789_SendFiveByteCommand(st7789, ST7789_CASET, xStart0, xStart1,  xEnd0,  xEnd1);
	ST7789_SendFiveByteCommand(st7789, ST7789_RASET, yStart0, yStart1,  yEnd0,  yEnd1);
	ST7789_SendOneByteCommand(st7789, ST7789_INVON);
	ST7789_SendOneByteCommand(st7789, ST7789_NORON);
	ST7789_SendOneByteCommand(st7789, ST7789_DISPON);

	ST7789_FillScreen(st7789, 0);

	st7789->descriptor.spiChipSelect(st7789, 1);
}

void ST7789_SetPixel(ST7789_st* st7789, uint16_t x, uint16_t y, uint16_t color){
	if ((x < 0) ||(x >= st7789->descriptor.width) || (y < 0) || (y >= st7789->descriptor.height)){
		return;
	}

	ST7789_SetWindow(st7789, x, y, x, y);
	ST7789_RamWrite(st7789, &color, 1);
}

void ST7789_FillScreen(ST7789_st* st7789, uint16_t color){
	ST7789_FillRect(st7789, 0, 0, st7789->descriptor.width, st7789->descriptor.height, color);
}

void ST7789_FillRect(ST7789_st* st7789, int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color){
	if ((x >= st7789->descriptor.width) || (y >= st7789->descriptor.height)){
		return;
	}
	if ((x + w) > st7789->descriptor.width){
		w = st7789->descriptor.width - x;
	}
	if ((y + h) > st7789->descriptor.height){
		h = st7789->descriptor.height - y;
	}

	ST7789_SetWindow(st7789, x, y, x + w - 1, y + h - 1);
	ST7789_RamWrite(st7789, &color, (h * w));
}



static void ST7789_SetWindow(ST7789_st* st7789, uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1){
	st7789->descriptor.spiChipSelect(st7789, 0);

	ST7789_ColumnSet(st7789, x0, x1);
	ST7789_RowSet(st7789, y0, y1);

	ST7789_SendOneByteCommand(st7789, ST7789_RAMWR);

	st7789->descriptor.spiChipSelect(st7789, 1);
}

static void ST7789_ColumnSet(ST7789_st* st7789, uint16_t columnStart, uint16_t columnEnd){
  if (columnStart > columnEnd){
    return;
  }
  if (columnStart > st7789->descriptor.width){
    return;
  }

  columnStart += st7789->xStart;
  columnEnd += st7789->xStart;

  ST7789_SendFiveByteCommand(st7789, ST7789_CASET, columnStart >> 8, columnStart & 0xFF, columnEnd >> 8, columnEnd & 0xFF);
}

static void ST7789_RowSet(ST7789_st* st7789, uint16_t rowStart, uint16_t rowEnd){
  if (rowStart > rowEnd){
    return;
  }
  if (rowEnd > st7789->descriptor.height){
    return;
  }

  rowStart += st7789->yStart;
  rowEnd += st7789->yStart;

  ST7789_SendFiveByteCommand(st7789, ST7789_RASET, rowStart >> 8, rowStart & 0xFF, rowEnd >> 8, rowEnd & 0xFF);
}

static void ST7789_RamWrite(ST7789_st* st7789, uint16_t *pBuff, uint32_t len){

	st7789->descriptor.spiChipSelect(st7789, 0);
	st7789->descriptor.selectDataCommand(st7789, ST7789_DATA_LEVEL);

	uint8_t buff[2];
	buff[0] = *pBuff >> 8;
	buff[1] = *pBuff & 0xFF;

	while (len--){
		st7789->descriptor.spiTransmit(st7789, buff, 2);
	}

	st7789->descriptor.spiChipSelect(st7789, 1);
}

static void ST7789_SendCommand(ST7789_st* st7789, uint8_t command){
	st7789->descriptor.selectDataCommand(st7789, ST7789_COMMAND_LEVEL);
	st7789->descriptor.spiTransmit(st7789, &command, 1);
}

static void ST7789_SendData(ST7789_st* st7789, uint8_t* data, size_t size){
	st7789->descriptor.selectDataCommand(st7789, ST7789_DATA_LEVEL);
	st7789->descriptor.spiTransmit(st7789, data, size);
}

static void ST7789_SendOneByteCommand(ST7789_st* st7789, ST7789_OneByteCommand_et command){
	st7789->descriptor.selectDataCommand(st7789, ST7789_COMMAND_LEVEL);
	st7789->descriptor.spiTransmit(st7789, &command, 1);
}

static void ST7789_SendTwoByteCommand(ST7789_st* st7789, ST7789_TwoByteCommand_et command, uint8_t value){
	st7789->descriptor.selectDataCommand(st7789, ST7789_COMMAND_LEVEL);
	st7789->descriptor.spiTransmit(st7789, &command, 1);

	st7789->descriptor.selectDataCommand(st7789, ST7789_DATA_LEVEL);
	st7789->descriptor.spiTransmit(st7789, &value, 1);
}

static void ST7789_SendThreeByteCommand(ST7789_st* st7789, ST7789_ThreeByteCommand_et command, uint8_t value1, uint8_t value2){
	uint8_t buf[3] = {value1, value2};

	st7789->descriptor.selectDataCommand(st7789, ST7789_COMMAND_LEVEL);
	st7789->descriptor.spiTransmit(st7789, &command, 1);
	st7789->descriptor.selectDataCommand(st7789, ST7789_DATA_LEVEL);
	st7789->descriptor.spiTransmit(st7789, buf, 3);
}

static void ST7789_SendFourByteCommand(ST7789_st* st7789, ST7789_FourByteCommand_et command, uint8_t value1, uint8_t value2, uint8_t value3){
	uint8_t buf[3] = {value1, value2, value3};

	st7789->descriptor.selectDataCommand(st7789, ST7789_COMMAND_LEVEL);
	st7789->descriptor.spiTransmit(st7789, &command, 1);
	st7789->descriptor.selectDataCommand(st7789, ST7789_DATA_LEVEL);
	st7789->descriptor.spiTransmit(st7789, buf, 3);
}

static void ST7789_SendFiveByteCommand(ST7789_st* st7789, ST7789_FourByteCommand_et command, uint8_t value1, uint8_t value2, uint8_t value3, uint8_t value4){
	uint8_t buf[4] = {value1, value2, value3, value4};

	st7789->descriptor.selectDataCommand(st7789, ST7789_COMMAND_LEVEL);
	st7789->descriptor.spiTransmit(st7789, &command, 1);
	st7789->descriptor.selectDataCommand(st7789, ST7789_DATA_LEVEL);
	st7789->descriptor.spiTransmit(st7789, buf, 4);
}
