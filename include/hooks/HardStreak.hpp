#pragma once

#include <utilities.hpp>
#include <Geode/Geode.hpp>
#include <Geode/modify/HardStreak.hpp>
using namespace geode::prelude;

struct HookedHardStreak : Modify<HookedHardStreak, HardStreak> {
  struct Fields {
    std::vector<utilities::Unit> m_configuration;
    // if this is true, the configuration is invalid, and the trail will not get drawn
    bool m_broken = false;
  };

	static void onModify(auto& self);
   
  $override
  bool init();
  
  void clearAllChildren();

  void setConfiguration(std::vector<utilities::Unit>&);

  $override
  void stopStroke();

  $override 
	void updateStroke(float);

};

