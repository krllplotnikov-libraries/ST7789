#include "ST7789.h"
#include "ST7789_Commands.h"
#include "string.h"

#define ST7789_COMMAND_LEVEL 0
#define ST7789_DATA_LEVEL 1

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

static void ST7789_RAMWrite(ST7789_st* st7789, uint16_t* data, uint32_t len);
static void ST7789_RAMWriteOneColor(ST7789_st* st7789, uint16_t color, uint32_t len);

void ST7789_Init(ST7789_st* st7789, ST7789_Descriptor_st* descriptor){
	st7789->descriptor.selectDataCommand = descriptor->selectDataCommand;
	st7789->descriptor.spiChipSelect = descriptor->spiChipSelect;
	st7789->descriptor.spiTransmit = descriptor->spiTransmit;
	st7789->descriptor.height = descriptor->height;
	st7789->descriptor.width = descriptor->width;

	st7789->rotation = ST7789_ROTATION_VERTICAL;
	st7789->inversion = ST7789_INVERTED;

	st7789->xStart = 0;
	st7789->yStart = 0;

	uint8_t xStart0 = st7789->xStart >> 8;
	uint8_t xStart1 = st7789->xStart & 0xFF;
	uint8_t xEnd0 = (st7789->descriptor.width - 1) >> 8;
	uint8_t xEnd1 = (st7789->descriptor.width - 1) & 0xFF;

	uint8_t yStart0 = st7789->yStart >> 8;
	uint8_t yStart1 = st7789->yStart & 0xFF;
	uint8_t yEnd0 = (st7789->descriptor.height - 1) >> 8;
	uint8_t yEnd1 = (st7789->descriptor.height - 1) & 0xFF;

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
}

void ST7789_SetRotation(ST7789_st* st7789, ST7789_Rotation_et rotation){
	switch(rotation){
	case ST7789_ROTATION_VERTICAL:
		ST7789_SendTwoByteCommand(st7789, ST7789_MADCTL, ST7789_MADCTL_RGB);
		break;
	case ST7789_ROTATION_HORISONTAL:
		ST7789_SendTwoByteCommand(st7789, ST7789_MADCTL, ST7789_MADCTL_MX | ST7789_MADCTL_MV | ST7789_MADCTL_RGB);
		break;
	}
}

void ST7789_SetInversion(ST7789_st* st7789, ST7789_Inversion_et inversion){

	switch(inversion){
	case ST7789_INVERTED:
		ST7789_SendOneByteCommand(st7789, ST7789_INVON);
		break;
	case ST7789_NOT_INVERTED:
		ST7789_SendOneByteCommand(st7789, ST7789_INVOFF);
		break;
	}
}

void ST7789_SetOffsetX(ST7789_st* st7789, uint8_t offset){
	st7789->xStart = offset;
}

void ST7789_SetOffsetY(ST7789_st* st7789, uint8_t offset){
	st7789->yStart = offset;
}

void ST7789_SetPixel(ST7789_st* st7789, uint16_t x, uint16_t y, uint16_t color){
	if ((x < 0) ||(x >= st7789->descriptor.width) || (y < 0) || (y >= st7789->descriptor.height)){
		return;
	}

	uint8_t buff[2];
	buff[0] = color >> 8;
	buff[1] = color & 0xFF;

	ST7789_SetWindow(st7789, x, y, x, y);

	ST7789_SendCommand(st7789, ST7789_RAMWR);
	ST7789_SendData(st7789, buff, 2);
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
	ST7789_RAMWriteOneColor(st7789, color, (h * w));
}



static void ST7789_SetWindow(ST7789_st* st7789, uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1){
	ST7789_ColumnSet(st7789, x0, x1);
	ST7789_RowSet(st7789, y0, y1);
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

static void ST7789_RAMWriteOneColor(ST7789_st* st7789, uint16_t color, uint32_t len){
	uint8_t buff[2];
	buff[0] = color >> 8;
	buff[1] = color & 0xFF;

	ST7789_SendCommand(st7789, ST7789_RAMWR);
	while (len--){
		ST7789_SendData(st7789, buff, 2);
	}
}

static void ST7789_RAMWrite(ST7789_st* st7789, uint16_t* data, uint32_t len){
	ST7789_SendCommand(st7789, ST7789_RAMWR);
	ST7789_SendData(st7789, data, len);
}

static void ST7789_SendCommand(ST7789_st* st7789, uint8_t command){
	st7789->descriptor.spiChipSelect(st7789, 0);

	st7789->descriptor.selectDataCommand(st7789, ST7789_COMMAND_LEVEL);
	st7789->descriptor.spiTransmit(st7789, &command, 1);

	st7789->descriptor.spiChipSelect(st7789, 1);
}

static void ST7789_SendData(ST7789_st* st7789, uint8_t* data, size_t size){
	st7789->descriptor.spiChipSelect(st7789, 0);

	st7789->descriptor.selectDataCommand(st7789, ST7789_DATA_LEVEL);
	st7789->descriptor.spiTransmit(st7789, data, size);

	st7789->descriptor.spiChipSelect(st7789, 1);
}

static void ST7789_SendOneByteCommand(ST7789_st* st7789, ST7789_OneByteCommand_et command){
	st7789->descriptor.spiChipSelect(st7789, 0);

	st7789->descriptor.selectDataCommand(st7789, ST7789_COMMAND_LEVEL);
	st7789->descriptor.spiTransmit(st7789, &command, 1);

	st7789->descriptor.spiChipSelect(st7789, 1);
}

static void ST7789_SendTwoByteCommand(ST7789_st* st7789, ST7789_TwoByteCommand_et command, uint8_t value){
	st7789->descriptor.spiChipSelect(st7789, 0);

	st7789->descriptor.selectDataCommand(st7789, ST7789_COMMAND_LEVEL);
	st7789->descriptor.spiTransmit(st7789, &command, 1);

	st7789->descriptor.selectDataCommand(st7789, ST7789_DATA_LEVEL);
	st7789->descriptor.spiTransmit(st7789, &value, 1);

	st7789->descriptor.spiChipSelect(st7789, 1);
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

	st7789->descriptor.spiChipSelect(st7789, 0);

	st7789->descriptor.selectDataCommand(st7789, ST7789_COMMAND_LEVEL);
	st7789->descriptor.spiTransmit(st7789, &command, 1);
	st7789->descriptor.selectDataCommand(st7789, ST7789_DATA_LEVEL);
	st7789->descriptor.spiTransmit(st7789, buf, 3);

	st7789->descriptor.spiChipSelect(st7789, 1);
}

static void ST7789_SendFiveByteCommand(ST7789_st* st7789, ST7789_FourByteCommand_et command, uint8_t value1, uint8_t value2, uint8_t value3, uint8_t value4){
	uint8_t buf[4] = {value1, value2, value3, value4};

	st7789->descriptor.spiChipSelect(st7789, 0);

	st7789->descriptor.selectDataCommand(st7789, ST7789_COMMAND_LEVEL);
	st7789->descriptor.spiTransmit(st7789, &command, 1);
	st7789->descriptor.selectDataCommand(st7789, ST7789_DATA_LEVEL);
	st7789->descriptor.spiTransmit(st7789, buf, 4);

	st7789->descriptor.spiChipSelect(st7789, 1);
}
