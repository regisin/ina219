#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include "ina219.h"

int main(int argc, char *argv[]){

	float SHUNT_OHMS = 0.1;
	float MAX_EXPECTED_AMPS = 0.2;

	INA219 i(SHUNT_OHMS, MAX_EXPECTED_AMPS);
	i.configure(RANGE_32V, GAIN_AUTO, ADC_12BIT, ADC_12BIT);

	int c = 0;
	while(c <= 10){
		std::cout << "Bus Voltave = " << i.voltage() << "V" << std::endl;
		c++;
		sleep(1);
	}

	return 0;
}