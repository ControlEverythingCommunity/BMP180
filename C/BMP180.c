// Distributed with a free-will license.
// Use it any way you want, profit or free, provided it fits in the licenses of its associated works.
// BMP180
// This code is designed to work with the BMP180_I2CS I2C Mini Module available from ControlEverything.com.
// https://www.controleverything.com/content/Pressure?sku=BMP180_I2CS#tabs-0-product_tabset-2

#include <stdio.h>
#include <stdlib.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <math.h>

void main() 
{
	// Create I2C bus
	int file;
	char *bus = "/dev/i2c-1";
	if((file = open(bus, O_RDWR)) < 0) 
	{
		printf("Failed to open the bus. \n");
		exit(1);
	}
	// Get I2C device, VEML6070 I2C address is 0x77(119)
	ioctl(file, I2C_SLAVE, 0x77);

	// Calibration Cofficients stored in EEPROM of the device
	// Read 22 bytes of data from address 0xAA(170)
	char data[22] = {0};
	read(file, data, 22);

	// Convert the data
	int AC1 = data[0] * 256 + data[1];
	int AC2 = data[2] * 256 + data[3];
	int AC3 = data[4] * 256 + data[5];
	int AC4 = data[6] * 256 + data[7];
	int AC5 = data[8] * 256 + data[9];
	int AC6 = data[10] * 256 + data[11];
	int B1 = data[12] * 256 + data[13];
	int B2 = data[14] * 256 + data[15];
	int MB = data[16] * 256 + data[17];
	int MC = data[18] * 256 + data[19];
	int MD = data[20] * 256 + data[21];
	sleep(1);

	// Select measurement control register(0xF4)
	// Enable temperature measurement(0x2E)
	char config[2]= {0};
	config[0] = 0xF4;
	config[1] = 0x2E;
	write(file, config, 2);
	sleep(1);
	
	// Read 2 bytes of data from register(0xF6)
	// temp msb, temp lsb
	char reg[1] = {0xF6};
	write(file, reg, 1);
	if(read(file, data, 2) != 2)
	{
		printf("Erorr : Input/output Erorr \n");
		exit(1);
	}

	// Convert the data
	int temp = (data[0] * 256 + data[1]);

	// Select measurement control register(0xf4)
	// Enable pressure measurement, OSS = 1(0x74)
	config[0] = 0xF4;
	config[1] = 0x74;
	write(file, config, 2);
	sleep(1);
	
	// Read 3 bytes of data from register(0xF6)
	// pres msb1, pres msb, pres lsb
	reg[0] = 0xF6;
	write(file, reg, 1);
	read(file, data, 3);

	// Convert the data
	long pres = (data[0] * 65536 + (data[1] * 256) + data[2]) / 128;

	// Callibration for Temperature
	long X1 = (temp - AC6) * AC5 / 32768.0;
	long X2 = (MC * 2048.0) / (X1 + MD);
	long B5 = X1 + X2;
	long cTemp = ((B5 + 8.0) / 16.0) / 10.0;
	long fTemp = cTemp * 1.8 + 32;

	// Calibration for Pressure
	long B6 = B5 - 4000;
	X1 = (B2 * (B6 * B6 / 4096.0)) / 2048.0;
	X2 = AC2 * B6 / 2048.0;
	long X3 = X1 + X2;
	long B3 = (((AC1 * 4 + X3) * 2) + 2) / 4.0;
	X1 = AC3 * B6 / 8192.0;
	X2 = (B1 * (B6 * B6 / 2048.0)) / 65536.0;
	X3 = ((X1 + X2) + 2) / 4.0;
	long B4 = AC4 * (X3 + 32768) / 32768.0;
	long B7 = ((pres - B3) * (25000.0));
	long pressure = 0.0;
	if(B7 < 2147483648L)
	{
		pressure = (B7 * 2) / B4;
	}
	else
	{
		pressure = (B7 / B4) * 2;
	}
	X1 = (pressure / 256.0) * (pressure / 256.0);
	X1 = (X1 * 3038.0) / 65536.0;
	X2 = ((-7357) * pressure) / 65536.0;
	long pressure1 = (pressure + (X1 + X2 + 3791) / 16.0) / 100;

	// Calculate Altitude
	long altitude = 44330 * (1 - pow(pressure1/1013.25, 0.1903));

	// Output data to screen
	printf("Altitude : %.2f m \n", altitude);
	printf("Pressure : %.2f hPa \n", pressure1);
	printf("Temperature in Celsius : %.2f C \n", cTemp);
	printf("Temperature in Fahrenheit : %.2f F \n", fTemp);
}
