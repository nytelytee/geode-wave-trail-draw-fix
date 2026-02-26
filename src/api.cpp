#include <constants.hpp>
#include <utilities.hpp>
#include <globals.hpp>
#include <hooks/HardStreak.hpp>

using namespace geode::prelude;

// there is no api header yet, there may be one in the future
// interact with these events directly at your own risk, they can be changed at any time

$execute {

  // set the global configuration, silently ignore config parsing errors
  SetConfigurationEvent("set-configuration"_spr).listen([](matjson::Value config) {
      Result<std::vector<utilities::Part>> newConfigResult = config.as<std::vector<utilities::Part>>();
      if (newConfigResult.isOk()) configuration = *newConfigResult.ok();
      return ListenerResult::Stop;
  }).leak();

  // set the global configuration, put config parsing errors into second argument
  SetConfigurationEventResult("set-configuration"_spr).listen([](matjson::Value config, std::optional<std::string>* error) {
      Result<std::vector<utilities::Part>> newConfigResult = config.as<std::vector<utilities::Part>>();
      if (newConfigResult.isOk()) configuration = *newConfigResult.ok();
      *error = newConfigResult.err();
      return ListenerResult::Stop;
  }).leak();

  // get the global configuration
  GetSpecificConfigurationEvent("get-configuration"_spr).listen([](HardStreak* streak, matjson::Value* config) {
      *config = configuration;
      return ListenerResult::Stop;
  }).leak();

  // get the configuration of a specific HardStreak
  GetSpecificConfigurationEvent("get-configuration"_spr).listen([](HardStreak* streak, matjson::Value* config) {
      *config = static_cast<HookedHardStreak*>(streak)->m_fields->m_configuration;
      return ListenerResult::Stop;
  }).leak();

  // these are named differently from set-configuration, as they actually update a streak's configuration explicitly
  // HardStreaks store their configurations after they're set, so subsequent updates to the global configuration will
  // not affect already existing HardStreaks, to explicitly change those configurations, you call these events
  
  // update the configuration of a specific HardStreak, silently ignore config parsing errors
  UpdateConfigurationEvent("update-configuration"_spr).listen([](HardStreak* streak, matjson::Value config) {
      Result<std::vector<utilities::Part>> newConfigResult = config.as<std::vector<utilities::Part>>();
      if (newConfigResult.isOk()) static_cast<HookedHardStreak*>(streak)->setConfiguration(*newConfigResult);
      return ListenerResult::Stop;
  }).leak();

  // update the configuration of a specific HardStreak, put config parsing errors into second argument
  UpdateConfigurationEventResult("update-configuration"_spr).listen([](HardStreak* streak, matjson::Value config, std::optional<std::string>* error) {
      Result<std::vector<utilities::Part>> newConfigResult = config.as<std::vector<utilities::Part>>();
      if (newConfigResult.isOk()) static_cast<HookedHardStreak*>(streak)->setConfiguration(*newConfigResult);
      *error = newConfigResult.err();
      return ListenerResult::Stop;
  }).leak();
};

