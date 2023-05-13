// Signal K application template file.
//
// This application demonstrates core SensESP concepts in a very
// concise manner. You can build and upload the application as is
// and observe the value changes on the serial port monitor.
//
// You can use this source file as a basis for your own projects.
// Remove the parts that are not relevant to you, and add your own code
// for external hardware libraries.
#include "sensesp.h"
#include "sensesp/sensors/analog_input.h"
#include "sensesp/sensors/digital_input.h"
#include "sensesp/sensors/sensor.h"
#include "sensesp/signalk/signalk_output.h"
#include "sensesp/system/lambda_consumer.h"
#include "sensesp/system/valueproducer.h"
#include "sensesp/transforms/linear.h"   // per bozza windspeed
#include "sensesp/transforms/lambda_transform.h"
//#include "sensesp/sensors/repeat_sensor.h"   // per bozza windspeed
#include "sensesp_app_builder.h"
#include "Wire.h"
#include "math.h"
#include "dir_transform.h"
#include <map>
//#include "VDO_filter.h"

using namespace sensesp;


reactesp::ReactESP app;
//Adafruit_ADS1115 ads1115;

// The setup function performs one-time application initialization.
void setup() {
#ifndef SERIAL_DEBUG_DISABLED
  SetupSerialDebug(115200);
#endif   
  
  
 
  // Construct the global SensESPApp() object
  SensESPAppBuilder builder;
  sensesp::sensesp_app = (&builder)
                    // Set a custom hostname for the app.
                    ->set_hostname("ESP-GY91-FREETIME")
                    // Optionally, hard-code the WiFi and Signal K server
                    // settings. This is normally not needed.
                    //->set_wifi("My WiFi SSID", "my_wifi_password")
                    ->set_sk_server("10.10.10.2", 3000)
                    ->enable_ota("123456789")
                    ->get_app();
 
  
 
  // GPIO number to use for the analog input
  const uint8_t kAnalogInputPin = 36;
  // Define how often (in milliseconds) new samples are acquired
  const unsigned int kAnalogInputReadInterval = 500;
  // Define the produced value at the maximum input voltage (3.3V).
  // A value of 3.3 gives output equal to the input voltage.
  const float kAnalogInputScale = 3.3;

  // Create a new Analog Input Sensor that reads an analog input pin
  // periodically.
  auto* analog_input = new AnalogInput(
      kAnalogInputPin, kAnalogInputReadInterval, "", kAnalogInputScale);

  // Add an observer that prints out the current value of the analog input
  // every time it changes.
  /*analog_input->attach([analog_input]() {
    debugD("Analog input value: %f", analog_input->get());
  });*/

  // Set GPIO pin 15 to output and toggle it every 650 ms

  const uint8_t kDigitalOutputPin = 15;
  const unsigned int kDigitalOutputInterval = 650;
  pinMode(kDigitalOutputPin, OUTPUT);
  app.onRepeat(kDigitalOutputInterval, [kDigitalOutputPin]() {
    digitalWrite(kDigitalOutputPin, !digitalRead(kDigitalOutputPin));
  });

  // Read GPIO 14 every time it changes

  const uint8_t kDigitalInput1Pin = 14;
  auto* digital_input1 =
      new DigitalInputChange(kDigitalInput1Pin, INPUT_PULLUP, CHANGE);

  // Connect the digital input to a lambda consumer that prints out the
  // value every time it changes.

  // Test this yourself by connecting pin 15 to pin 14 with a jumper wire and
  // see if the value changes!

  digital_input1->connect_to(new LambdaConsumer<bool>(
      [](bool input) { debugD("Digital input value changed: %d", input); }));

  // Create another digital input, this time with RepeatSensor. This approach
  // can be used to connect external sensor library to SensESP!

  const uint8_t kDigitalInput2Pin = 13;
  const unsigned int kDigitalInput2Interval = 1000;

  // Configure the pin. Replace this with your custom library initialization
  // code!
  pinMode(kDigitalInput2Pin, INPUT_PULLUP);

  // Define a new RepeatSensor that reads the pin every 100 ms.
  // Replace the lambda function internals with the input routine of your custom
  // library.

  // Again, test this yourself by connecting pin 15 to pin 13 with a jumper
  // wire and see if the value changes!

  auto* digital_input2 = new RepeatSensor<bool>(
      kDigitalInput2Interval,
      [kDigitalInput2Pin]() { return digitalRead(kDigitalInput2Pin); });

  // Connect the analog input to Signal K output. This will publish the
  // analog input value to the Signal K server every time it changes.
  analog_input->connect_to(new SKOutputFloat(
      "sensors.analog_input.voltage",         // Signal K path
      "/sensors/analog_input/voltage",        // configuration path, used in the
                                              // web UI and for storing the
                                              // configuration
      new SKMetadata("V",                     // Define output units
                     "Analog input voltage")  // Value description
      ));

  // Connect digital input 2 to Signal K output.
  digital_input2->connect_to(new SKOutputBool(
      "sensors.digital_input2.value",          // Signal K path
      "/sensors/digital_input2/value",         // configuration path
      new SKMetadata("",                       // No units for boolean values
                     "Digital input 2 value")  // Value description
      ));

 sensesp::sensesp_app->start();
}

void loop() {app.tick();}
  
  // using ADS1115

  //float vpp_sinecos = 5.0;
  //float vscale = vpp_sinecos*500; // Peak is half value of vpp and readings are in mV 
 
                                 //is obtained by multiplying vpp * 1000 and dividing by 2
  //adsGain_t gain = GAIN_TWOTHIRDS;
  
  //std::map <adsGain_t, float> gain_to_multiplier {{GAIN_TWOTHIRDS, 0.1875},{GAIN_ONE, 0.125},{GAIN_TWO, 0.0625},
  //                                                {GAIN_FOUR, 0.03125}, {GAIN_EIGHT,0.015625}, {GAIN_SIXTEEN,0.0078125}
  //                                               };
  //float gain_multiplier = gain_to_multiplier.at(gain); // 2/3x gain +/- 6.144V  1 bit = 3mV      0.1875mV (default)
  //const static JsonObject ads1115_param  PROGMEM = {
  //               root:
  //                 { "VPP", "vpp_sinecos"},
//                { "GAIN","gain_multiplier"}

 // };
 // auto* ads1115_conf = new Configurable ("/sensors/wind_ads1115", "parameters for vpp and gain multiplier of ads1115");
 // ads1115_conf->set_configuration (ads1115_param);
 // ads1115.setGain(adsGain_t (gain_multiplier));
//ads1115.begin(0x48);
/*
  auto* readsine = new RepeatSensor <float> (
                        ADS1115_read_interval, 
                        [vscale, gain_multiplier]() {
                                     return  (gain_multiplier*ads1115.readADC_Differential_0_1()-vscale)/vscale;
                                   } //ReadADC returns mV                                                    
                                            );
  auto* readcosine = new RepeatSensor <float> (
                          ADS1115_read_interval, 
                          [vscale, gain_multiplier]() { 
                                      return  (gain_multiplier*ads1115.readADC_Differential_2_3()-vscale)/vscale;
                                   } //ReadADC returns mV
                                          );
  auto* direction = new DirTransform () ;
  
  readsine
          ->connect_to (new Linear(sinecal, sineoff, config_path_wind_sinecal))
          ->connect_to (direction,0)
          ;
  readcosine
          ->connect_to (new Linear(cosinecal, cosineoff, config_path_wind_cosinecal))
          ->connect_to (direction, 1)
          ->connect_to ((new SKOutputFloat(
          sk_path_wa, config_path_skpath_wa,
          new SKMetadata ("rad", "wind angle apparent")
          )))
          ;
  
  // for debug
  readsine->attach([readsine]() {
    debugD("Readsine value: %f", readsine->get());
  });
  readcosine->attach([readcosine]() {
    debugD("Readcosine value: %f", readcosine->get());
  });
  // end debug
  //not using internal ESP32 ADC. Replaced with TI ADS 1115 for better accuracy
*/


  // Start networking, SK server connections and other SensESP internals
 