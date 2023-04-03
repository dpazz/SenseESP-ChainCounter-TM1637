#include "VDO_filter.h"
 
namespace sensesp {
 
static float absf(float val) {
  if (val < 0) {
    return -val;
  } else {
    return val;
  }
}

static float calc_WS (float new_value){
    // The following converts revolutions per 100 seconds (rps) to knots x 100
    // This calculation follows the Peet Bros. piecemeal calibration data
    float ms, rps= new_value*100; // m/s stored as m/s*100
        // Formule valide per anemometro Peet-Bros da rivedere per VDO/STOWE MHU
        if (rps < 323)
        {
          ms = (rps * rps * -11)/11507 + (293 * rps)/115 - 12;
        }
        else if (rps < 5436)
        {
          ms = (rps * rps / 2)/11507 + (220 * rps)/115 + 96;
        }
        else
        {
          ms = (rps * rps * 11)/11507 - (957 * rps)/115 + 28664;
        }
        // fine formule Peet-Bros anemometro
    return ms/100;

}


 
VDO_Filter::VDO_Filter(int band_0, int band_1, int max_dev_0, int max_dev_1, int max_dev_2, int timeout,
                           String config_path)
    : FloatTransform(config_path),
      band_0_{band_0},
      band_1_{band_1},
      max_dev_0_{max_dev_0},
      max_dev_1_{max_dev_1},
      max_dev_2_{max_dev_2},
      timeout_{timeout} {
  load_configuration();
  
}
 
void VDO_Filter::set_input(float new_value, uint8_t input_channel) {
  
  float time_new = millis();
  //if ((time_new-time_old) < timeout_) {
    float new_out = calc_WS (new_value);
  //  float delta = absf(new_out - output);
  //  if (new_value < band_0_) {
  //      if (delta < max_dev_0_) this->emit(new_out);
  //  }
  //  else if (new_value < band_1_) {
  //      if (delta < max_dev_1_) this->emit(new_out);
  /*  } else if (delta < max_dev_2_) */ this->emit(new_out);
  //}
  time_old = time_new;
}
 
void VDO_Filter::get_configuration(JsonObject& root) {
  root["band_0"] = band_0_;
  root["band_1"] = band_1_;
  root["max_dev_0"] = max_dev_0_;
  root["max_dev_1"] = max_dev_1_;
  root["max_dev_2"] = max_dev_2_;
  root["timeout"] = timeout_;
}
 
static const char SCHEMA[] PROGMEM = R"({
    "type": "object",
    "properties": {
        "band_0": { "title": "First WS band", "description": "Within this number of Kn from 0 ", "type": "number" },
        "band_1": { "title": "Second WS band", "description": "Within this number of Kn from band_0 limit ", "type": "number" },
        "max_dev_0": { "title": "Maximum pct deviation in band_0", "description": "Maximum percentage of difference in change of value to allow forwarding in band_0", "type": "number" },
        "max_dev_1": { "title": "Maximum pct deviation in band_1", "description": "Maximum percentage of difference in change of value to allow forwarding in band_1", "type": "number" },
        "max_dev_2": { "title": "Maximum pct deviation in band_2", "description": "Maximum percentage of difference in change of value to allow forwarding over band_1", "type": "number" },
        "timeout": { "title": "timeout", "description": "Maximum time between consecutive filtered inputs to allow forwarding", "type": "number" }
    }
  })";
 
String VDO_Filter::get_config_schema() { return FPSTR(SCHEMA); }
 
bool VDO_Filter::set_configuration(const JsonObject& config) {
  String expected[] = {"band_0", "band_1", "max_dev_0", "max_dev_1", "max_dev_2", "timeout"};
  for (auto str : expected) {
    if (!config.containsKey(str)) {
      return false;
    }
  }
  band_0_ = config["band_0"];
  band_1_ = config["band_1"];
  max_dev_0_ = config["max_dev_0"];
  max_dev_1_ = config["max_dev_1"];
  max_dev_2_ = config["max_dev_2"];
  timeout_ = config["timeout"];
  return true;
}
 
}  // namespace sensesp

