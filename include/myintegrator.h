#ifndef _myintegrator_H_
#define _myintegrator_H_

#include "sensesp/transforms/transform.h"

namespace sensesp {

static const char MY_INTEGRATOR_SCHEMA[] PROGMEM = R"###({
    "type": "object",
    "properties": {
        "k": { "title": "circonferenza_barbotin", "type": "number" },
        "value": {"title": "calumo", "type": "number"},
        "r": { "title": "reset", "type": "number" }
    }
  })###";
// calumo = italian translation of "rode deployed"
// circonferenza_barbotin = italian translation of "gypsy_circum"
/**
 * @brief My Integrator integrates (accumulates) the incoming values.
 *
 * The integrator output value is the sum of the all previous values plus
 * the latest value, multiplied by the coefficient k.
 * if the set_input is on channel 1 the value is subtracted instead
 *
 * @tparam C Consumer (incoming) data type
 * @tparam P Producer (output) data type
 */
template <class C, class P>
class MyIntegratorT : public Transform<C, P> {
 public:
  /**
   * @brief Construct a new Integrator T object
   *
   * @param k Multiplier coefficient
   * @param value Initial value of the accumulator
   * @param config_path Configuration path
   */
  MyIntegratorT(P k = 1, P value = 0, String config_path = "")
      : Transform<C, P>(config_path), k{k}, value{value} {   
    this->load_configuration();
  }

  virtual void start() override final {
    // save the integrator value every 10 s
    // NOTE: was disabled at first because interrupts start throwing
    // exceptions when the interval between savings was 5 s only.
    ReactESP::app->onRepeat(10000, [this](){ this->save_configuration(); });
  }

  virtual void set_input (P input, uint8_t inputChannel = 0) override final {
    if (inputChannel == 1) {
      if ( input == 0) decrement = true;
      else decrement = false;
      return;
    }
    P m = k;
    if (decrement) m = -k;
    if ( r == 1 ) {this->reset(); debugD ("reset accumulator by webUI");}
    else  value += input * m;
    if (value <= 0.01) value = 0; // neg not allowed and also cumulated roundings errors
    this->emit(value);
  }

  virtual void reset() final { value = 0; r = 0;}

  virtual void get_configuration(JsonObject& doc) override final{
    doc["k"] = k;
    doc["value"] = value;
    doc["r"] = r;
  }
  virtual bool set_configuration(const JsonObject& config) override final {
    String expected[] = {"k", "value", "r"};
    for (auto str : expected) {
      if (!config.containsKey(str)) {
        return false;
      }
    }
    k = config["k"];
    value = config["value"];
    r = config["r"];
    return true;
  }
  virtual String get_config_schema() override final {
    return FPSTR(MY_INTEGRATOR_SCHEMA);
  }

 P value = 0;
 private:
 int r = 0;
  P k;
  bool decrement = false;
};

typedef MyIntegratorT<float, float> Integrator;
typedef MyIntegratorT<int, int> Accumulator;

}  // namespace sensesp
#endif
