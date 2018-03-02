#include "ina219.h"

#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <bitset>
#include <math.h>
#include <float.h>

INA219::INA219(float shunt_resistance)
{
	init_i2c(__ADDRESS);

	_shunt_ohms = shunt_resistance;
	_min_device_current_lsb = __CALIBRATION_FACTOR / (_shunt_ohms * __MAX_CALIBRATION_VALUE);
	_auto_gain_enabled = false;
}
INA219::INA219(float shunt_resistance, uint8_t address)
{
	init_i2c(address);

	_shunt_ohms = shunt_resistance;
	_min_device_current_lsb = __CALIBRATION_FACTOR / (_shunt_ohms * __MAX_CALIBRATION_VALUE);
	_auto_gain_enabled = false;
}
INA219::INA219(float shunt_resistance, float max_expected_amps)
{
	init_i2c(__ADDRESS);

	_shunt_ohms = shunt_resistance;
	_max_expected_amps = max_expected_amps;
	_min_device_current_lsb = __CALIBRATION_FACTOR / (_shunt_ohms * __MAX_CALIBRATION_VALUE);
	_auto_gain_enabled = false;
}
INA219::INA219(float shunt_resistance, float max_expected_amps, uint8_t address)
{
	init_i2c(address);

	_shunt_ohms = shunt_resistance;
	_max_expected_amps = max_expected_amps;
	_min_device_current_lsb = __CALIBRATION_FACTOR / (_shunt_ohms * __MAX_CALIBRATION_VALUE);
	_auto_gain_enabled = false;
}

void
INA219::init_i2c(uint8_t address)
{
	char *filename = (char*)"/dev/i2c-1";
	if ((_file_descriptor = open(filename, O_RDWR)) < 0)
	{
		// NS_FATAL_ERROR("Failed to open the i2c bus, error number: " << _file_descriptor);
	}
	if (ioctl(_file_descriptor, I2C_SLAVE, address) < 0)
	{
		// NS_FATAL_ERROR("Failed to acquire bus access and/or talk to slave.");
	}
}

uint16_t
INA219::read_register(uint8_t register_address)
{
	uint8_t buf[3];
	buf[0] = register_address;
	if (write(_file_descriptor, buf, 1) != 1) {
		// NS_FATAL_ERROR("Failed to set register.");
	}
	usleep(1000);
	if (read(_file_descriptor, buf, 2) != 2) {
		// NS_FATAL_ERROR("Failed to read register value.");
	}
	return (buf[0] << 8) | buf[1];
}
void
INA219::write_register(uint8_t register_address, uint16_t register_value)
{
	uint8_t buf[3];
	buf[0] = register_address;
	buf[1] = register_value >> 8;
	buf[2] = register_value & 0xFF;
	
	if (write(_file_descriptor, buf, 3) != 3)
	{
		// NS_FATAL_ERROR("Failed to write to the i2c bus.");
	}
	// NS_LOG_DEBUG("write register 0x" << std::hex << register_address << ": 0x" << register_value << "0b" << std::bitset<16>(register_value).to_string());
}

void
INA219::configure(int voltage_range, int gain, int bus_adc, int shunt_adc)
{
	int len = sizeof(__BUS_RANGE) / sizeof(__BUS_RANGE[0]);
	if (voltage_range > len-1) {
		// NS_FATAL_ERROR("Invalid voltage range, must be one of: RANGE_16V, RANGE_32V");
	}
	_voltage_range = voltage_range;

	if (_max_expected_amps) {
		if (gain == GAIN_AUTO) {
			_auto_gain_enabled = true;
			_gain = determine_gain(_max_expected_amps);
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

	// NS_LOG_DEBUG("gain set to " << __GAIN_VOLTS[_gain] << "V");
	// NS_LOG_DEBUG("shunt ohms: " << _shunt_ohms 
	// << ", bus max volts: " << __BUS_RANGE[voltage_range] 
	// << "V, shunt volts max: " << __GAIN_VOLTS[_gain]
	// << "V, max expected amps: " << max_expected_amps
	// << "A, bus ADC: " << bus_adc
	// << ", shunt ADC: " << shunt_adc);

	calibrate(__BUS_RANGE[voltage_range], __GAIN_VOLTS[_gain], _max_expected_amps);
	uint16_t calibration = (voltage_range << __BRNG | _gain << __PG0 | bus_adc << __BADC1 | shunt_adc << __SADC1 | __CONT_SH_BUS);
	// NS_LOG_DEBUG("configuration: 0x" << std::hex << calibration);
	write_register(__REG_CONFIG, calibration);
}
int
INA219::determine_gain(float max_expected_amps)
{
	float shunt_v = max_expected_amps * _shunt_ohms;
	if (shunt_v > __GAIN_VOLTS[3]) {
		// NS_FATAL_ERROR("Expected amps " << max_expected_amps << "A, out of range, use a lower value shunt resistor");
	}
	float gain = FLT_MAX;
	int gain_index;
	for (int i = 0; i < sizeof(__GAIN_VOLTS)/sizeof(__GAIN_VOLTS[0]); i++ ) {
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
INA219::calibrate(int bus_volts_max, float shunt_volts_max, float max_expected_amps)
{
	// NS_LOG_DEBUG("calibrate called with: bus max volts: " << bus_volts_max
	// << "V, max shunt volts: " << shunt_volts_max << "V"
	// << "V, max expected amps: " << max_expected_amps);
	float max_possible_amps = shunt_volts_max / _shunt_ohms;
	// NS_LOG_DEBUG("max possible current: " << max_possible_amps << "A");
	_current_lsb = determine_current_lsb(max_expected_amps, max_possible_amps);
	// NS_LOG_DEBUG("current LSB: " << std::scientific << _current_lsb << " A/bit");
	_power_lsb = _current_lsb * 20.0;
	// NS_LOG_DEBUG("power LSB: " << std::scientific << _power_lsb << " W/bit");
	float max_current = _current_lsb * 32767;
	// NS_LOG_DEBUG("max current before overflow: " << max_current << "A");
	float max_shunt_voltage = max_current * _shunt_ohms;
	// NS_LOG_DEBUG("max shunt voltage before overflow: " << (max_shunt_voltage * 1000) << "mV");
	uint16_t calibration = (uint16_t) trunc(__CALIBRATION_FACTOR / (_current_lsb * _shunt_ohms));
	// NS_LOG_DEBUG("calibration: 0x" << std::hex << calibration << std::dec << " (" << calibration << ")");
	// NS_LOG_DEBUG("configuration: 0x" << std::hex << calibration);
	write_register(__REG_CONFIG, calibration);
}
float
INA219::determine_current_lsb(float max_expected_amps, float max_possible_amps)
{
	float current_lsb;
	if (max_expected_amps) {
		float nearest = roundf(max_possible_amps * 1000.0) / 1000.0;
		if (max_expected_amps > nearest) {
			// NS_FATAL_ERROR("Expected current " << max_expected_amps << "A is greater than max possible current " << max_possible_amps << "A");
		}
		// NS_LOG_DEBUG("max expected current: " << max_expected_amps << "A");

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
void
INA219::sleep()
{
	uint16_t config = read_register(__REG_CONFIG);
	write_register(__REG_CONFIG, config & 0xFFF8);
}
void
INA219::wake()
{
	uint16_t config = read_register(__REG_CONFIG);
	write_register(__REG_CONFIG, config | 0x0007);
	// 40us delay to recover from powerdown (p14 of spec)
	usleep(40);
}
void
INA219::reset()
{
	write_register(__REG_CONFIG, __RST);
}
bool
INA219::has_current_overflow()
{
	uint16_t ovf = read_register(__REG_BUSVOLTAGE) & 1;
    return (ovf == 1);
}

float
INA219::voltage()
{
	uint16_t value = read_register(__REG_BUSVOLTAGE) >> 3;
    return float(value) * __BUS_MILLIVOLTS_LSB / 1000.0;
}
float
INA219::shunt_voltage()
{
	handle_current_overflow();
	int16_t shunt_voltage = (int16_t) read_register(__REG_SHUNTVOLTAGE); // (int) because it is a signed integer
	return __SHUNT_MILLIVOLTS_LSB * shunt_voltage;
}
float
INA219::supply_voltage()
{
	return voltage() + (shunt_voltage() / 1000.0);
}
float
INA219::current()
{
	handle_current_overflow();
	uint16_t current_raw = read_register(__REG_CURRENT);
	int16_t current = (int16_t)current_raw;
	if (current > 32767) current -= 65536;
	return  current * _current_lsb * 1000.0;
}
float
INA219::power()
{
	handle_current_overflow();
	uint16_t power_raw = read_register(__REG_POWER);
	int16_t power = (int16_t)power_raw;
	return power * _power_lsb * 1000.0;
}
void
INA219::handle_current_overflow()
{
	if (_auto_gain_enabled) {
		while (has_current_overflow()) {
			increase_gain();
		}
	} else {
		if (has_current_overflow()) {
			// NS_LOG_DEBUG("Current out of range (overflow), for gain " << __GAIN_VOLTS[_gain] << "V");
		}
	}
}
void
INA219::increase_gain()
{
	// NS_LOG_DEBUG("Current overflow detected - attempting to increase gain");

	uint16_t config = read_register(__REG_CONFIG);
	int gain = (config & 0x1800) >> __PG0;

	int len = sizeof( __GAIN_VOLTS ) / sizeof( __GAIN_VOLTS[0] );
	if (gain < len - 1){
		gain++;
		calibrate(__BUS_RANGE[_voltage_range], __GAIN_VOLTS[gain], _max_expected_amps);
		uint16_t config = read_register(__REG_CONFIG);
        config = config & 0xE7FF;
        write_register(__REG_CONFIG, config | (gain << __PG0));
        _gain = gain;
		// NS_LOG_DEBUG("gain set to: " << __GAIN_VOLTS[gain] << "V");
		// 1ms delay required for new configuration to take effect,
		// otherwise invalid current/power readings can occur.
		usleep(1000);
	} else {
		// NS_LOG_DEBUG("Device limit reach, gain cannot be increased");
		// NS_LOG_DEBUG("Current out of range (overflow), for gain " << __GAIN_VOLTS[gain] << "V");
	}
}
