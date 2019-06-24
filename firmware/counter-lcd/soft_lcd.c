/*
 * wPi_soft_lcd
 *      Library to use PCF8574 based LCD via software I2C bus.
 *      using WiringPi library GPIO.
 *
 *      Adapted to run in a PIC16F88.
 *
 *      Electronica y ciencia.
 *      https://github.com/electronicayciencia/wPi_soft_lcd
 *      https://electronicayciencia.blogspot.com/
 *
 *      Reinoso G.
 */


#ifndef _SOFT_LCD_C
#define _SOFT_LCD_C

#include "soft_lcd.h"



int lcd_create(lcd_t *lcd, int addr, int lines) {

	if ( !_pcf8574_check(addr) ) return 0;

	lcd->_addr         = addr;
	lcd->_lines        = lines;
	lcd->err           = 0;
	lcd->fcn_set       = LCD_FCN_4BIT | LCD_FCN_5x8;

	if (lines > 1)
		 lcd->fcn_set |= LCD_FCN_2LINES;

	lcd->cursor_set    = LCD_CURSOR_MOVE_CUR | LCD_CURSOR_LEFT;
	lcd->display_set   = LCD_DISPLAY_ON | LCD_DISPLAY_CURSOR_OFF | LCD_DISPLAY_BLINK_OFF;
	lcd->entrymode_set = LCD_ENTRYMODE_CURSOR_DECR | LCD_ENTRYMODE_SCROLL_OFF;

	lcd->backlight     = LCD_BKLIGHT;
	lcd->replace_UTF8_chars = 1;
	
	delay_ms(10);
	lcd_reset(lcd);
	lcd_init(lcd);

	return 1;
}

void lcd_init(lcd_t *lcd) {
	lcd_reconfig(lcd);
	lcd_clear(lcd);
	lcd_home(lcd);
}


/* send configuration parameters to LCD */
void lcd_reconfig(lcd_t *lcd) {
	lcd_reconfig_fcn(lcd);
	lcd_reconfig_cursor(lcd);
	lcd_reconfig_display(lcd);
	lcd_reconfig_entrymode(lcd);
}

void lcd_reconfig_fcn(lcd_t *lcd) { 
	lcd_raw(lcd, LCD_WRITE, LCD_CMD_FCN_SET | lcd->fcn_set);
}
void lcd_reconfig_cursor(lcd_t *lcd) {
	lcd_raw(lcd, LCD_WRITE, LCD_CMD_CURSOR_SET | lcd->cursor_set);
}
void lcd_reconfig_display(lcd_t *lcd) {
	lcd_raw(lcd, LCD_WRITE, LCD_CMD_DISPLAY_SET | lcd->display_set);
}
void lcd_reconfig_entrymode(lcd_t *lcd) {
	lcd_raw(lcd, LCD_WRITE, LCD_CMD_ENTRYMODE_SET | lcd->entrymode_set);
}

void lcd_home (lcd_t *lcd) {
	lcd_raw(lcd, LCD_WRITE, LCD_CMD_HOME);
}

void lcd_clear (lcd_t *lcd) {
	lcd_raw(lcd, LCD_WRITE, LCD_CMD_CLEAR);
	delay_us(2000);
}

/* Convenient shortcuts */
void lcd_on(lcd_t *lcd) {
	lcd->display_set |= LCD_DISPLAY_ON;
	lcd_reconfig_display(lcd);
}

void lcd_off(lcd_t *lcd) {
	lcd->display_set &= ~LCD_DISPLAY_ON;
	lcd_reconfig_display(lcd);
}

void lcd_backlight_on(lcd_t *lcd) {
	lcd->backlight = LCD_BKLIGHT;
	lcd_reconfig_display(lcd);
}

void lcd_backlight_off(lcd_t *lcd) {
	lcd->backlight = 0;
	lcd_reconfig_display(lcd);
}

void lcd_cursor_on(lcd_t *lcd) {
	lcd->display_set |= LCD_DISPLAY_CURSOR_ON;
	lcd_reconfig_display(lcd);
}

void lcd_cursor_off(lcd_t *lcd) {
	lcd->display_set &= ~LCD_DISPLAY_CURSOR_ON;
	lcd_reconfig_display(lcd);
}

void lcd_blink_on(lcd_t *lcd) {
	lcd->display_set |= LCD_DISPLAY_BLINK_ON;
	lcd_reconfig_display(lcd);
}

void lcd_blink_off(lcd_t *lcd) {
	lcd->display_set &= ~LCD_DISPLAY_BLINK_ON;
	lcd_reconfig_display(lcd);
}

void lcd_pos(lcd_t *lcd, int row, int col) {
	int rows_value[] = {0x00, 0x40, 0x14, 0x54};
	lcd_pos_raw(lcd, rows_value[row] + col);
}

void lcd_pos_raw(lcd_t *lcd, int pos) {
	lcd_raw(lcd, LCD_WRITE, LCD_CMD_DDRAM_SET | pos);
}

/* Set LCD controller into a known state and set 4 bit mode */
void lcd_reset (lcd_t *lcd) {
	delay_us(45000);
	_pcf8574_put(lcd, LCD_D5 | LCD_D4);

	delay_us(5000);
	_pcf8574_put(lcd, LCD_D5 | LCD_D4);

	delay_us(1000);
	_pcf8574_put(lcd, LCD_D5 | LCD_D4);

	/* we assume pcf8574 and 4bit mode for now */
	if (lcd->fcn_set & LCD_FCN_8BIT) return;

	_pcf8574_put(lcd, LCD_CMD_FCN_SET | LCD_FCN_4BIT);
}


/* Prints string in actual cursor position */
void lcd_print(lcd_t *lcd, char *s) {
	unsigned int i = 0;

	while(s[i]) {
		if (s[i] == '\n') {
			_lcd_nextline(lcd);
		}
		else {
		//printf("Char: %02x\n", s[i]);
			lcd_raw(lcd, LCD_WRITE | LCD_RS, s[i]);
		}
	i++;
	}
}

/* Create characters in the CGRAM table
 * Note that character 0 may be defined, but cannot be used because \x00 is
 * not valid inside a string */
void lcd_create_char(lcd_t *lcd, int n, char *data) {
	if (n > 8) return;
	
	int cursor_pos = lcd_read_pos_raw(lcd);

	lcd_raw(lcd, LCD_WRITE, LCD_CMD_CGRAM_SET + 8 * n);

	int i;
	for (i = 0; i < 8; i++) {
		lcd_raw(lcd, LCD_WRITE | LCD_RS, data[i]);
	}

	lcd_pos_raw(lcd, cursor_pos);
}

/* Read cursor pos and busy flag */
int lcd_read_pos_raw (lcd_t *lcd) {
	return lcd_read_raw(lcd, 0);
}

/* Read data at cursor and shift */
int lcd_read_data (lcd_t *lcd) {
	return lcd_read_raw(lcd, LCD_RS);
}

/* Reads the cursor position and sits it at
 * the beginning of the next line */
void _lcd_nextline(lcd_t *lcd) {
	int pos = lcd_read_pos_raw(lcd);
	//printf("Cursor was at %d.\n", lcd_read_pos_raw(lcd));

	/* LCD should not be busy now */
	if (pos & LCD_BUSY_FLAG) {
		lcd->err = 1;
		return;
	}

	/* Different LCD lines have different ranges */
	switch (lcd->_lines) {
		case 1:
			lcd_pos_raw(lcd, 0x00);
			break;
		case 2:
			if (pos < 0x40)
				lcd_pos_raw(lcd, 0x40);
			break;
		case 4:
			if (pos < 0x14)                     // first line
				lcd_pos_raw(lcd, 0x40);
			else if (pos >= 0x40 && pos < 0x54) // second line
				lcd_pos_raw(lcd, 0x14);
			else if (pos >= 0x14 && pos < 0x40) // third line
				lcd_pos_raw(lcd, 0x54);
			break;
	}
}

/* Writting a raw command */
void lcd_raw (lcd_t *lcd, int lcd_opts, int data) {
	int upper = (data >> 4) & 0xF;
	int lower = data & 0xF;

	lcd_opts |= lcd->backlight;

	//printf("Data: %02x\n", data);

	_pcf8574_put(lcd, (upper << 4) | lcd_opts);
	_pcf8574_put(lcd, (lower << 4) | lcd_opts);
}

/* Reading a raw byte */
int lcd_read_raw (lcd_t *lcd, int rs) {
	int u = _pcf8574_get(lcd, rs);
	int l = _pcf8574_get(lcd, rs);

	return  (u<<4) + l;
}

/* Send a nibble and status lines to PCF8574
 * Sets error condition in lcd if any command is not ack'ed
 * For write operations, the command is executed after enabled falling edge */
void _pcf8574_put (lcd_t *lcd, int lines) {
	//printf("Sending lines: %02x\n", lines);
	i2c_start();

	if (
		(i2c_write(lcd->_addr << 1 | I2C_WRITE_CMD) == I2C_ACK) &&
		(i2c_write(lines | LCD_ENABLED) == I2C_ACK) &&
		(i2c_write(lines)               == I2C_ACK)
	) {
		i2c_stop();
		return;
	}

	i2c_stop();
	lcd->err = LCD_ERR_I2C;
}

/* Reads a nibble from data lines of PCF8574
 * For read operations:
 *   - set data lines as input writing them 1
 *   - raise R/W while Enabled is still down
 *   - raise Enabled
 *   - read value
 *   - low enable
 * To read data under cursor, set RS
 * TODO: Set error condition in lcd if any command is not ack'ed */
int _pcf8574_get (lcd_t *lcd, int rs) {
	int rbyte;
	int lines = LCD_D4|LCD_D5|LCD_D6|LCD_D7
		| LCD_READ
		| rs
		| lcd->backlight;

	/* Set reading lines and reading mode */
	i2c_start();
		i2c_write(lcd->_addr << 1 | I2C_WRITE_CMD);
		i2c_write(lines);
		i2c_write(lines | LCD_ENABLED);
	i2c_stop();

	/* Actually read lines */
	i2c_start();
		i2c_write(lcd->_addr << 1 | I2C_READ_CMD);
		rbyte = i2c_read();
		i2c_write(I2C_NACK);
	i2c_stop();

	/* Unset read mode */
	i2c_start();
		i2c_write(lcd->_addr << 1 | I2C_WRITE_CMD);
		i2c_write(lines & ~LCD_ENABLED);
		i2c_write(lines & ~LCD_READ);
	i2c_stop();

	//printf("Readed byte: %02x\n", rbyte);
	return rbyte >> 4;
}


/* check if PCF8574 driver is ready */
int _pcf8574_check (int addr) {
	i2c_start();

	int r = i2c_write(addr << 1 | I2C_WRITE_CMD);
	
	if (r != I2C_ACK) return 0;
	
	i2c_stop();
	return 1;
}

#endif