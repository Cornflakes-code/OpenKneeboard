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
#include <OpenKneeboard/CursorEvent.h>
#include <OpenKneeboard/D2DErrorRenderer.h>
#include <OpenKneeboard/DXResources.h>
#include <OpenKneeboard/ITab.h>
#include <OpenKneeboard/ITabView.h>
#include <OpenKneeboard/TabViewUILayer.h>

#include <OpenKneeboard/config.h>
#include <OpenKneeboard/utf8.h>

namespace OpenKneeboard {

TabViewUILayer::TabViewUILayer(const DXResources& dxr) {
  mErrorRenderer = std::make_unique<D2DErrorRenderer>(dxr);
  dxr.mD2DDeviceContext->CreateSolidColorBrush(
    {1.0f, 1.0f, 1.0f, 1.0f},
    D2D1::BrushProperties(),
    mErrorBackgroundBrush.put());
}
TabViewUILayer::~TabViewUILayer() = default;

void TabViewUILayer::PostCursorEvent(
  const IUILayer::NextList&,
  const Context& context,
  const EventContext&,
  const CursorEvent& ev) {
  if (ev.mTouchState == CursorTouchState::NOT_NEAR_SURFACE) {
    mCursorPoint.reset();
    context.mTabView->PostCursorEvent({});
    return;
  }
  mCursorPoint = {ev.mX, ev.mY};
  context.mTabView->PostCursorEvent(ev);
}

IUILayer::Metrics TabViewUILayer::GetMetrics(
  const IUILayer::NextList&,
  const Context& context) const {
  const constexpr Metrics errorMapping {
    {ErrorRenderWidth, ErrorRenderHeight},
    {0, 0, ErrorRenderWidth, ErrorRenderHeight},
    {0, 0, ErrorRenderWidth, ErrorRenderHeight},
    {ScalingKind::Vector},
  };
  auto tabView = context.mTabView;
  if (!tabView) {
    return errorMapping;
  }
  const auto nextSize = tabView->GetNativeContentSize();

  if (nextSize.width == 0 || nextSize.height == 0) {
    return errorMapping;
  }

  const D2D1_SIZE_F size {
    static_cast<FLOAT>(nextSize.width),
    static_cast<FLOAT>(nextSize.height),
  };

  return {
    size,
    {0, 0, size.width, size.height},
    {0, 0, size.width, size.height},
    tabView->GetScalingKind(),
  };
}

void TabViewUILayer::Render(
  RenderTargetID rtid,
  const IUILayer::NextList&,
  const Context& context,
  ID2D1DeviceContext* d2d,
  const D2D1_RECT_F& rect) {
  const auto tabView = context.mTabView;

  if (!tabView) {
    this->RenderError(d2d, _("No Tab View"), rect);
    return;
  }
  auto tab = tabView->GetTab();
  if (!tab) {
    this->RenderError(d2d, _("No Tab"), rect);
    return;
  }
  const auto pageCount = tab->GetPageCount();
  if (pageCount == 0) {
    this->RenderError(d2d, _("No Pages"), rect);
    return;
  }

  tab->RenderPage(rtid, d2d, tabView->GetPageID(), rect);
}

void TabViewUILayer::RenderError(
  ID2D1DeviceContext* d2d,
  std::string_view text,
  const D2D1_RECT_F& rect) {
  d2d->FillRectangle(rect, mErrorBackgroundBrush.get());
  mErrorRenderer->Render(d2d, text, rect);
}

std::optional<D2D1_POINT_2F> TabViewUILayer::GetCursorPoint() const {
  return mCursorPoint;
}

}// namespace OpenKneeboard
