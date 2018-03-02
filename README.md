# Raspberry Pi C++ Library for Voltage and Current Sensors Using the INA219

*This is a C++ version of the Python library for the same sensor, that can be found [HERE](https://github.com/chrisb2/pi_ina219).*




This C++ library supports the [INA219](http://www.ti.com/lit/ds/symlink/ina219.pdf) voltage, current and power monitor sensor from Texas Instruments. The intent of the library is to make it easy to use the quite complex functionality of this sensor.  

The library currently only supports _continuous_ reads of voltage and power, but not _triggered_ reads.

The library supports the detection of _overflow_ in the current/power calculations which results in meaningless values for these readings.

The low power mode of the INA219 is supported, so if only occasional reads are being made in a battery based system, current consumption can be minimized.

## Installation and Build

Build:

```shell
git clone https://github.com/regisin/pi_ina219cpp.git
cd pi_ina219cpp
make all
```

Running examples after compiling _all_:

```shell
cd pi_ina219cpp/build/examples

./simple-auto-gain
./auto-gain-high-resolution
./manual-gain-high-resolution
```

## Usage

The address of the sensor unless otherwise specified is the default of _0x40_.

Note that the bus voltage is that on the load side of the shunt resister, if you want the voltage on the supply side then you should add the bus voltage and shunt voltage together, or use the *supply_voltage()* function.

### Simple - Auto Gain

This mode is great for getting started, as it will provide valid readings until the device current capability is exceeded for the value of the shunt resistor connected (3.2A for 0.1&Omega; shunt resistor). It does this by automatically adjusting the gain as required until the maximum is reached.

The downside of this approach is reduced current and power resolution.


```cpp
#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include "ina219.h"

int main(int argc, char *argv[]){

	std::cout << "Simple Auto Gain example" << std::endl;

	float SHUNT_OHMS = 0.1;

	INA219 i(SHUNT_OHMS);
	i.configure(RANGE_32V, GAIN_AUTO, ADC_12BIT, ADC_12BIT);

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
```

### Advanced - Auto Gain, High Resolution

In this mode by understanding the maximum current expected in your system and specifying this in the script you can achieve the best possible current and power resolution. The library will calculate the best gain to achieve the highest resolution based on the maximum expected current.

In this mode if the current exceeds the maximum specified, the gain will be automatically increased, so a valid reading will still result, but at a lower resolution.

```cpp
#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include "ina219.h"

int main(int argc, char *argv[]){

	std::cout << "Simple Auto Gain example" << std::endl;

	float SHUNT_OHMS = 0.1;
    float MAX_EXPECTED_AMPS = 3.0;

	INA219 i(SHUNT_OHMS, MAX_EXPECTED_AMPS);
	i.configure(RANGE_16V, GAIN_AUTO, ADC_12BIT, ADC_12BIT);

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
```

### Advanced - Manual Gain, High Resolution

In this mode by understanding the maximum current expected in your system and specifying this and the gain in the script you can always achieve the  best possible current and power resolution, at the price of missing current and power values if a current overflow occurs.

```cpp
#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include "ina219.h"

int main(int argc, char *argv[]){

	std::cout << "Simple Auto Gain example" << std::endl;

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
```

### Sensor Address

The sensor address may be altered as follows:

```cpp
INA219 i(SHUNT_OHMS, 0x41);

//or

INA219 i(SHUNT_OHMS, MAX_EXPECTED_AMPS, 0x41);
```

### Low Power Mode

The sensor may be put in low power mode between reads as follows:

```cpp
i.configure(RANGE_16V, GAIN_AUTO, ADC_12BIT, ADC_12BIT);
while (True)
{
    std::cout << "Bus voltage: " << i.voltage() << " V" << std::endl;
    i.sleep();
    sleep(60);
    i.wake();
}
```

Note that if you do not wake the device after sleeping, the value returned from a read will be the previous value taken before sleeping.

## Functions

* `INA219()` constructs the class.
The arguments, are:
    * shunt_ohms: The value of the shunt resistor in Ohms (mandatory).
    * max_expected_amps: The maximum expected current in Amps (optional). **Device only supports upto 3.2A.**
    * address: The I2C address of the INA219, defaults to *0x40* (optional).
* `configure()` configures and calibrates how the INA219 will take measurements.
The arguments, which are all mandatory, are:
    * voltage_range: The full scale voltage range, this is either 16V or 32V, 
    represented by one of the following constants (optional).
        * RANGE_16V: Range zero to 16 volts
        * RANGE_32V: Range zero to 32 volts (**default**). **Device only supports upto 26V.**
    * gain: The gain, which controls the maximum range of the shunt voltage, 
        represented by one of the following constants (optional). 
        * GAIN_1_40MV: Maximum shunt voltage 40mV
        * GAIN_2_80MV: Maximum shunt voltage 80mV
        * GAIN_4_160MV: Maximum shunt voltage 160mV
        * GAIN_8_320MV: Maximum shunt voltage 320mV
        * GAIN_AUTO: Automatically calculate the gain (**default**)
    * bus_adc: The bus ADC resolution (9, 10, 11, or 12-bit), or
        set the number of samples used when averaging results, represented by
        one of the following constants (optional).
        * ADC_9BIT: 9 bit, conversion time 84us.
        * ADC_10BIT: 10 bit, conversion time 148us.
        * ADC_11BIT: 11 bit, conversion time 276us.
        * ADC_12BIT: 12 bit, conversion time 532us (**default**).
        * ADC_2SAMP: 2 samples at 12 bit, conversion time 1.06ms.
        * ADC_4SAMP: 4 samples at 12 bit, conversion time 2.13ms.
        * ADC_8SAMP: 8 samples at 12 bit, conversion time 4.26ms.
        * ADC_16SAMP: 16 samples at 12 bit, conversion time 8.51ms
        * ADC_32SAMP: 32 samples at 12 bit, conversion time 17.02ms.
        * ADC_64SAMP: 64 samples at 12 bit, conversion time 34.05ms.
        * ADC_128SAMP: 128 samples at 12 bit, conversion time 68.10ms.
    * shunt_adc: The shunt ADC resolution (9, 10, 11, or 12-bit), or
        set the number of samples used when averaging results, represented by
        one of the following constants (optional).
        * ADC_9BIT: 9 bit, conversion time 84us.
        * ADC_10BIT: 10 bit, conversion time 148us.
        * ADC_11BIT: 11 bit, conversion time 276us.
        * ADC_12BIT: 12 bit, conversion time 532us (**default**).
        * ADC_2SAMP: 2 samples at 12 bit, conversion time 1.06ms.
        * ADC_4SAMP: 4 samples at 12 bit, conversion time 2.13ms.
        * ADC_8SAMP: 8 samples at 12 bit, conversion time 4.26ms.
        * ADC_16SAMP: 16 samples at 12 bit, conversion time 8.51ms
        * ADC_32SAMP: 32 samples at 12 bit, conversion time 17.02ms.
        * ADC_64SAMP: 64 samples at 12 bit, conversion time 34.05ms.
        * ADC_128SAMP: 128 samples at 12 bit, conversion time 68.10ms.
* `voltage()` Returns the bus voltage in volts (V).
* `supply_voltage()` Returns the supply voltage in volts (V). This is the sum of the bus voltage and shunt voltage.
* `shunt_voltage()` Returns the shunt voltage in millivolts (mV).
* `current()` Returns the bus current in milliamps (mA).
* `power()` Returns the bus power consumption in milliwatts (mW).
* `has_current_overflow()` Returns 'true' if an overflow has occured.
* `sleep()` Put the INA219 into power down mode.
* `wake()` Wake the INA219 from power down mode.
* `reset()` Reset the INA219 to its default configuration.

## Performance

TO-DO.

## Debugging

TO-DO.

## Testing

TO-DO.

## Coding Standard

TO-DO.