# pragma once

#include <Geode/loader/Dispatch.hpp>
#include <Geode/Geode.hpp>
using namespace geode::prelude;

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
