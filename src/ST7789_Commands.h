#ifndef ST7789_COMMANDS_H_
#define ST7789_COMMANDS_H_

typedef enum{
	ST7789_SWRESET           = 0x01,
	ST7789_SLPOUT            = 0x11,
	ST7789_NORON             = 0x13,
	ST7789_INVON             = 0x21,
	ST7789_DISPON            = 0x29,
	ST7789_RAMWR             = 0x2C,
	ST7789_INVOFF            = 0x20,
	ST7789_DISPOFF           = 0x28,
}ST7789_OneByteCommand_et;

typedef enum{
	ST7789_COLMOD            = 0x3A,
	ST7789_MADCTL            = 0x36,
}ST7789_TwoByteCommand_et;

typedef enum{
	STUB_THREE
}ST7789_ThreeByteCommand_et;

typedef enum{
	STUB_FOUR
}ST7789_FourByteCommand_et;

typedef enum{
	ST7789_CASET              = 0x2A,
	ST7789_RASET              = 0x2B,
}ST7789_FiveByteCommand_et;

#endif
