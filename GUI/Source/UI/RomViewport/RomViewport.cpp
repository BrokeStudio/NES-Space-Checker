#include "UI/RomViewport/RomViewport.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <iterator>
#include <vector>

namespace
{
  constexpr int BYTES_PER_ROW = 64;
  constexpr double MIN_ZOOM = 0.01;
  constexpr double MAX_ZOOM = 256.0;
  constexpr float BANK_HORIZONTAL_GAP = 20.0f;
  constexpr float BANK_VERTICAL_GAP = 20.0f;
  constexpr float HEADER_HEIGHT = 18.0f;
  constexpr float STATS_HEIGHT = 32.0f;
  constexpr float BANK_SLOT_WIDTH = BYTES_PER_ROW + BANK_HORIZONTAL_GAP;
  constexpr float PRG_SLOT_HEIGHT = HEADER_HEIGHT + 256.0f + STATS_HEIGHT + BANK_VERTICAL_GAP;

  struct BankLayout
  {
    const Nes::Bank *bank = nullptr;
    ImVec2 gridPosition;
    int rows = 0;
  };

  struct Layout
  {
    std::vector<BankLayout> banks;
    ImVec2 size = ImVec2(0.0f, 0.0f);
  };

  Layout build_layout(const Nes::Document &document, const RomViewport::Settings &settings)
  {
    Layout layout;
    int slot = 0;
    const int banksPerRow = std::max(1, settings.banksPerRow);
    const auto appendRegion = [&layout, &slot, banksPerRow](const std::vector<Nes::Bank> &banks, const int rows)
    {
      if (banks.empty())
        return;

      if (slot % banksPerRow != 0)
        slot += banksPerRow - (slot % banksPerRow);
      for (const Nes::Bank &bank : banks)
      {
        const int column = slot % banksPerRow;
        const int row = slot / banksPerRow;
        layout.banks.push_back({&bank, ImVec2(column * BANK_SLOT_WIDTH, row * PRG_SLOT_HEIGHT + HEADER_HEIGHT), rows});
        layout.size.x = std::max(layout.size.x, column * BANK_SLOT_WIDTH + static_cast<float>(BYTES_PER_ROW));
        layout.size.y = std::max(layout.size.y, row * PRG_SLOT_HEIGHT + HEADER_HEIGHT + rows + STATS_HEIGHT);
        ++slot;
      }
    };

    if (settings.showPrg)
      appendRegion(document.prgBanks, 256);
    if (settings.showChr)
      appendRegion(document.chrBanks, 128);
    return layout;
  }

  ImVec2 world_to_screen(const ImVec2 &world, const ImVec2 &origin, const RomViewport::State &state)
  {
    return ImVec2(origin.x + state.pan.x + static_cast<float>(world.x * state.zoom),
                  origin.y + state.pan.y + static_cast<float>(world.y * state.zoom));
  }

  ImU32 blend_color(const ImVec4 &emptyColor, const ImVec4 &regionColor, const float density)
  {
    const float amount = std::clamp(density, 0.0f, 1.0f);
    return ImGui::ColorConvertFloat4ToU32(ImVec4(
        emptyColor.x + (regionColor.x - emptyColor.x) * amount,
        emptyColor.y + (regionColor.y - emptyColor.y) * amount,
        emptyColor.z + (regionColor.z - emptyColor.z) * amount,
        1.0f));
  }

  bool rectangle_visible(const ImVec2 &minimum, const ImVec2 &maximum, const ImVec2 &clipMinimum, const ImVec2 &clipMaximum)
  {
    return maximum.x >= clipMinimum.x && maximum.y >= clipMinimum.y && minimum.x <= clipMaximum.x && minimum.y <= clipMaximum.y;
  }

  double next_zoom_step(const double currentZoom, const int direction)
  {
    constexpr double FRACTIONAL_STEPS[] = {0.015625, 0.03125, 0.0625, 0.125, 0.25, 0.5, 1.0};
    constexpr double EPSILON = 0.000001;

    if (direction > 0)
    {
      for (const double step : FRACTIONAL_STEPS)
      {
        if (step > currentZoom + EPSILON)
          return step;
      }
      return std::min(MAX_ZOOM, std::floor(currentZoom + EPSILON) + 1.0);
    }

    if (currentZoom > 1.0 + EPSILON)
      return std::max(1.0, std::ceil(currentZoom - EPSILON) - 1.0);
    for (int index = static_cast<int>(std::size(FRACTIONAL_STEPS)) - 1; index >= 0; --index)
    {
      if (FRACTIONAL_STEPS[index] < currentZoom - EPSILON)
        return FRACTIONAL_STEPS[index];
    }
    return MIN_ZOOM;
  }

  void draw_bank(ImDrawList *drawList, const Nes::Document &document, const BankLayout &layout,
                 const RomViewport::Settings &settings, const RomViewport::State &state,
                 const ImVec2 &origin, const ImVec2 &clipMinimum, const ImVec2 &clipMaximum)
  {
    const Nes::Bank &bank = *layout.bank;
    const ImVec4 regionColor = bank.type == Nes::RegionType::Prg ? settings.prgColor : settings.chrColor;
    const ImVec4 emptyColor(0.12f, 0.13f, 0.14f, 1.0f);
    const ImVec2 gridMinimum = world_to_screen(layout.gridPosition, origin, state);
    const ImVec2 gridMaximum = world_to_screen(ImVec2(layout.gridPosition.x + BYTES_PER_ROW, layout.gridPosition.y + layout.rows), origin, state);
    if (!rectangle_visible(gridMinimum, gridMaximum, clipMinimum, clipMaximum))
      return;

    const ImU32 borderColor = IM_COL32(190, 190, 190, 255);

    const int firstColumn = std::clamp(static_cast<int>(std::floor((clipMinimum.x - gridMinimum.x) / state.zoom)), 0, BYTES_PER_ROW);
    const int lastColumn = std::clamp(static_cast<int>(std::ceil((clipMaximum.x - gridMinimum.x) / state.zoom)), 0, BYTES_PER_ROW);
    const int firstRow = std::clamp(static_cast<int>(std::floor((clipMinimum.y - gridMinimum.y) / state.zoom)), 0, layout.rows);
    const int lastRow = std::clamp(static_cast<int>(std::ceil((clipMaximum.y - gridMinimum.y) / state.zoom)), 0, layout.rows);

    if (state.zoom < 1.0)
    {
      const int groupSize = std::max(1, static_cast<int>(std::ceil(1.0 / state.zoom)));
      const int startRow = firstRow - firstRow % groupSize;
      const int startColumn = firstColumn - firstColumn % groupSize;
      for (int row = startRow; row < lastRow; row += groupSize)
      {
        for (int column = startColumn; column < lastColumn; column += groupSize)
        {
          const int endRow = std::min(row + groupSize, layout.rows);
          const int endColumn = std::min(column + groupSize, BYTES_PER_ROW);
          std::size_t occupied = 0;
          std::size_t count = 0;
          for (int sampleRow = row; sampleRow < endRow; ++sampleRow)
          {
            for (int sampleColumn = column; sampleColumn < endColumn; ++sampleColumn)
            {
              const std::size_t byteIndex = static_cast<std::size_t>(sampleRow * BYTES_PER_ROW + sampleColumn);
              occupied += document.bytes[bank.fileOffset + byteIndex] != document.emptyValue ? 1u : 0u;
              ++count;
            }
          }
          const float density = count == 0 ? 0.0f : static_cast<float>(occupied) / static_cast<float>(count);
          const ImVec2 minimum = world_to_screen(ImVec2(layout.gridPosition.x + column, layout.gridPosition.y + row), origin, state);
          const ImVec2 maximum = world_to_screen(ImVec2(layout.gridPosition.x + endColumn, layout.gridPosition.y + endRow), origin, state);
          drawList->AddRectFilled(minimum, maximum, blend_color(emptyColor, regionColor, density));
        }
      }
    }
    else
    {
      const bool drawGrid = state.zoom >= 4.0;
      const ImVec2 valueSize = ImGui::CalcTextSize("FF");
      const bool drawValues = state.zoom >= std::max(valueSize.x + 6.0f, valueSize.y + 4.0f);
      bool tooltipShown = false;
      const ImU32 occupiedColor = ImGui::ColorConvertFloat4ToU32(regionColor);
      const ImU32 emptyByteColor = ImGui::ColorConvertFloat4ToU32(emptyColor);
      for (int row = firstRow; row < lastRow; ++row)
      {
        for (int column = firstColumn; column < lastColumn; ++column)
        {
          const std::size_t byteIndex = static_cast<std::size_t>(row * BYTES_PER_ROW + column);
          const std::uint8_t value = document.bytes[bank.fileOffset + byteIndex];
          const ImVec2 minimum = world_to_screen(ImVec2(layout.gridPosition.x + column, layout.gridPosition.y + row), origin, state);
          const ImVec2 maximum = world_to_screen(ImVec2(layout.gridPosition.x + column + 1, layout.gridPosition.y + row + 1), origin, state);
          const bool occupied = value != document.emptyValue;
          drawList->AddRectFilled(minimum, maximum, occupied ? occupiedColor : emptyByteColor);
          if (drawGrid)
            drawList->AddRect(minimum, maximum, IM_COL32(0, 0, 0, 90));
          if (drawValues)
          {
            char text[3];
            std::snprintf(text, sizeof(text), "%02X", value);
            const float luminance = occupied ? regionColor.x * 0.299f + regionColor.y * 0.587f + regionColor.z * 0.114f : 0.12f;
            const ImU32 textColor = luminance > 0.58f ? IM_COL32_BLACK : IM_COL32_WHITE;
            drawList->AddText(ImVec2(minimum.x + (maximum.x - minimum.x - valueSize.x) * 0.5f,
                                     minimum.y + (maximum.y - minimum.y - valueSize.y) * 0.5f),
                              textColor, text);

            if (!tooltipShown && ImGui::IsItemHovered() && ImGui::IsMouseHoveringRect(minimum, maximum, true))
            {
              ImGui::BeginTooltip();
              ImGui::Text("File offset: $%zX", bank.fileOffset + byteIndex);
              ImGui::Text("Bank offset: $%zX", byteIndex);
              ImGui::EndTooltip();
              tooltipShown = true;
            }
          }
        }
      }
    }

    drawList->AddRect(gridMinimum, gridMaximum, borderColor);

    if (state.zoom >= 1.0)
    {
      // A 64-byte row means that 16 rows represent 1 KiB.
      constexpr int ROWS_PER_KIB = 16;
      constexpr int KIB_PER_MAJOR_TICK = 4;
      const float tickScale = std::clamp(static_cast<float>(std::sqrt(state.zoom)), 1.0f, 4.0f);
      for (int row = 0; row <= layout.rows; row += ROWS_PER_KIB)
      {
        const int kibOffset = row / ROWS_PER_KIB;
        const bool majorTick = kibOffset % KIB_PER_MAJOR_TICK == 0;
        const float tickLength = (majorTick ? 8.0f : 4.0f) * tickScale;
        const float tickThickness = majorTick ? 2.0f : 1.0f;
        const float tickY = gridMinimum.y + static_cast<float>(row * state.zoom);
        if (tickY >= clipMinimum.y - tickThickness && tickY <= clipMaximum.y + tickThickness)
        {
          drawList->AddLine(ImVec2(gridMaximum.x, tickY), ImVec2(gridMaximum.x + tickLength, tickY),
                            borderColor, tickThickness);
        }
      }
    }

    if (state.zoom >= 0.35)
    {
      char title[32];
      const char *typeName = bank.type == Nes::RegionType::Prg ? "PRG" : "CHR";
      if (state.zoom < 1)
      {
        std::snprintf(title, sizeof(title), "%s", typeName);
        drawList->AddText(ImVec2(gridMinimum.x, gridMinimum.y - ImGui::GetTextLineHeight() * 2 - 2.0f), IM_COL32_WHITE, title);
        std::snprintf(title, sizeof(title), "$%02zX", bank.index);
        drawList->AddText(ImVec2(gridMinimum.x, gridMinimum.y - ImGui::GetTextLineHeight() - 2.0f), IM_COL32_WHITE, title);
      }
      else
      {
        std::snprintf(title, sizeof(title), "%s $%02zX", typeName, bank.index);
        drawList->AddText(ImVec2(gridMinimum.x, gridMinimum.y - ImGui::GetTextLineHeight() - 2.0f), IM_COL32_WHITE, title);
      }

      if (state.zoom >= 1)
      {
        char freeBytesText[48];
        char freePercentText[32];
        const std::size_t freeBytes = bank.size - bank.occupiedBytes;
        const double freePercent = bank.size == 0 ? 0.0 : 100.0 * static_cast<double>(freeBytes) / static_cast<double>(bank.size);
        std::snprintf(freeBytesText, sizeof(freeBytesText), "%zu free", freeBytes);
        std::snprintf(freePercentText, sizeof(freePercentText), "(%.1f%%)", freePercent);
        const ImU32 statisticsColor = IM_COL32(210, 210, 210, 255);
        drawList->AddText(ImVec2(gridMinimum.x, gridMaximum.y + 2.0f), statisticsColor, freeBytesText);
        drawList->AddText(ImVec2(gridMinimum.x, gridMaximum.y + ImGui::GetTextLineHeight() + 2.0f), statisticsColor, freePercentText);
      }
    }
  }
}

namespace RomViewport
{
  void request_fit(State &state)
  {
    state.fitRequested = true;
  }

  void request_one_to_one(State &state)
  {
    state.zoom = 1.0;
    state.pan = ImVec2(20.0f, 20.0f);
    state.fitRequested = false;
  }

  void render(const Nes::Document *document, const Settings &settings, State &state)
  {
    const ImVec2 canvasPosition = ImGui::GetCursorScreenPos();
    ImVec2 canvasSize = ImGui::GetContentRegionAvail();
    canvasSize.x = std::max(canvasSize.x, 1.0f);
    canvasSize.y = std::max(canvasSize.y, 1.0f);
    ImGui::InvisibleButton("##rom_canvas", canvasSize,
                           ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight | ImGuiButtonFlags_MouseButtonMiddle);
    const bool hovered = ImGui::IsItemHovered();
    ImDrawList *drawList = ImGui::GetWindowDrawList();
    const ImVec2 canvasMaximum(canvasPosition.x + canvasSize.x, canvasPosition.y + canvasSize.y);
    drawList->AddRectFilled(canvasPosition, canvasMaximum, IM_COL32(24, 26, 29, 255));
    drawList->PushClipRect(canvasPosition, canvasMaximum, true);

    if (document == nullptr)
    {
      drawList->AddText(ImVec2(canvasPosition.x + 20.0f, canvasPosition.y + 20.0f), IM_COL32(210, 210, 210, 255),
                        "Open a NES ROM or drop one onto the window.");
      drawList->PopClipRect();
      return;
    }

    const Layout layout = build_layout(*document, settings);
    if (layout.banks.empty())
    {
      drawList->AddText(ImVec2(canvasPosition.x + 20.0f, canvasPosition.y + 20.0f), IM_COL32(210, 210, 210, 255),
                        "No visible PRG or CHR region.");
      drawList->PopClipRect();
      return;
    }

    if (state.fitRequested)
    {
      const double horizontalZoom = (canvasSize.x - 40.0) / std::max(1.0f, layout.size.x);
      const double verticalZoom = (canvasSize.y - 40.0) / std::max(1.0f, layout.size.y);
      state.zoom = std::clamp(std::min(horizontalZoom, verticalZoom), MIN_ZOOM, MAX_ZOOM);
      state.pan = ImVec2(static_cast<float>((canvasSize.x - layout.size.x * state.zoom) * 0.5),
                         static_cast<float>((canvasSize.y - layout.size.y * state.zoom) * 0.5));
      state.fitRequested = false;
    }

    ImGuiIO &io = ImGui::GetIO();
    if (hovered && io.MouseWheel != 0.0f)
    {
      const double previousZoom = state.zoom;
      const int direction = io.MouseWheel > 0.0f ? 1 : -1;
      const int stepCount = std::max(1, static_cast<int>(std::round(std::abs(io.MouseWheel))));
      for (int step = 0; step < stepCount; ++step)
        state.zoom = next_zoom_step(state.zoom, direction);
      const ImVec2 mouseInCanvas(io.MousePos.x - canvasPosition.x, io.MousePos.y - canvasPosition.y);
      const double worldX = (mouseInCanvas.x - state.pan.x) / previousZoom;
      const double worldY = (mouseInCanvas.y - state.pan.y) / previousZoom;
      state.pan.x = static_cast<float>(mouseInCanvas.x - worldX * state.zoom);
      state.pan.y = static_cast<float>(mouseInCanvas.y - worldY * state.zoom);
    }

    if (hovered && (ImGui::IsMouseDragging(ImGuiMouseButton_Middle) ||
                    ImGui::IsMouseDragging(ImGuiMouseButton_Right) ||
                    (ImGui::IsKeyDown(ImGuiKey_Space) && ImGui::IsMouseDragging(ImGuiMouseButton_Left))))
    {
      state.pan.x += io.MouseDelta.x;
      state.pan.y += io.MouseDelta.y;
    }

    for (const BankLayout &bank : layout.banks)
      draw_bank(drawList, *document, bank, settings, state, canvasPosition, canvasPosition, canvasMaximum);

    char zoomText[48];
    std::snprintf(zoomText, sizeof(zoomText), "Zoom: %.2f%%", state.zoom * 100.0);
    const ImVec2 zoomTextSize = ImGui::CalcTextSize(zoomText);
    const ImVec2 overlayMinimum(canvasPosition.x + 8.0f, canvasPosition.y + 8.0f);
    const ImVec2 overlayMaximum(overlayMinimum.x + zoomTextSize.x + 12.0f, overlayMinimum.y + zoomTextSize.y + 8.0f);
    drawList->AddRectFilled(overlayMinimum, overlayMaximum, IM_COL32(12, 13, 15, 210), 4.0f);
    drawList->AddRect(overlayMinimum, overlayMaximum, IM_COL32(100, 105, 112, 220), 4.0f);
    drawList->AddText(ImVec2(overlayMinimum.x + 6.0f, overlayMinimum.y + 4.0f), IM_COL32_WHITE, zoomText);

    drawList->PopClipRect();
  }
}
