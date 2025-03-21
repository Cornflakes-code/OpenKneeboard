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
#include <OpenKneeboard/RenderTargetID.h>
#include <OpenKneeboard/ThreadGuard.h>
#include <OpenKneeboard/UniqueID.h>

#include <OpenKneeboard/inttypes.h>

#include <cstdint>

#include <d2d1_1.h>

namespace OpenKneeboard {

enum class SuggestedPageAppendAction {
  SwitchToNewPage,
  KeepOnCurrentPage,
};

enum class ScalingKind {
  /* The source is equivalent to a 2D-array of pixels; scaling is slow and low
   * quality.
   *
   * The native content size should be **strongly** preferred.
   */
  Bitmap,
  /* The source can be rendered at any resolution with high quality, without
   * significiant performance issues.
   */
  Vector,
};

class IPageSource {
 public:
  virtual ~IPageSource();

  virtual PageIndex GetPageCount() const = 0;
  virtual std::vector<PageID> GetPageIDs() const = 0;

  virtual ScalingKind GetScalingKind(PageID) = 0;
  virtual D2D1_SIZE_U GetNativeContentSize(PageID) = 0;
  virtual void RenderPage(
    RenderTargetID,
    ID2D1DeviceContext*,
    PageID,
    const D2D1_RECT_F& rect)
    = 0;

  Event<> evNeedsRepaintEvent;
  Event<SuggestedPageAppendAction> evPageAppendedEvent;
  Event<> evContentChangedEvent;
  Event<EventContext, PageID> evPageChangeRequestedEvent;
  Event<> evAvailableFeaturesChangedEvent;
  ThreadGuard mThreadGuard;
};

}// namespace OpenKneeboard
