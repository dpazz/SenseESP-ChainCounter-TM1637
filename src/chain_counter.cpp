
#include <Arduino.h>
#include "TM1637.h"
#include "mysignalk_value_listener.h"
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
int CLK_PIN = 22;
int DIO_PIN = 21                                                                                                                                                                                              ;
float displayed_value = 0; //stores the last value sent to LCD
TM1637 four_digit (CLK_PIN, DIO_PIN);
/**
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

bool stop_decrement = false;
bool already_reset = false;
bool first_run = true;
float value = 0.0f;                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                            
float value_ = 0.0f; //the rounded to 1 decimal value

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

   int COUNTER_PIN = 16;

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

   int UP_PIN = 17;

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

  

  four_digit.begin(); //start lcd device

  /**
   * An IntegratorT<int, float> called "accumulator" adds up all the counts it
   * receives (which are ints) and multiplies each count by gypsy_circum, which
   * is the amount of chain, in meters, that is moved by each revolution of the
   * windlass. (Since gypsy_circum is a float, the output of this transform must
   * be a float, which is why we use IntegratorT<int, float>). It can be
   * configured in the Config UI at accum_config_path. The configs are:
   * float gypsy_circum, float anchor_deployed and int reset 
   */
   String accum_config_path = "/accumulator/config";
   float gypsy_circum = 0.32;
   auto* accumulator =
      new Integrator (gypsy_circum, value, accum_config_path);
    accumulator->set_description(
      "accumulator adds up all the counts it receives multiplied "
      "by the float gypsy_circum (circonferenza_barbotin in italian), "
      "which is the amount of chain, in meters, that is moved by each revolution "
      "of the windlass");

    value_ = (round(value = accumulator->value) * 10)/10;
    debugD("After accumulator load_configuration - value_ = %00.0f", value_);
    if ( value_ > 0 ) {
      stop_decrement = false;
      already_reset = false;
    } else {
      stop_decrement = true;
      already_reset = true;
    }
    four_digit.setFloatDigitCount(2);
    four_digit.setDp(2);
    four_digit.clearScreen();  // Resets the digital display
    four_digit.offMode();
    four_digit.colonOff();
    if (value_ > 0) {
      uint8_t offset = 0U;
      four_digit.onMode();
      if (value_ <= 9.90 ){offset = 1U;}
      //four_digit.colonOn();
      four_digit.display(value_,true,true,offset);
    }
    displayed_value = value_;

  /**
   * An intermediate transform that catch the emitted value from the accumulator
   * to drive a TM1637 4-digit LCD display to show the count in parallel with what
   * is tranmitted to signalk. The transform, after having used the value to manage
   * the physical counter, emits the value itself in order to be used (if necessary)
   * to further processing steps. In the present case the value itself is connected 
   * to skoutputfloat
   */
   
  // This lambda function "drives" the counting on LCD TM1637 display  
  // using the TM1637 class "wrapper" to display the count. The
  // final "-> float" refers to the return type of the function
  // As for the input tranformation it is left unchanged. The lambda
  // function in this particular case "spills" the input value just 
  // to drive the 4digit display remaining a "nop" as for information
  // transfer
  auto digital_counter_function = [gypsy_circum] (float input,
                                      float& digit_value_displayed
                                     ) -> float {
        value_ = float(round(input*10)/10); // prepare a rounded float
                                            // with 1 significant decimal
        if (digit_value_displayed >= value_)  {         // decrementing 
          if (value_ <= gypsy_circum ) {    // nearby all chain recovered
            if ( !already_reset  )  {       // only the first time
                debugD("reset LCD: decrement nearby 0");
                stop_decrement = true;
                four_digit.clearScreen();
                four_digit.display(00.00f);
                four_digit.offMode();
              } else  debugD("don't need to reset LCD again: displayed_value already = 0");
              already_reset = true;
          }
        }
        if ( !stop_decrement ){
              debugD("write value_ on lcd");
              if (value_ > 0) {
                uint8_t offset = 0U;
                four_digit.onMode();
                if (value_ <= 9.90 ){offset = 1U;};
                //four_digit.colonOn();
                four_digit.display(value_,true,true,offset);
              }  
        }
        digit_value_displayed = value_; // lcd_value = the rounded float with 1 decimal on digital counter
        if (digit_value_displayed > 0) {stop_decrement = false; already_reset = false;}
        return value = input ;//NOP as for info transfer   
                                
  };
          
  // This is an array of parameter information, providing the keys and
  // descriptions required to display user-friendly values in the configuration
  // interface.

    const ParamInfo* digitcounter_param_webUI_info_struct = new ParamInfo[1]{
         {"displayed_value", "auxScreen_value"}};

    String digitcounter_config_path = "/digitcounter/config";
    //HINT: use only int, float or bool parameters because
    //the web UI automatic detect of types to build
    //interface schema doesn't work with other parameter
    //types like e.g char or uint8_t. At least for the
    //current SignalK/SensESP release (2.7.2 at 08 Jan 2024)

    auto counterdisplay = new LambdaTransform <float, float, float&>
                                               (  digital_counter_function,
                                                  displayed_value,
                                                  digitcounter_param_webUI_info_struct,
                                                  digitcounter_config_path 
                                               ) ;
    counterdisplay->set_description(
      "counterdisplay drives the counting on a TM1637 lcd display "
      "(in parallel with what is transmitted to signalk) using TM1637 wrapper class "
      "output is equal to input since the transform is used to drive the physical unit only");
 
   
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
   * chain_counter is connected to accumulator, which is connected to 
   * the counterdisplay "stub" and finally to the SKOutputNumber, 
   * which sends the final result to the indicated path on the Signal K server.
   * (Note that each data type has its own version of SKOutput:
   * SKOutputNumber for floats, SKOutputInt, SKOutputBool, and SKOutputString.)
   */
   String sk_path = "navigation.anchor.rodeDeployed";
   String sk_path_config_path = "/rodeDeployed/sk";

   up->connect_to(accumulator, 1); //channel 1 for up/down deploying direction
   chain_counter->connect_to(accumulator)
      ->connect_to(counterdisplay)
      ->connect_to(new SKOutputFloat(sk_path, sk_path_config_path, metadata));
                    
   /**
   * a FloatValueListener of the SignalK path containing the value of the remote reset 
   * of rode deployed. The config parameter of this listener is the time between readings
   * of the subject SignalK path.
   * if the value received from this path is = 1 the counter will be reset (as if the
   * local reset button was pressed) by connecting it to rreset_consumer
   */

   
   String rreset_sk_path = "navigation.anchor.rodeDeployedRemoteReset"; // to be evaluated if it has to be made configurable 
   int rreset_read_delay = 800; // should be a little less of the update interval of the SK path value so as to avoid info "lags"
   String rreset_config_path = "/remote_reset/config";
   auto*  rreset_rode_deployed =new IntSKListener (
              rreset_sk_path,
              rreset_read_delay,
              rreset_config_path
           );
  /**
   * DigitalInputState monitors a physical button connected to BUTTON_PIN.
   * Because its interrupt type is CHANGE, it will emit a value when the button
   * is pressed, and again when it's released, but that's OK - our
   * LambdaConsumer function will act only on the press, and ignore the release.
   * DigitalInputChange looks for a change every read_delay ms, which can be
   * configured at read_delay_config_path in the Config UI.
   */
 
   uint8_t BUTTON_PIN = 35;

   
   int interrupt_type = CHANGE;
   String button_config_path = "";//"/button_watcher/interrupt_type"; 
   
   auto* button_watcher = new DigitalInputChange(
      BUTTON_PIN, INPUT_PULLUP, interrupt_type, button_config_path);
   
   /*
   int button_read_delay = 20;
   String button_config_path = "/button_watcher/read_delay"; 
   
   auto* button_watcher = new DigitalInputState(
      BUTTON_PIN, INPUT, button_read_delay, button_config_path);
   */
  
  /**
   * Create a DebounceInt to make sure we get a nice, clean signal from the
   * button. Set the debounce delay period to 15 ms, which can be configured at
   * debounce_config_path in the Config UI.
   */
   
   int debounce_delay = 50;
   String debounce_config_path = "/debounce/delay";
   auto* debounce = new DebounceInt(debounce_delay, debounce_config_path);
   
  /**
   * When the button is pressed (or released), it will call the lambda
   * expression (or "function") that's called by the LambdaConsumer. This is the
   * function - notice that it calls reset() only when the input is 0, which
   * indicates a button press. It ignores the button release. If your button
   * goes to GND when pressed, make it "if (input == 0)".
   */
   
   auto reset_function = [accumulator](int input) {
    if (input == 0 && !first_run) {         // first_run bool intoduced to avoid
                                            // spurious interrups catched by reset pin
                                            // at boot time.
      debugD("reset_function called");
      accumulator->reset();                 // Resets the output and stored value to 0.0
      value_ = 0.0f;
      value = 0.0f;
      already_reset= true;
      four_digit.clearScreen();// Resets to 0 the digital display
      four_digit.display(value_,true ,true,0);
      four_digit.offMode();
    }
    first_run = false;
   };

  auto* button_consumer = new LambdaConsumer<int>(reset_function);
  auto* rreset_consumer = new LambdaConsumer <int> (reset_function);

  /* Connect the button_watcher to the debounce to the button_consumer. */
  button_watcher->connect_to(debounce)->connect_to(button_consumer);
  
  /* Connect the rrset_consumer to the rreset_rode_deployed. */
  rreset_rode_deployed->connect_to(rreset_consumer);
  /* Finally, start the SensESPApp */
  sensesp_app->start();
  ota.start();
};

// The loop function is called in an endless loop during program execution.
// It simply calls `app.tick()` which will then execute all reactions as needed.
void loop() { app.tick(); }
