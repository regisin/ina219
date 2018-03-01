#ifndef _LIB_INA219_PI_2
#define _LIB_INA219_PI_2

#define RANGE_16V                           0 // Range 0-16 volts
#define RANGE_32V                           1 // Range 0-32 volts

#define GAIN_1_40MV                         0 // Maximum shunt voltage 40mV
#define GAIN_2_80MV                         1 // Maximum shunt voltage 80mV
#define GAIN_4_160MV                        2 // Maximum shunt voltage 160mV
#define GAIN_8_320MV                        3 // Maximum shunt voltage 320mV
#define GAIN_AUTO                          -1 // Determine gain automatically

#define ADC_9BIT                            0  // 9-bit conversion time  84us.
#define ADC_10BIT                           1  // 10-bit conversion time 148us.
#define ADC_11BIT                           2  // 11-bit conversion time 2766us.
#define ADC_12BIT                           3  // 12-bit conversion time 532us.
#define ADC_2SAMP                           9  // 2 samples at 12-bit, conversion time 1.06ms.
#define ADC_4SAMP                           10 // 4 samples at 12-bit, conversion time 2.13ms.
#define ADC_8SAMP                           11 // 8 samples at 12-bit, conversion time 4.26ms.
#define ADC_16SAMP                          12 // 16 samples at 12-bit,conversion time 8.51ms
#define ADC_32SAMP                          13 // 32 samples at 12-bit, conversion time 17.02ms.
#define ADC_64SAMP                          14 // 64 samples at 12-bit, conversion time 34.05ms.
#define ADC_128SAMP                         15 // 128 samples at 12-bit, conversion time 68.10ms.

#define __ADDRESS                           0x40

#define __REG_CONFIG                        0x00
#define __REG_SHUNTVOLTAGE                  0x01
#define __REG_BUSVOLTAGE                    0x02
#define __REG_POWER                         0x03
#define __REG_CURRENT                       0x04
#define __REG_CALIBRATION                   0x05

#define __RST                               15
#define __BRNG                              13
#define __PG1                               12
#define __PG0                               11
#define __BADC4                             10
#define __BADC3                             9
#define __BADC2                             8
#define __BADC1                             7
#define __SADC4                             6
#define __SADC3                             5
#define __SADC2                             4
#define __SADC1                             3
#define __MODE3                             2
#define __MODE2                             1
#define __MODE1                             0

#define __OVF                               1
#define __CNVR                              2

#define __CONT_SH_BUS                       7

#define __SHUNT_MILLIVOLTS_LSB              0.01    // 10uV
#define __BUS_MILLIVOLTS_LSB                4       // 4mV
#define __CALIBRATION_FACTOR                0.04096
#define __MAX_CALIBRATION_VALUE             0xFFFE  // Max value supported (65534 decimal)

// In the spec (p17) the current LSB factor for the minimum LSB is
// documented as 32767, but a larger value (100.1% of 32767) is used
// to guarantee that current overflow can always be detected.
#define __CURRENT_LSB_FACTOR                32800


#include <stdint.h>

class INA219{
    /* Constructors */
    public:
        INA219(int shunt_resistance);                                           // Use default values, auto gain
        INA219(int shunt_resistance, uint8_t address);                          // Auto gain
        INA219(int shunt_resistance, float max_expected_amps);                  // Custom max amps
        INA219(int shunt_resistance, float max_expected_amps, uint8_t address); // Custom device address and amps
    
    
    /* Private functions */
    private:
        void        _configure(int voltage_range, int gain, int bus_adc, int shunt_adc);
        void        _calibrate(int bus_volts_max, float shunt_volts_max, float max_expected_amps);

        uint16_t    _voltage_register();
        uint16_t    _read_voltage_register();
        int16_t     _shunt_voltage_register();
        int16_t     _current_register();
        uint16_t    _power_register();

        uint16_t    _read_gain();
        uint16_t    _read_configuration();

        void        _configuration_register(uint16_t register_value);
        void        _calibration_register(uint16_t register_value);


        bool        _has_current_overflow();
        void        _handle_current_overflow();
        
        void        _configure_gain(int gain);
        int         _determine_gain(float max_expected_amps);
        void        _increase_gain();
        
        
        float       _determine_current_lsb(float max_expected_amps, float max_possible_amps);
        float       _calculate_min_current_lsb();

        bool        __validate_voltage_range(int voltage_range);
        
        void        __write_register(uint8_t register_address, uint16_t register_value);
        uint16_t    __read_register(uint8_t register_address);


    /* Private viarables */
    private:
        int     _file_descriptor;
        int     _shunt_ohms;
        float   _max_expected_amps;
        float   _min_device_current_lsb;
        bool    _auto_gain_enabled;
        int     _voltage_range;
        int     _gain;
        int     _current_lsb;
        int     _power_lsb;


        int _fd;
        uint8_t _address;
        void init();



    /* Public functions */
    public:
        void    configure(int voltage_range, int gain, int bus_adc, int shunt_adc);
        float   voltage();
        float   shunt_voltage();
        float   supply_voltage();
        float   current();
        float   power();
        
        void    sleep();
        void    wake();
        void    reset();

        bool    current_overflow();


    /* Public variables, because cant #define arrays */
    public:
        float __GAIN_VOLTS[4]   = {0.04, 0.08, 0.16, 0.32};
        int   __BUS_RANGE[2]    = {16, 32};
};

#endif