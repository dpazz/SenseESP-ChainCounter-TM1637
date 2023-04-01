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
#include "sensesp/transforms/frequency.h"   // per bozza windspeed
#include "sensesp_app_builder.h"
//#include "VDO_filter.h"

using namespace sensesp;


reactesp::ReactESP app;

// The setup function performs one-time application initialization.
void setup() {
#ifndef SERIAL_DEBUG_DISABLED
  SetupSerialDebug(115200);
#endif
  
  
 
  // Construct the global SensESPApp() object
  SensESPAppBuilder builder;
  sensesp_app = (&builder)
                    // Set a custom hostname for the app.
                    ->set_hostname("ESP-windnmea-dpazz")
                    // Optionally, hard-code the WiFi and Signal K server
                    // settings. This is normally not needed.
                    //->set_wifi("My WiFi SSID", "my_wifi_password")
                    //->set_sk_server("192.168.3.235", 3000)
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
  analog_input->attach([analog_input]() {
    debugD("Analog input value: %f", analog_input->get());
  });

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
 
 
  // bozza wind speed sensor

  const char* sk_path = "sensors.wind.speed";
  const char* config_path = "/sensors/wind_speed";
  const char* config_path_read_delay = "/sensors/wind_speed/read_delay"; // time between reads in milliseconds
  const char* config_path_DEBOUNCE = "/sensors/wind_speed/ignore_interval"; // minimum time between events in milliseconds
  
  const char* config_path_TIMEOUT = "/sensors/wind_speed/timeout";   // Maximum time allowed between events in millisec.
  const char* config_path_calibrate = "/sensors/wind_speed/calibrate"; // to normalize in Hz the measured event frequency
  const char* config_path_skpath = "/sensors/wind_speed/sk";
  
  const unsigned int DEBOUNCE = 10; // minimum time between interrupts in milliseconds
  const unsigned int TIMEOUT = 1500;   // Maximum time allowed between speed pulses in millisec.
  //const float multiplier = 0.0035;
  const unsigned int read_delay = 500;
  const float multiplier = 100*(1/read_delay); // only a starting guess 

  // Knots is actually stored as (Knots * 100). Deviations below should match these units.
  const int BAND_0 =  10 * 100;
  const int BAND_1 =  80 * 100;

  const int SPEED_DEV_LIMIT_0 =  5 * 100;     // Deviation from last measurement to be valid. Band_0: 0 to 10 knots
  const int SPEED_DEV_LIMIT_1 = 10 * 100;     // Deviation from last measurement to be valid. Band_1: 10 to 80 knots
  const int SPEED_DEV_LIMIT_2 = 30 * 100;     // Deviation from last measurement to be valid. Band_2: 80+ knots

  uint8_t pin = 4;
 
  auto* sensor = new DigitalInputDebounceCounter(pin, INPUT_PULLUP, RISING, read_delay, DEBOUNCE, config_path_read_delay);

  sensor
      /*->connect_to (new VDO_Filter (
                                    BAND_0, BAND_1, 
                                    SPEED_DEV_LIMIT_0, SPEED_DEV_LIMIT_1, SPEED_DEV_LIMIT_2,
                                    TIMEOUT, config_path_TIMEOUT
                                  ))*/

       ->connect_to(new Frequency(
          multiplier, config_path_calibrate))  // connect the output of sensor
                                               // to the input of Frequency()                         
       ->connect_to(new SKOutputFloat(
          sk_path, config_path_skpath,
          new SKMetadata ("hz", "Anemometer cups rotation frequency")
          ));  // connect the output of Frequency()
               // to a Signal K Output as a number
  // Start networking, SK server connections and other SensESP internals
  sensesp_app->start();
}

void loop() {app.tick();}