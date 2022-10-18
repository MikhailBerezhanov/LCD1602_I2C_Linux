#include <stdexcept>
#include <string>
#include <mutex>
#include <memory>
#include <cstring>
#include <cerrno>
#include <cstdio>

extern "C"{
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
}

#include "i2c.hpp"

#define ARRAY_SIZE(a)   (sizeof(a) / sizeof(*a))

namespace hw{

static std::string dev_ = "/dev/i2c-5"; 	// устройство i2c в ОС 
static std::mutex mutex_; 					// синхронизация совместного доступа к шине i2c

// Инициализация устройства I2C
void i2c_init(const std::string &dev)
{
	std::lock_guard<std::mutex> lck(mutex_);
	dev_ = dev;
}

// Интерфейс приемопередачи данных по I2C
static bool i2c_rdwr(struct i2c_msg *msgs, int nmsgs)
{
	struct i2c_rdwr_ioctl_data msgset;
	msgset.msgs = msgs;
	msgset.nmsgs = nmsgs;

	if(msgs == nullptr || nmsgs <= 0){
		return false;
	} 

	std::lock_guard<std::mutex> lck(mutex_);

	int fd = open(dev_.c_str(), O_RDWR);
	if(fd < 0){
		throw std::runtime_error(std::string("open device '") + dev_ + "' failed: " + strerror(errno));
	} 

	if(ioctl(fd, I2C_RDWR, &msgset) < 0){
		close(fd);
		return false;
	} 

	close(fd);
	return true;
}

/**
  * @описание   Передача данных по линии I2C 
  * @параметры
  *     Входные:
  *         slave_address - адрес подчиненного устройства, которому отправляются данные
  *         reg - адрес регистра подчиненного устройства, в который будет запись
  *         *buf - указатель на массив байт данных
  *         len - размер массива данных
  * @исключения: std::runtime_error
 */
void i2c_write(uint8_t slave_address, uint16_t reg, const uint8_t *buf, uint16_t len)
{
	uint16_t reg_len = (reg > 0xFF) ? 2 : 1;
	uint16_t data_len = reg_len + len;
	struct i2c_msg msgs[1];
	errno = 0;

	std::unique_ptr<uint8_t[]> data ( new (std::nothrow) uint8_t[data_len] );

	if( !data ){
		throw std::runtime_error(std::string("i2c_write: data alloc failed. len: ") + std::to_string(len));
	} 

	msgs[0].addr = slave_address >> 1;
	msgs[0].flags = 0;
	msgs[0].buf = data.get();
	msgs[0].len = data_len;

	if(reg_len == 2){
		data[0] = reg >> 8;
		data[1] = reg & 0xFF;
		memcpy(data.get() + 2, buf, len);
	}
	else{
		data[0] = reg;
		memcpy(data.get() + 1, buf, len);
	}

	if( !i2c_rdwr(msgs, ARRAY_SIZE(msgs)) ) {
		throw std::runtime_error(std::string("i2c_write error (addr: " + std::to_string(slave_address) + ") - ") + strerror(errno)); 
	}
}

// Поддержка SMBus передачи
void i2c_write_byte(uint8_t slave_address, uint8_t byte)
{
	errno = 0;

	struct i2c_msg msgs[1];
	unsigned char write_buf[1];

	write_buf[0] = byte;

	msgs[0].addr = slave_address >> 1;
	msgs[0].flags = 0;
	msgs[0].len = 1;
	msgs[0].buf = write_buf;

	if( !i2c_rdwr(msgs, ARRAY_SIZE(msgs)) ) {
		throw std::runtime_error(std::string("i2c_write_byte error (addr: " + std::to_string(slave_address) + ") - ") + strerror(errno)); 
	}
}

/**
  * @описание   Чтение данных по линии I2C
  * @параметры
  *     Входные:
  *         slave_address - адрес подчиненного устройства, которому отправляются данные
  *         reg - адрес регистра подчиненного устройства, из которого будет чтение
  *     Выходные:
  *         *buf - указатель на буфер с прочитанным данными
  *         len - размер буфера с прочитанными данными
  * @исключения: std::runtime_error
 */
void i2c_read(uint8_t slave_address, uint16_t reg, uint8_t *buf, uint16_t len)
{
	uint16_t reg_len = (reg > 0xFF) ? 2 : 1;;
	uint8_t reg_data[2];
	struct i2c_msg msgs[2];
	errno = 0;

	if(reg_len == 2){
		reg_data[0] = reg >> 8;
		reg_data[1] = reg & 0xFF;
	}
	else{
		reg_data[0] = reg;
	}

	msgs[0].addr = slave_address >> 1;
	msgs[0].flags = 0;
	msgs[0].buf = reg_data;
	msgs[0].len = reg_len;
	msgs[1].addr = slave_address >> 1;
	msgs[1].flags = I2C_M_RD;
	msgs[1].buf = buf;
	msgs[1].len = len;

	if( !i2c_rdwr(msgs, ARRAY_SIZE(msgs)) ) {
		throw std::runtime_error(std::string("i2c_read error (addr: " + std::to_string(slave_address) + ") -") + strerror(errno));
	}
}

} // namespace hw
