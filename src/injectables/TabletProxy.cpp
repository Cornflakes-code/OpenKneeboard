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
#include "FindMainWindow.h"
#include "InjectedDLLMain.h"

#include <OpenKneeboard/GetMainHWND.h>
#include <OpenKneeboard/WintabTablet.h>

#include <OpenKneeboard/dprint.h>
#include <OpenKneeboard/scope_guard.h>
#include <OpenKneeboard/version.h>

#include <Windows.h>

#include <functional>
#include <stop_token>
#include <thread>

namespace OpenKneeboard {

/* PS >
 * [System.Diagnostics.Tracing.EventSource]::new("OpenKneeboard.TabletProxy")
 * 3bc3e07d-9c0e-51d5-2ef7-e7e706548c79
 */
TRACELOGGING_DEFINE_PROVIDER(
  gTraceProvider,
  "OpenKneeboard.TabletProxy",
  (0x3bc3e07d, 0x9c0e, 0x51d5, 0x2e, 0xf7, 0xe7, 0xe7, 0x06, 0x54, 0x8c, 0x79));

/** Work around for the lack of background access in the wintab API.
 *
 * When injected, this DLL will forward wintab events back to the OpenKneeboard
 * window.
 */
class TabletProxy final {
 public:
  TabletProxy();
  ~TabletProxy();

  TabletProxy(const TabletProxy&) = delete;
  TabletProxy& operator=(const TabletProxy&) = delete;

 private:
  void Initialize();
  void Detach();
  void WatchForEnvironmentChanges(std::stop_token);

  bool mInitialized = false;
  std::jthread mWatchThread;

  static WNDPROC mWindowProc;
  static HWND mTargetWindow;
  static std::unique_ptr<WintabTablet> mTablet;

  static LRESULT CALLBACK
  HookedWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
};

WNDPROC TabletProxy::mWindowProc;
HWND TabletProxy::mTargetWindow;
std::unique_ptr<WintabTablet> TabletProxy::mTablet;

static const MainWindowInfo::VersionInfo gThisVersion {
  Version::Major,
  Version::Minor,
  Version::Patch,
  Version::Build,
};

TabletProxy::TabletProxy() {
  Initialize();

  mWatchThread = std::jthread {
    std::bind_front(&TabletProxy::WatchForEnvironmentChanges, this)};
}

TabletProxy::~TabletProxy() {
  Detach();
}

void TabletProxy::Initialize() {
  if (mInitialized) {
    return;
  }

  mTargetWindow = FindMainWindow();
  if (!mTargetWindow) {
    return;
  }

  const scope_guard cleanupOnFailure([this]() {
    if (mInitialized) {
      return;
    }
    dprint("Failed to initialize TabletProxy, cleaning up");
    mTargetWindow = {};
    mTablet = {};
  });

  char title[256];
  auto titleLen = GetWindowTextA(mTargetWindow, title, sizeof(title));
  dprintf("Main window: {}", std::string_view(title, titleLen));

  mTablet = std::make_unique<WintabTablet>(
    mTargetWindow, WintabTablet::Priority::ForegroundOnly);
  if (!mTablet->IsValid()) {
    return;
  }

  mWindowProc = reinterpret_cast<WNDPROC>(SetWindowLongPtr(
    mTargetWindow,
    GWLP_WNDPROC,
    reinterpret_cast<LONG_PTR>(&HookedWindowProc)));
  if (!mWindowProc) {
    const auto err = GetLastError();
    if (err == ERROR_INVALID_WINDOW_HANDLE) {
      return;
    }
    dprintf("Failed to install windowproc: {}", err);
    OPENKNEEBOARD_BREAK;
    return;
  }
  mInitialized = true;
}

void TabletProxy::WatchForEnvironmentChanges(std::stop_token stop) {
  while (!(mInitialized || stop.stop_requested())) {
    std::this_thread::sleep_for(std::chrono::seconds(5));
    Initialize();
  }
  // Maybe we previously attached to a splash screen
  while (!stop.stop_requested()) {
    std::this_thread::sleep_for(std::chrono::seconds(5));
    if (FindMainWindow() == mTargetWindow) {
      continue;
    }
    Detach();
    Initialize();
  }
}

void TabletProxy::Detach() {
  if (!mInitialized) {
    return;
  }

  SetWindowLongPtr(
    mTargetWindow, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(mWindowProc));
  mTargetWindow = {};
  mWindowProc = {};
  mTablet = {};
  mInitialized = false;
}

LRESULT TabletProxy::HookedWindowProc(
  HWND hwnd,
  UINT uMsg,
  WPARAM wParam,
  LPARAM lParam) {
  if (hwnd == mTargetWindow && mTablet->CanProcessMessage(uMsg)) {
    const auto openKneeboard = OpenKneeboard::GetMainWindowInfo();
    if (openKneeboard && openKneeboard->mVersion == gThisVersion) {
      SendNotifyMessage(openKneeboard->mHwnd, uMsg, wParam, lParam);
    }
  }

  return CallWindowProc(mWindowProc, hwnd, uMsg, wParam, lParam);
}

}// namespace OpenKneeboard

using namespace OpenKneeboard;

namespace {

std::unique_ptr<TabletProxy> gInstance;

DWORD WINAPI ThreadEntry(LPVOID ignored) {
  dprint("Creating TabletProxy instance");
  gInstance = std::make_unique<TabletProxy>();
  dprint("Installed Tablet Proxy");
  return S_OK;
}

}// namespace

BOOL WINAPI DllMain(HINSTANCE hinst, DWORD dwReason, LPVOID reserved) {
  return InjectedDLLMain(
    "OpenKneeboard-TabletProxy",
    gInstance,
    &ThreadEntry,
    hinst,
    dwReason,
    reserved);
}
