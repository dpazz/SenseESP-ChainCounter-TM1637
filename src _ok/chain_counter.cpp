
#include <Arduino.h>
#include "sensesp/sensors/digital_input.h"
#include "sensesp/signalk/signalk_output.h"
#include "sensesp/system/lambda_consumer.h"
#include "sensesp/transforms/debounce.h"
#include "sensesp/transforms/lambda_transform.h"
#include "sensesp_app.h"
#include "sensesp_app_builder.h"
#include "sensesp/system/startable.h"
#include "sensesp/net/ota.h"
#include "myintegrator.h"

#if !(USING_DEFAULT_ARDUINO_LOOP_STACK_SIZE)
  uint16_t USER_CONFIG_ARDUINO_LOOP_STACK_SIZE = 16384;
#endif

using namespace sensesp;
sensesp::OTA ota("123456789");
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
reactesp::ReactESP app;

IPAddress staticIP (10,10,10,4);
IPAddress gateway(10,10,10,1);
IPAddress subnetmask (255,255,255,0);
IPAddress dns1 (10,10,10,1);
IPAddress dns2 (8,8,8,8);

inline void init_pin (uint8_t pin, bool level) {
    pinMode(pin, OUTPUT);
    digitalWrite (pin, level);
}

inline void pulse_pin (uint8_t pin, uint32_t duration_ms = 15, bool to_level = HIGH) {
    digitalWrite (pin, to_level);
    delay (duration_ms);
    digitalWrite (pin,!to_level);
}   
    
int inc_pin_, dec_pin_, res_pin_;
int duration_ms_;
int value_ = 0;
int displayed_value = 0;
bool stop_decrement = false;
bool already_reset = false;
float value = 0.0f;

void setup() {
  SetupSerialDebug(115200);


  OTA ota("123456789");

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

   int COUNTER_PIN = 19;

   int counter_read_delay = 1000;
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
   * The high state is derived from the power supply (12V) being connected to up button,
   * after a voltage divider to output a logical TTL (3.3V) level
   */

   int UP_PIN = 21;

   int up_read_delay = 200;
   String up_config_path = "/up/up_read_delay";
   auto* up =
      new DigitalInputState(UP_PIN, INPUT_PULLUP, up_read_delay,up_config_path);
      up->set_description(
      "A digital input pin connected to the up button driving the windlass "
      "by which is it possible to count bidirectionally the chain deployed or "
      "recovered. It is implicitly assumed that if the digital pin connected to  "
      "the up button is in HIGH (1) state the chain is deployed downward"
      );

  /**
   * An intermediate transform that catch the emitted value froam the accumulator
   * to drive the counting on a physical counter display in parallel with what
   * is tranmitted to signalk. The counter has both increment and decrement
   * wires plus the reset wire (to be used when the accumulator is reset). The
   * transform, after having used the value to manage the physical counter, 
   * emits the value itself in order to be connected to skoutputfloat
   */
   int COUNTER_INC_PIN = 16;
   int COUNTER_DEC_PIN = 17;
   int COUNTER_RES_PIN = 18;
   int pulse_length_ms = 55;
   int inc_pin = COUNTER_INC_PIN;
   int dec_pin = COUNTER_DEC_PIN;
   int res_pin = COUNTER_RES_PIN;
   int pulse_duration = pulse_length_ms;
  // This lambda function "drives" the counting on digital display  
  // using the pin interface to inc, dec or reset the count. The
  // final "-> float" refers to the return type of the function
  // As for the input tranformation it is left unchanged. The lambda
  // function in this particular case operates as a "stub" to drive
  // the counter display device remaining a "nop" as for information transfer
 
  auto digital_counter_function = [] (float input,
                                      int inc_pin,
                                      int dec_pin,
                                      int res_pin,
                                      int duration_ms
                                     ) -> float  {
        value_ = round(input);
        if (value_ == 0) { //means that "input" is less than 0,5
          if (!already_reset) {
            if ( displayed_value > 0  )  { //decrementing 
                debugD("pulse on reset pin: decrement reaches 0");
                stop_decrement = true;
                pulse_pin(res_pin_, 2*duration_ms_, HIGH);
              } else  debugD("don't need to pulse on reset pin: displayed_value already = 0");
              already_reset = true;
          }
        } else if ((value_- displayed_value) >= 1) { // incrementing
              debugD("pulse on increment pin");
              pulse_pin(inc_pin_, duration_ms_, HIGH);
        } else if ((value_- displayed_value) <= -1) { //decrementing
            if ( !stop_decrement ){
              debugD("pulse on decrement pin");
              pulse_pin(dec_pin_, duration_ms_, HIGH);
            }
        }
        displayed_value = value_; // displayed_value = the positive integer digit on digital counter
        if (displayed_value > 0) {stop_decrement = false; already_reset = false;}
                                      
        return value = input ;
  };
          
  // This is an array of parameter information, providing the keys and
  // descriptions required to display user-friendly values in the configuration
  // interface.

    const ParamInfo* digitcounter_param_webUI_info_struct = new ParamInfo[4]{
         {"increment_pin", "Increment_pin"}, {"decrement_pin", "Decrement_pin"}, {"reset_pin", "Reset_pin"},
         {"duration_ms", "Pulse_duration_ms"}};

    String digitcounter_config_path = "/digitcounter/config";

    auto counterdisplay = new LambdaTransform <float, float, int, int, int, int>
                                               (  digital_counter_function,
                                                  inc_pin, dec_pin, res_pin,
                                                  pulse_duration,
                                                  digitcounter_param_webUI_info_struct,
                                                  digitcounter_config_path 
                                               ) ;
    counterdisplay->set_description(
      "counterdisplay drives the counting on a physical counter display "
      "(in parallel with what is transmitted to signalk) via inc, dec and res digital pins "
      "output is equal to input since the transform is used to drive the physical unit only");

    inc_pin_ = inc_pin;
    init_pin(inc_pin_, LOW);
    dec_pin_ = dec_pin;
    init_pin(dec_pin_, LOW);
    res_pin_ = res_pin;
    init_pin(res_pin_,LOW);
    duration_ms_ = pulse_duration;

  /**
   * An IntegratorT<int, float> called "accumulator" adds up all the counts it
   * receives (which are ints) and multiplies each count by gypsy_circum, which
   * is the amount of chain, in meters, that is moved by each revolution of the
   * windlass. (Since gypsy_circum is a float, the output of this transform must
   * be a float, which is why we use IntegratorT<int, float>). It can be
   * configured in the Config UI at accum_config_path. The configs are:
   * float gypsy_circum, float anchor_deployed and int reset 
   */
   float gypsy_circum = 0.32;
   String accum_config_path = "/accumulator/config";
   auto* accumulator =
      new Integrator (gypsy_circum, value, accum_config_path);
   accumulator->set_description(
      "accumulator adds up all the counts it receives multiplied "
      "by the float gypsy_circum (circonferenza_barbotin in italian), "
      "which is the amount of chain, in meters, that is moved by each revolution "
      "of the windlass");

    value_ = round(value = accumulator->value);
    debugD("After load_configuration - value_ = %02d", value_);
    if ( value_ >=0 ) {
      pulse_pin(res_pin_, 2*duration_ms_, HIGH); //reset digital display
      delay(duration_ms_);
      for (size_t i = 0; i < value_; i++) {pulse_pin (inc_pin_, duration_ms_, HIGH); delay (duration_ms_);}
        //set digital display to the value stored in SPIFFS
    }
    displayed_value = value_;

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
  
   auto reset_function = [accumulator, COUNTER_RES_PIN](int input) {
    if (input == 1) {
      debugD("reset_function called");
      accumulator->reset();                 // Resets the output and stored value to 0.0
      value_ = 0;
      value = 0.0f;
      already_reset= true;
      pulse_pin(COUNTER_RES_PIN,1000, HIGH);// Resets to 0 the digital display
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
  /* Finally, start the SensESPApp */
  sensesp_app->start();
  ota.start();
};

// The loop function is called in an endless loop during program execution.
// It simply calls `app.tick()` which will then execute all reactions as needed.
void loop() { app.tick(); }
