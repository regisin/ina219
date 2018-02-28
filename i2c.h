#ifndef _LIB_I2C_PI
#define _LIB_I2C_PI

#include <stdint.h>

class I2C{
    /* Constructors */
    public:
        I2C(uint8_t address);
    
    
    /* Private functions */
    // private:
    

    /* Private viarables */
    private:
        int     _file_descriptor;
        uint8_t device_address;
        
    

    /* Public functions */
    public:
        uint16_t read_register(uint8_t register_address);
        void write_register(uint8_t register_address, uint16_t register_value);

    

    /* Public variables, because cant #define arrays */
    // public:
};

#endif