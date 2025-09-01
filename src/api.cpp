#include <constants.hpp>
#include <utilities.hpp>
#include <globals.hpp>
#include <hooks/HardStreak.hpp>

using namespace geode::prelude;

// there is no api header yet, there may be one in the future
// interact with these events directly at your own risk, they can be changed at any time

$execute {

  // set the global configuration, silently ignore config parsing errors
  new EventListener(+[](matjson::Value config) {
      Result<std::vector<utilities::Part>> newConfigResult = config.as<std::vector<utilities::Part>>();
      if (newConfigResult.isOk()) configuration = *newConfigResult.ok();
      return ListenerResult::Stop;
  }, SetConfigurationFilter("set-configuration"_spr));

  // set the global configuration, put config parsing errors into second argument
  new EventListener(+[](matjson::Value config, std::optional<std::string>* error) {
      Result<std::vector<utilities::Part>> newConfigResult = config.as<std::vector<utilities::Part>>();
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
      Result<std::vector<utilities::Part>> newConfigResult = config.as<std::vector<utilities::Part>>();
      if (newConfigResult.isOk()) static_cast<HookedHardStreak*>(streak)->setConfiguration(*newConfigResult);
      return ListenerResult::Stop;
  }, UpdateConfigurationFilter("update-configuration"_spr));

  // update the configuration of a specific HardStreak, put config parsing errors into second argument
  new EventListener(+[](HardStreak* streak, matjson::Value config, std::optional<std::string>* error) {
      Result<std::vector<utilities::Part>> newConfigResult = config.as<std::vector<utilities::Part>>();
      if (newConfigResult.isOk()) static_cast<HookedHardStreak*>(streak)->setConfiguration(*newConfigResult);
      *error = newConfigResult.err();
      return ListenerResult::Stop;
  }, UpdateConfigurationFilterResult("update-configuration"_spr));

};

