#include "ina219_2.h"

#include <bitset>
#include <math.h>
#include <float.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>

/* Constructors */
INA219::INA219(int shunt_resistance)
{
	_address = __ADDRESS;
    init();

	_shunt_ohms = shunt_resistance;
	_min_device_current_lsb = __CALIBRATION_FACTOR / (_shunt_ohms * __MAX_CALIBRATION_VALUE);
	_auto_gain_enabled = false;
}
INA219::INA219(int shunt_resistance, uint8_t address)
{
	_address = address;
    init();

	_shunt_ohms = shunt_resistance;
	_min_device_current_lsb = __CALIBRATION_FACTOR / (_shunt_ohms * __MAX_CALIBRATION_VALUE);
	_auto_gain_enabled = false;
}
INA219::INA219(int shunt_resistance, float max_expected_amps)
{
    _address = __ADDRESS;
    init();

	_shunt_ohms = shunt_resistance;
	_max_expected_amps = max_expected_amps;
	_min_device_current_lsb = __CALIBRATION_FACTOR / (_shunt_ohms * __MAX_CALIBRATION_VALUE);
	_auto_gain_enabled = false;
}
INA219::INA219(int shunt_resistance, float max_expected_amps, uint8_t address)
{
	_address = address;
    init();

	_shunt_ohms = shunt_resistance;
	_max_expected_amps = max_expected_amps;
	_min_device_current_lsb = __CALIBRATION_FACTOR / (_shunt_ohms * __MAX_CALIBRATION_VALUE);
	_auto_gain_enabled = false;
}

void
INA219::init()
{
    char *filename = (char*)"/dev/i2c-1";

	if ((_fd = open(filename, O_RDWR)) < 0)
	{
		// "Failed to open the i2c bus: " << _file_descriptor
	}
	if (ioctl(_fd, I2C_SLAVE, _address) < 0)
	{
		// "Failed to acquire bus access and/or talk to slave."
	}
}

/* Private functions */
uint16_t
INA219::__read_register(uint8_t register_address)
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
	return (buf[0] << 8) | buf[1];
}
void
INA219::__write_register(uint8_t register_address, uint16_t register_value)
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

void
INA219::_configure(int voltage_range, int gain, int bus_adc, int shunt_adc)
{
    uint16_t configuration = (voltage_range << __BRNG | gain << __PG0 | bus_adc << __BADC1 | shunt_adc << __SADC1 |__CONT_SH_BUS);
    _configuration_register(configuration);
}

void
INA219::_calibrate(int bus_volts_max, float shunt_volts_max, float max_expected_amps)
{
    float max_possible_amps = shunt_volts_max / _shunt_ohms;
    _current_lsb = _determine_current_lsb(max_expected_amps, max_possible_amps);
    _power_lsb = _current_lsb * 20;
    float max_current = _current_lsb * 32767;
    float max_shunt_voltage = max_current * _shunt_ohms;
    uint16_t calibration = (uint16_t) trunc(__CALIBRATION_FACTOR / (_current_lsb * _shunt_ohms));
    _calibration_register(calibration);
}

uint16_t
INA219::_voltage_register()
{
    uint16_t register_value = _read_voltage_register();
    return register_value >> 3;
}
uint16_t
INA219::_read_voltage_register()
{
    return __read_register(__REG_BUSVOLTAGE);
}
int16_t
INA219::_shunt_voltage_register()
{
    return (int16_t)__read_register(__REG_SHUNTVOLTAGE);
}
int16_t
INA219::_current_register()
{
    return (int16_t)__read_register(__REG_CURRENT);
}
uint16_t
INA219::_power_register()
{
    return __read_register(__REG_POWER);
}

uint16_t
INA219::_read_gain()
{
    int16_t configuration = _read_configuration();
    uint16_t gain = (configuration & 0x1800) >> __PG0;
    return gain;
}
uint16_t
INA219::_read_configuration()
{
    return __read_register(__REG_CONFIG);
}

void
INA219::_configuration_register(uint16_t register_value)
{
    __write_register(__REG_CONFIG, register_value);
}
void
INA219::_calibration_register(uint16_t register_value)
{
    __write_register(__REG_CALIBRATION, register_value);
}

bool
INA219::_has_current_overflow()
{
    int ovf = _read_voltage_register() & __OVF;
    return (ovf == 1);
}
void
INA219::_handle_current_overflow()
{
    if (_auto_gain_enabled) {
            while (_has_current_overflow()) {
                _increase_gain();
            }
    } else {
            if (_has_current_overflow()) {
                // raise DeviceRangeError(self.__GAIN_VOLTS[self._gain])
            }
    }
}

void
INA219::_configure_gain(int gain)
{
    uint16_t configuration = _read_configuration();
    configuration = configuration & 0xE7FF;
    _configuration_register(configuration | (gain << __PG0));
    _gain = gain;
}
int
INA219::_determine_gain(float max_expected_amps)
{
    double shunt_v = max_expected_amps * _shunt_ohms;
    if (shunt_v > __GAIN_VOLTS[3]) {
        // raise ValueError(self.__RNG_ERR_MSG % max_expected_amps)
    }
    float gain = FLT_MAX;
	int gain_index;
	for (int i = 0; i < sizeof(__GAIN_VOLTS)/sizeof(__GAIN_VOLTS[0]); i++) {
		float v = __GAIN_VOLTS[i];
		if (v > shunt_v) {
			if (v < gain) {
				gain = v;
				gain_index = i;
			}
		}
	}
    return gain_index;
}
void
INA219::_increase_gain()
{
    int gain = (int) _read_gain();
    int len = sizeof(__GAIN_VOLTS) / sizeof(__GAIN_VOLTS[0]);
    if (gain < len - 1) {
        gain = gain + 1;
        _calibrate(__BUS_RANGE[_voltage_range], __GAIN_VOLTS[gain], _max_expected_amps);
        _configure_gain(gain);
        // 1ms delay required for new configuration to take effect,
        // otherwise invalid current/power readings can occur.
        usleep(1000);
    } else {
        // raise DeviceRangeError(self.__GAIN_VOLTS[gain], True)
    }
}

float
INA219::_determine_current_lsb(float max_expected_amps, float max_possible_amps)
{
    float current_lsb;
    if (max_expected_amps) {
        if (max_expected_amps > roundf(max_possible_amps * 1000.0) / 1000.0) {
            // raise ValueError(self.__AMP_ERR_MSG % (max_expected_amps, max_possible_amps))
        }
        if (max_expected_amps < max_possible_amps) {
            current_lsb = max_expected_amps / __CURRENT_LSB_FACTOR;
        } else {
            current_lsb = max_possible_amps / __CURRENT_LSB_FACTOR;
        }
    } else {
        current_lsb = max_possible_amps / __CURRENT_LSB_FACTOR;
    }

    if (current_lsb < _min_device_current_lsb) {
        current_lsb = _min_device_current_lsb;
    }
    return current_lsb;
}
float
INA219::_calculate_min_current_lsb()
{
    return __CALIBRATION_FACTOR / (_shunt_ohms * __MAX_CALIBRATION_VALUE);
}

bool
INA219::__validate_voltage_range(int voltage_range)
{
    int len = sizeof(__BUS_RANGE) / sizeof(__BUS_RANGE[0]);
    if (voltage_range > (len - 1)) {
        return false;
    }
    return true;
}

/* Public functions */
void
INA219::configure(int voltage_range, int gain, int bus_adc, int shunt_adc)
{
    if (__validate_voltage_range(voltage_range)) {
        _voltage_range = voltage_range;

        if (_max_expected_amps) {
            if (gain == GAIN_AUTO) {
                _auto_gain_enabled = true;
                _gain = _determine_gain(_max_expected_amps);
            } else {
                _gain = gain;
            }
        } else {
            if (gain != GAIN_AUTO) {
                _gain = gain;
            } else {
                _auto_gain_enabled = true;
                _gain = GAIN_1_40MV;
            }
        }

        _calibrate(__BUS_RANGE[voltage_range], __GAIN_VOLTS[_gain], _max_expected_amps);
        _configure(voltage_range, _gain, bus_adc, shunt_adc);
    } else {
        // invalid voltage!
    }
}
float
INA219::voltage()
{
    // Returns the bus voltage in volts.
    uint16_t value = _voltage_register();
    return float(value) * __BUS_MILLIVOLTS_LSB / 1000.0;
}
float
INA219::shunt_voltage()
{
    // Returns the shunt voltage in millivolts.
    // A DeviceRangeError exception is thrown if current overflow occurs.
    _handle_current_overflow();
    return float(_shunt_voltage_register()) * __SHUNT_MILLIVOLTS_LSB;
}
float
INA219::supply_voltage()
{
    // Returns the bus supply voltage in volts. This is the sum of
    // the bus voltage and shunt voltage. A DeviceRangeError
    // exception is thrown if current overflow occurs.
    return voltage() + (float(shunt_voltage()) / 1000.0);
}
float
INA219::current()
{
    // Returns the bus current in milliamps. A DeviceRangeError
    // exception is thrown if current overflow occurs.
    _handle_current_overflow();
    return float(_current_register() * _current_lsb * 1000.0);
}
float
INA219::power()
{
    // Returns the bus power consumption in milliwatts.
    // A DeviceRangeError exception is thrown if current overflow occurs.
    _handle_current_overflow();
    return float(_power_register() * _power_lsb * 1000.0);
}

void
INA219::sleep()
{
    // Put the INA219 into power down mode.
    uint16_t configuration = _read_configuration();
    _configuration_register(configuration & 0xFFF8);
}
void
INA219::wake()
{
    // Wake the INA219 from power down mode
    uint16_t configuration = _read_configuration();
    _configuration_register(configuration | 0x0007);
    // 40us delay to recover from powerdown (p14 of spec)
    usleep(40);
}
void
INA219::reset()
{
    // Reset the INA219 to its default configuration.
    _configuration_register(1 << __RST);
}

bool
INA219::current_overflow()
{
    // Returns true if the sensor has detect current overflow. In
    // this case the current and power values are invalid.
    return _has_current_overflow();
}