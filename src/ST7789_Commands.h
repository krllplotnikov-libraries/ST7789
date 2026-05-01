#ifndef ST7789_COMMANDS_H_
#define ST7789_COMMANDS_H_

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
