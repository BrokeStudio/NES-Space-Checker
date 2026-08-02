#pragma once

#include <cstdint>

#include "Core/Nes.hpp"
#include "imgui.h"

namespace RomViewport
{
  struct Settings
  {
    bool showPrg = true;
    bool showChr = true;
    int banksPerRow = 16;
    ImVec4 prgColor = ImVec4(0.19f, 1.0f, 0.19f, 1.0f);
    ImVec4 chrColor = ImVec4(0.31f, 0.44f, 1.0f, 1.0f);
  };

  struct State
  {
    double zoom = 1.0;
    ImVec2 pan = ImVec2(20.0f, 20.0f);
    bool fitRequested = true;
  };

  void request_fit(State &state);
  void request_one_to_one(State &state);
  void render(const Nes::Document *document, const Settings &settings, State &state, bool showZoomOverlay = true);
}
