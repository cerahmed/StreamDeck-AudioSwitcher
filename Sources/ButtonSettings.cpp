/* Copyright (c) 2018-present, Fred Emmott
 *
 * This source code is licensed under the MIT-style license found in the
 * LICENSE file.
 */

#include <regex>
#include "ButtonSettings.h"

#include <StreamDeckSDK/ESDLogger.h>

#include "audio_json.h"

NLOHMANN_JSON_SERIALIZE_ENUM(
  DeviceMatchStrategy,
  {
    {DeviceMatchStrategy::ID, "ID"},
    {DeviceMatchStrategy::Fuzzy, "Fuzzy"},
    {DeviceMatchStrategy::Regex, "Regex"},
  });

void from_json(const nlohmann::json& j, ButtonSettings& bs) {
  if (!j.contains("direction")) {
    return;
  }

  bs.direction = j.at("direction");

  if (j.contains("role")) {
    bs.role = j.at("role");
  }

  if (j.contains("primary")) {
    const auto& primary = j.at("primary");
    if (primary.is_string()) {
      bs.primaryDevice.id = primary;
    } else {
      bs.primaryDevice = primary;
    }
  }

  if (j.contains("secondary")) {
    const auto& secondary = j.at("secondary");
    if (secondary.is_string()) {
      bs.secondaryDevice.id = secondary;
    } else {
      bs.secondaryDevice = secondary;
    }
  }

  if (j.contains("matchStrategy")) {
    bs.matchStrategy = j.at("matchStrategy");
  }

  if (j.contains("primaryRegexPattern")) {
    bs.primaryRegexPattern = j.at("primaryRegexPattern");
  }

  if (j.contains("secondaryRegexPattern")) {
    bs.secondaryRegexPattern = j.at("secondaryRegexPattern");
  }
}

void to_json(nlohmann::json& j, const ButtonSettings& bs) {
  j = {
    {"direction", bs.direction},
    {"role", bs.role},
    {"primary", bs.primaryDevice},
    {"secondary", bs.secondaryDevice},
    {"matchStrategy", bs.matchStrategy},
    {"primaryRegexPattern", bs.primaryRegexPattern},
    {"secondaryRegexPattern", bs.secondaryRegexPattern},
  };
}

namespace {
std::string GetVolatileID(
  const AudioDeviceInfo& device,
  DeviceMatchStrategy strategy,
  std::string regexPattern) {

  if (device.id.empty()) {
    return {};
  }

  if (strategy == DeviceMatchStrategy::ID) {
    return device.id;
  }

  if (GetAudioDeviceState(device.id) == AudioDeviceState::CONNECTED) {
    return device.id;
  }

  // if fuzzy matching OR regex pattern is empty
  if (strategy == DeviceMatchStrategy::Fuzzy || regexPattern == "") {
    for (const auto& [otherID, other] : GetAudioDeviceList(device.direction)) {
      if (
        device.interfaceName == other.interfaceName
        && device.endpointName == other.endpointName
        && GetAudioDeviceState(device.id) == AudioDeviceState::CONNECTED) {
        ESDDebug(
          "Fuzzy device match for {}/{}",
          device.interfaceName,
          device.endpointName);
        return otherID;
      }
    }
    ESDDebug(
      "Failed fuzzy match for {}/{}", device.interfaceName, device.endpointName);
    return device.id;
  }

  // if regex matching AND regex pattern exists
  if (strategy == DeviceMatchStrategy::Regex) {
    std::regex rgx;
    std::smatch deviceMatch;
    // try regex pattern -> if it fails, return selected device id
    try
    {
      rgx = std::basic_regex(regexPattern);
    }
    catch(const std::exception& e)
    {
      return device.id;
    }
    
    for (const auto& [otherID, other] : GetAudioDeviceList(device.direction)) {
      // check if other device is connected -> otherwise continue to next
      if (GetAudioDeviceState(otherID) != AudioDeviceState::CONNECTED) {
        continue;
      }
      // if other device matches regex pattern -> return its id
      if (std::regex_search(other.displayName, deviceMatch, rgx)) {
        ESDDebug(
          "Found device {} that matches regex {}",
          other.displayName,
          regexPattern);
        return otherID;
      }
    }
    
    // if none of the devices match regex pattern -> return selected device id
    ESDDebug("No device found matching regex {} -> returning selected device id", regexPattern);
    return device.id;
  }
}
}// namespace

std::string ButtonSettings::VolatilePrimaryID() const {
  return GetVolatileID(primaryDevice, matchStrategy, primaryRegexPattern);
}

std::string ButtonSettings::VolatileSecondaryID() const {
  return GetVolatileID(secondaryDevice, matchStrategy, secondaryRegexPattern);
}
