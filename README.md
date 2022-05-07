# LCD1602 I2C Linux driver
### with custom and RU characters support


This driver is for following hardware:

* __HD44780__ controller -based LCDs
* [__WH1602B__](https://www.chipdip.ru/product/wh1602b-yyh-ctk) family LCDs
* controllers analogue to HD44780 -based LCDs

connected with __PCF8574(A)__ I2C port expander.

Connection conditions:

* 5V power-supply (both LCD and I2C logic levels of SDA, SCL)
* I2C clock rates: 100 kHz or 400 kHz

![](https://habrastorage.org/webt/3k/dg/yx/3kdgyxcwq-ixmbmqn3pcpshpft4.jpeg)


### Compiling and Building

To use this driver in your project just add `i2c.cpp` , `i2c.hpp`, `lcd1602.cpp`, `lcd1602.hpp` files to your sources.   Compilation (linkage) should be done with `-lpthread` to make i2c operation thread-safe.  

```sh
# Buildong with your main and g++ compiler
g++ your_main.cpp i2c.cpp lcd1602.cpp -lpthread 
```

For more details `Makefile` as an example provided.

### Driver API
First of all include lcd1602.hpp to your project and init lcd before any other methods:

```C
#include "lcd1602.hpp"

int main(int argc, char* argv[])
{
	LCD1602 lcd;
	lcd.init(PCF8574A_ADDR, "/dev/i2c-0");

	return 0;
}
```

If some other devices are connected to i2c bus and you want to work with it using i2c.cpp interface, it's possible to init i2c separately from lcd. In that case no need to provide i2c_dev in lcd.init().  

```C
#include "i2c.hpp"
#include "lcd1602.hpp"

int main(int argc, char* argv[])
{
	i2c_init("/dev/i2c-0");

	// Read some device's registers
	uint8_t buf[2];
	i2c_read(0x34, 0x0012, buf, sizeof buf);

	// ...

	LCD1602 lcd;
	lcd.init(PCF8574A_ADDR);

	return 0;
}
```

After lcd.init() performed, it is ready for methods calls:

* `clear()` - clear the screen
* `control(bool backlight, bool cursor = false, bool blink = false)` - enable backlight, cursor indication, cursor blinking
* `return_home()` - move cursor at position (0, 0)
* `set_cursor(uint8_t row, uint8_t col)` - set cursor at position (row, col)
* `scroll_right()` - move all the screen to the right side by 1 position
* `scroll_left()` - move all the screen to the left side by 1 position
* `left_to_right(bool on_off)` - enable 
* `autoscroll(bool on_off)` - enable screen autoscroll
* `get_addr()` - get curret i2c address
* `get_current_row()` - get cursor's row position
* `get_current_col()` - get cursor's column position
* `get_control()`  - get backligh, cursor indication, cursor blinking states
* `print(const std::string &str)` - print ENG string on the screen
* `print_ru(const std::string &str)` - print RU string on the screen

_Additionally, driver supports WH1602B_CTK implementation, that has hardware Cyrillic characters._

#### Custon characters

Driver supports custom character adding with special methods:

* `user_char_create(uint8_t location, const uint8_t *charmap)` - saves charmap at determined memory location (supported locations are 0..7).
* `user_char_print(uint8_t location)` - print character from determined location on the screen.

All characters are 5x8 bits, so charmap should be an array representing character bitmap

```C
// this is custom '2'
uint8_t charmap[8] = {
	0b11111,
	0b00001,
	0b00001,
	0b11111,
	0b10000,
	0b10000,
	0b11111,
	0b00000
};
```

You can also use [LCD Custom Character Generator](https://maxpromer.github.io/LCD-Character-Creator/)


_NOTE: `print_ru` uses custom characters feature, so calling this method will overwite
user defined set_


### Utility

For functionality tests utility can be built by caling _make_

```sh
# Build LCD control command line utilidy for your host system 
make
```

When crosscompilation needed, global variable __CXX__ should be set before _make_ call.

```sh
# Setup environment for yout target (toolchain)
export CXX=arm-linux-gnueabihf-g++
# Build LCD control command line utilidy for your target system 
make
```

Usage: provide __i2c_device__ (as /dev/i2c-0) and [optional] __i2c_address__ (by default PCF8574A (0x7E) address will be used)

`./lcd_util <i2c_dev> [addr <dec_addr>] <command>`

List of supported commands:

* `init`- init LCD interface (should be called once before using any other command);
* `test`- init and print test messages;
* `bl <0 | 1>`- 0 - disable backlight, 1 - enable (default 0);
* `c <0 | 1>`- 0 - disable cursor, 1 - enable (default 0);
* `b <0 | 1>`- disable blinking cursor (default 0);
* `clear`- clear the screen;
* `home`- return cursor to 0,0 position;
* `scroll <l | r>`- scroll screen (left of right);
* `ltr <0 | 1>`- enable left to right printing;
* `autoscroll <0 | 1>`- enable screen autoscroll;
* `set_c <row , col>`- set cursor position;
* `print <str>`- print string;
* `printwc <unicode>`- print unicode character;

Additional keys:
* `-V`, `--version`
* `--help`

```sh
# Check version
./lcd_util -V
# Usage example
./lcd_util /dev/i2c-0 addr 127 init
./lcd_util /dev/i2c-0 print "hello World"
./lcd_util /dev/i2c-0 printwc 223 
```

 

