#include <Geode/Geode.hpp>
#include <constants.hpp>
#include <globals.hpp>

using namespace geode::prelude;

std::vector<utilities::Part> configuration;

matjson::Value defaultConfiguration = matjson::parse(R"(
  [
    {
      "subparts": [{"color": null}]
    },
    {
      "start": -0.3333333,
      "end": 0.3333333,
      "subparts": [{"color": "#ffffff", "opacity": 0.65, "nonsolid-only": true}]
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
      "subparts": [
        {"color": "#E50000"},
        {"color": "#FF8D00"},
        {"color": "#FFEE00"},
        {"color": "#028121"},
        {"color": "#004CFF"},
        {"color": "#770088"}
      ]
    }
  ]
  */
  Mod* mod = Mod::get();
  if (mod->hasSavedValue("_custom_configuration")) {
    SetConfigurationEvent("set-configuration"_spr).send(mod->getSavedValue<matjson::Value>("_custom_configuration"));
  } else {
    SetConfigurationEvent("set-configuration"_spr).send(defaultConfiguration);
  }
}
