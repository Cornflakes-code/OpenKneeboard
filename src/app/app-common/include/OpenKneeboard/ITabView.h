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
#pragma once

#include <OpenKneeboard/Events.h>
#include <OpenKneeboard/IPageSource.h>

#include <OpenKneeboard/inttypes.h>

#include <memory>

#include <d2d1.h>

namespace OpenKneeboard {

struct CursorEvent;
class ITab;

enum class TabMode {
  NORMAL,
  NAVIGATION,
};

class ITabView {
 public:
  ITabView();
  virtual ~ITabView();

  virtual std::shared_ptr<ITab> GetRootTab() const = 0;

  virtual void SetPageID(PageID) = 0;
  virtual PageID GetPageID() const = 0;
  virtual std::vector<PageID> GetPageIDs() const = 0;
  virtual std::shared_ptr<ITab> GetTab() const = 0;

  virtual D2D1_SIZE_U GetNativeContentSize() const = 0;
  virtual ScalingKind GetScalingKind() const = 0;

  virtual void PostCursorEvent(const CursorEvent&) = 0;

  virtual TabMode GetTabMode() const = 0;
  virtual bool SupportsTabMode(TabMode) const = 0;
  virtual bool SetTabMode(TabMode) = 0;

  Event<CursorEvent> evCursorEvent;
  Event<> evNeedsRepaintEvent;
  Event<> evPageChangedEvent;
  Event<> evContentChangedEvent;
  Event<PageIndex> evPageChangeRequestedEvent;
  Event<> evAvailableFeaturesChangedEvent;
  Event<> evTabModeChangedEvent;
  Event<> evBookmarksChangedEvent;
};

}// namespace OpenKneeboard
