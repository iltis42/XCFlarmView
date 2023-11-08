#include "ili9341.h"
#include "frame_buffer.h"
#include <esp_log.h>


//
// Timings
//

#define ILI9341_RESX_PULSE_MS 10
#define ILI9341_RESX_CANCEL_MS 120

#define ILI9341_SOFTWARE_RESET_DELAY_MS 5
#define ILI9341_SOFTWARE_RESET_SLEEP_MODE_DELAY_MS 120

#define ILI9341_SLEEP_IN_DELAY_MS 5
#define ILI9341_SLEEP_OUT_DELAY_MS 120

//
// Commands
//

#define ILI9341_NO_OPERATION 0x00

#define ILI9341_SOFTWARE_RESET 0x01

#define ILI9341_READ_DISPLAY_ID 0x04

#define ILI9341_READ_DISPLAY_STATUS 0x09

#define ILI9341_READ_DISPLAY_POWER_MODE 0x0a

#define ILI9341_READ_DISPLAY_MADCTL 0x0b

#define ILI9341_READ_DISPLAY_PIXEL_FORMAT 0x0c

#define ILI9341_READ_DISPLAY_IMAGE_MODE 0x0d

#define ILI9341_READ_DISPLAY_SIGNAL_MODE 0x0e

#define ILI9341_READ_DISPLAY_SELF_DIAGNOSTIC_RESULT 0x0f

#define ILI9341_SLEEP_IN 0x10

#define ILI9341_SLEEP_OUT 0x11

#define ILI9341_PARTIAL_DISPLAY_MODE_ON 0x12

#define ILI9341_NORMAL_DISPLAY_MODE_ON 0x13

#define ILI9341_DISPLAY_INVERSION_OFF 0x20

#define ILI9341_DISPLAY_INVERSION_ON 0x21

#define ILI9341_GAMMA_SET 0x26

#define ILI9341_DISPLAY_OFF 0x28

#define ILI9341_DISPLAY_ON 0x29

#define ILI9341_COLUMN_ADDRESS_SET 0x2a

#define ILI9341_ROW_ADDRESS_SET 0x2b

#define ILI9341_MEMORY_WRITE 0x2c

#define ILI9341_MEMORY_READ 0x2e

#define ILI9341_COLOR_SET 0x2d

#define ILI9341_PARTIAL_AREA 0x30

#define ILI9341_VERTICAL_SCROLLING_DEFINITION 0x33

#define ILI9341_TEARING_EFFECT_LINE_OFF 0x34

#define ILI9341_TEARING_EFFECT_LINE_ON 0x35

#define ILI9341_MEMORY_DATA_ACCESS_CONTROL 0x36

#define ILI9341_MEMORY_DATA_ACCESS_CONTROL_PAGE_ADDRESS_TOP_TO_BOTTOM (0<<7)
#define ILI9341_MEMORY_DATA_ACCESS_CONTROL_PAGE_ADDRESS_BOTTOM_TO_TOP (1<<7)
#define ILI9341_MEMORY_DATA_ACCESS_CONTROL_COLUMN_ADDRESS_LEFT_TO_RIGHT (0<<6)
#define ILI9341_MEMORY_DATA_ACCESS_CONTROL_COLUMN_ADDRESS_RIGHT_TO_LEFT (1<<6)
#define ILI9341_MEMORY_DATA_ACCESS_CONTROL_PAGE_COLUMN_ORDER_NORMAL (0<<5)
#define ILI9341_MEMORY_DATA_ACCESS_CONTROL_PAGE_COLUMN_ORDER_REVERSE (1<<5)
#define ILI9341_MEMORY_DATA_ACCESS_CONTROL_LINE_ADDRESS_ORDER_LCD_REFRESH_TOP_TO_BOTTOM (0<<4)
#define ILI9341_MEMORY_DATA_ACCESS_CONTROL_LINE_ADDRESS_ORDER_LCD_REFRESH_BOTTOM_TO_TOP (1<<4)
#define ILI9341_MEMORY_DATA_ACCESS_CONTROL_RGB (0<<3)
#define ILI9341_MEMORY_DATA_ACCESS_CONTROL_BGR (1<<3)
#define ILI9341_MEMORY_DATA_ACCESS_CONTROL_DISPLAY_DATA_LATCH_DATA_ORDER_LCD_REFRESH_LEFT_TO_RIGHT (0<<0)
#define ILI9341_MEMORY_DATA_ACCESS_CONTROL_DISPLAY_DATA_LATCH_DATA_ORDER_LCD_REFRESH_RIGHT_TO_LEFT (1<<0)

#define ILI9341_VERTICAL_SCROLL_START_ADDRESS_OF_RAM 0x37

#define ILI9341_IDLE_MODE_OFF 0x38

#define ILI9341_IDLE_MODE_ON 0x39

#define ILI9341_INTERFACE_PIXEL_FORMAT 0x3a

#define ILI9341_INTERFACE_PIXEL_FORMAT_RGB_INTERFACE_BIT 4
#define ILI9341_INTERFACE_PIXEL_FORMAT_RGB_INTERFACE_65K (5<<ILI9341_INTERFACE_PIXEL_FORMAT_RGB_INTERFACE_BIT)
#define ILI9341_INTERFACE_PIXEL_FORMAT_RGB_INTERFACE_262K (6<<ILI9341_INTERFACE_PIXEL_FORMAT_RGB_INTERFACE_BIT)
#define ILI9341_INTERFACE_PIXEL_FORMAT_COLOR_BIT 0
#define ILI9341_INTERFACE_PIXEL_FORMAT_COLOR_12BIT (3<<ILI9341_INTERFACE_PIXEL_FORMAT_COLOR_BIT)
#define ILI9341_INTERFACE_PIXEL_FORMAT_COLOR_16BIT (5<<ILI9341_INTERFACE_PIXEL_FORMAT_COLOR_BIT)
#define ILI9341_INTERFACE_PIXEL_FORMAT_COLOR_18BIT (6<<ILI9341_INTERFACE_PIXEL_FORMAT_COLOR_BIT)
#define ILI9341_INTERFACE_PIXEL_FORMAT_COLOR_24BIT_TRUNCATED (7<<ILI9341_INTERFACE_PIXEL_FORMAT_COLOR_BIT)

#define ILI9341_WRITE_MEMORY_CONTINUE 0x3c

#define ILI9341_READ_MEMORY_CONTINUE 0x3e

#define ILI9341_SET_TEAR_SCANLINE 0x44

#define ILI9341_GET_SCANLINE 0x45

#define ILI9341_WRITE_DISPLAY_BRIGHTNESS 0x51

#define ILI9341_READ_DISPLAY_BRIGHTNESS_VALUE 0x52

#define ILI9341_WRITE_CTRL_DISPLAY 0x53

#define ILI9341_WRITE_CTRL_DISPLAY_BRIGHTNESS_CONTROL_BLOCK_BIT 5
#define ILI9341_WRITE_CTRL_DISPLAY_BRIGHTNESS_CONTROL_BLOCK_OFF (0<<ILI9341_WRITE_CTRL_DISPLAY_BRIGHTNESS_CONTROL_BLOCK_BIT)
#define ILI9341_WRITE_CTRL_DISPLAY_BRIGHTNESS_CONTROL_BLOCK_ON (1<<ILI9341_WRITE_CTRL_DISPLAY_BRIGHTNESS_CONTROL_BLOCK_BIT)
#define ILI9341_WRITE_CTRL_DISPLAY_DIMMING_BIT 3
#define ILI9341_WRITE_CTRL_DISPLAY_DIMMING_OFF (0<<ILI9341_WRITE_CTRL_DISPLAY_DIMMING_BIT)
#define ILI9341_WRITE_CTRL_DISPLAY_DIMMING_ON (1<<ILI9341_WRITE_CTRL_DISPLAY_DIMMING_BIT)
#define ILI9341_WRITE_CTRL_DISPLAY_BACKLIGHT_CONTROL_BIT 2
#define ILI9341_WRITE_CTRL_DISPLAY_BACKLIGHT_CONTROL_OFF (0<<ILI9341_WRITE_CTRL_DISPLAY_BACKLIGHT_CONTROL_BIT)
#define ILI9341_WRITE_CTRL_DISPLAY_BACKLIGHT_CONTROL_ON (1<<ILI9341_WRITE_CTRL_DISPLAY_BACKLIGHT_CONTROL_BIT)

#define ILI9341_READ_CTRL_VALUE_DISPLAY 0x54

#define ILI9341_WRITE_CONTENT_ADAPTIVE_BRIGHTNESS_CONTROL_AND_COLOR_ENHANCEMENT 0x55

#define ILI9341_READ_CONTENT_ADAPTIVE_BRIGHTNESS_CONTROL_ 0x56

#define ILI9341_WRITE_CABC_MINIMUM_BRIGHTNESS 0x5e

#define ILI9341_READ_CABC_MINIMUM_BRIGHTNESS 0x5f

#define ILI9341_READ_AUTOMATIC_BRIGHTNESS_CONTROL_SELF_DIAGNOSTIC_RESULT 0x68

#define ILI9341_READ_ID1 0xda

#define ILI9341_READ_ID2 0xdb

#define ILI9341_READ_ID3 0xdc

// ILI9341_WRITE_DISPLAY_BRIGHTNESS

// ILI9341_WRITE_CTRL_DISPLAY

// ILI9341_WRITE_CONTENT_ADAPTIVE_BRIGHTNESS_CONTROL_AND_COLOR_ENHANCEMENT
// ILI9341_WRITE_CABC_MINIMUM_BRIGHTNESS

// ILI9341_PARTIAL_DISPLAY_MODE_ON
// ILI9341_NORMAL_DISPLAY_MODE_ON
// ILI9341_PARTIAL_AREA

// ILI9341_VERTICAL_SCROLLING_DEFINITION
// ILI9341_VERTICAL_SCROLL_START_ADDRESS_OF_RAM

// ILI9341_TEARING_EFFECT_LINE_OFF
// ILI9341_TEARING_EFFECT_LINE_ON
// ILI9341_SET_TEAR_SCANLINE

// ILI9341_GAMMA_SET





//
// Helpers
//

static void set_interface_pixel_format(eglib_t *eglib) {
	ili9341_config_t *display_config;
	uint8_t interface_pixel_format = 0;

	display_config = eglib_GetDisplayConfig(eglib);

	interface_pixel_format = 0;
	switch(display_config->color) {
		case ILI9341_COLOR_12_BIT:
			interface_pixel_format |= ILI9341_INTERFACE_PIXEL_FORMAT_COLOR_12BIT;
			break;
		case ILI9341_COLOR_16_BIT:
			interface_pixel_format |= ILI9341_INTERFACE_PIXEL_FORMAT_COLOR_16BIT;
			break;
		case ILI9341_COLOR_18_BIT:
			interface_pixel_format |= ILI9341_INTERFACE_PIXEL_FORMAT_COLOR_18BIT;
			break;
	}
	eglib_SendCommandByte(eglib, ILI9341_INTERFACE_PIXEL_FORMAT);
	eglib_SendDataByte(eglib, interface_pixel_format);
}

static void set_memory_data_access_control(eglib_t *eglib) {
	ili9341_config_t *display_config;
	uint8_t memory_data_access_control;

	display_config = eglib_GetDisplayConfig(eglib);

	memory_data_access_control = 0;
	switch(display_config->page_address) {
		case ILI9341_PAGE_ADDRESS_TOP_TO_BOTTOM:
			memory_data_access_control |= ILI9341_MEMORY_DATA_ACCESS_CONTROL_PAGE_ADDRESS_TOP_TO_BOTTOM;
			break;
		case ILI9341_PAGE_ADDRESS_BOTTOM_TO_TOP:
			memory_data_access_control |= ILI9341_MEMORY_DATA_ACCESS_CONTROL_PAGE_ADDRESS_BOTTOM_TO_TOP;
			break;
	}
	switch(display_config->colum_address) {
		case ILI9341_COLUMN_ADDRESS_LEFT_TO_RIGHT:
			memory_data_access_control |= ILI9341_MEMORY_DATA_ACCESS_CONTROL_COLUMN_ADDRESS_LEFT_TO_RIGHT;
			break;
		case ILI9341_COLUMN_ADDRESS_RIGHT_TO_LEFT:
			memory_data_access_control |= ILI9341_MEMORY_DATA_ACCESS_CONTROL_COLUMN_ADDRESS_RIGHT_TO_LEFT;
			break;
	}
	switch(display_config->page_column_order) {
		case ILI9341_PAGE_COLUMN_ORDER_NORMAL:
			memory_data_access_control |= ILI9341_MEMORY_DATA_ACCESS_CONTROL_PAGE_COLUMN_ORDER_NORMAL;
			break;
		case ILI9341_PAGE_COLUMN_ORDER_REVERSE:
			memory_data_access_control |= ILI9341_MEMORY_DATA_ACCESS_CONTROL_PAGE_COLUMN_ORDER_REVERSE;
			break;
	}
	switch(display_config->vertical_refresh) {
		case ILI9341_VERTICAL_REFRESH_TOP_TO_BOTTOM:
			memory_data_access_control |= ILI9341_MEMORY_DATA_ACCESS_CONTROL_LINE_ADDRESS_ORDER_LCD_REFRESH_TOP_TO_BOTTOM;
			break;
		case ILI9341_VERTICAL_REFRESH_BOTTOM_TO_TOP:
			memory_data_access_control |= ILI9341_MEMORY_DATA_ACCESS_CONTROL_LINE_ADDRESS_ORDER_LCD_REFRESH_BOTTOM_TO_TOP;
			break;
	}
	memory_data_access_control |= ILI9341_MEMORY_DATA_ACCESS_CONTROL_BGR;
	switch(display_config->horizontal_refresh) {
		case ILI9341_HORIZONTAL_REFRESH_LEFT_TO_RIGHT:
			memory_data_access_control |= ILI9341_MEMORY_DATA_ACCESS_CONTROL_DISPLAY_DATA_LATCH_DATA_ORDER_LCD_REFRESH_LEFT_TO_RIGHT;
			break;
		case ILI9341_HORIZONTAL_REFRESH_RIGHT_TO_LEFT:
			memory_data_access_control |= ILI9341_MEMORY_DATA_ACCESS_CONTROL_DISPLAY_DATA_LATCH_DATA_ORDER_LCD_REFRESH_RIGHT_TO_LEFT;
			break;
	}
	eglib_SendCommandByte(eglib, ILI9341_MEMORY_DATA_ACCESS_CONTROL);
	eglib_SendDataByte(eglib, memory_data_access_control);
}

static void set_column_address(eglib_t *eglib, uint16_t x_start, uint16_t x_end) {
	uint8_t buff[4];
	// ESP_LOGI("ili", "set_column_address %d %d", x_start, x_end );
	set_memory_data_access_control(eglib);
	eglib_SendCommandByte(eglib, ILI9341_COLUMN_ADDRESS_SET);
	buff[0] = (x_start&0xFF00)>>8;
	buff[1] = x_start&0xFF;
	buff[2] = (x_end&0xFF00)>>8;
	buff[3] = x_end&0xFF;
	eglib_SendData(eglib, buff, sizeof(buff));
}

static void set_row_address(eglib_t *eglib, uint16_t y_start, uint16_t y_end) {
	uint8_t buff[4];
	// ESP_LOGI("ili", "set_row_address %d %d", y_start, y_end );
	eglib_SendCommandByte(eglib, ILI9341_ROW_ADDRESS_SET);
	buff[0] = (y_start&0xFF00)>>8;
	buff[1] = y_start&0xFF;
	buff[2] = (y_end&0xFF00)>>8;
	buff[3] = y_end&0xFF;
	eglib_SendData(eglib, buff, sizeof(buff));
}

static uint8_t get_bits_per_pixel(eglib_t *eglib) {
	ili9341_config_t *display_config;

	display_config = eglib_GetDisplayConfig(eglib);

	switch(display_config->color) {
		case ILI9341_COLOR_12_BIT:
			return 12;
			break;
		case ILI9341_COLOR_16_BIT:
			return 16;
			break;
		case ILI9341_COLOR_18_BIT:
			return 24;
			break;
		default:
			while(true);
	}
}

static uint8_t get_bytes_per_pixel(eglib_t *eglib) {
	ili9341_config_t *display_config;
	display_config = eglib_GetDisplayConfig(eglib);
	switch(display_config->color) {
		case ILI9341_COLOR_12_BIT:
			return 2;
			break;
		case ILI9341_COLOR_16_BIT:
			return 2;
			break;
		case ILI9341_COLOR_18_BIT:
			return 3;
			break;
		default:
			while(true);
	}
}

static void clear_memory(eglib_t *eglib) {
	return; // +++++++++++++++++++++++++++++++++++++++++ return to help debugging
	ili9341_config_t *display_config;
	uint32_t memory_size;
	display_config = eglib_GetDisplayConfig(eglib);

	memory_size = (display_config->width * display_config->height * get_bits_per_pixel(eglib)) / 8;
	ESP_LOGI("ili9341", "memory size: %d %d %d %d", memory_size, display_config->width, display_config->height, get_bits_per_pixel(eglib) );

	set_column_address(eglib, 0, display_config->width - 1);
	set_row_address(eglib, 0, display_config->height - 1);
	eglib_SendCommandByte(eglib, ILI9341_MEMORY_WRITE);
	uint8_t buf[256] = { 255 };
	for(uint32_t addr=0 ; addr < memory_size ; addr+= 256 ){
		ESP_LOGI("ili9341", "addr: %d", addr );
		eglib_SendData(eglib, buf, 256 );
	}
}

static void send_pixel(eglib_t *eglib, color_t color) {
	ili9341_config_t *display_config;
	uint8_t buff[3];

	display_config = eglib_GetDisplayConfig(eglib);

	switch(display_config->color) {
		case ILI9341_COLOR_12_BIT:
			buff[0] = color.r & 0xf0;
			buff[0] |= (color.g & 0xf0) >> 4;
			buff[1] = color.b & 0xf0;
			for(uint8_t i=0 ; i < 2 ; i++)
				eglib_SendDataByte(eglib, buff[i]);
			break;
		case ILI9341_COLOR_16_BIT:
			buff[0] = color.r & 0xf8;
			buff[0] |= color.g >> 5;
			buff[1] = (color.g >> 2) << 5;
			buff[1] |= color.b >> 3;
			for(uint8_t i=0 ; i < 2 ; i++)
				eglib_SendDataByte(eglib, buff[i]);
			break;
		case ILI9341_COLOR_18_BIT:
			buff[0] = color.r & ~0x03;
			buff[1] = color.g & ~0x03;
			buff[2] = color.b & ~0x03;
			for(uint8_t i=0 ; i < 3 ; i++)
				eglib_SendDataByte(eglib, buff[i]);
			break;
		default:
			while(true);
	}
}

//
// Display
//

/*

Sample Code for 172x320 Display
void lcd_Initial(void)
{
  	CS=1;
	delayms(5);
	RES=0;
	delayms(10);
	RES=1;
	delayms(120);


	Write_Cmd(0x11);
//	delay_ms(120);

//	Write_Cmd(0x36);
//	if(USE_HORIZONTAL==0)Write_Cmd_Data(0x00);
//	else if(USE_HORIZONTAL==1)Write_Cmd_Data(0xC0);
//	else if(USE_HORIZONTAL==2)Write_Cmd_Data(0x70);
//	else Write_Cmd_Data(0xA0);
 *
 *
 	Write_Cmd(0x3A);
 	Write_Cmd_Data(0x05);

	Write_Cmd(0xB2);       // Poch setting, this is defaults, can omit
	Write_Cmd_Data(0x0C);
	Write_Cmd_Data(0x0C);
	Write_Cmd_Data(0x00);
	Write_Cmd_Data(0x33);
	Write_Cmd_Data(0x33);

	Write_Cmd(0xB7);        // Gate Control
	Write_Cmd_Data(0x35);

	Write_Cmd(0xBB);        //  VCOM 0..1H
	Write_Cmd_Data(0x35);

	Write_Cmd(0xC0);       // LMC Control, 2C is default
	Write_Cmd_Data(0x2C);

	Write_Cmd(0xC2);      // defaults VRH control enable
	Write_Cmd_Data(0x01);

	Write_Cmd(0xC3);      // VRH, default    0x0B
	Write_Cmd_Data(0x13);

	Write_Cmd(0xC4);      // VDVS, default
	Write_Cmd_Data(0x20);

	Write_Cmd(0xC6);      //
	Write_Cmd_Data(0x0F);

	Write_Cmd(0xD0);    // POWER CONTROL
	Write_Cmd_Data(0xA4);
	Write_Cmd_Data(0xA1);

	Write_Cmd(0xD6);    // ????
	Write_Cmd_Data(0xA1);

	Write_Cmd(0xE0);
	Write_Cmd_Data(0xF0);
	Write_Cmd_Data(0x00);
	Write_Cmd_Data(0x04);
	Write_Cmd_Data(0x04);
	Write_Cmd_Data(0x04);
	Write_Cmd_Data(0x05);
	Write_Cmd_Data(0x29);
	Write_Cmd_Data(0x33);
	Write_Cmd_Data(0x3E);
	Write_Cmd_Data(0x38);
	Write_Cmd_Data(0x12);
	Write_Cmd_Data(0x12);
	Write_Cmd_Data(0x28);
	Write_Cmd_Data(0x30);

	Write_Cmd(0xE1);
	Write_Cmd_Data(0xF0);
	Write_Cmd_Data(0x07);
	Write_Cmd_Data(0x0A);
	Write_Cmd_Data(0x0D);
	Write_Cmd_Data(0x0B);
	Write_Cmd_Data(0x07);
	Write_Cmd_Data(0x28);
	Write_Cmd_Data(0x33);
	Write_Cmd_Data(0x3E);
	Write_Cmd_Data(0x36);
	Write_Cmd_Data(0x14);
	Write_Cmd_Data(0x14);
	Write_Cmd_Data(0x29);
	Write_Cmd_Data(0x32);

	Write_Cmd(0x21);  // invert display

	Write_Cmd(0x11);  // sleep out
	delayms(120);
	Write_Cmd(0x29);  // Display on
	delayms(10);



}
*/

static void init(eglib_t *eglib) {
	// Hardware reset
	eglib_SetReset(eglib, 0);
	eglib_DelayMs(eglib, ILI9341_RESX_PULSE_MS);
	eglib_SetReset(eglib, 1);
	eglib_DelayMs(eglib, ILI9341_RESX_CANCEL_MS);

	// Software reset
	eglib_CommBegin(eglib);
	eglib_SendCommandByte(eglib, ILI9341_SOFTWARE_RESET);
	eglib_CommEnd(eglib);
	eglib_DelayMs(eglib, ILI9341_SOFTWARE_RESET_DELAY_MS);

	// comm begin again after SW reset
	eglib_CommBegin(eglib);

	// Out of sleep mode
	eglib_SendCommandByte(eglib, ILI9341_SLEEP_OUT);
	eglib_DelayMs(eglib, ILI9341_SLEEP_OUT_DELAY_MS);

	// Memory Data Access Control
	set_memory_data_access_control(eglib);

	// we need invers, datasheet says oposite
	eglib_SendCommandByte(eglib, ILI9341_DISPLAY_INVERSION_ON); // 0809 was OFF
	// after sw reset, but unless we explicitly set it,
	// ILI9341_DISPLAY_INVERSION_ON will have no effect.
	eglib_SendCommandByte(eglib, ILI9341_NORMAL_DISPLAY_MODE_ON);

	// Frame Rate C6
	eglib_SendCommandByte(eglib, 0xC6);
	eglib_SendDataByte(eglib, 0x15);     // F = 60 Hz  1F = 39 Hz

	eglib_SendCommandByte(eglib, 0xB7);  // GATE Control
	eglib_SendDataByte(eglib, 0x35);     // defaults 35h HN:VGH,  LN:VGL  0..7

    // set pixel format
	set_interface_pixel_format(eglib);

	// UCG_C14(0x0b6, 0x00a, 0x082 | (1<<5), 0x027, 0x000),  /* display function control (POR values, except for shift direction bit) */
	eglib_SendCommandByte(eglib, 0xb6 );
	eglib_SendDataByte(eglib,0x0a );
	eglib_SendDataByte(eglib, 0xE2 );  // Bit6: GS,  Bit5: SS  (82 def)
	eglib_SendDataByte(eglib,0x27 );
	eglib_SendDataByte(eglib,0x00 );

	// UCG_C11(0x0c0, 0x021),                /* power control 1 (reference voltage level), POR=21 */
	eglib_SendCommandByte(eglib, 0xc0 );
	eglib_SendDataByte(eglib,0x21 );

	// UCG_C11(0x0c1, 0x002),                /* power control 2 (step up factor), POR=2 */
	eglib_SendCommandByte(eglib, 0xc1 );
	eglib_SendDataByte(eglib,0x02 );

	//   VRH (C3):  08 (4.1V) default -> 13 (4.5V) in sample  0..3F
	eglib_SendCommandByte(eglib, 0xc3 );
	eglib_SendDataByte(eglib,0x00 );

	//   VRL (C4):  20 (0V) default -> none in sample        0..3F
	eglib_SendCommandByte(eglib, 0xc4 );
	eglib_SendDataByte(eglib,0x30 );

	// CABCCTRL
	eglib_SendCommandByte(eglib, 0xc7 );
	eglib_SendDataByte(eglib,0x00 );  // not set in example, 00 -> default

	// C5 VCOMOFFEST  0x20 default   0..3Fh
	eglib_SendCommandByte(eglib, 0xc5 );
	eglib_SendDataByte(eglib,0x20 );

	// VCOM 77
	eglib_SendCommandByte(eglib, 0xBB );
	eglib_SendDataByte(eglib,0x35 );        // 0..3Fh, default 20

	// POWER CONTROL
	eglib_SendCommandByte(eglib, 0xD0 );
	eglib_SendDataByte(eglib,0xA4 );
	eglib_SendDataByte(eglib,0xA1 );

	// UCG_C15(0x0cb, 0x039, 0x02c, 0x000, 0x034, 0x002),    /* power control A (POR values) */
	eglib_SendCommandByte(eglib, 0xcb );
	uint8_t seqpc[] = { 0x039, 0x02c, 0x000, 0x034, 0x002 };
	eglib_Send(eglib, HAL_DATA, seqpc, sizeof(seqpc) );


	// UCG_C13(0x0cf, 0x000, 0x081, 0x030),  /* power control B (POR values) */
	eglib_SendCommandByte(eglib, 0xcf );
	uint8_t seqcb[] = { 0x000, 0x081, 0x030 };
	eglib_Send(eglib, HAL_DATA, seqcb, sizeof(seqcb) );

	// UCG_C13(0x0e8, 0x084, 0x011, 0x07a),  /* timer driving control A (POR values) */
	eglib_SendCommandByte(eglib, 0xe8 );
	uint8_t seqca[] = { 0x084, 0x011, 0x07a };
	eglib_Send(eglib, HAL_DATA, seqca, sizeof(seqca) );

	// UCG_C12(0x0ea, 0x066, 0x000),         /* timer driving control B (POR values) */
	eglib_SendCommandByte(eglib, 0xea );
	uint8_t seqcp[] = { 0x066, 0x000  };
	eglib_Send(eglib, HAL_DATA, seqcp, sizeof(seqcp) );

	//  UCG_C14(  0x02a, 0x000, 0x000, 0x000, 0x0ef),              /* Horizontal GRAM Address Set */
	eglib_SendCommandByte(eglib, ILI9341_COLUMN_ADDRESS_SET );
	uint8_t seqga[] = { 0x000, 0x000, 0x000, 0x0ab};               // AB = 171
	eglib_Send(eglib, HAL_DATA, seqga, sizeof(seqga) );

	//  UCG_C14(  0x02b, 0x000, 0x000, 0x001, 0x03f),              /* Vertical GRAM Address Set */
	eglib_SendCommandByte(eglib, ILI9341_ROW_ADDRESS_SET );
	uint8_t seqra[] = {0x000, 0x000, 0x001, 0x03f };               // 13F = 319
	eglib_Send(eglib, HAL_DATA, seqra, sizeof(seqra) );


	// Gamma PVGAMCTRL
	eglib_SendCommandByte(eglib, 0xE0 );
	eglib_SendDataByte(eglib, 0xF0 );
	eglib_SendDataByte(eglib, 0x00 );
	eglib_SendDataByte(eglib, 0x04 );
	eglib_SendDataByte(eglib, 0x04 );
	eglib_SendDataByte(eglib, 0x04 );
	eglib_SendDataByte(eglib, 0x05 );
	eglib_SendDataByte(eglib, 0x29 );
	eglib_SendDataByte(eglib, 0x33 );
	eglib_SendDataByte(eglib, 0x3E );
	eglib_SendDataByte(eglib, 0x38 );
	eglib_SendDataByte(eglib, 0x12 );
	eglib_SendDataByte(eglib, 0x12 );
	eglib_SendDataByte(eglib, 0x28 );
	eglib_SendDataByte(eglib, 0x30 );

	// Gamma NVGAMCTRL
	eglib_SendCommandByte(eglib, 0xE1 );
	eglib_SendDataByte(eglib, 0xF0 );
	eglib_SendDataByte(eglib, 0x07 );
	eglib_SendDataByte(eglib, 0x0A );
	eglib_SendDataByte(eglib, 0x0D );
	eglib_SendDataByte(eglib, 0x0B );
	eglib_SendDataByte(eglib, 0x07 );
	eglib_SendDataByte(eglib, 0x28 );
	eglib_SendDataByte(eglib, 0x33 );
	eglib_SendDataByte(eglib, 0x3E );
	eglib_SendDataByte(eglib, 0x36 );
	eglib_SendDataByte(eglib, 0x14 );
	eglib_SendDataByte(eglib, 0x14 );
	eglib_SendDataByte(eglib, 0x29 );
	eglib_SendDataByte(eglib, 0x32 );

	// UCG_C10(  0x02c),               /* Write Data to GRAM */
	eglib_SendCommandByte(eglib, ILI9341_MEMORY_WRITE );

	// UCG_C10(0x029),                               /* display on */
	eglib_SendCommandByte(eglib, ILI9341_DISPLAY_ON );

	eglib_CommEnd(eglib);

	ESP_LOGI("ili", "init()");
};

static void sleep_in(eglib_t *eglib) {
	eglib_CommBegin(eglib);
	eglib_SendCommandByte(eglib, ILI9341_SLEEP_IN);
	eglib_CommEnd(eglib);
	eglib_DelayMs(eglib, ILI9341_SLEEP_IN_DELAY_MS);
};

static void sleep_out(eglib_t *eglib) {
	eglib_CommBegin(eglib);
	eglib_SendCommandByte(eglib, ILI9341_SLEEP_OUT);
	eglib_CommEnd(eglib);
	eglib_DelayMs(eglib, ILI9341_SLEEP_OUT_DELAY_MS);
};

static void get_dimension(
	eglib_t *eglib,
	coordinate_t *width, coordinate_t*height
) {
	ili9341_config_t *display_config;

	display_config = eglib_GetDisplayConfig(eglib);

	*width = display_config->width;;
	*height = display_config->height;
};

static void get_pixel_format(eglib_t *eglib, enum pixel_format_t *pixel_format) {
	ili9341_config_t *display_config;

	display_config = eglib_GetDisplayConfig(eglib);

	switch(display_config->color) {
		case ILI9341_COLOR_12_BIT:
			*pixel_format = PIXEL_FORMAT_12BIT_RGB;
			break;
		case ILI9341_COLOR_16_BIT:
			*pixel_format = PIXEL_FORMAT_16BIT_RGB;
			break;
		case ILI9341_COLOR_18_BIT:
			*pixel_format = PIXEL_FORMAT_18BIT_RGB_24BIT;
			break;
		default:
			while(true);
	}
}

static void draw_pixel_color(
	eglib_t *eglib,
	coordinate_t x, coordinate_t y, color_t color
) {
	eglib_CommBegin(eglib);
	y+=34;

	set_column_address(eglib, x, x);
	set_row_address(eglib, y, y);

	eglib_SendCommandByte(eglib, ILI9341_MEMORY_WRITE);

	send_pixel(eglib, color);

	eglib_CommEnd(eglib);
};

static void draw_line(
	eglib_t *eglib,
	coordinate_t x,
	coordinate_t y,
	enum display_line_direction_t direction,
	coordinate_t length,
	color_t (*get_next_color)(eglib_t *eglib)
) {
	// ESP_LOGI("ili DL","x:%d y:%d dir:%d len:%d", x, y, direction, length );
	eglib_CommBegin(eglib);
	y+=34;
	if(direction == DISPLAY_LINE_DIRECTION_RIGHT) {
		// ESP_LOGI("DL","x:%d y:%d RIGHT %d", x, y, length  );
		set_column_address(eglib, x, x + length );
		set_row_address(eglib, y, y );
	}
	else if(direction == DISPLAY_LINE_DIRECTION_DOWN) {
		// ESP_LOGI("DL","x:%d y:%d DOWN %d", x, y, length  );
		set_column_address(eglib, x, x );
		set_row_address(eglib, y, y+length );
	}
	else if(direction == DISPLAY_LINE_DIRECTION_UP) {
		// ESP_LOGI("DL","x:%d y:%d UP %d", x, y, length  );
		set_column_address(eglib, x, x );
		set_row_address(eglib, y-length, y );
	}
	else if(direction == DISPLAY_LINE_DIRECTION_LEFT) {
		// ESP_LOGI("DL","x:%d y:%d LEFT %d", x, y, length );
		set_column_address(eglib, x-length, x );  // fake right direction
		set_row_address(eglib, y, y );
	}
	else{
		ESP_LOGW("draw_line","draw_line method not implemented");
	}
	eglib_SendCommandByte(eglib, ILI9341_MEMORY_WRITE);
	color_t color = eglib->drawing.color_index[0];
	int len_pix=get_bytes_per_pixel(eglib);  // max 3
	uint8_t buf[4] = { 0,0,0,0 };
	ili9341_config_t *display_config = eglib_GetDisplayConfig(eglib);
	switch(display_config->color) {
	case ILI9341_COLOR_12_BIT:
		buf[0] = color.r & 0xf0;
		buf[0] |= (color.g & 0xf0) >> 4;
		buf[1] = color.b & 0xf0;
		break;
	case ILI9341_COLOR_16_BIT:
		buf[0] = color.r & 0xf8;
		buf[0] |= color.g >> 5;
		buf[1] = (color.g >> 2) << 5;
		buf[1] |= color.b >> 3;
		break;
	case ILI9341_COLOR_18_BIT:
		buf[0] = color.r & ~0x03;
		buf[1] = color.g & ~0x03;
		buf[2] = color.b & ~0x03;
		break;
	default:
		while(true);
	}
	for( int i=0; i<length; i++ ){
		for(int i=0;i < len_pix; i++)
			eglib_SendDataByte(eglib, buf[i]);
	}
	eglib_CommEnd(eglib);
}

static void send_buffer(
	eglib_t *eglib,
	void *buffer_ptr,
	coordinate_t x, coordinate_t y,
	coordinate_t width, coordinate_t height
) {
	ili9341_config_t *display_config;
	uint8_t *buffer = (uint8_t *)buffer_ptr;
	display_config = eglib_GetDisplayConfig(eglib);
	// if((uint32_t)x * get_bits_per_pixel(eglib) % 8)
	//	x -= 1;
	eglib_CommBegin(eglib);
	y+=34;
    set_column_address(eglib, x, x + width -1);
    set_row_address(eglib, y-height -1, y );
    eglib_SendCommandByte(eglib, ILI9341_MEMORY_WRITE);
    eglib_SendData( eglib, buffer, width*height*get_bytes_per_pixel(eglib) );
	eglib_CommEnd(eglib);
}

static bool refresh(eglib_t *eglib) {
	(void)eglib;
	return false;
}

/*
 *   set scroll margins for hardware scroll function
 */
void set_scroll_margins( eglib_t *eglib, coordinate_t top, coordinate_t bottom ){
	ili9341_config_t *display_config = eglib_GetDisplayConfig(eglib);
	if (top + bottom <= display_config->height ) {
		eglib_CommBegin(eglib);
		uint16_t middle = display_config->height - top -bottom;
		uint8_t data[6];
		eglib_SendCommandByte(eglib, ILI9341_VERTICAL_SCROLLING_DEFINITION );
		data[0] = top >> 8;
		data[1] = top & 0xff;
		data[2] = middle >> 8;
		data[3] = middle & 0xff;
		data[4] = bottom >> 8;
		data[5] = bottom & 0xff;
		eglib_SendData( eglib, data, 6 );
		eglib_CommEnd(eglib);
	}
}

/*
 *   scroll display by the number of row lines as given
 */
void scroll( eglib_t *eglib, coordinate_t lines ){
	eglib_CommBegin(eglib);
	eglib_SendCommandByte(eglib, ILI9341_VERTICAL_SCROLL_START_ADDRESS_OF_RAM );
	uint8_t data[2];
	data[0] = lines >> 8;
	data[1] = lines & 0xff;
	eglib_SendData( eglib, data, 2 );
	eglib_CommEnd(eglib);
}

const display_t ili9341 = {
	.comm = {
		.four_wire_spi = &((hal_four_wire_spi_config_t){
			.mode = 0,
			.bit_numbering = HAL_MSB_FIRST,
			.cs_setup_ns = 15,
			.cs_hold_ns = 15,
			.cs_disable_ns = 40,
			.dc_setup_ns = 10,
			.dc_hold_ns = 10,
			.sck_cycle_ns = 66,  // 15.1MHz Datasheet
			// .sck_cycle_ns = 47, // 21.2MHz Overclock
		}),
		.i2c = NULL,
		.parallel_8_bit_8080 = &((hal_parallel_8_bit_8080_config_t){
			.csx_setup_ns = 15,
			.csx_hold_ns = 10,
			.csx_disable_ns = 0,
			.dcx_setup_ns = 0,
			.wrx_cycle_ns = 66,
			.wrx_high_ns = 15,
			.wrx_low_ns = 15,
		}),
	},
	.init = init,
	.sleep_in = sleep_in,
	.sleep_out = sleep_out,
	.get_dimension = get_dimension,
	.get_pixel_format = get_pixel_format,
	.draw_pixel_color = draw_pixel_color,
	.draw_line = draw_line,
	.send_buffer = send_buffer,
	.refresh = refresh,
	.set_scroll_margins = set_scroll_margins,
	.scroll = scroll
};

//
// Custom functions
//

void ili9341_SetDisplayInversion(eglib_t *eglib, bool inversion) {
	eglib_CommBegin(eglib);
	if(inversion)
		eglib_SendCommandByte(eglib, ILI9341_DISPLAY_INVERSION_OFF);
	else
		eglib_SendCommandByte(eglib, ILI9341_DISPLAY_INVERSION_ON);
	eglib_CommEnd(eglib);
}

void ili9341_SetIdleMode(eglib_t *eglib, bool idle) {
	eglib_CommBegin(eglib);
	if(idle)
		eglib_SendCommandByte(eglib, ILI9341_IDLE_MODE_ON);
	else
		eglib_SendCommandByte(eglib, ILI9341_IDLE_MODE_OFF);
	eglib_CommEnd(eglib);
}

