/*
 * OpenKneeboard
 *
 * Copyright (C) 2022 Fred Emmott <fred@fredemmott.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; version 2.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301,
 * USA.
 */
#include <OpenKneeboard/GameEvent.h>
#include <OpenKneeboard/Win32.h>

#include <OpenKneeboard/config.h>
#include <OpenKneeboard/dprint.h>
#include <OpenKneeboard/json.h>
#include <OpenKneeboard/tracing.h>

#include <shims/winrt/base.h>

#include <Windows.h>

#include <charconv>
#include <chrono>
#include <string_view>

static uint32_t hex_to_ui32(const std::string_view& sv) {
  if (sv.empty()) {
    return 0;
  }

  uint32_t value = 0;
  std::from_chars(&sv.front(), &sv.front() + sv.length(), value, 16);
  return value;
}

static winrt::file_handle gMailslotHandle;

static bool OpenMailslotHandle() {
  if (gMailslotHandle) {
    return true;
  }

  static std::chrono::steady_clock::time_point sLastAttempt {};
  const auto now = std::chrono::steady_clock::now();
  if (now - sLastAttempt < std::chrono::seconds(1)) {
    return false;
  }
  sLastAttempt = now;

  gMailslotHandle = OpenKneeboard::Win32::CreateFileW(
    OpenKneeboard::GameEvent::GetMailslotPath(),
    GENERIC_WRITE,
    FILE_SHARE_READ,
    nullptr,
    OPEN_EXISTING,
    0,
    NULL);

  return static_cast<bool>(gMailslotHandle);
}

namespace OpenKneeboard {

#define CHECK_PACKET(condition) \
  if (!(condition)) { \
    dprintf("Check failed at {}:{}: {}", __FILE__, __LINE__, #condition); \
    return {}; \
  }

GameEvent::operator bool() const {
  return !(name.empty() || value.empty());
}

GameEvent GameEvent::Unserialize(std::string_view packet) {
  // "{:08x}!{}!{:08x}!{}!", name size, name, value size, value
  CHECK_PACKET(packet.ends_with("!"));
  CHECK_PACKET(packet.size() >= sizeof("12345678!!12345678!!") - 1);

  const auto nameLen = hex_to_ui32(packet.substr(0, 8));
  CHECK_PACKET(packet.size() >= 8 + nameLen + 8 + 4);
  const uint32_t nameOffset = 9;
  const std::string name(packet.substr(nameOffset, nameLen));

  const uint32_t valueLenOffset = nameOffset + nameLen + 1;
  CHECK_PACKET(packet.size() >= valueLenOffset + 10);
  const auto valueLen = hex_to_ui32(packet.substr(valueLenOffset, 8));
  const uint32_t valueOffset = valueLenOffset + 8 + 1;
  CHECK_PACKET(packet.size() == valueOffset + valueLen + 1);
  const std::string value(packet.substr(valueOffset, valueLen));

  return {name, value};
}

std::vector<std::byte> GameEvent::Serialize() const {
  const auto str = std::format(
    "{:08x}!{}!{:08x}!{}!", name.size(), name, value.size(), value);
  const auto first = reinterpret_cast<const std::byte*>(str.data());
  return {first, first + str.size()};
}

void GameEvent::Send() const {
  TraceLoggingThreadActivity<gTraceProvider> activity;
  TraceLoggingWriteStart(
    activity,
    "GameEvent::Send()",
    TraceLoggingValue(this->name.c_str(), "Name"),
    TraceLoggingBinary(this->value.c_str(), this->value.size(), "Value"));

  if (!OpenMailslotHandle()) {
    TraceLoggingWriteStop(
      activity,
      "GameEvent::Send()",
      TraceLoggingValue("Couldn't open mailslot", "Result"));
    return;
  }
  const auto packet = this->Serialize();

  if (WriteFile(
        gMailslotHandle.get(),
        packet.data(),
        static_cast<DWORD>(packet.size()),
        nullptr,
        nullptr)) {
    TraceLoggingWriteStop(
      activity, "GameEvent::Send()", TraceLoggingValue("Success", "Result"));
    return;
  }

  gMailslotHandle.close();
  gMailslotHandle = {};
  TraceLoggingWriteTagged(activity, "Closed handle");

  if (!OpenMailslotHandle()) {
    TraceLoggingWriteStop(
      activity,
      "GameEvent::Send()",
      TraceLoggingValue("Couldn't reopen handle", "Result"));
    return;
  }
  TraceLoggingWriteTagged(activity, "Reopened handle");
  if (WriteFile(
        gMailslotHandle.get(),
        packet.data(),
        static_cast<DWORD>(packet.size()),
        nullptr,
        nullptr)) {
    TraceLoggingWriteStop(
      activity, "GameEvent::Send()", TraceLoggingValue("Success", "Result"));
  } else {
    TraceLoggingWriteStop(
      activity,
      "GameEvent::Send()",
      TraceLoggingValue("Error", "Result"),
      TraceLoggingValue(GetLastError(), "Error"));
  }
}

const wchar_t* GameEvent::GetMailslotPath() {
  static std::wstring sPath;
  if (sPath.empty()) {
    sPath = std::format(
      L"\\\\.\\mailslot\\{}.events.v1.3", OpenKneeboard::ProjectNameW);
  }
  return sPath.c_str();
}

OPENKNEEBOARD_DEFINE_JSON(SetTabByIDEvent, mID, mPageNumber, mKneeboard);
OPENKNEEBOARD_DEFINE_JSON(SetTabByNameEvent, mName, mPageNumber, mKneeboard);
OPENKNEEBOARD_DEFINE_JSON(SetTabByIndexEvent, mIndex, mPageNumber, mKneeboard);
OPENKNEEBOARD_DEFINE_JSON(SetProfileByIDEvent, mID);
OPENKNEEBOARD_DEFINE_JSON(SetProfileByNameEvent, mName);

NLOHMANN_JSON_SERIALIZE_ENUM(
  SetBrightnessEvent::Mode,
  {
    {SetBrightnessEvent::Mode::Absolute, "Absolute"},
    {SetBrightnessEvent::Mode::Relative, "Relative"},
  });
OPENKNEEBOARD_DEFINE_JSON(SetBrightnessEvent, mBrightness, mMode);

}// namespace OpenKneeboard
