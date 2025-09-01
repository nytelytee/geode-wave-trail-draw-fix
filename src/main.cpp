#include <Geode/Geode.hpp>
#include <Geode/loader/Dispatch.hpp>

#include <constants.hpp>
#include <utilities.hpp>
#include <globals.hpp>
#include <hooks/HardStreak.hpp>

using namespace geode::prelude;

std::vector<utilities::Unit> configuration;

$execute {

  // set the global configuration, silently ignore config parsing errors
  new EventListener(+[](matjson::Value config) {
      Result<std::vector<utilities::Unit>> newConfigResult = config.as<std::vector<utilities::Unit>>();
      if (newConfigResult.isOk()) configuration = *newConfigResult.ok();
      return ListenerResult::Stop;
  }, SetConfigurationFilter("set-configuration"_spr));

  // set the global configuration, put config parsing errors into second argument
  new EventListener(+[](matjson::Value config, std::optional<std::string>* error) {
      Result<std::vector<utilities::Unit>> newConfigResult = config.as<std::vector<utilities::Unit>>();
      if (newConfigResult.isOk()) configuration = *newConfigResult.ok();
      *error = newConfigResult.err();
      return ListenerResult::Stop;
  }, SetConfigurationFilterResult("set-configuration"_spr));

  // get the global configuration
  new EventListener(+[](matjson::Value* config) {
      *config = configuration;
      return ListenerResult::Stop;
  }, GetConfigurationFilter("get-configuration"_spr));

  // get the configuration of a specific HardStreak
  new EventListener(+[](HardStreak* streak, matjson::Value* config) {
      *config = static_cast<HookedHardStreak*>(streak)->m_fields->m_configuration;
      return ListenerResult::Stop;
  }, GetSpecificConfigurationFilter("get-configuration"_spr));

  // these are named differently from set-configuration, as they actually update a streak's configuration explicitly
  // HardStreaks store their configurations after they're set, so subsequent updates to the global configuration will
  // not affect already existing HardStreaks, to explicitly change those configurations, you call these events
  
  // update the configuration of a specific HardStreak, silently ignore config parsing errors
  new EventListener(+[](HardStreak* streak, matjson::Value config) {
      Result<std::vector<utilities::Unit>> newConfigResult = config.as<std::vector<utilities::Unit>>();
      if (newConfigResult.isOk()) static_cast<HookedHardStreak*>(streak)->setConfiguration(*newConfigResult);
      return ListenerResult::Stop;
  }, UpdateConfigurationFilter("update-configuration"_spr));

  // update the configuration of a specific HardStreak, put config parsing errors into second argument
  new EventListener(+[](HardStreak* streak, matjson::Value config, std::optional<std::string>* error) {
      Result<std::vector<utilities::Unit>> newConfigResult = config.as<std::vector<utilities::Unit>>();
      if (newConfigResult.isOk()) static_cast<HookedHardStreak*>(streak)->setConfiguration(*newConfigResult);
      *error = newConfigResult.err();
      return ListenerResult::Stop;
  }, UpdateConfigurationFilterResult("update-configuration"_spr));

};

matjson::Value defaultConfiguration = matjson::parse(R"(
  [
    {
      "parts": [{
        "type": "nytelyte.wave_trail_draw_fix/color",
        "data": {"color": null}
      }]
    },
    {
      "start": -0.3333333,
      "end": 0.3333333,
      "parts": [{
          "type": "nytelyte.wave_trail_draw_fix/color",
          "data": {
              "color": "#ffffff",
              "opacity-factor": 1.0,
              "skip-if-solid": true
          }
      }]
    }
  ]
)").unwrap();

$on_mod(Loaded) {
  // you can play around with adding a custom configuration using this, it is however intentionally not supported right now, hence the _ prefix
  // playing around will require reading and understanding the source code to know what options are supported;
  // custom configurations will be supported Soon:tm:; the intent is to make a mod that depends on this one, which would add wave trails to the icon kit
  // that mod would be the one taking care of the custom configurations
  // for now, here's an example of what you can do:
  /*
  [
    {
      "parts": [
        {"type": "nytelyte.wave_trail_draw_fix/color", "data": {"color": "#E50000"}},
        {"type": "nytelyte.wave_trail_draw_fix/color", "data": {"color": "#FF8D00"}},
        {"type": "nytelyte.wave_trail_draw_fix/color", "data": {"color": "#FFEE00"}},
        {"type": "nytelyte.wave_trail_draw_fix/color", "data": {"color": "#028121"}},
        {"type": "nytelyte.wave_trail_draw_fix/color", "data": {"color": "#004CFF"}},
        {"type": "nytelyte.wave_trail_draw_fix/color", "data": {"color": "#770088"}}
      ]
    }
  ]
  */
  // different mods may also implement their own nodes for drawing ("type"), you can use ColorDrawNode as an example
  Mod* mod = Mod::get();
  if (mod->hasSavedValue("_custom_configuration")) {
    SetConfigurationEvent("set-configuration"_spr, mod->getSavedValue<matjson::Value>("_custom_configuration")).post();
  } else {
    SetConfigurationEvent("set-configuration"_spr, defaultConfiguration).post();
  }
}

