#include <iostream>
#include <stdio.h>
#include <unistd.h>

#include <math.h>
#include <float.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>

int main(int argc, char *argv[]){
	float SHUNT_OHMS = 0.1;
	float MAX_EXPECTED_AMPS = 0.2;
    int FD;
    int BUS_RANGE = 32;
    


	char *filename = (char*)"/dev/i2c-1";
	if ((FD = open(filename, O_RDWR)) < 0)
	{
		// NS_FATAL_ERROR("Failed to open the i2c bus, error number: " << _file_descriptor);
	}
	if (ioctl(FD, I2C_SLAVE, 0x40) < 0)
	{
		// NS_FATAL_ERROR("Failed to acquire bus access and/or talk to slave.");
	}

	
	
	float _min_device_current_lsb = 0.04096 / (SHUNT_OHMS * 0xFFFE);
	bool _auto_gain_enabled = false;


	int GAIN = 0;
    float GAIN_VOLTS = 0.04;

    uint16_t calibration = (BUS_RANGE << 13 | GAIN << 11 | 3 << 7 | 3 << 3 | 7);

    uint8_t buf[3];
	buf[0] = 0x00;
	buf[1] = (calibration >> 8) & 0xFF;
	buf[2] = calibration & 0xFF;
	
	if (write(FD, buf, 3) != 3)
	{
		// NS_FATAL_ERROR("Failed to write to the i2c bus.");
	}




	int c = 0;
	while(c < 5){
        float v;


    
        buf[0] = 0x02;
        if (write(FD, buf, 1) != 1) {
            // NS_FATAL_ERROR("Failed to set register.");
        }
        usleep(1000);
        if (read(FD, buf, 2) != 2) {
            // NS_FATAL_ERROR("Failed to read register value.");
        }
        v = (4.0 * (float)(buf[0] | (buf[1] << 8)) ) / 1000.0;

		std::cout << "Bus Voltave = " << v << "V" << std::endl;
		c++;
		sleep(1);
	}

	return 0;
}