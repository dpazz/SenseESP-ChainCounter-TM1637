# SenseESP-ChainCounter-TM1637

This repository is derived from  template for [SensESP](https://github.com/SignalK/SensESP/) projects.

Comprehensive documentation for SensESP, including how to get started with your own project, is available at the [SensESP documentation site](https://signalk.org/SensESP/).


## A bi-directional chain counter.

 this is an example that supersedes the basic chain counter implementation found in sensesp examples using the "down" button of the windlass read through a digitalinput pin as a direction (up or down) driver. The  reset function is still valid to set the count to 0 value, but in this implementation the value emitted should count the effective chain meters deployed.

 The reset function is activated both from a physical button or by setting to 1 the reset param in the web UI config of the accumulator.
 
 An intermediate transform is provided in order to catch the emitted value from the accumulator and drive a TM1637 4-digit LCD display to show the count in parallel with what is transmitted to signalk. The transform, after having used the value to manage the physical counter, emits the value itself in order to be connected to skoutputfloat.

 > [!NOTE] 
 > Hardware notes :

 > The hardware used for this project is a very popular and cheap Hall sensor NJK-5002C connected to the 12 V power. The voltage of
 > signal wire coming from this sensor is reduced by a voltage divider in order to obtain the required 3.3V TTL input for each pulse
 > (a common 10 kOhm potentiometer can be used to obtain the reduction). A simple neodimium magnet should be installed in the gypsy 
 > (if not yet factory provided) facing the fixed installation of the hall sensor at every rotation. Is up to you to build a "marine
 > grade" weather proof casing & installation of the two (magnet & sensor) fitting to your boat windlass. Many examples of this can be
 > found on YouTube.
 >
 > The same voltage reduction is done to the wire connected to the UP pin in order to catch the direction of the count. It is assumed
 > that when the wire is connected to +12 V (and reduced to 3.3 V at UP Pin connection) the chain is deployed downward. To obtain this
 > is enough to connect the wire to the + side of the NO relay that rotates downward the chain windlass (when the relay is closed the
 > the + side is driven at 12V).
 
 > [!NOTE] 
 > Hardware sensor and display images:
 >
 > ![alt text](https://github.com/dpazz/SenseESP-ChainCounter-TM1637/blob/main/resources/NJK-5002C_Hall_sensor_details.jpg)

 > [!NOTE]
 > As you can see the sensor power voltage range covers both 12V and 24V.

 > ![alt text](https://github.com/dpazz/SenseESP-ChainCounter-TM1637/blob/main/resources/TM1637.jpeg)

 > [!NOTE]
 > The 4-digit I2c TM1637 display can be found in two versions with two different digit dimension (0.36" or 0.54")
 > The two version are with "colon" active or "decimal dots" active (due to limitation of the coding chip unable to drive
 > both the dots and the colon). This code example uses the "colon" type (and some adaption in the code was used to cope with
 > that) because of immediate availability in the testing lab. The colon is hence used as decimal separator. The better "dot"
 > version of the device should be used for better decimal representation (and code slightly modified accordingly)
