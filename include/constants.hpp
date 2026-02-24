# pragma once

#include <Geode/loader/Dispatch.hpp>
#include <Geode/Geode.hpp>
using namespace geode::prelude;

const float epsilon = 10e-6f;

using SetConfigurationEvent = geode::Dispatch<matjson::Value>;
using SetConfigurationEventResult = geode::Dispatch<matjson::Value, std::optional<std::string>*>;
using GetConfigurationEvent = geode::Dispatch<matjson::Value*>;
using UpdateConfigurationEvent = geode::Dispatch<HardStreak*, matjson::Value>;
using UpdateConfigurationEventResult = geode::Dispatch<HardStreak*, matjson::Value, std::optional<std::string>*>;
using GetSpecificConfigurationEvent = geode::Dispatch<HardStreak*, matjson::Value*>;