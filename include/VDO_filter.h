#ifndef VDO_filter_H
#define VDO_filter_H
#include "sensesp/transforms/transform.h"   // per bozza windspeed 
 
namespace sensesp {
 
class VDO_Filter : public FloatTransform {
 public:
  VDO_Filter(int band_0 = 0, int band_1 = 80,
               int max_dev_0 = 5, int max_dev_1 =10, int max_dev_2 = 30,
               int timeout = 1000,
               String config_path = "");
 
  virtual void set_input(float new_value, uint8_t input_channel = 0) override;
  virtual void get_configuration(JsonObject& doc) override;
  virtual bool set_configuration(const JsonObject& config) override;
  virtual String get_config_schema() override;
 
 protected:
  int band_0_;
  int band_1_;
  int max_dev_0_;
  int max_dev_1_;
  int max_dev_2_;
  int timeout_;
  float time_old = 0.0;
};
 
}  // namespace sensesp
 
#endif