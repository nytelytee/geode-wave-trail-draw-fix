# pragma once

#include <Geode/loader/Dispatch.hpp>
#include <Geode/Geode.hpp>
using namespace geode::prelude;

const std::map<std::string, GLenum> blendEnumMap = {
  {"GL_ZERO", GL_ZERO},
  {"GL_ONE", GL_ONE},
  {"GL_SRC_COLOR", GL_SRC_COLOR},
  {"GL_ONE_MINUS_SRC_COLOR", GL_ONE_MINUS_SRC_COLOR},
  {"GL_DST_COLOR", GL_DST_COLOR},
  {"GL_ONE_MINUS_DST_COLOR", GL_ONE_MINUS_DST_COLOR},
  {"GL_SRC_ALPHA", GL_SRC_ALPHA},
  {"GL_ONE_MINUS_SRC_ALPHA", GL_ONE_MINUS_SRC_ALPHA},
  {"GL_DST_ALPHA", GL_DST_ALPHA},
  {"GL_ONE_MINUS_DST_ALPHA", GL_ONE_MINUS_DST_ALPHA},
  //{"GL_CONSTANT_COLOR", GL_CONSTANT_COLOR},
  //{"GL_ONE_MINUS_CONSTANT_COLOR", GL_ONE_MINUS_CONSTANT_COLOR},
  //{"GL_CONSTANT_ALPHA", GL_CONSTANT_ALPHA},
  //{"GL_ONE_MINUS_CONSTANT_ALPHA", GL_ONE_MINUS_CONSTANT_ALPHA},
  {"GL_SRC_ALPHA_SATURATE", GL_SRC_ALPHA_SATURATE},
  //{"GL_SRC1_COLOR", GL_SRC1_COLOR},
  //{"GL_ONE_MINUS_SRC1_COLOR", GL_ONE_MINUS_SRC1_COLOR},
  //{"GL_SRC1_ALPHA", GL_SRC1_ALPHA},
  //{"GL_ONE_MINUS_SRC1_ALPHA", GL_ONE_MINUS_SRC1_ALPHA},
};

const std::map<std::string, _ccBlendFunc> blendFuncMap = {
  {"additive", {GL_SRC_ALPHA, GL_ONE}},
  {"normal", {GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA}},
};

const std::map<std::string, GLenum> blendEqMap = {
  {"GL_FUNC_ADD", GL_FUNC_ADD},
  {"GL_FUNC_SUBTRACT", GL_FUNC_SUBTRACT},
  {"GL_FUNC_REVERSE_SUBTRACT", GL_FUNC_REVERSE_SUBTRACT},
  {"GL_MIN", 0x8007},
  {"GL_MAX", 0x8008},

  {"add", GL_FUNC_ADD},
  {"subtract", GL_FUNC_SUBTRACT},
  {"reverse-subtract", GL_FUNC_REVERSE_SUBTRACT},
  {"min", 0x8007},
  {"max", 0x8008},
};

const float epsilon = 10e-6f;


using SetConfigurationEvent = geode::DispatchEvent<matjson::Value>;
using SetConfigurationEventResult = geode::DispatchEvent<matjson::Value, std::optional<std::string>*>;
using GetConfigurationEvent = geode::DispatchEvent<matjson::Value*>;
using SetConfigurationFilter = geode::DispatchFilter<matjson::Value>;
using SetConfigurationFilterResult = geode::DispatchFilter<matjson::Value, std::optional<std::string>*>;
using GetConfigurationFilter = geode::DispatchFilter<matjson::Value*>;
using UpdateConfigurationEvent = geode::DispatchEvent<HardStreak*, matjson::Value>;
using UpdateConfigurationEventResult = geode::DispatchEvent<HardStreak*, matjson::Value, std::optional<std::string>*>;
using GetSpecificConfigurationEvent = geode::DispatchEvent<HardStreak*, matjson::Value*>;
using UpdateConfigurationFilter = geode::DispatchFilter<HardStreak*, matjson::Value>;
using UpdateConfigurationFilterResult = geode::DispatchFilter<HardStreak*, matjson::Value, std::optional<std::string>*>;
using GetSpecificConfigurationFilter = geode::DispatchFilter<HardStreak*, matjson::Value*>;

