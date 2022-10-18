#include <cstring>
#include <cstdarg>

extern "C"{
    #include <unistd.h>
}

#include "i2c.hpp"
#include "lcd1602.hpp"

using namespace hw;

// LCD controller pinout
#define PIN_RS 		( (uint8_t)(1 << 0) )	// 1 - Send Data, 0 - Send Command
#define PIN_RW   	( (uint8_t)(1 << 1) )	// Read \ Write to CG or DDRAM
#define PIN_EN 		( (uint8_t)(1 << 2) )	// Enable sending
#define PIN_VO 		( (uint8_t)(1 << 3) )	// Backlight control

// Commands
#define LCD_CLEARDISPLAY 		0x01
#define LCD_RETURNHOME 			0x02
#define LCD_ENTRYMODESET 		0x04
#define LCD_DISPLAYCONTROL 		0x08
#define LCD_CURSORSHIFT 		0x10
#define LCD_FUNCTIONSET 		0x20
#define LCD_SETCGRAMADDR 		0x40
#define LCD_SETDDRAMADDR 		0x80

// Flags for display entry mode
#define LCD_ENTRYRIGHT 			0x00
#define LCD_ENTRYLEFT 			0x02
#define LCD_ENTRYSHIFTINCREMENT	0x01
#define LCD_ENTRYSHIFTDECREMENT	0x00

// Flags for display on/off control
#define LCD_DISPLAYON 			0x04
#define LCD_DISPLAYOFF 			0x00
#define LCD_CURSORON 			0x02
#define LCD_CURSOROFF 			0x00
#define LCD_BLINKON 			0x01
#define LCD_BLINKOFF 			0x00

// Flags for display/cursor shift
#define LCD_DISPLAYMOVE 		0x08
#define LCD_CURSORMOVE 			0x00
#define LCD_MOVERIGHT 			0x04
#define LCD_MOVELEFT 			0x00

// Flags for function set
#define LCD_8BITMODE 			0x10
#define LCD_4BITMODE 			0x00
#define LCD_2LINE 				0x08
#define LCD_1LINE 				0x00
#define LCD_5x10DOTS 			0x04
#define LCD_5x8DOTS 			0x00

// Index of user custom character bitmap
#define B_NOIDX 				0xFF
#define B_MAXIDX				7

// Russian chars hash-table (symbol unicode -> bitmap)
std::unordered_map<wchar_t, LCD1602::custom_char> LCD1602::ru_symb_table { 
	{1041, {B_NOIDX, {0b11111,0b10000,0b10000,0b11110,0b10001,0b10001,0b11110,0b00000}}}, // Б
	{1043, {B_NOIDX, {0b11111,0b10000,0b10000,0b10000,0b10000,0b10000,0b10000,0b00000}}}, // Г
	{1044, {B_NOIDX, {0b00110,0b01010,0b01010,0b01010,0b01010,0b01010,0b11111,0b10001}}}, // Д
	{1046, {B_NOIDX, {0b10101,0b10101,0b10101,0b01110,0b10101,0b10101,0b10101,0b00000}}}, // Ж 
	{1047, {B_NOIDX, {0b01110,0b10001,0b00001,0b00110,0b00001,0b10001,0b01110,0b00000}}}, // З
	{1048, {B_NOIDX, {0b10001,0b10001,0b10001,0b10011,0b10101,0b11001,0b10001,0b00000}}}, // И
	{1049, {B_NOIDX, {0b10101,0b10001,0b10001,0b10011,0b10101,0b11001,0b10001,0b00000}}}, // Й
	{1051, {B_NOIDX, {0b00111,0b01001,0b01001,0b01001,0b01001,0b01001,0b10001,0b00000}}}, // Л
	{1055, {B_NOIDX, {0b11111,0b10001,0b10001,0b10001,0b10001,0b10001,0b10001,0b00000}}}, // П
	{1059, {B_NOIDX, {0b10001,0b10001,0b10001,0b01111,0b00001,0b10001,0b01110,0b00000}}}, // У
	{1060, {B_NOIDX, {0b00100,0b01110,0b10101,0b10101,0b10101,0b01110,0b00100,0b00000}}}, // Ф
	{1062, {B_NOIDX, {0b10010,0b10010,0b10010,0b10010,0b10010,0b10010,0b11111,0b00001}}}, // Ц
	{1063, {B_NOIDX, {0b10001,0b10001,0b10001,0b01111,0b00001,0b00001,0b00001,0b00000}}}, // Ч
	{1064, {B_NOIDX, {0b10001,0b10001,0b10001,0b10101,0b10101,0b10101,0b11111,0b00000}}}, // Ш
	{1065, {B_NOIDX, {0b10001,0b10001,0b10001,0b10101,0b10101,0b10101,0b11111,0b00001}}}, // Щ
	{1066, {B_NOIDX, {0b11000,0b01000,0b01000,0b01110,0b01001,0b01001,0b01110,0b00000}}}, // Ъ
	{1067, {B_NOIDX, {0b10001,0b10001,0b10001,0b11101,0b10011,0b10011,0b11101,0b00000}}}, // Ы
	{1068, {B_NOIDX, {0b10000,0b10000,0b10000,0b11110,0b10001,0b10001,0b11110,0b00000}}}, // Ь
	{1069, {B_NOIDX, {0b01110,0b10001,0b00001,0b00111,0b00001,0b10001,0b01110,0b00000}}}, // Э
	{1070, {B_NOIDX, {0b10010,0b10101,0b10101,0b11101,0b10101,0b10101,0b10010,0b00000}}}, // Ю
	{1071, {B_NOIDX, {0b01111,0b10001,0b10001,0b01111,0b00101,0b01001,0b10001,0b00000}}}, // Я
	{1073, {B_NOIDX, {0b00011,0b01100,0b10000,0b11110,0b10001,0b10001,0b01110,0b00000}}}, // б
	{1074, {B_NOIDX, {0b00000,0b00000,0b11110,0b10001,0b11110,0b10001,0b11110,0b00000}}}, // в
	{1075, {B_NOIDX, {0b00000,0b00000,0b11110,0b10000,0b10000,0b10000,0b10000,0b00000}}}, // г
	{1076, {B_NOIDX, {0b00000,0b00000,0b00110,0b01010,0b01010,0b01010,0b11111,0b10001}}}, // д
	{1105, {B_NOIDX, {0b01010,0b00000,0b01110,0b10001,0b11111,0b10000,0b01111,0b00000}}}, // ё
	{1078, {B_NOIDX, {0b00000,0b00000,0b10101,0b10101,0b01110,0b10101,0b10101,0b00000}}}, // ж
	{1079, {B_NOIDX, {0b00000,0b00000,0b01110,0b10001,0b00110,0b10001,0b01110,0b00000}}}, // з
	{1080, {B_NOIDX, {0b00000,0b00000,0b10001,0b10011,0b10101,0b11001,0b10001,0b00000}}}, // и
	{1081, {B_NOIDX, {0b01010,0b00100,0b10001,0b10011,0b10101,0b11001,0b10001,0b00000}}}, // й
	{1082, {B_NOIDX, {0b00000,0b00000,0b10010,0b10100,0b11000,0b10100,0b10010,0b00000}}}, // к
	{1083, {B_NOIDX, {0b00000,0b00000,0b00111,0b01001,0b01001,0b01001,0b10001,0b00000}}}, // л
	{1084, {B_NOIDX, {0b00000,0b00000,0b10001,0b11011,0b10101,0b10001,0b10001,0b00000}}}, // м
	{1085, {B_NOIDX, {0b00000,0b00000,0b10001,0b10001,0b11111,0b10001,0b10001,0b00000}}}, // н
	{1087, {B_NOIDX, {0b00000,0b00000,0b11111,0b10001,0b10001,0b10001,0b10001,0b00000}}}, // п
	{1090, {B_NOIDX, {0b00000,0b00000,0b11111,0b00100,0b00100,0b00100,0b00100,0b00000}}}, // т
	{1092, {B_NOIDX, {0b00000,0b00000,0b00100,0b01110,0b10101,0b01110,0b00100,0b00000}}}, // ф
	{1094, {B_NOIDX, {0b00000,0b00000,0b10010,0b10010,0b10010,0b10010,0b11111,0b00001}}}, // ц
	{1095, {B_NOIDX, {0b00000,0b00000,0b10001,0b10001,0b01111,0b00001,0b00001,0b00000}}}, // ч
	{1096, {B_NOIDX, {0b00000,0b00000,0b10101,0b10101,0b10101,0b10101,0b11111,0b00000}}}, // ш
	{1097, {B_NOIDX, {0b00000,0b00000,0b10101,0b10101,0b10101,0b10101,0b11111,0b00001}}}, // щ
	{1098, {B_NOIDX, {0b00000,0b00000,0b11000,0b01000,0b01110,0b01001,0b01110,0b00000}}}, // ъ
	{1099, {B_NOIDX, {0b00000,0b00000,0b10001,0b10001,0b11101,0b10011,0b11101,0b00000}}}, // ы
	{1100, {B_NOIDX, {0b00000,0b00000,0b10000,0b10000,0b11110,0b10001,0b11110,0b00000}}}, // ь
	{1101, {B_NOIDX, {0b00000,0b00000,0b01110,0b10001,0b00111,0b10001,0b01110,0b00000}}}, // э
	{1102, {B_NOIDX, {0b00000,0b00000,0b10010,0b10101,0b11101,0b10101,0b10010,0b00000}}}, // ю
	{1103, {B_NOIDX, {0b00000,0b00000,0b01111,0b10001,0b01111,0b00101,0b01001,0b00000}}}, // я
};

// The data must be manually clocked into the LCD controller by toggling
// the CLK (Enable) line after the data has been placed on D4-D7
// Display interface starts in 8-bit mode by default

// 8-bit mode sending
void LCD1602::send_8bit(uint8_t data)
{
	uint8_t data_arr[3];

	data_arr[0] = data;
	data_arr[1] = data | PIN_EN;
	data_arr[2] = data & ~PIN_EN;

	for(size_t i = 0; i < sizeof(data_arr); ++i){
		i2c_write_byte(this->address, data_arr[i]);
	}

	usleep(50);
}

// 4-bit mode sending
void LCD1602::send_4bit(uint8_t data, uint8_t flags)
{
	// PCF8574 connected with 4-bit interface to D4..D7 pins of HD44780
	// so one data transfer must be made in two operations for 4-bit data
	uint8_t up = data & 0xF0;
	uint8_t lo = (data << 4) & 0xF0;

	uint8_t data_arr[4];
	data_arr[0] = up | flags | this->backlight_flag | PIN_EN;
	data_arr[1] = up | flags | this->backlight_flag;
	data_arr[2] = lo | flags | this->backlight_flag | PIN_EN;
	data_arr[3] = lo | flags | this->backlight_flag;

	for(size_t i = 0; i < sizeof(data_arr); ++i){
		i2c_write_byte(this->address, data_arr[i]);
	}

	usleep(50);	// commands need > 37us to settle
} 

// Data sending through i2c port expander
void LCD1602::send_data(uint8_t data) 
{ 
	this->send_4bit(data, PIN_RS); 
}

void LCD1602::init(uint8_t lcd_addr, const std::string &i2c_dev) 
{
	if( !i2c_dev.empty() ){
		i2c_init(i2c_dev);
	}

	this->address = lcd_addr;

	// SEE PAGE 45/46 FOR INITIALIZATION SPECIFICATION!according to datasheet, 
	// we need at least 40ms after power rises above 2.7V before sending commands. 
	//usleep(50000);

	// We start in 8bit mode, try to set 4 bit mode
	// 4-bit mode activation (according to the hitachi HD44780 datasheet figure 24, pg 46)
	this->send_8bit(0b00110000);
	usleep(4500);	// wait for more than 4.1ms
	this->send_8bit(0b00110000);
	usleep(150);	// wait for more than 100us
	this->send_8bit(0b00110000);

	this->send_8bit(0b00100000);
	// 4-bit interface finally. Now commands can be sent

	// Function set: 2 lines, 5x8 font format
	this->display_function |= LCD_4BITMODE | LCD_2LINE | LCD_5x8DOTS;
	this->send_command(LCD_FUNCTIONSET | this->display_function);
	// usleep(2000);

	// display & cursor home
	this->return_home();
	this->current_row = 0;
	this->current_col = 0;

	// display on, right shift, underline off, blink off
	//this->send_command(0b00001100);

	// Set backlight ON by default
	this->control(true, false, false);
	this->display_mode = LCD_ENTRYLEFT | LCD_ENTRYSHIFTDECREMENT;
	// Set the entry mode
	this->send_command(LCD_ENTRYMODESET | this->display_mode);

	// Clear display 
	this->clear();

	this->reset_ru_symb_table();
}

// Control the backlight, cursor, and blink
// The cursor is an underline and is separate and distinct
// from the blinking block option
void LCD1602::control(bool backlight, bool cursor, bool blink)
{
	// Enable backlight ?
	if(backlight){
		this->backlight_flag = LCD_BACKLIGHT;
		this->display_control |= LCD_DISPLAYON;
    }
	else{
		this->backlight_flag = LCD_NOBACKLIGHT;
		this->display_control &= ~LCD_DISPLAYON;
    }

	// Dislpay cursor ?
	if(cursor){
		this->display_control |= LCD_CURSORON;
    }
	else{
		this->display_control &= ~LCD_CURSORON;
	}

	// Blinking cursor ?
	if(blink){
		this->display_control |= LCD_BLINKON;
    }
	else{
		this->display_control &= ~LCD_BLINKON;
	}

	this->send_command(LCD_DISPLAYCONTROL | this->display_control);
}

std::tuple<bool, bool, bool> LCD1602::get_control() const
{
	std::tuple<bool, bool, bool> res { 
		this->display_control & LCD_DISPLAYON, 
		this->display_control & LCD_CURSORON, 
		this->display_control & LCD_BLINKON };

	return res;
}

// Scroll the entire display without changing the RAM
void LCD1602::scroll_left(void) 
{
	this->send_command(LCD_CURSORSHIFT | LCD_DISPLAYMOVE | LCD_MOVELEFT);
}

void LCD1602::scroll_right(void) 
{
	this->send_command(LCD_CURSORSHIFT | LCD_DISPLAYMOVE | LCD_MOVERIGHT);
}

// Set text direction
void LCD1602::left_to_right(bool on_off) 
{
	if(on_off){
		// for text that flows Left to Right
		this->display_mode |= LCD_ENTRYLEFT;
	}
	else{
		// for text that flows Right to Left
		this->display_mode &= ~LCD_ENTRYLEFT;
	}
	
	this->send_command(LCD_ENTRYMODESET | this->display_mode);
}

// This will 'right justify' text from the cursor
void LCD1602::autoscroll(bool on_off) 
{
	if(on_off){
		this->display_mode |= LCD_ENTRYSHIFTINCREMENT;
	}
	// 'left justify' text from the cursor
	else{
		this->display_mode &= ~LCD_ENTRYSHIFTINCREMENT; // noautoscroll
	}
	
	this->send_command(LCD_ENTRYMODESET | this->display_mode);
}

// Allows to fill the first 8 CGRAM locations with custom characters
void LCD1602::user_char_create(uint8_t location, const uint8_t *charmap) 
{
	location &= 0x07; // we only have 8 locations (0-7)
	this->send_command(LCD_SETCGRAMADDR | (location << 3));

	for (int i = 0; i < 8; ++i) {
		this->send_data(charmap[i]);
	}
}

// Prints user-characters from CGRAM memory (location 0-7)
void LCD1602::user_char_print(uint8_t location)
{
	this->send_data(location);
	++current_col;
}

// Clears entire display and sets DDRAM address 0 in address counter
void LCD1602::clear()
{
	this->send_command(LCD_CLEARDISPLAY);
	usleep(2000); // this command takes a long time
}

// Return home sets DDRAM address 0 into the address counter, and returns the display to its 
// original status if it was shifted. The DDRAM contents do not change. The cursor or 
// blinking go to the left edge of the display (in the first line if 2 lines are displayed).
void LCD1602::return_home()
{
	this->send_command(LCD_RETURNHOME);
	usleep(2000); // this command takes a long time
}

// Set the LCD cursor position
void LCD1602::set_cursor(uint8_t row, uint8_t col)
{
	uint8_t row_offsets[] = { 0x00, 0x40, 0x14, 0x54 };

	if(row > this->num_rows){
		row = this->num_rows - 1;
	} 

	this->send_command(LCD_SETDDRAMADDR | (col + row_offsets[row]));
	this->current_row = row;
    this->current_col = col;
}

// --- Russian language support --- 

// Clear current software symbols indexes
void LCD1602::reset_ru_symb_table()
{
	this->current_symb_idx = 0;

	for(auto &kv : ru_symb_table){
		kv.second.bitmap_idx = B_NOIDX;
	}
}

// Prints existing or creates in CGDRAM than prints new Cyrrilic character
void LCD1602::print_ru_char(const uint8_t *charmap, uint8_t *index)
{
	// Print existing symbol
	if(*index != B_NOIDX){
		this->user_char_print(*index);
		return;
	}
  	// Create new symbol. After CGDRAM update, cursor pisiton is reset - saving current position.
	uint8_t row = get_current_row();
	uint8_t col = get_current_col();
	// New symbol will be placed to memory with current sign-generator index (from 0 to 7)
	this->user_char_create(this->current_symb_idx, charmap);
	this->set_cursor(row, col);
	this->user_char_print(this->current_symb_idx);

	// Save created character's index, and increment sign-generator index
	*index = this->current_symb_idx++;

	if(this->current_symb_idx >= B_MAXIDX){
		this->reset_ru_symb_table();
	}
}

// Mixed print - supports both ENG and RU symbols
// Scans wc to choose correct print method and add new RU symbol if neede
void LCD1602::print_wc(wchar_t wc) 
{
	// RU-letters that are equal to ENG symbols
	switch(wc){
		// А
		case 1040: this->print("A"); break;
		// В
		case 1042: this->print("B"); break;
		// Е
		// Ё
		case 1045: 
		case 1025: this->print("E"); break;
		// К
		case 1050: this->print("K"); break;
		// M
		case 1052: this->print("M"); break;
		// H
		case 1053: this->print("H"); break;
		// O
    	case 1054: this->print("O"); break;
    	// P
		case 1056: this->print("P"); break;
		// C
		case 1057: this->print("C"); break;
		// T
		case 1058: this->print("T"); break;
		// X 
		case 1061: this->print("X"); break;
		// а
		case 1072: this->print("a"); break;
		// е
		case 1077: this->print("e"); break;
		// o
		case 1086: this->print("o"); break;
		// p 
		case 1088: this->print("p"); break;
		// c
		case 1089: this->print("c"); break;
		// y
		case 1091: this->print("y"); break;
		// x
		case 1093: this->print("x"); break;
		// Знак градуса
		case 0x00B0: this->user_char_print(223); break;

		default:
		{
			// Check if symbol is RU
			auto it = ru_symb_table.find(wc);

			if(it != ru_symb_table.end()){
				this->print_ru_char(it->second.bitmap, &it->second.bitmap_idx);
				break;
			}

			// Else symbol is ENG - just print
			this->print_char(static_cast<char>(wc));
		}
	}
}

// Multibyte character to widechar convertion
// returns number of bytes
static size_t mbtowc(const char *in, wchar_t *out, uint8_t mb_num) 
{
	if(mb_num != 2){
		return 0;
	} 

	if( (in[0] & 0xC0) == 0xC0 && (in[1] & 0x80) == 0x80 ) {
		*out = ((in[0] & 0x1F) << 6) + (in[1] & 0x3F);
 		return 2;
	}
    else {
		*out = in[0];
		return 1;
	}
}

inline void LCD1602::align(size_t str_len, Alignment align_type)
{
	if((align_type == Alignment::NO) || (align_type == Alignment::LEFT)){
		return;
	}

	int shift = this->get_num_cols() - str_len;
	if(shift <= 0){
		return;
	}

	if(align_type == Alignment::CENTER){
		shift /= 2;
	}

	// Alignment::CENTER
	// Alignment::RIGHT
	while(shift--) {
		this->print_char(' ');
	}
}

// ENG string print
void LCD1602::print_str(const char *str, Alignment align_type) 
{
	align(strlen(str), align_type);

	while(*str) {
		this->print_char(*str);
		++str;
	}
}

void LCD1602::print(const char *fmt, ...)
{
	char str[128] = {0};

	va_list args;
	va_start(args, fmt);

	vsnprintf(str, sizeof(str), fmt, args);

	// Send data to print
	this->print_str(str, Alignment::NO);

	fmt = va_arg(args, const char*);

	// Cleaning
	va_end(args);
}

void LCD1602::print_with_padding(const std::string &str, char symb)
{	
	this->print_str(str.c_str(), Alignment::NO);

	int indent_len = this->get_num_cols() - this->get_current_col();

	if(indent_len > 0){
		std::string padding(indent_len, symb);
		this->print_str(padding.c_str(), Alignment::NO);
	}
}

// ENG + RU string print
// Cyrrilic symbols are software generated. Max 8 RU-letters on the screen at one time.
void LCD1602::print_ru(const char *str) 
{
	wchar_t wstr;
	size_t shift = 0;
	size_t size = std::strlen(str);

	while(shift < size){
		shift += mbtowc(str + shift, &wstr, 2);
		this->print_wc(wstr);
	}
}


// --- WH1602B_CTK implementation ---

// RU-symbol unicode -> ROM memory address.
const std::unordered_map<wchar_t, uint8_t> WH1602B_CTK::symb_codes = {
	{1025, 162}, // Ё
	{1041, 160}, // Б
	{1043, 161}, // Г
	{1044, 224}, // Д
	{1046, 163}, // Ж 
	{1047, 164}, // З
	{1048, 165}, // И
	{1049, 166}, // Й
	{1051, 167}, // Л
	{1055, 168}, // П
	{1059, 169}, // У
	{1060, 170}, // Ф
	{1062, 225}, // Ц
	{1063, 171}, // Ч
	{1064, 172}, // Ш
	{1065, 226}, // Щ
	{1066, 173}, // Ъ
	{1067, 174}, // Ы
	{1069, 175}, // Э
	{1070, 176}, // Ю
	{1071, 177}, // Я
	{1073, 178}, // б
	{1074, 179}, // в
	{1075, 180}, // г
	{1076, 227}, // д
	{1105, 181}, // ё
	{1078, 182}, // ж
	{1079, 183}, // з
	{1080, 184}, // и
	{1081, 185}, // й
	{1082, 186}, // к
	{1083, 187}, // л
	{1084, 188}, // м
	{1085, 189}, // н
	{1087, 190}, // п
	{1090, 191}, // т
	{1092, 228}, // ф
	{1094, 229}, // ц
	{1095, 192}, // ч
	{1096, 193}, // ш
	{1097, 230}, // щ
	{1098, 194}, // ъ
	{1099, 195}, // ы
	{1100, 196}, // ь
	{1101, 197}, // э
	{1102, 198}, // ю
	{1103, 199}, // я
};

// Mixed print. Uses ROM symbols table.
void WH1602B_CTK::print_wc(wchar_t wc) 
{
	// RU-letters that are equal to ENG symbols
	switch(wc){
		// А
		case 1040: this->user_char_print('A'); break;
		// В
		case 1042: this->user_char_print('B'); break;
		// Е
		case 1045: this->user_char_print('E'); break;
		// К
		case 1050: this->user_char_print('K'); break;
		// M
		case 1052: this->user_char_print('M'); break;
		// H
		case 1053: this->user_char_print('H'); break;
		// O
    	case 1054: this->user_char_print('O'); break;
    	// P
		case 1056: this->user_char_print('P'); break;
		// C
		case 1057: this->user_char_print('C'); break;
		// T
		case 1058: this->user_char_print('T'); break;
		// X 
		case 1061: this->user_char_print('X'); break;
		// Ь
		case 1068: this->user_char_print('b'); break;
		// а
		case 1072: this->user_char_print('a'); break;
		// е
		case 1077: this->user_char_print('e'); break;
		// o
		case 1086: this->user_char_print('o'); break;
		// p 
		case 1088: this->user_char_print('p'); break;
		// c
		case 1089: this->user_char_print('c'); break;
		// y
		case 1091: this->user_char_print('y'); break;
		// x
		case 1093: this->user_char_print('x'); break;
		// Знак градуса
		case 0x00B0: this->user_char_print(223); break;

		default:
		{
			auto it = symb_codes.find(wc);

			if(it != symb_codes.end()){
				this->user_char_print(it->second);
				break;
			}

			// ENG symbols are at the same addresses as their ASCII codes
			this->user_char_print(static_cast<const char>(wc));
		}
	}
}

void WH1602B_CTK::print_str(const char *str, Alignment align_type)
{
	wchar_t wstr = 0;
	size_t shift = 0;
	size_t bytes_num = 0;
	size_t symbols_num = number_of_symbols(str, &bytes_num);

	LCD1602::align(symbols_num, align_type);

	while(shift < bytes_num){
		shift += mbtowc(str + shift, &wstr, 2);
		this->print_wc(wstr);
	}
}

size_t number_of_symbols(const char *str, size_t *bytes_num)
{
	size_t bytes_count = 0;
	size_t symbols_num = 0;

	const unsigned char *ptr = reinterpret_cast<const unsigned char*>(str);

	while(*ptr){

		// Check UTF-8 octets (ASCII or Cyrillic)

		if((*ptr >> 7) == 0x00){
			// 1-byte char
			++ptr;
			++bytes_count;
		}
		else if((*ptr & 0xE0) == 0xC0){
			// 2-bytes char
			ptr += 2;
			bytes_count += 2;
		}
		else if((*ptr & 0xF0) == 0xE0){
			// 3-bytes char
			ptr += 3;
			bytes_count += 3;
		}
		else if((*ptr & 0xF8) == 0xF0){
			// 4-bytes char
			ptr += 4;
			bytes_count += 4;
		}
		else{
			// Unknown octet
			return 0;	
		}

		++symbols_num;
	}

	if(bytes_num){
		*bytes_num = bytes_count;
	}

	return symbols_num;
}