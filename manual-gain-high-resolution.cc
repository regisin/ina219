#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include "ina219.h"

int main(int argc, char *argv[]){

	std::cout << "Manual Gain High Resolution example" << std::endl;

	float SHUNT_OHMS = 0.1;
    float MAX_EXPECTED_AMPS = 3.0;

	INA219 i(SHUNT_OHMS, MAX_EXPECTED_AMPS);
	i.configure(RANGE_16V, GAIN_1_40MV, ADC_12BIT, ADC_12BIT);

	int c = 0;
	while(c < 5){
		std::cout << "---T: " << c << std::endl;
		std::cout << "Bus Voltave = " << i.voltage() << " V" << std::endl;
		std::cout << "Supply Voltave = " << i.supply_voltage() << " V" << std::endl;
		std::cout << "Shunt Voltave = " << i.shunt_voltage() << " mV" << std::endl;
		std::cout << "Current = " << i.current() << " mA" << std::endl;
		std::cout << "Power = " << i.power() << " mW" << std::endl;
		c++;
		sleep(1);
	}

	return 0;
}