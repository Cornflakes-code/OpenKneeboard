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

#include <OpenKneeboard/DXResources.h>
#include <OpenKneeboard/Events.h>
#include <OpenKneeboard/IKneeboardView.h>
#include <OpenKneeboard/SHM.h>

#include <OpenKneeboard/config.h>
#include <OpenKneeboard/final_release_deleter.h>

#include <shims/winrt/base.h>

#include <memory>
#include <mutex>
#include <optional>

#include <d2d1.h>
#include <d2d1_1.h>
#include <d3d11.h>
#include <d3d11_3.h>

namespace OpenKneeboard {
class CursorEvent;
class CursorRenderer;
class KneeboardState;
class ITab;
class ToolbarAction;
struct GameInstance;
struct DXResources;

class InterprocessRenderer final
  : private EventReceiver,
    public std::enable_shared_from_this<InterprocessRenderer> {
 public:
  ~InterprocessRenderer();
  static winrt::fire_and_forget final_release(
    std::unique_ptr<InterprocessRenderer>);

  static std::shared_ptr<InterprocessRenderer> Create(
    const DXResources&,
    KneeboardState*);

  std::underlying_type_t<SHM::ConsumerKind> GetConsumers() const;

 private:
  InterprocessRenderer();
  void Init(const DXResources&, KneeboardState*);

  // If we replace the shared_ptr while a draw is in progress,
  // we need to delay things a little
  static std::mutex sSingleInstance;
  std::unique_lock<std::mutex> mInstanceLock;
  winrt::apartment_context mOwnerThread;

  std::vector<RenderTargetID> mRenderTargetIDs;
  EventContext mEventContext;
  OpenKneeboard::SHM::Writer mSHM;
  DXResources mDXR;

  KneeboardState* mKneeboard = nullptr;

  bool mNeedsRepaint = true;

  // TODO: move to DXResources
  winrt::com_ptr<ID3D11DeviceContext4> mD3DContext;
  winrt::com_ptr<ID3D11Fence> mFence;
  winrt::handle mFenceHandle;
  std::atomic_flag mRendering;

  struct SharedTextureResources {
    winrt::com_ptr<ID3D11RenderTargetView> mTextureRTV;
    winrt::com_ptr<ID3D11Texture2D> mTexture;
    winrt::handle mSharedHandle;
  };

  struct Layer {
    SHM::LayerConfig mConfig;
    std::shared_ptr<IKneeboardView> mKneeboardView;

    winrt::com_ptr<ID3D11Texture2D> mCanvasTexture;
    winrt::com_ptr<ID2D1Bitmap1> mCanvasBitmap;
    winrt::com_ptr<ID3D11ShaderResourceView> mCanvasSRV;

    std::array<SharedTextureResources, TextureCount> mSharedResources;

    bool mIsActiveForInput = false;
  };
  std::array<Layer, MaxLayers> mLayers;

  std::shared_ptr<GameInstance> mCurrentGame;

  std::chrono::steady_clock::time_point mConsumersClearedAt;
  std::underlying_type_t<SHM::ConsumerKind> mConsumers {};

  void MarkDirty();
  void RenderNow();
  void Render(RenderTargetID, Layer&);

  void Commit(uint8_t layerCount) noexcept;

  void OnGameChanged(DWORD processID, const std::shared_ptr<GameInstance>&);
};

}// namespace OpenKneeboard
