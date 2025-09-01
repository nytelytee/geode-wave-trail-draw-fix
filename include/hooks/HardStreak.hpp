#pragma once

#include <utilities.hpp>
#include <Geode/Geode.hpp>
#include <Geode/modify/HardStreak.hpp>
using namespace geode::prelude;

struct HookedHardStreak : Modify<HookedHardStreak, HardStreak> {
  struct Fields {
    std::vector<utilities::Part> m_configuration;
  };

	static void onModify(auto& self);
   
  $override
  bool init();
  
  void clearAllChildren();

  void setConfiguration(std::vector<utilities::Part>&);

  void drawTriangle(CCPoint const&, CCPoint const&, CCPoint const&, ccColor4B color);

  $override 
	void updateStroke(float);

};
