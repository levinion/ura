#pragma once

namespace ura {

enum UraSceneLayer {
  Clear = -50,
  Background = 0,
  Bottom = 50,
  Normal = 100,
  Top = 150,
  Floating = 200,
  Fullscreen = 250,
  Popup = 300,
  Overlay = 350,
  LockScreen = 400
};

} // namespace ura
