#include "sensesp/sensors/digital_input.h"
#include "sensesp/signalk/signalk_output.h"
#include "sensesp/system/lambda_consumer.h"
#include "sensesp/transforms/debounce.h"
#include "sensesp_app.h"
#include "sensesp_app_builder.h"
#include "digitalcounterdriver.h"
#include "myintegrator.h"
#if !(USING_DEFAULT_ARDUINO_LOOP_STACK_SIZE)
  uint16_t USER_CONFIG_ARDUINO_LOOP_STACK_SIZE = 16384;
#endif

using namespace sensesp;

/**
 * This example illustrates an anchor chain counter. Note that it
 * doesn't distinguish between chain being let out and chain being
 * taken in, so the intended usage is this: Press the button to make
 * sure the counter is at 0.0. Let out chain until the counter shows
 * the amount you want out. Once the anchor is set, press the button
 * again to reset the counter to 0.0. As you bring the chain in, the
 * counter will show how much you have brought in. Press the button
 * again to reset the counter to 0.0, to be ready for the next anchor
 * deployment.
 *
 * A bi-directional chain counter is possible,
 * this is an example that supersedes the basic implementation found in
 * sensesp examples using the "down" button of the windlass read through a
 * digitalinput pin as a direction (up or down) driver. The fore amentioned
 * reset function is still valid to set the count to 0 value, but in this
 * implementation the value emitted should count the effective chain
 * meters deployed.
 */

IPAddress staticIP (10,10,10,4);
IPAddress gateway(10,10,10,1);
IPAddress subnetmask (255,255,255,0);
IPAddress dns1 (10,10,10,1);
IPAddress dns2 (8,8,8,8);

ReactESP app;


void setup() {
  SetupSerialDebug(115200);

  SensESPAppBuilder builder;
   if (WiFi.config(staticIP, gateway, subnetmask, dns1, dns2) == false) {
    debugD("Fixed IP configuration failed."); delay(5000);
   }
   sensesp_app = builder.set_hostname("ChainCounter")
                    ->set_wifi("4G-WIFI-FREE-TIME", "pazzagliawless")
                    ->get_app();
  /**
   * DigitalInputCounter will count the revolutions of the windlass with a
   * Hall Effect Sensor connected to COUNTER_PIN. It will output its count
   * every counter_read_delay ms, which can be configured in the Config UI at
   * counter_config_path.
   */

   uint8_t COUNTER_PIN = 19;

   unsigned int counter_read_delay = 1000;
   String counter_config_path = "/chain_counter/read_delay";
   auto* chain_counter =
      new DigitalInputCounter(COUNTER_PIN, INPUT_PULLUP, RISING,
                              counter_read_delay, counter_config_path);
  /**
   * Using one digital input pin connected to the up button driving
   * the windlass is it possible to count bidirectionally the chain deployed or
   * recovered summing or subtracting gypsy circum to accumulator (see MyTransform
   * class definition above). The digitalinputState of this pin is connected to the
   * channel 1 input of MyIntegrator transfom in order to decrement the accumulator
   * if this pin is at LOW (0) value. It is implicity assumed that if the digital pin
   * connected to the up button is in HIGH (1) state the chain is deployed downward.
   */

   uint8_t UP_PIN = 21;

   unsigned int up_read_delay = 200;
   String up_config_path = "/up/up_read_delay";
   auto* up =
      new DigitalInputState(UP_PIN, INPUT_PULLUP, up_read_delay,up_config_path);

  /**
   * An IntegratorT<int, float> called "accumulator" adds up all the counts it
   * receives (which are ints) and multiplies each count by gypsy_circum, which
   * is the amount of chain, in meters, that is moved by each revolution of the
   * windlass. (Since gypsy_circum is a float, the output of this transform must
   * be a float, which is why we use IntegratorT<int, float>). It can be
   * configured in the Config UI at accum_config_path.
   */
   float gypsy_circum = 0.32;
   String accum_config_path = "/accumulator/config";
   auto* accumulator =
      new Integrator (gypsy_circum, 0.0, accum_config_path);

  /**
   * An intermediate transform that catch the emitted value froam the accumulator
   * to drive the counting on a physical counter display in parallel with what
   * is tranmitted to signalk. The counter has both increment and decrement
   * wires plus the reset wire (to be used when the accumulator is reset). The
   * transform after having used the value to manage the counter emits the value
   * itself in order to be connected to skoutputfloat
   */
   uint8_t COUNTER_INC_PIN = 16;
   uint8_t COUNTER_DEC_PIN = 17;
   uint8_t COUNTER_RES_PIN = 18;
   unsigned int pulse_length_ms = 55;
   //float INIT_DISPLAYED_VALUE = 0;
   String digitcounter_config_path = "/digitcounter/config";
  auto* counterdisplay = new DigitalCounterDriver
                                                ( 0.0f,//INIT_DISPLAYED_VALUE,
                                                  COUNTER_INC_PIN, COUNTER_DEC_PIN, COUNTER_RES_PIN,
                                                  pulse_length_ms,
                                                  digitcounter_config_path 
                                                 );

  /**
   * There is no path for the amount of anchor rode deployed in the current
   * Signal K specification. By creating an instance of SKMetaData, we can send
   * a partial or full defintion of the metadata that other consumers of Signal
   * K data might find useful. (For example, Instrument Panel will benefit from
   * knowing the units to be displayed.) The metadata is sent only the first
   * time the data value is sent to the server.
   */
   SKMetadata* metadata = new SKMetadata();
   metadata->units_ = "m";
   metadata->description_ = "Anchor Rode Deployed";
   metadata->display_name_ = "Rode Deployed";
   metadata->short_name_ = "Rode Out";

  /**
   * chain_counter is connected to accumulator, which is connected to an
   * SKOutputNumber, which sends the final result to the indicated path on the
   * Signal K server. (Note that each data type has its own version of SKOutput:
   * SKOutputNumber for floats, SKOutputInt, SKOutputBool, and SKOutputString.)
   */
   String sk_path = "navigation.anchor.rodeDeployed";
   String sk_path_config_path = "/rodeDeployed/sk";

   up->connect_to(accumulator, 1); //channel 1 for up/down deploying direction
   chain_counter->connect_to(accumulator)
      ->connect_to(counterdisplay)
      ->connect_to(new SKOutputFloat(sk_path, sk_path_config_path, metadata));

  /**
   * DigitalInputChange monitors a physical button connected to BUTTON_PIN.
   * Because its interrupt type is CHANGE, it will emit a value when the button
   * is pressed, and again when it's released, but that's OK - our
   * LambdaConsumer function will act only on the press, and ignore the release.
   * DigitalInputChange looks for a change every read_delay ms, which can be
   * configured at read_delay_config_path in the Config UI.
   */
  /*
   int read_delay = 10;
   String read_delay_config_path = "/button_watcher/read_delay";
   auto* button_watcher = new DigitalInputChange(BUTTON_PIN, INPUT_PULLUP, CHANGE,
                                                read_delay_config_path);
  */
 
   uint8_t BUTTON_PIN = 34;

   int read_delay = 10;
   String read_delay_config_path = "/button_watcher/read_delay";
   
   auto* button_watcher = new DigitalInputState(
      BUTTON_PIN, INPUT, read_delay, read_delay_config_path);

  /**
   * Create a DebounceInt to make sure we get a nice, clean signal from the
   * button. Set the debounce delay period to 15 ms, which can be configured at
   * debounce_config_path in the Config UI.
   */
   int debounce_delay = 15;
   String debounce_config_path = "/debounce/delay";
   auto* debounce = new DebounceInt(debounce_delay, debounce_config_path);

  /**
   * When the button is pressed (or released), it will call the lambda
   * expression (or "function") that's called by the LambdaConsumer. This is the
   * function - notice that it calls reset() only when the input is 0, which
   * indicates a button press. It ignores the button release. If your button
   * goes to GND when pressed, make it "if (input == 0)".
   */
  
   auto reset_function = [accumulator, counterdisplay, COUNTER_RES_PIN](int input) {
    if (input == 1) {
      debugD("reset_function called");
      accumulator->reset();                                 // Resets the output to 0.0
      counterdisplay->pulse_pin(COUNTER_RES_PIN,1000, HIGH);// Resets to 0 the digital display
    }
   };

  /**
   * Create the LambdaConsumer that calls reset_function, Because DigitalInputChange
   * outputs an int, the version of LambdaConsumer we need isLambdaConsumer<int>.
   * While this approach - defining the lambda function (above) separate from the
   * LambdaConsumer (below) - is simpler to understand, there is a more concise approach:
   *
    auto* button_consumer = new LambdaConsumer<int>([accumulator, 
                                                     counterdisplay,
                                                     COUNTER_RES_PIN](int input) {
      if (input == 0) {
        accumulator->reset();
        counterdisplay->pulse_pin(COUNTER_RES_PIN,1000, LOW);
      }
    });
   *
  */
  auto* button_consumer = new LambdaConsumer<int>(reset_function);

  /* Connect the button_watcher to the debounce to the button_consumer. */
  button_watcher->connect_to(debounce)->connect_to(button_consumer);
  //accumulator->start();   // to save accumulator config and accumulated value
                            // of rode deployed

  /* Finally, start the SensESPApp */
  sensesp_app->start();
}

// The loop function is called in an endless loop during program execution.
// It simply calls `app.tick()` which will then execute all reactions as needed.
void loop() { app.tick(); }
