# Chromaduino-LED-Matrix
/*----------------------------------------------------------------------------
* DisplayMaster is using five Colorduino slave modules as
* LED Matrix. Each slave has to be programmed with a 
* slave address (Wire/I2C) and connected to SDA/SCL
* Max power consumption at 5VDC is 1500mA with five slaves
*
* Anders Wilhelmsson March '17
* - Multiple slaves.  1 slave  = 1482 bytes Uno/Mega
*                     2 slaves = 1684 bytes Uno/Mega
*                     3 slaves = 1876 bytes Uno/Mega
*                     4 slaves = 2068 bytes Mega
*                     5 slaves = 2260 bytes Mega 5932 bytes free
* - Separate white balance per slave
* - Morph demo
* - Scroll demo 
*
* The project is based on open source.   
* https://github.com/funnypolynomial/Chromaduino
* 
* 
*----------------------------------------------------------------------------*/
