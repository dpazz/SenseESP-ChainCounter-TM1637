
#ifndef _digitalcounterdriver_H_
#define _digitalcounterdriver_H_

#include "sensesp/transforms/transform.h"

namespace sensesp {
/*
static const char DIGITAL_COUNTER_SCHEMA[] PROGMEM = R"({
    "type": "object",
    "properties": {
        "duration_ms": { "title": "pulse_duration_ms", "type": "number" },
        "value" {"title": "Value_displayed", "type": "number"}
    }
  })";
*/
class DigitalCounterDriver : public FloatTransform  {
    
    public:
    DigitalCounterDriver (float value = 0, 
                          uint8_t inc_pin = 16, uint8_t dec_pin = 17, uint8_t res_pin = 18,
                          unsigned int duration_ms = 50,
                          String config_path = "") : FloatTransform (config_path) {
        this->load_configuration();
        inc_pin_ = inc_pin;
        init_pin(inc_pin_, LOW);
        dec_pin_ = dec_pin;
        init_pin(dec_pin_, LOW);
        res_pin_ = res_pin;
        init_pin(res_pin_,LOW);
        duration_ms_ = duration_ms;
        value_ = round(value);
        debugD("After DigitalCounterDriver load_configuration - value_ = %02d", value_);
        displayed_value = value_;
        //if ( value != 0 ) starting = false; else starting = true;
        // pulse on reset pin for 1 sec.
        //pulse_pin(res_pin_, 1000, HIGH);
    }

    virtual void init_pin (uint8_t pin, bool level) {
        pinMode(pin, OUTPUT);
        digitalWrite (pin, level);
    }

    virtual void pulse_pin (uint8_t pin, unsigned int duration_ms = 15, bool to_level = HIGH) {
        digitalWrite (pin, to_level);
        delay (duration_ms);
        digitalWrite (pin,!to_level);
    }
    
    virtual void set_input (float input, uint8_t inputchannel = 0) {
        value_ = round(input);
        if (value_ == 0) { //means that "input" is between 0 and 0,5
            if (!already_reset) {
                if ( displayed_value > 0  )  { //decrementing 
                        debugD("pulse on reset pin: decrement reaches 0");
                        stop_decrement = true;
                        pulse_pin(res_pin_, 2*duration_ms_, HIGH);
                } else  debugD("don't need to pulse on reset pin: displayed_value already = 0");
                already_reset = true;
            }
        } else {
            if ((value_- displayed_value) >= 1) { // incrementing
                if ( !starting ) {
                    debugD("pulse on increment pin");
                    pulse_pin(inc_pin_, duration_ms_, HIGH);
                }  
                starting = false; // to avoid spurious first increment at power on
            }
            else if ((value_- displayed_value) <= -1) { //decrementing
                if (!stop_decrement){
                    debugD("pulse on decrement pin");
                    pulse_pin(dec_pin_, duration_ms_, HIGH);
                }
            }
        }
        displayed_value = value_; // displayed_value = the positive integer digit on digital counter
        if (displayed_value > 0) {stop_decrement = false; already_reset = false;}
        this->emit(input);
    }
/*
    virtual void get_configuration(JsonObject& doc) override final {
    doc["duration_ms"] = duration_ms_;
    doc["value"] = value_;
  }
  virtual bool set_configuration(const JsonObject& config) override final {
    String expected[] = {"k", "r", "value"};
    for (auto str : expected) {
      if (!config.containsKey(str)) {
        return false;
      }
    }
    duration_ms_ = config["duration_ms"];
    value_ = config["value"];
    return true;
  }
  virtual String get_config_schema() override {
    return FPSTR(DIGITAL_COUNTER_SCHEMA);
  }
*/

    private:
    uint8_t inc_pin_, dec_pin_, res_pin_;
    unsigned int duration_ms_;
    int value_ = 0;
    int displayed_value = 0;
    bool stop_decrement = false;
    bool already_reset = false;
    bool starting = true;

} ;
}  // namespace sensesp
#endif
