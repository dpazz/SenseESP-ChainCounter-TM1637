#ifndef _dir_transform_H_
#define _dir_transform_H_
 
#include "sensesp/transforms/transform.h"
 
namespace sensesp {
 
class DirTransform : public FloatTransform {
 public:
  DirTransform();
  virtual void set_input(float input, uint8_t inputChannel) override;
 
 private:
  uint8_t received = 0;
  float inputs[2];
};
 
}  // namespace sensesp
 
#endif