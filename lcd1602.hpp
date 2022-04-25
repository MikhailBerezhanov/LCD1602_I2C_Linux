//
// -- Description:
// I2C 2x16 LCD display driver (HD44780 controller + PCF8574(A) I2C port expander)
//
// -- Features:
// 1. ENG and RU symbols printing support
// 2. Additional class WH1602B_CTK for Cyrillic LCD implementation
//
// -- LCD Connection:
// The LCD controller is wired to the I2C port expander with the upper 4 bits
// (P4-P7) connected to the data lines (D4-D7) and the lower 4 bits (P0-P3) used as
// control lines. Here are the control line definitions:
//
// Command (0) / Data (1) (aka RS) (P0)
// R/W                             (P1)
// Enable/CLK                      (P2) 
// Backlight control               (P3)
//

#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>

// Supported i2c-adapters chip addresses 
#define PCF8574A_ADDR   		0x7E
#define PCF8574_ADDR    		0x4E

// Flags for backlight control
#define LCD_BACKLIGHT 			0x08
#define LCD_NOBACKLIGHT 		0x00

class LCD1602
{
public:

	// User character data
	typedef struct {
		uint8_t bitmap_idx;
		uint8_t bitmap[8];
	}custom_char;

	LCD1602(uint8_t lcd_addr = PCF8574A_ADDR): address(lcd_addr){}

	virtual ~LCD1602() = default;

	// Initialization method must be called before any other
	void init(uint8_t lcd_addr, const std::string &i2c_dev = "");
	
	// Configure methods
	void clear();
	void control(bool backlight, bool cursor = false, bool blink = false);
	void return_home();
	void set_cursor(uint8_t row, uint8_t col);
	void scroll_right();
	void scroll_left();
	void left_to_right(bool on_off);
	void autoscroll(bool on_off);

	// Get info methods
	uint8_t get_addr() const { return address; }
	uint8_t get_current_row() const { return current_row; }
	uint8_t get_current_col() const { return current_col; }
	uint8_t get_control() const { return display_control; }

	// Printing methods

	// ENG string only
	virtual void print(const char ch){
		this->send_data(static_cast<uint8_t>(ch));
		++current_col;
	}
	virtual void print(const char *str);
	virtual void print(const std::string &str) { this->print(str.c_str()); }

	// ENG + RU string support 
	// Cyrrilic symbols are software generated. Max 8 RU-letters on the screen at one time.
	void print_ru(const wchar_t wc) { this->print_wc(wc); }
	void print_ru(const char *str);
	void print_ru(const std::string &str) { this->print_ru(str.c_str()); }

	// User-defined charecters methods (location: 0-7)
	void user_char_create(uint8_t location, const uint8_t *charmap);
	void user_char_print(uint8_t location);

private:
	uint8_t address = 0;					// i2c port expander chip address
	uint8_t numlines = 2;					// number of screen lines
	uint8_t backlight_flag = LCD_BACKLIGHT;	// backlight status

	uint8_t display_function = 0;			// function set status
	uint8_t display_control = 0;			// control status (backlight, cursor, blink)
	uint8_t display_mode = 0;				// mode set status

	// Cursor position
	uint8_t current_row = 0;
	uint8_t current_col = 0;

	// Data flow operations
	void send_8bit(uint8_t data);
	void send_4bit(uint8_t data, uint8_t flags);
	void send_command(uint8_t cmd) { this->send_4bit(cmd, 0); }
	void send_data(uint8_t data);

	// RU characters support with CGRAM implementation methods
	uint8_t current_symb_idx = 0;
	static std::unordered_map<wchar_t, custom_char> ru_symb_table;	// symbol unicode -> bitmap
	void reset_ru_symb_table();
	void print_ru_char(const uint8_t *charmap, uint8_t *index);

	// Mixed print - supports both ENG and RU symbols
	virtual void print_wc(const wchar_t wc);
};


// WH1602B_CTK LCD has embedded hardware Cyrillic characters ROM support.
// This display is pin-to-pin compatible with LCD1602 and can be also connected  
// through PCF8574(A) i2c port expander.  
class WH1602B_CTK final: public LCD1602
{
public:
	WH1602B_CTK(uint8_t lcd_addr = PCF8574A_ADDR): LCD1602(lcd_addr) {}

	// Print ENG + RU strings (Hardware supports Cyrillic symbols)
	void print(const char c) override { this->print_wc(static_cast<wchar_t>(c)); }
	void print(const char *str) override;
	void print(const std::string &str) override { this->print(str.c_str()); }

private:
	static const std::unordered_map<wchar_t, uint8_t> symb_codes;
	void print_wc(const wchar_t wc) override;
};

