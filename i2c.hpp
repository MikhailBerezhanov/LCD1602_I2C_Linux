#pragma once

#include <cstdint>
#include <string>

namespace hw{

// Инициализация I2C с указанием используемого устройства (например, /dev/i2c-5)
void i2c_init(const std::string &dev);

/**
  * @описание	Передача данных по линии I2C 
  * @параметры
  *     Входные:
  * 		slave_address - адрес подчиненного устройства, которому отправляются данные
  * 		reg - адрес регистра подчиненного устройства, в который будут записаны данные
  *			*buf - указатель на массив байт данных
  * 		len - буфер для хранения строки-результата выполнения команды
  * @исключения: std::runtime_error
 */
void i2c_write(uint8_t slave_address, uint16_t reg, const uint8_t *buf, uint16_t len);

// Поддержка SMBus передачи
void i2c_write_byte(uint8_t slave_address, uint8_t byte);

/**
  * @описание   Чтение данных по линии I2C
  * @параметры
  *     Входные:
  * 		slave_address - адрес подчиненного устройства, которому отправляются данные
  * 		reg - адрес регистра подчиненного устройства, из которого будет чтение
  *     Выходные:
  *			*buf - указатель на буфер с прочитанным данными
  * 		len - размер буфера с прочитанными данными
  * @исключения: std::runtime_error
 */
void i2c_read(uint8_t slave_address, uint16_t reg, uint8_t *buf, uint16_t len);

} // namespace hw