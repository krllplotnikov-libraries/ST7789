#include "ST7789.h"
#include "ST7789_Commands.h"
#include "string.h"

#define ST7789_COMMAND_LEVEL 0
#define ST7789_DATA_LEVEL 1



//-----Private Function Definition-----
static void ST7789_SendCommand(ST7789_st* st7789, uint8_t command);
static void ST7789_SendDataArray(ST7789_st* st7789, uint8_t* data, size_t size);
static void ST7789_SendDataByte(ST7789_st* st7789, uint8_t data);

static void ST7789_ColumnSet(ST7789_st* st7789, uint16_t columnStart, uint16_t columnEnd);
static void ST7789_RowSet(ST7789_st* st7789, uint16_t rowStart, uint16_t rowEnd);
static void ST7789_SetWindow(ST7789_st* st7789, uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);

static void ST7789_RAMWrite(ST7789_st* st7789, uint8_t* data, uint32_t len);



//-----Public Function Definition-----
//-----Initialization-----
void ST7789_Init(ST7789_st* st7789, ST7789_Descriptor_st* descriptor){
	st7789->descriptor.selectDataCommand = descriptor->selectDataCommand;
	st7789->descriptor.spiChipSelect = descriptor->spiChipSelect;
	st7789->descriptor.spiTransmit = descriptor->spiTransmit;
	st7789->descriptor.height = descriptor->height;
	st7789->descriptor.width = descriptor->width;

	st7789->rotation = ST7789_ROTATION_VERTICAL;
	st7789->inversion = ST7789_INVERTED;

	st7789->xOffset = 0;
	st7789->yOffset = 0;

	uint8_t xStart0 = st7789->xOffset >> 8;
	uint8_t xStart1 = st7789->xOffset & 0xFF;
	uint8_t xEnd0 = (st7789->descriptor.width - 1) >> 8;
	uint8_t xEnd1 = (st7789->descriptor.width - 1) & 0xFF;

	uint8_t yStart0 = st7789->yOffset >> 8;
	uint8_t yStart1 = st7789->yOffset & 0xFF;
	uint8_t yEnd0 = (st7789->descriptor.height - 1) >> 8;
	uint8_t yEnd1 = (st7789->descriptor.height - 1) & 0xFF;

	ST7789_SendCommand(st7789, ST7789_SWRESET);
	ST7789_SendCommand(st7789, ST7789_SLPOUT);

	ST7789_SendCommand(st7789, ST7789_COLMOD);
	ST7789_SendDataByte(st7789, ST7789_COLORMODE_65K | ST7789_COLORMODE_16BIT);

	ST7789_SendCommand(st7789, ST7789_MADCTL);
	ST7789_SendDataByte(st7789, ST7789_MADCTL_RGB);

	ST7789_SendCommand(st7789, ST7789_CASET);
	ST7789_SendDataByte(st7789, xStart0);
	ST7789_SendDataByte(st7789, xStart1);
	ST7789_SendDataByte(st7789, xEnd0);
	ST7789_SendDataByte(st7789, xEnd1);

	ST7789_SendCommand(st7789, ST7789_RASET);
	ST7789_SendDataByte(st7789, yStart0);
	ST7789_SendDataByte(st7789, yStart1);
	ST7789_SendDataByte(st7789, yEnd0);
	ST7789_SendDataByte(st7789, yEnd1);

	ST7789_SendCommand(st7789, ST7789_INVON);
	ST7789_SendCommand(st7789, ST7789_NORON);
	ST7789_SendCommand(st7789, ST7789_DISPON);
}

void ST7789_SetRotation(ST7789_st* st7789, ST7789_Rotation_et rotation){
	st7789->rotation = rotation;
	switch(rotation){
	case ST7789_ROTATION_VERTICAL:
		ST7789_SendCommand(st7789, ST7789_MADCTL);
		ST7789_SendDataByte(st7789, ST7789_MADCTL_RGB);
		break;
	case ST7789_ROTATION_HORISONTAL:
		ST7789_SendCommand(st7789, ST7789_MADCTL);
		ST7789_SendDataByte(st7789, ST7789_MADCTL_MX | ST7789_MADCTL_MV | ST7789_MADCTL_RGB);
		break;
	}
}

void ST7789_SetInversion(ST7789_st* st7789, ST7789_Inversion_et inversion){
	st7789->inversion = inversion;
	switch(inversion){
	case ST7789_INVERTED:
		ST7789_SendCommand(st7789, ST7789_INVON);
		break;
	case ST7789_NOT_INVERTED:
		ST7789_SendCommand(st7789, ST7789_INVOFF);
		break;
	}
}

void ST7789_SetOffsetX(ST7789_st* st7789, uint8_t offset){
	st7789->xOffset = offset;
}

void ST7789_SetOffsetY(ST7789_st* st7789, uint8_t offset){
	st7789->yOffset = offset;
}



//-----Draw-----
void ST7789_SetPixel(ST7789_st* st7789, uint16_t x, uint16_t y, uint16_t color){
	if ((x < 0) ||(x >= st7789->descriptor.width) || (y < 0) || (y >= st7789->descriptor.height)){
		return;
	}

	color = ((color & 0xFF) << 8) | (color >> 8);

	ST7789_SetWindow(st7789, x, y, x, y);
	ST7789_RAMWrite(st7789, (uint8_t*)&color, 2);
}

void ST7789_SetPixelArray(ST7789_st* st7789, uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color){
	if ((x >= st7789->descriptor.width) || (y >= st7789->descriptor.height)){
		return;
	}
	if ((x + w) > st7789->descriptor.width){
		w = st7789->descriptor.width - x;
	}
	if ((y + h) > st7789->descriptor.height){
		h = st7789->descriptor.height - y;
	}

	if(h * w <= DISPLAY_BUFFER_SIZE){
		for(uint16_t i = 0; i < h; i++){
			for(uint16_t j = 0; j < w; j++){
				st7789->tempBuffer[j + i * w] = ((color & 0xFF) << 8) | (color >> 8);
			}
		}
		ST7789_SetWindow(st7789, x, y, x + w - 1, y + h - 1);
		ST7789_RAMWrite(st7789, (uint8_t*)st7789->tempBuffer, h * w * 2);
	}
	else{
		for(uint16_t i = 0; i < h; i++){
			for(uint16_t j = 0; j < w; j++){
				st7789->tempBuffer[j] = ((color & 0xFF) << 8) | (color >> 8);
			}
			ST7789_SetWindow(st7789, x, y + i, x + w - 1, y + i);
			ST7789_RAMWrite(st7789, (uint8_t*)st7789->tempBuffer, w * 2);
		}
	}
}

void ST7789_FillScreen(ST7789_st* st7789, uint16_t color){
	ST7789_SetPixelArray(st7789, 0, 0, st7789->descriptor.width, st7789->descriptor.height, color);
}


//-----Private Function Implementation-----
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

	columnStart += st7789->xOffset;
	columnEnd += st7789->xOffset;

	uint8_t arr[4] = {0};
	arr[0] = columnStart >> 8;
	arr[1] = columnStart & 0xFF;
	arr[2] = columnEnd >> 8;
	arr[3] = columnEnd & 0xFF;

	ST7789_SendCommand(st7789, ST7789_CASET);
	ST7789_SendDataArray(st7789, arr, 4);
}

static void ST7789_RowSet(ST7789_st* st7789, uint16_t rowStart, uint16_t rowEnd){
	if (rowStart > rowEnd){
		return;
	}
	if (rowEnd > st7789->descriptor.height){
		return;
	}

	rowStart += st7789->yOffset;
	rowEnd += st7789->yOffset;

	uint8_t arr[4] = {0};
	arr[0] = rowStart >> 8;
	arr[1] = rowStart & 0xFF;
	arr[2] = rowEnd >> 8;
	arr[3] = rowEnd & 0xFF;

	ST7789_SendCommand(st7789, ST7789_RASET);
	ST7789_SendDataArray(st7789, arr, 4);
}

static void ST7789_RAMWrite(ST7789_st* st7789, uint8_t* data, uint32_t len){
	ST7789_SendCommand(st7789, ST7789_RAMWR);
	ST7789_SendDataArray(st7789, data, len);
}



//-----Send-----
static void ST7789_SendCommand(ST7789_st* st7789, uint8_t command){
	st7789->descriptor.spiChipSelect(st7789, 0);

	st7789->descriptor.selectDataCommand(st7789, ST7789_COMMAND_LEVEL);
	st7789->descriptor.spiTransmit(st7789, &command, 1);

	st7789->descriptor.spiChipSelect(st7789, 1);
}

static void ST7789_SendDataArray(ST7789_st* st7789, uint8_t* data, size_t size){
	st7789->descriptor.spiChipSelect(st7789, 0);

	st7789->descriptor.selectDataCommand(st7789, ST7789_DATA_LEVEL);
	st7789->descriptor.spiTransmit(st7789, data, size);

	st7789->descriptor.spiChipSelect(st7789, 1);
}

static void ST7789_SendDataByte(ST7789_st* st7789, uint8_t data){
	st7789->descriptor.spiChipSelect(st7789, 0);

	st7789->descriptor.selectDataCommand(st7789, ST7789_DATA_LEVEL);
	st7789->descriptor.spiTransmit(st7789, &data, 1);

	st7789->descriptor.spiChipSelect(st7789, 1);
}
