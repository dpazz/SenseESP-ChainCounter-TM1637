
#ifndef _digitalcounterdriver_H_
#define _digitalcounterdriver_H_

#include "sensesp/transforms/transform.h"
#include "sensesp/transforms/lambda_transform.h"

namespace sensesp {
        
  // This is an array of parameter information, providing the keys and
  // descriptions required to display user-friendly values in the configuration
  // interface.

  const ParamInfo* DigitalCounterDriver_lambda_param = new ParamInfo[4]{
         {"increment_pin", "Increment_pin"}, {"decrement_pin", "Decrement_pin"}, {"reset_pin", "Reset_pin"},
         {"duration_ms", "Duration_ms"}};

    // Here we create a new LambdaTransform objects. The template parameters
    // (five floats in this example) correspond to the following types:
    //
    // 1. Input type of the transform function
    // 2. Output type of the transform function
    // 3. Type of parameter 1
    // 4. Type of parameter 2
    // 5. Type of parameter 3
    //
    // The function arguments are:
    // 1. The tranform function
    // 2-4. Default values for parameters
    // 5. Parameter data for the web config UI
    // 6. Configuration path for the transform

  class DigitalCounterDriver : public LambdaTransform <float, float, uint8_t, uint8_t, uint8_t, uint32_t>{
    public:
    DigitalCounterDriver  //<float, uint8_t, uint8_t, uint8_t, uint32_t, String>
                         ( float value = 0, 
                          uint8_t inc_pin = 16, uint8_t dec_pin = 17, uint8_t res_pin = 18,
                          uint32_t duration_ms = 50,
                          String config_path = "") : inc_pin_(inc_pin),
                                                     dec_pin_(dec_pin),
                                                     res_pin_(res_pin),
                                                     duration_ms_(duration_ms),
                                                      LambdaTransform
                                                        (digital_counter_function ( {value},
                                                                                    {inc_pin},
                                                                                    {dec_pin},
                                                                                    {res_pin},
                                                                                    {duration_ms}),

                                                                                      //<IN> & <OUT>  float
                                                         //value,
                                                         inc_pin, dec_pin, res_pin, //uint8_t's
                                                         duration_ms,                 //uint32_t
                                                         DigitalCounterDriver_lambda_param, config_path)
    
    {   
        debugD("start of new DigitalCounterDriver");
        while (true) {};
        this->load_configuration();
        this->set_description(
                              "counterdisplay drives the counting on a physical counter display "
                              "in parallel with what is tranmitted to signalk "
                              "output = input that is used only to drive the physical unit");
        //inc_pin_ = inc_pin;
        init_pin(inc_pin_, LOW);
        //dec_pin_ = dec_pin;
        init_pin(dec_pin_, LOW);
        //res_pin_ = res_pin;
        init_pin(res_pin_,LOW);
        //duration_ms_ = duration_ms;
        value_ = round(value);
        debugD("After DigitalCounterDriver load_configuration - value_ = %02d", value_);
        displayed_value = value_;
        //if ( value != 0 ) starting = false; else starting = true; //to be revised if it's still useful
    }


    virtual void init_pin (uint8_t pin, bool level) {
        pinMode(pin, OUTPUT);
        digitalWrite (pin, level);
    }

    virtual void pulse_pin (uint8_t pin, uint32_t duration_ms = 15, bool to_level = HIGH) {
        digitalWrite (pin, to_level);
        delay (duration_ms);
        digitalWrite (pin,!to_level);
    }

    private:

    // This transform function "drives" the counting on a physical digital counter
    // using its pin interface to inc, dec or reset the count. The final "-> float"
    // refers to the return type of the function.
    // As for the input tranformation it is left unchanged. The lambda
    // transfom in this particular case operates as a "stub" to drive
    // the counter display device remaining a "nop" as for information transfer
   
    std::function <float (float,uint8_t,uint8_t,uint8_t,uint32_t)>
    digital_counter_function = [&] (float value ,
                                                                                                   uint8_t inc_pin,
                                                                                                   uint8_t dec_pin,
                                                                                                   uint8_t res_pin,
                                                                                                   uint32_t duration_ms
                                                                                                  )->float {
        value_ = round(value);
        if (value_ == 0) { //means that "input" is less than 0,5
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
            if ( !stop_decrement ){
              debugD("pulse on decrement pin");
              pulse_pin(dec_pin_, duration_ms_, HIGH);
            }
          }
        }
        displayed_value = value_; // displayed_value = the positive integer digit on digital counter
        if (displayed_value > 0) {stop_decrement = false; already_reset = false;}                                 
        return value ;
    };
    
    uint8_t inc_pin_, dec_pin_, res_pin_;
    uint32_t duration_ms_;
    int value_ = 0;
    int displayed_value = 0;
    bool stop_decrement = false;
    bool already_reset = false;
    bool starting = true;

  };
}  // namespace sensesp
#endif
