#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>

#include "i2c.h"

/* Constructor */
I2C::I2C(uint8_t address)
{
    device_address = address;

    char *filename = (char*)"/dev/i2c-1";

	if ((_file_descriptor = open(filename, O_RDWR)) < 0)
	{
		// "Failed to open the i2c bus: " << _file_descriptor
	}
	if (ioctl(_file_descriptor, I2C_SLAVE, address) < 0)
	{
		// "Failed to acquire bus access and/or talk to slave."
	}
}

void
I2C::write_register(uint8_t register_address, uint16_t register_value)
{
    uint8_t buf[3];
	buf[0] = register_address;
	buf[1] = (register_value >> 8) & 0xFF;
	buf[2] = register_value & 0xFF;
	
	if (write(_file_descriptor, buf, 3) != 3)
	{
		// "Failed to write to the i2c bus."
	}
}

uint16_t
I2C::read_register(uint8_t register_address)
{
    uint8_t buf[3];
	
    buf[0] = register_address;

	if (write(_file_descriptor, buf, 1) != 1) {
		// "Failed to set register."
	}

	usleep(1000);

	if (read(_file_descriptor, buf, 2) != 2) {
		// "Failed to read register value."
	}
	return buf[0] | (buf[1] >> 8);
}