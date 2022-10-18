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

#ifndef _LCD1602_HPP
#define _LCD1602_HPP

#include <cstdint>
#include <string>
#include <unordered_map>
#include <tuple>

// Supported i2c-adapters chip addresses 
#define PCF8574A_ADDR   		0x7E
#define PCF8574_ADDR    		0x4E

// Flags for backlight control
#define LCD_BACKLIGHT 			0x08
#define LCD_NOBACKLIGHT 		0x00

class LCD1602
{
public:

	//
	enum class Alignment : char
	{
		NO = 0,
		LEFT,
		RIGHT,
		CENTER,
	};

	// User character data
	typedef struct {
		uint8_t bitmap_idx;
		uint8_t bitmap[8];
	}custom_char;

	LCD1602(uint8_t lcd_addr = PCF8574A_ADDR): address(lcd_addr){}

	virtual ~LCD1602() = default;

	// Initialization method must be called before any other
	void init(uint8_t lcd_addr, const std::string &i2c_dev = "");
	void set_addr(uint8_t lcd_addr) { address = lcd_addr; }

	// Configure methods
	void clear();
	void control(bool backlight, bool cursor = false, bool blink = false);
	void return_home();
	void set_cursor(uint8_t row, uint8_t col);
	void scroll_right(void);
	void scroll_left(void);
	void left_to_right(bool on_off);
	void autoscroll(bool on_off);

	// Get info methods
	uint8_t get_addr() const { return address; }
	uint8_t get_num_rows() const { return num_rows; }
	uint8_t get_num_cols() const { return num_cols; }
	uint8_t get_current_row() const { return current_row; }
	uint8_t get_current_col() const { return current_col; }
	std::tuple<bool, bool, bool> get_control() const;
	bool get_backlight_state() const { return backlight_flag == LCD_BACKLIGHT; }

	// Printing methods

	// ENG string only
	virtual void print_char(char ch){
		this->send_data(static_cast<uint8_t>(ch));
		++current_col;
	}
	virtual void print(const char *fmt, ...);
	virtual void print(const std::string &str, Alignment align = Alignment::NO){ 
		this->print_str(str.c_str(), align); 
	}

	// Adds padding after str to fit row length (cols num. 
	// Note: can be used with spaces symbols to avoid clear() calls.
	virtual void print_with_padding(const std::string &str, char symb = ' ');

	// ENG + RU string support 
	// Cyrrilic symbols are software generated. Max 8 RU-letters on the screen at one time.
	void print_ru(const wchar_t wc) { this->print_wc(wc); }
	void print_ru(const char *str);
	void print_ru(const std::string &str) { this->print_ru(str.c_str()); }

	// User-defined charecters methods (location: 0-7)
	void user_char_create(uint8_t location, const uint8_t *charmap);
	void user_char_print(uint8_t location);
	inline void align(size_t len, Alignment align_type);

private:
	uint8_t address = 0;					// i2c port expander chip address
	uint8_t num_rows = 2;					// number of screen lines
	uint8_t num_cols = 16;					// number of columns in one screen line
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
	virtual void print_wc(wchar_t wc);
	virtual void print_str(const char *str, Alignment align_type);
};


// WH1602B_CTK LCD has embedded hardware Cyrillic characters ROM support.
// This display is pin-to-pin compatible with LCD1602 and can be also connected  
// through PCF8574(A) i2c port expander.  
class WH1602B_CTK final: public LCD1602
{
public:
	WH1602B_CTK(uint8_t lcd_addr = PCF8574A_ADDR): LCD1602(lcd_addr) {}

	// Print ENG + RU strings (Hardware supports Cyrillic symbols)
	// void print(const char *fmt, ...);
	// void print(const std::string &str);

	void print_char(char ch) override { this->print_wc(static_cast<wchar_t>(ch)); }

private:
	static const std::unordered_map<wchar_t, uint8_t> symb_codes;

	// print() functions uses print_wc() and print_str() as backend,
	// so only these two methods should be overrided
	void print_wc(wchar_t wc) override;
	void print_str(const char *str, Alignment align_type) override;
};

size_t number_of_symbols(const char *str, size_t *bytes_num = nullptr);

#endif
