/*
 * ADXL345.c
 *
 *  Created on: 14 ott 2017
 *      Author: calta
 */
#include "ADXL345.h"

// Init the i2c connection with the accelerometer
void initI2C()
{
	i2c_config_t conf;
	conf.mode = I2C_MODE_MASTER;
	conf.sda_io_num = SDA_PIN;
	conf.scl_io_num = SCL_PIN;
	conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
	conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
	conf.master.clk_speed = 100000;
	i2c_param_config(I2C_NUM_0, &conf);
	i2c_driver_install(I2C_NUM_0, I2C_MODE_MASTER, 0, 0, 0);
}



void initADXL345()
{
	adxl345I2CAddress = ADXL345_DEFAULT_ADDRESS;
	//reset the powe mngt registry
	i2cWriteByte(adxl345I2CAddress, ADXL345_RA_POWER_CTL, 0);
	//now enable the measurements
	setMeasureEnabled(true); //enable the meas
	setAutoSleepEnabled(false);
	//set the maximum sampling rate
	i2cWriteByte(adxl345I2CAddress, ADXL345_RA_BW_RATE, ADXL345_RATE_3200);

}


/** Get Device ID.
 * The DEVID register holds a fixed device ID code of 0xE5 (345 octal).
 * @return Device ID (should be 0xE5, 229 dec, 345 oct)
 * @see ADXL345_RA_DEVID
 */
uint8_t getDeviceID()
{
	uint8_t id;
	esp_err_t espRc;
	// send the request
	i2c_cmd_handle_t cmd = i2c_cmd_link_create();
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, (adxl345I2CAddress << 1) | I2C_MASTER_WRITE, 1);
	i2c_master_write_byte(cmd,  ADXL345_RA_DEVID, 1);
	i2c_master_stop(cmd);
	espRc = i2c_master_cmd_begin(I2C_NUM_0, cmd, 10 / portTICK_RATE_MS);
	i2c_cmd_link_delete(cmd);
	cmd = i2c_cmd_link_create();
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, (adxl345I2CAddress << 1) | I2C_MASTER_READ, 1);
	i2c_master_read_byte(cmd, &id, 1);
	i2c_master_stop(cmd);
	espRc = i2c_master_cmd_begin(I2C_NUM_0, cmd, 10 / portTICK_RATE_MS);
	i2c_cmd_link_delete(cmd);
	return id;

}

void setMeasureEnabled(bool enabled)
{
	i2cWiteBit(adxl345I2CAddress, ADXL345_RA_POWER_CTL, ADXL345_PCTL_MEASURE_BIT, enabled);
}

uint8_t i2cReadByte(uint8_t slaveAddress, uint8_t registerAddress)
{
	uint8_t id;
	esp_err_t espRc;
	// send the request
	i2c_cmd_handle_t cmd = i2c_cmd_link_create();
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, (slaveAddress << 1) | I2C_MASTER_WRITE, 1);
	i2c_master_write_byte(cmd,  registerAddress, 1);
	i2c_master_stop(cmd);
	espRc = i2c_master_cmd_begin(I2C_NUM_0, cmd, 10 / portTICK_RATE_MS);
	i2c_cmd_link_delete(cmd);
	cmd = i2c_cmd_link_create();
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, (slaveAddress << 1) | I2C_MASTER_READ, 1);
	i2c_master_read_byte(cmd, &id, 1);
	i2c_master_stop(cmd);
	espRc = i2c_master_cmd_begin(I2C_NUM_0, cmd, 10 / portTICK_RATE_MS);
	i2c_cmd_link_delete(cmd);
	return id;
}

void i2cWriteByte(uint8_t slaveAddress, uint8_t registerAddress, uint8_t data)
{
	i2c_cmd_handle_t cmd = i2c_cmd_link_create();
	//start the message creation
	i2c_master_start(cmd);
	//write the desired byte to the selected slave
	//perform the byteshift to the address in order to add as lsb the type of request (read or write)
	i2c_master_write_byte(cmd, (slaveAddress << 1) | I2C_MASTER_WRITE, 1 /* expect ack */);
	i2c_master_write_byte(cmd, registerAddress, 1);
	i2c_master_write_byte(cmd, data, 1); //reset all power settings
	i2c_master_stop(cmd);
	//send the message
	i2c_master_cmd_begin(I2C_NUM_0, cmd, 10/ portTICK_PERIOD_MS);
	//delete the cmd
	i2c_cmd_link_delete(cmd);
}

void i2cWiteBit(uint8_t slaveAddress, uint8_t registerAddress, uint8_t bitPosition, bool val)
{
	uint8_t regValue = i2cReadByte(slaveAddress, registerAddress);
	if (bitPosition < 8)
	{
		//change only the right bit
		regValue ^= (-val ^ regValue) & (1 << bitPosition);
		i2cWriteByte(slaveAddress, registerAddress, regValue);
	}
}

void setAutoSleepEnabled(bool enabled)
{
	i2cWiteBit(adxl345I2CAddress, ADXL345_RA_POWER_CTL, ADXL345_PCTL_AUTOSLEEP_BIT, enabled);
}

void getAcceleration(int16_t* x, int16_t* y, int16_t* z)
{
//	int16_t* tempx, tempy, tempz;
//	tempx = x;
//	tempy = y;
//	tempz = z;
	uint8_t val1M, val1L, val2M, val2L, val3M, val3L;
	esp_err_t espRc;
	// send the request
	i2c_cmd_handle_t cmd = i2c_cmd_link_create();
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, (adxl345I2CAddress << 1) | I2C_MASTER_WRITE, 1);
	i2c_master_write_byte(cmd,  ADXL345_RA_DATAX0, 1);
	i2c_master_stop(cmd);
	espRc = i2c_master_cmd_begin(I2C_NUM_0, cmd, 10 / portTICK_RATE_MS);
	i2c_cmd_link_delete(cmd);
	cmd = i2c_cmd_link_create();
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, (adxl345I2CAddress << 1) | I2C_MASTER_READ, 1);
	i2c_master_read_byte(cmd, &val1L, 0);
	i2c_master_read_byte(cmd, &val1M, 0);
	i2c_master_read_byte(cmd, &val2L, 0);
	i2c_master_read_byte(cmd, &val2M, 0);
	i2c_master_read_byte(cmd, &val3L, 0);
	i2c_master_read_byte(cmd, &val3M, 1);
	i2c_master_stop(cmd);
	espRc = i2c_master_cmd_begin(I2C_NUM_0, cmd, 10 / portTICK_RATE_MS);
	i2c_cmd_link_delete(cmd);

	int16_t x_accel = ((int16_t)val1M << 8) | val1L;
	int16_t y_accel = ((int16_t)val2M << 8) | val2L;
	int16_t z_accel = ((int16_t)val3M << 8) | val3L;
	*x = x_accel;
	*y = y_accel;
	*z = z_accel;
}


int16_t getAccelerationX()
{
	uint8_t val1;
	uint8_t val2;
	esp_err_t espRc;
	// send the request
	i2c_cmd_handle_t cmd = i2c_cmd_link_create();
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, (adxl345I2CAddress << 1) | I2C_MASTER_WRITE, 1);
	i2c_master_write_byte(cmd,  ADXL345_RA_DATAX0, 1);
	i2c_master_stop(cmd);
	espRc = i2c_master_cmd_begin(I2C_NUM_0, cmd, 10 / portTICK_RATE_MS);
	i2c_cmd_link_delete(cmd);
	cmd = i2c_cmd_link_create();
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, (adxl345I2CAddress << 1) | I2C_MASTER_READ, 1);
	i2c_master_read_byte(cmd, &val1, 0);
	i2c_master_read_byte(cmd, &val2, 1);
	i2c_master_stop(cmd);
	espRc = i2c_master_cmd_begin(I2C_NUM_0, cmd, 10 / portTICK_RATE_MS);
	i2c_cmd_link_delete(cmd);
	return ((int16_t)val2 << 8) | val1;
}
int16_t getAccelerationY()
{
	uint8_t val1;
	uint8_t val2;
	esp_err_t espRc;
	// send the request
	i2c_cmd_handle_t cmd = i2c_cmd_link_create();
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, (adxl345I2CAddress << 1) | I2C_MASTER_WRITE, 1);
	i2c_master_write_byte(cmd,  ADXL345_RA_DATAY0, 1);
	i2c_master_stop(cmd);
	espRc = i2c_master_cmd_begin(I2C_NUM_0, cmd, 10 / portTICK_RATE_MS);
	i2c_cmd_link_delete(cmd);
	cmd = i2c_cmd_link_create();
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, (adxl345I2CAddress << 1) | I2C_MASTER_READ, 1);
	i2c_master_read_byte(cmd, &val1, 0);
	i2c_master_read_byte(cmd, &val2, 1);
	i2c_master_stop(cmd);
	espRc = i2c_master_cmd_begin(I2C_NUM_0, cmd, 10 / portTICK_RATE_MS);
	i2c_cmd_link_delete(cmd);
	return ((int16_t)val2 << 8) | val1;
}
int16_t getAccelerationZ()
{
	uint8_t val1;
	uint8_t val2;
	esp_err_t espRc;
	// send the request
	i2c_cmd_handle_t cmd = i2c_cmd_link_create();
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, (adxl345I2CAddress << 1) | I2C_MASTER_WRITE, 1);
	i2c_master_write_byte(cmd,  ADXL345_RA_DATAZ0, 1);
	i2c_master_stop(cmd);
	espRc = i2c_master_cmd_begin(I2C_NUM_0, cmd, 10 / portTICK_RATE_MS);
	i2c_cmd_link_delete(cmd);
	cmd = i2c_cmd_link_create();
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, (adxl345I2CAddress << 1) | I2C_MASTER_READ, 1);
	i2c_master_read_byte(cmd, &val1, 0);
	i2c_master_read_byte(cmd, &val2, 1);
	i2c_master_stop(cmd);
	espRc = i2c_master_cmd_begin(I2C_NUM_0, cmd, 10 / portTICK_RATE_MS);
	i2c_cmd_link_delete(cmd);
	return ((int16_t)val2 << 8) | val1;
}

void task_i2cscanner(void *ignore) {
	ESP_LOGD(tag, ">> i2cScanner");
	i2c_config_t conf;
	conf.mode = I2C_MODE_MASTER;
	conf.sda_io_num = SDA_PIN;
	conf.scl_io_num = SCL_PIN;
	conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
	conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
	conf.master.clk_speed = 100000;
	i2c_param_config(I2C_NUM_0, &conf);

	i2c_driver_install(I2C_NUM_0, I2C_MODE_MASTER, 0, 0, 0);

	int i;
	esp_err_t espRc;
	printf("     0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f\n");
	printf("00:         ");
	for (i=3; i< 0x78; i++) {
		i2c_cmd_handle_t cmd = i2c_cmd_link_create();
		i2c_master_start(cmd);
		i2c_master_write_byte(cmd, (i << 1) | I2C_MASTER_WRITE, 1 /* expect ack */);
		i2c_master_stop(cmd);

		espRc = i2c_master_cmd_begin(I2C_NUM_0, cmd, 10/portTICK_PERIOD_MS);
		if (i%16 == 0) {
			printf("\n%.2x:", i);
		}
		if (espRc == 0) {
			printf(" %.2x", i);
		} else {
			printf(" --");
		}
		//ESP_LOGD(tag, "i=%d, rc=%d (0x%x)", i, espRc, espRc);
		i2c_cmd_link_delete(cmd);
	}
	printf("\n");
	vTaskDelete(NULL);
}

void printSomething()
{
	printf("Something\n");
}

