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

// clang-format off
#include "pch.h"
#include "CheckDCSHooks.h"
// clang-format on

#include "Globals.h"

#include <OpenKneeboard/DCSWorldInstance.h>
#include <OpenKneeboard/FilesDiffer.h>
#include <OpenKneeboard/Filesystem.h>
#include <OpenKneeboard/GamesList.h>
#include <OpenKneeboard/KneeboardState.h>
#include <OpenKneeboard/RuntimeFiles.h>

#include <OpenKneeboard/utf8.h>

#include <winrt/microsoft.ui.xaml.controls.h>

#include <microsoft.ui.xaml.window.h>

#include <format>
#include <set>

#include <shobjidl.h>

using namespace winrt::Microsoft::UI::Xaml;
using namespace winrt::Microsoft::UI::Xaml::Controls;
using namespace winrt;

namespace OpenKneeboard {

enum DCSHookInstallState { UP_TO_DATE, OUT_OF_DATE, NOT_INSTALLED };

static DCSHookInstallState GetHookInstallState(
  const std::filesystem::path& hooksDir,
  const std::filesystem::path& exeDir) {
  if (!std::filesystem::is_directory(hooksDir)) {
    return NOT_INSTALLED;
  }

  const auto dllDest = hooksDir / RuntimeFiles::DCSWORLD_HOOK_DLL;
  const auto luaDest = hooksDir / RuntimeFiles::DCSWORLD_HOOK_LUA;

  if (!(std::filesystem::is_regular_file(dllDest)
        && std::filesystem::is_regular_file(dllDest))) {
    return NOT_INSTALLED;
  }

  const auto dllSource = exeDir / RuntimeFiles::DCSWORLD_HOOK_DLL;
  const auto luaSource = exeDir / RuntimeFiles::DCSWORLD_HOOK_LUA;

  if (FilesDiffer(dllSource, dllDest) || FilesDiffer(luaSource, luaDest)) {
    return OUT_OF_DATE;
  }

  return UP_TO_DATE;
}

winrt::Windows::Foundation::IAsyncAction CheckDCSHooks(
  const XamlRoot& root,
  const std::filesystem::path& savedGamesPath) {
  if (!std::filesystem::exists(savedGamesPath)) {
    co_return;
  }

  const auto exeDir = Filesystem::GetRuntimeDirectory();

  const auto hooksDir = savedGamesPath / "Scripts" / "Hooks";
  const auto state = GetHookInstallState(hooksDir, exeDir);

  if (state == UP_TO_DATE) {
    co_return;
  }

  const auto dllSource = exeDir / RuntimeFiles::DCSWORLD_HOOK_DLL;
  const auto luaSource = exeDir / RuntimeFiles::DCSWORLD_HOOK_LUA;
  const auto dllDest = hooksDir / RuntimeFiles::DCSWORLD_HOOK_DLL;
  const auto luaDest = hooksDir / RuntimeFiles::DCSWORLD_HOOK_LUA;

  ContentDialog dialog;
  dialog.XamlRoot(root);
  dialog.Title(winrt::box_value(winrt::to_hstring(_("DCS Hooks"))));
  dialog.DefaultButton(ContentDialogButton::Primary);
  dialog.PrimaryButtonText(winrt::to_hstring(_("Retry")));
  dialog.CloseButtonText(winrt::to_hstring(_("Ignore")));

  std::error_code ec;
  do {
    if (ec) {
      auto result = co_await dialog.ShowAsync();
      if (result != ContentDialogResult::Primary) {
        break;
      }
    }

    if (!(std::filesystem::is_directory(hooksDir)
          || std::filesystem::create_directories(hooksDir, ec))) {
      dialog.Content(winrt::box_value(winrt::to_hstring(std::format(
        _("Failed to create {}: {} ({:#x})"),
        to_utf8(hooksDir),
        ec.message(),
        ec.value()))));
      continue;
    }

    if (!std::filesystem::copy_file(
          luaSource,
          luaDest,
          std::filesystem::copy_options::overwrite_existing,
          ec)) {
      dialog.Content(winrt::box_value(winrt::to_hstring(std::format(
        _("Failed to write to {}: {} ({:#x}) - if DCS is running, "
          "close DCS, and try again."),
        to_utf8(luaDest),
        ec.message(),
        ec.value()))));
      continue;
    }

    if (!std::filesystem::copy_file(
          dllSource,
          dllDest,
          std::filesystem::copy_options::overwrite_existing,
          ec)) {
      dialog.Content(winrt::box_value(winrt::to_hstring(std::format(
        _("Failed to write to {}: {} ({:#x}) - if DCS is running, "
          "close DCS, and try again."),
        to_utf8(luaDest),
        ec.message(),
        ec.value()))));
      continue;
    }

    co_return;
  } while (true);
}

concurrency::task<std::optional<std::filesystem::path>>
ChooseDCSSavedGamesFolder(
  const winrt::Microsoft::UI::Xaml::XamlRoot& xamlRoot,
  DCSSavedGamesSelectionTrigger trigger) {
  if (trigger == DCSSavedGamesSelectionTrigger::IMPLICIT) {
    ContentDialog dialog;
    dialog.XamlRoot(xamlRoot);
    dialog.Title(box_value(to_hstring(_("DCS Saved Games Location"))));
    dialog.Content(
      box_value(to_hstring(_("We couldn't find your DCS saved games folder; "
                             "would you like to set it now? "
                             "This is required for the DCS tabs to work."))));
    dialog.PrimaryButtonText(to_hstring(_("Choose Saved Games Folder")));
    dialog.CloseButtonText(to_hstring(_("Not Now")));
    dialog.DefaultButton(ContentDialogButton::Primary);

    if (co_await dialog.ShowAsync() != ContentDialogResult::Primary) {
      co_return {};
    }
  }

  constexpr winrt::guid thisCall {
    0xa6605cee,
    0x16ef,
    0x4bbb,
    {0x8d, 0x80, 0xf5, 0x73, 0xac, 0x5b, 0x0c, 0x95}};
  FilePicker picker(gMainWindow);
  picker.SettingsIdentifier(thisCall);
  picker.SuggestedStartLocation(FOLDERID_SavedGames);
  const auto folder = picker.PickSingleFolder();

  if (!folder) {
    co_return {};
  }

  co_return *folder;
}

winrt::Windows::Foundation::IAsyncAction CheckAllDCSHooks(
  const XamlRoot& root) {
  winrt::apartment_context uiThread;

  std::set<std::filesystem::path> dcsSavedGamesPaths;
  for (const auto& game: gKneeboard->GetGamesList()->GetGameInstances()) {
    auto dcs = std::dynamic_pointer_cast<DCSWorldInstance>(game);
    if (!dcs) {
      continue;
    }
    auto& path = dcs->mSavedGamesPath;
    if (path.empty()) {
      co_await uiThread;
      auto chosenPath = co_await ChooseDCSSavedGamesFolder(
        root, DCSSavedGamesSelectionTrigger::IMPLICIT);
      if (!chosenPath) {
        continue;
      }
      path = *chosenPath;
      gKneeboard->SaveSettings();
    }

    if (!dcsSavedGamesPaths.contains(path)) {
      dcsSavedGamesPaths.emplace(path);
    }
  }

  for (const auto& path: dcsSavedGamesPaths) {
    co_await CheckDCSHooks(root, path);
  }
}

}// namespace OpenKneeboard
