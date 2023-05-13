#include "dir_transform.h"
 
namespace sensesp {
 
// direction (rad) from phase (sine, cosine) readings
// a value is spit out when both phases are received
 
DirTransform::DirTransform() : FloatTransform() {}
 
void DirTransform::set_input(float input, uint8_t inputChannel) {
  inputs[inputChannel] = input;
  received |= 1 << inputChannel;
  if (received ==
      0b11) {  // for 2 channels, use 0b11. For 3 channels, use b111 and so on.
    received = 0;  // recalculates after all values are updated. Remove if a
                   // recalculation is required after an update of any value.
 
 
    this->emit(atan2(inputs[0],inputs[1]));  // rad output calculating quadrant thanks to inputs sign
  }
}

 
}  // namespace sensesp