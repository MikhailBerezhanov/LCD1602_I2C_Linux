#include <iostream>
#include <string>
#include <cstring>

extern "C"{
#include <unistd.h>		// sleep
}

#include "i2c.hpp"
#include "lcd1602.hpp"

#ifndef VERSION
#define VERSION 	"1.1"
#endif

using namespace std;

static void LCD_test(const string &i2c_device, int argc, char **argv);

void show_usage()
{
	cout << "-- LCD1602 util v." << VERSION << " --\n\n";

	cout << "Call as following\n(provide i2c_device (as /dev/i2c-0) and [optional] i2c_address):\n\n";
	cout << "./lcd_util <i2c_dev> [addr <dec_addr>] <command>\n\n";

	cout << "List of supported commands:\n";
	cout << "\\_ init\t\t\t- first time init LCD\n";
	cout << "\\_ test\t\t\t- init and print test messages (default cmd)\n";
	cout << "\\_ bl <0 | 1>\t\t- disable backlight\n";
	cout << "\\_ c <0 | 1>\t\t- disable cursor\n";
	cout << "\\_ b <0 | 1>\t\t- disable blinking cursor\n";
	cout << "\\_ clear\t\t- clear screen\n";
	cout << "\\_ home\t\t\t- return cursor to 0,0 position\n";
	cout << "\\_ scroll <l | r>\t- scroll screen (left of right)\n";
	cout << "\\_ ltr <0 | 1>\t\t- enable left to right printing\n";
	cout << "\\_ autoscroll <0 | 1>\t- enable screen autoscroll\n";
	cout << "\\_ set_c <row , col>\t- set cursor position\n";
	cout << "\\_ print <str>\t\t- print string\n";
	cout << "\\_ printwc <unicode>\t- print unicode character" << endl;
}

int main(int argc, char* argv[])
{
	if(argc < 2){
		show_usage();
		return 0;
	}

	if(!strcmp(argv[1], "-V") || (!strcmp(argv[1], "--version"))){
		cout << VERSION << endl;
		return 0;
	}

	if(!strcmp(argv[1], "--help")){
		show_usage();
		return 0;
	}

	string i2c_dev = argv[1];

	try{
		LCD_test(i2c_dev, argc, argv);
	}
	catch(const exception &e){
		cerr << e.what() << endl;
		return 1;
	}

	return 0;
}

static void LCD_test(const string &i2c_device, int argc, char **argv)
{
	uint8_t lcd_addr = PCF8574A_ADDR;	// by default
	int cmd_idx = 2;

	if(argc >= 3){
		if((string(argv[2]) == "addr") && (argc > 3)){
			lcd_addr = (uint8_t)atoi(argv[3]);
			cmd_idx = 4;
		}
	}

	// In order to avoid init phase each util call, providing lcd_addr in the constructor
	LCD1602 lcd(lcd_addr);
	i2c_init(i2c_device);

	if(argc <= cmd_idx){
		cerr << "Invalid usage. See --help" << endl;
		return;
	}

	string cmd{argv[cmd_idx]};

	if(cmd == "test"){
		
		lcd.init(lcd_addr);

		cout << "Printing test messages..." << endl;

		for(int i = 0; i < 10; ++i){
			lcd.print_ru("ABCDEFG АБВГДЕёЖ");
			lcd.set_cursor(1, 0);
			lcd.print("HIJKLMN ");
			lcd.print_ru("ЗИЙКЛМНО");
			sleep(5);
			lcd.clear();

			lcd.print("OPQRSTU ");
			lcd.print_ru("ПРСТУФХЦ");
			lcd.set_cursor(1, 0);
			lcd.print("VWXYZ ");
			lcd.print_ru("ЧШЩЪЫЬЭЮЯ");
			sleep(5);
			lcd.clear();

			lcd.print_ru("abcdefg абвгдеёж");
			lcd.set_cursor(1, 0);
			lcd.print_ru("hijklmn зийклмно");
			sleep(5);
			lcd.clear();

			lcd.print_ru("opqrstu прстуфхц");
			lcd.set_cursor(1, 0);
			lcd.print_ru("vwxyz чшщъыьэюя");
			sleep(5);
			lcd.clear();
		}

		return;
	}

	// Инициализация необходима только вначале работы с дисплеем 1 раз
	if(cmd == "init"){
		int addr = static_cast<int>(lcd_addr);
		cout << "Trying lcd addr: " << addr << " (0x" << hex << addr << ")" << endl;
		lcd.init(lcd_addr);
	}
	else if(cmd == "bl"){	// Backlight
		bool on = false;
		if(argc > (cmd_idx + 1)){
			on = (atoi(argv[cmd_idx + 1])) ? true : false;
		}

		cout << "Setting lcd backlight: " << on << endl;
		lcd.control(on);
	} 
	else if(cmd == "c"){	// Cursor
		bool on = false;

		if(argc > (cmd_idx + 1)){
			on = (atoi(argv[cmd_idx + 1])) ? true : false;
		}

		cout << "Highlighting lcd cursor: " << on << endl;
	    lcd.control(true, on);
	}
	else if(cmd == "b"){	// Blinking cursor
		bool on = false;

		if(argc > (cmd_idx + 1)){
			on = (atoi(argv[cmd_idx + 1])) ? true : false;
		}

		cout << "Setting lcd blink: " << on << endl;
	    lcd.control(true, true, on);
	}
	else if(cmd == "clear"){
		lcd.clear();
	}
	else if(cmd == "home"){
		lcd.return_home();
	}
	else if(cmd == "scroll"){
		bool left = true;
		if(argc > (cmd_idx + 1)){
			left = (std::string(argv[cmd_idx + 1]) == "l") ? true : false;
		}

		if(left){
			lcd.scroll_left();
		}
		else{
			lcd.scroll_right();
		}
	}
	else if(cmd == "ltr"){
		bool on = false;

		if(argc > (cmd_idx + 1)){
			on = (atoi(argv[cmd_idx + 1])) ? true : false;
		}

		lcd.left_to_right(on);
	}
	else if(cmd == "autoscroll"){
		bool on = false;

		if(argc > (cmd_idx + 1)){
			on = (atoi(argv[cmd_idx + 1])) ? true : false;
		}

		lcd.autoscroll(on);
	}
	else if(cmd == "set_c"){
		uint8_t row = 0;
		uint8_t col = 0;

		if(argc > (cmd_idx + 2)){
			row = (uint8_t)atoi(argv[cmd_idx + 1]);
			col = (uint8_t)atoi(argv[cmd_idx + 2]);
		}

		cout << "Setting lcd cursor to: " << row << "," << col << endl;
		lcd.set_cursor(row, col);
	}
	else if(cmd == "print"){
		if(argc <= (cmd_idx + 1)){
			cerr << "No text string provided for puts." << endl;
			return;
		}

		lcd.print(argv[3]);
	}
	else if(cmd == "printwc"){
		if(argc <= (cmd_idx + 1)){
			cerr << "No char code provided for putwc." << endl;
			return;
		}

		wchar_t wc = atoi(argv[3]); // symbol unicode
		lcd.print_ru(wc);
	}
	else{
		cerr << "Unsupported cmd: " << cmd << endl;
		return;
	}
}