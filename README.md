# SensESP Project Template

This repository is derived from  template for [SensESP](https://github.com/SignalK/SensESP/) projects.

Comprehensive documentation for SensESP, including how to get started with your own project, is available at the [SensESP documentation site](https://signalk.org/SensESP/).


 A bi-directional chain counter
 this is an example that supersedes the basic chain counter implementation found in sensesp examples using the "down" button of the windlass read through a digitalinput pin as a direction (up or down) driver. The  reset function is still valid to set the count to 0 value, but in this implementation the value emitted should count the effective chain meters deployed.

 The reset function is activated both from a physical button or by setting to 1 the reset param in the web UI config of the accumulator.
 
 An intermediate transform is provided in order to catch the emitted value from the accumulator and drive a TM1637 4-digit LCD display to show the count in parallel with what is tranmitted to signalk. The transform, after having used the value to manage the physical counter, emits the value itself in order to be connected to skoutputfloat