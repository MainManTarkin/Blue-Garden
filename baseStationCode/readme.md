# Blue Garden Base Station

## General Part Description

This part of blue garden is responsible for connecting to all of the sensor devices it is linked to, collecting their sample data, and using that to determine if it should or should not water the ground. This side of the projects code uses the ESP-IDF library for an ESP32 device.

-------------

## Building

Because this code relies on the ESP-IDF that has it own compiler for building projects, The user should make a copy of the __gatt_client__ example located in the __Espressif\frameworks\esp-idf-v5.0\examples\bluetooth\bluedroid\ble__ directory. Once the copy has been made the user should then copy the code from this repository into __gatt_client__(copy)__\main__. Start the __ESP-IDF 5.0__ app and change the directory to your copy directory. you will need to make some changes to the sdkconfig before you build the program.      
     
begin with 
> idf.py menuconfig

once you the IDF configuration editor you should do this

> Component --> FreeRTOS --> configENABLE_BACKWARD_COMPATIBILITY   

the next step is optional but helps keep the terminal clean

> Component --> Log Output --> No Output (or) Error

Once you have this set up you save the changes by pressing __SHIFT-Q__ and accepting the changes.

once everything is done build the program or flash it to an ESP32.     
>idf.py build

or     

> idf.py -p (name of COM port) flash monitor

--------------
## Hardware Information

Unlike the sensor device the base station is just an ESP32 development kit ,in my case it was a ESP-WROOM-32D.

## BLE Information

Because this acts as a central BLE device there is no service or characteristic information needed to be given as that can all be seen in the [sensor readme](../sensorDeviceCode/readme.md).

## Emulated Console information

In the current version of __Blue Garden__ the base station is only controllable through usb uart terminal that has nine commands.

1. help - this commands prints all of the useable commands and description on how to use them.

2. scanCommand | arg1 - a number for the max possible discoverable device you want to scan for| - this command, with its singal argument, is used to scan the area for unlinked sensor that can be connected to. The devices it finds are placed in a list that can be referenced as connectable devices for other commands. This command should return 0 if device are found and no error occur.

3. checkScanList - prints the list of all non-linked devices in the area that you can connect and link to.
4. linkCommand | arg1 - index of device in scan list to connect to| - this command is used to link to a device in the area. should print "added sensor" if successful.
5. checkCommand | arg1 - index of device found in linked device list| - prints the information about the given sensor device such as current soil moisture content, trigger level, and other ID stuff.
6. setTriggerCommand | arg1 - index of device in linked list, arg2 - new trigger level that cause it to start watering (should be a percentage with range from 0-100%) | - this command is used to update a given device's trigger level in the linked device list.
7. checkLinkedList - prints all the linked device to this base station and their information
8. removeDevice | arg1 - index of device in linked device list to remove | - remove given device from linked device list (__NOTE__ removing a device from the linked device list does not reset its ID. You have to reset the sensor manually for it to be discoverable again. |TO BE FIXED IN LATER VERSION).
9. forceUpdate | arg1 - index of linked device to force a soil moisture update on | - this forces a given sensor to perform a soil moisture sample and if the returned value is less than the trigger level it begin to water the soil until the value is above the trigger level.