#ifndef _mysk_value_listener_H
#define _mysk_value_listener_H

#include <ArduinoJson.h>

#include "sensesp/system/observable.h"
#include "sensesp/system/valueproducer.h"
#include "sensesp/signalk/signalk_listener.h"

namespace sensesp {
static const char MY_VALUE_LISTENER_SCHEMA[] PROGMEM = R"###({
    "type": "object",
    "properties": {
        "read_delay": { "title": "read_delay", "type": "number" },
        "sk_path": {"title": "sk_path", "type": "string"}
    }
  })###";
/**
 * @brief An ValueProducer that listens to specific Signal K paths
 * and emits its value whenever it changes.
 */
template <class T>
class MySKValueListener : public SKListener, public ValueProducer<T> {
 public:
  /**
   * @param sk_path The Signal K path you want to listen to for value changes
   * @param listen_delay The minimum interval between updates in ms
   */
  MySKValueListener(String sk_path, int listen_delay = 1000,
                  String config_path = "")
      : SKListener(sk_path, listen_delay, config_path) {
    this->load_configuration();
    if (sk_path == "") {
      debugE("SKValueListener: User has provided no sk_path to listen to.");
    }
  }

  void parse_value(const JsonObject& json) override {
    this->emit(json["value"].as<T>());
  }
  virtual void get_configuration(JsonObject& doc) override final{
    doc["read_delay"] = listen_delay;
    doc["sk_path"] = sk_path;
  }
  virtual bool set_configuration(const JsonObject& config) override final {
    String expected[] = {"read_delay", "sk_path"};
    for (auto str : expected) {
      if (!config.containsKey(str)) {
        return false;
      }
    }
    listen_delay = config["read_delay"];
    sk_path = config["sk_path"].as<const char*>();
    return true;
  }
  virtual String get_config_schema() override final {
    return FPSTR(MY_VALUE_LISTENER_SCHEMA);
  }

 private:
 int listen_delay;
 String sk_path;
};

typedef MySKValueListener<float> FloatSKListener;
typedef MySKValueListener<int> IntSKListener;
typedef MySKValueListener<bool> BoolSKListener;
typedef MySKValueListener<String> StringSKListener;

}  // namespace sensesp

#endif
