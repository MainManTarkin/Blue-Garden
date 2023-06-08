# Blue Garden Sensor

## General Part Description

Responsible for taking the soil moisture readings and at the moment providing the control signal for a pump or valve to turn on.
The code relies on the __Nordic Semiconductors__ SDK 17.1.0 library.

-------------

## Building

The code for this device was compiled using __SEGGER Embedded Studio__. To build this program it is recommended to make a copy of the __ble_app_template__ program located in the __nRF5_SDK_17.1.0_ddde560\examples\ble_peripheral__ directory. Once completed, you should copy the code of this repository into the new directory of __ble_app_template__ (or whatever you named it). copy the __sdk_config.h__ file into the directory of __nRF5_SDK_17.1.0_ddde560\examples\ble_peripheral\ble_app_template\pca10056\s140\config__. Make sure to add the files you copied into the project's solution and then build the solution. 

--------------

## Hardware Information

The sensor device uses a nRF52840 SoC as it main processing unit, radio module, and Analog-digital-converter. 
Since the sensor needs to store information across power-cycles it uses an I2C EEPROM (I could for the life of me not get the SoC internal flash to work with nordic's s140 softdevice) of greater than 128 bits (16 bytes) to store information.    

### Pins used

* SDA Pin - 26
* SCL Pin - 27
* ADC calibrating LED Pin - 15 (pulls down)
* ADC calibrate complete LED Pin - 16
* ADC calibrate button Pin - 25
* ADC sample Pins - 3(sample signal), 2(ref)
* dipswitch 1 Pin - 11
* dipswitch 2 Pin - 12
* advertise and connection LED Pin - 13 (pull down)

----------------

### Start State

The device determines the mode it starts in from a power-cycle by checking the state of two pins. The modes the device can start in is defined by the following table:     

| Pin 1 | Pin 2 | Mode             |
|-------|-------|------------------|
| 0     | 0     | Default Mode     |
| 1     | 0     | Default Mode     |
| 0     | 1     | Calibration mode |
| 1     | 1     | Reset Mode       |

The modes are as follows:

* Default Mode - The standard mode to enter. This is where the device will act as a discoverable peripheral and can be connected to for garden moisture sensing
* Calibration Mode - this is used to set the range the samples will work in. you should always calibrate after a reset or first power-on. The order of calibration is dryiest soil then saturated soil, after these two samples are taken the __calibrate complete LED__ will stay on after the data has been written to the EEPROM and verified that it has indeed been written.
* Reset Mode - resets the contents in the EEPROM to their NULL values then moves into __Default Mode__

------------------
## BLE Information

At the momement the device keeps advertising until a central device connects to it. When the connected device disconnects the sensor begins to advertise again.

### Scan Packet

much of the needed information for connecting to this device is found in the scan packet.

>water garderning scan packet    
\-------------------------------------------------------------------    
length|type of packet| manfu id| full device name| device id|     
\-------------------------------------------------------------------    
length = 1 byte    
type of packet = 1 byte    
manfu id = 2 bytes    
full device name = 15 bytes : "bluetoothGWater"   
device id = 4 bytes    
scan response data packet = 23 bytes;   

---------------------

### Sevices and Characteristics

The device has one one service and three characteristics that are used. All the UUID are custom 128 bit values.

* Soil Moisture Sensor Service :UUID - 0xBC8ABF45CA0550BA4042B000001464F3 | UUID for main sensor service      
    1. Device ID :UUID - 0xBC8ABF45CA0550BA4042B000011464F3 | is used for writing the device id that will show up in the scan packet   
    2. Command Input :UUID - 0xBC8ABF45CA0550BA4042B000021464F3 | is used for writting commands (such a perform sample or turn on watering device)       
    3. sample output :UUID - 0xBC8ABF45CA0550BA4042B000031464F3 | is used for reading the latest sample took. This characteristic notifies the centeral device of any change to the value.     


------------

### Commands

The device is controlled with a small set of commands sent to the __Command Input__ characteristic. The commands are as follows

* Take Sample - 0x00 | causes the sensor to take a soil moisture sample of the dirt it is in and send it back to the base station as a percentage
* Water Spot - 0x01 | causes the device to hold the watering signal until it is sent again.