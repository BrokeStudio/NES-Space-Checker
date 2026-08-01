#include "App/App.hpp"

#include <cstdio>
#include <filesystem>
#include <optional>
#include <string>
#include <utility>

#include "Core/Logger/Logger.hpp"
#include "Core/Nes.hpp"
#include "UI/Dialog/FileDialog.hpp"
#include "UI/RomViewport/RomViewport.hpp"
#include "imgui.h"
#include "imgui_internal.h"

namespace App
{
  namespace
  {
    bool exitRequested = false;
    bool showDemo = false;
    bool showSettings = false;
    Logger log;
    std::optional<Nes::Document> document;
    RomViewport::Settings viewportSettings;
    RomViewport::State viewportState;
    int emptyMode = 0;
    std::uint8_t customEmptyValue = 0x00;

    void load_file(const std::filesystem::path &path)
    {
      Nes::ParseResult result = Nes::load_document(path);
      if (!result.success)
      {
        log.addf(LogLevel::Error, "Could not load '%s': %s", path.string().c_str(), result.error.c_str());
        return;
      }

      document = std::move(result.document);
      const std::uint8_t emptyValue = emptyMode == 0 ? 0x00 : (emptyMode == 1 ? 0xFF : customEmptyValue);
      document->analyze(emptyValue);
      RomViewport::request_one_to_one(viewportState);
      log.addf(LogLevel::Info, "Loaded '%s': %zu PRG bank(s), %zu CHR bank(s)",
               document->path.filename().string().c_str(), document->prgBanks.size(), document->chrBanks.size());
      if (!result.warning.empty())
        log.addf(LogLevel::Warn, "%s", result.warning.c_str());
    }

    void open_file_dialog()
    {
      Dialog::fileExt = ".nes";
      Dialog::callback = [](const std::string &path, const std::string &filename)
      {
        std::filesystem::path selectedPath(path);
        if (std::filesystem::is_directory(selectedPath))
          selectedPath /= filename;
        load_file(selectedPath);
      };
      Dialog::showFileOpen = true;
    }

    void close_file()
    {
      if (!document)
        return;
      log.addf(LogLevel::Info, "Closed '%s'", document->path.filename().string().c_str());
      document.reset();
      RomViewport::request_fit(viewportState);
    }

    void apply_empty_value()
    {
      if (!document)
        return;
      const std::uint8_t value = emptyMode == 0 ? 0x00 : (emptyMode == 1 ? 0xFF : customEmptyValue);
      document->analyze(value);
    }

    void handle_shortcuts()
    {
      const ImGuiIO &io = ImGui::GetIO();
      if (io.WantTextInput)
        return;
      if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_O, false))
        open_file_dialog();
      if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Q, false))
        exitRequested = true;
      if (io.KeyCtrl && (ImGui::IsKeyPressed(ImGuiKey_W, false) || ImGui::IsKeyPressed(ImGuiKey_F4, false)))
        close_file();
    }

    void render_main_menu_bar()
    {
      if (!ImGui::BeginMainMenuBar())
        return;

      if (ImGui::BeginMenu("File"))
      {
        if (ImGui::MenuItem("Open...", "Ctrl+O"))
          open_file_dialog();
        if (!document)
          ImGui::BeginDisabled();
        if (ImGui::MenuItem("Close", "Ctrl+W"))
          close_file();
        if (!document)
          ImGui::EndDisabled();
        ImGui::Separator();
        if (ImGui::MenuItem("Exit", "Ctrl+Q"))
          exitRequested = true;
        ImGui::EndMenu();
      }

      if (ImGui::BeginMenu("View"))
      {
        ImGui::MenuItem("Settings", nullptr, &showSettings);
        ImGui::Separator();
        if (!document)
          ImGui::BeginDisabled();
        if (ImGui::MenuItem("Fit all"))
          RomViewport::request_fit(viewportState);
        if (ImGui::MenuItem("100%"))
          RomViewport::request_one_to_one(viewportState);
        if (!document)
          ImGui::EndDisabled();
#if _DEBUG
        ImGui::Separator();
        ImGui::MenuItem("ImGui demo", nullptr, &showDemo);
#endif
        ImGui::EndMenu();
      }
      ImGui::EndMainMenuBar();
    }

    void dockspace_begin()
    {
      ImGuiViewport *viewport = ImGui::GetMainViewport();
      ImGui::SetNextWindowPos(viewport->WorkPos);
      ImGui::SetNextWindowSize(viewport->WorkSize);
      ImGui::SetNextWindowViewport(viewport->ID);

      constexpr ImGuiWindowFlags HOST_FLAGS = ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar |
                                               ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
                                               ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus |
                                               ImGuiWindowFlags_NoNavFocus;
      ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
      ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
      ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
      ImGui::Begin("##DockHost", nullptr, HOST_FLAGS);
      ImGui::PopStyleVar(3);

      const ImGuiID dockspaceId = ImGui::GetID("MainDockSpace");
      ImGui::DockSpace(dockspaceId, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_NoTabBar);
      static bool layoutInitialized = false;
      if (!layoutInitialized)
      {
        layoutInitialized = true;
        ImGui::DockBuilderRemoveNode(dockspaceId);
        ImGui::DockBuilderAddNode(dockspaceId, ImGuiDockNodeFlags_DockSpace);
        ImGui::DockBuilderSetNodeSize(dockspaceId, viewport->WorkSize);
        ImGuiID mainDock = dockspaceId;
        const ImGuiID leftDock = ImGui::DockBuilderSplitNode(mainDock, ImGuiDir_Left, 0.22f, nullptr, &mainDock);
        ImGui::DockBuilderDockWindow("Settings", leftDock);
        ImGui::DockBuilderDockWindow("Viewport", mainDock);
        ImGui::DockBuilderFinish(dockspaceId);
      }
      ImGui::End();
    }

    void render_document_information()
    {
      if (!document)
      {
        ImGui::TextDisabled("No ROM loaded");
        return;
      }

      const Nes::Header &header = document->header;
      ImGui::TextWrapped("%s", document->path.filename().string().c_str());
      if (ImGui::BeginTable("nes_header", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
      {
        const auto row = [](const char *label, const char *value)
        {
          ImGui::TableNextRow();
          ImGui::TableNextColumn();
          ImGui::TextUnformatted(label);
          ImGui::TableNextColumn();
          ImGui::TextUnformatted(value);
        };
        char value[64];
        row("Format", Nes::header_version_name(header.version));
        std::snprintf(value, sizeof(value), "%u", header.mapperId);
        row("Mapper", value);
        if (header.version == Nes::HeaderVersion::Nes20)
        {
          std::snprintf(value, sizeof(value), "%u", header.submapperId);
          row("Submapper", value);
        }
        row("Mirroring", Nes::mirroring_name(header.mirroring));
        std::snprintf(value, sizeof(value), "%llu KiB", static_cast<unsigned long long>(header.prgRomSize / 1024));
        row("PRG ROM", value);
        std::snprintf(value, sizeof(value), "%llu KiB", static_cast<unsigned long long>(header.chrRomSize / 1024));
        row("CHR ROM", value);
        row("Trainer", header.hasTrainer ? "Yes" : "No");
        row("Battery", header.hasBattery ? "Yes" : "No");
        ImGui::EndTable();
      }
    }

    void render_settings()
    {
      if (!showSettings)
        return;

      if (!ImGui::Begin("Settings", &showSettings))
      {
        ImGui::End();
        return;
      }
      if (ImGui::Button("Open ROM...", ImVec2(-1.0f, 0.0f)))
        open_file_dialog();

      ImGui::SeparatorText("Document");
      render_document_information();

      ImGui::SeparatorText("Display");
      bool visibilityChanged = false;
      visibilityChanged |= ImGui::Checkbox("Show PRG", &viewportSettings.showPrg);
      visibilityChanged |= ImGui::Checkbox("Show CHR", &viewportSettings.showChr);
      ImGui::SetNextItemWidth(-1.0f);
      if (ImGui::SliderInt("Banks per row", &viewportSettings.banksPerRow, 1, 64))
        visibilityChanged = true;
      if (visibilityChanged)
        RomViewport::request_fit(viewportState);
      ImGui::ColorEdit3("PRG color", &viewportSettings.prgColor.x);
      ImGui::ColorEdit3("CHR color", &viewportSettings.chrColor.x);

      ImGui::SeparatorText("Empty byte");
      bool emptyChanged = false;
      emptyChanged |= ImGui::RadioButton("$00", &emptyMode, 0);
      ImGui::SameLine();
      emptyChanged |= ImGui::RadioButton("$FF", &emptyMode, 1);
      ImGui::SameLine();
      emptyChanged |= ImGui::RadioButton("Custom", &emptyMode, 2);
      if (emptyMode != 2)
        ImGui::BeginDisabled();
      ImGui::SetNextItemWidth(-1.0f);
      emptyChanged |= ImGui::InputScalar("##custom_empty", ImGuiDataType_U8, &customEmptyValue, nullptr, nullptr, "%02X",
                                         ImGuiInputTextFlags_CharsHexadecimal);
      if (emptyMode != 2)
        ImGui::EndDisabled();
      if (emptyChanged)
        apply_empty_value();

      ImGui::SeparatorText("Navigation");
      if (ImGui::Button("Fit all"))
        RomViewport::request_fit(viewportState);
      ImGui::SameLine();
      if (ImGui::Button("100%"))
        RomViewport::request_one_to_one(viewportState);
      ImGui::Text("Zoom: %.2f%% (%.2f px/byte)", viewportState.zoom * 100.0, viewportState.zoom);
      ImGui::TextDisabled("Wheel: zoom");
      ImGui::TextDisabled("Middle/right drag or Space + drag: pan");

      ImGui::SeparatorText("Log");
      if (ImGui::BeginChild("##log", ImVec2(0.0f, 0.0f), ImGuiChildFlags_Borders))
      {
        for (const LogLine &line : log.lines())
        {
          const ImVec4 color = line.level == LogLevel::Error ? ImVec4(1.0f, 0.35f, 0.35f, 1.0f)
                               : line.level == LogLevel::Warn ? ImVec4(1.0f, 0.75f, 0.25f, 1.0f)
                                                            : ImGui::GetStyleColorVec4(ImGuiCol_Text);
          ImGui::PushStyleColor(ImGuiCol_Text, color);
          ImGui::TextWrapped("%s", line.text.c_str());
          ImGui::PopStyleColor();
        }
      }
      ImGui::EndChild();
      ImGui::End();
    }
  }

  void on_file_drop(const std::string &selectedPath)
  {
    load_file(selectedPath);
  }

  Events render()
  {
    Dialog::render();
    handle_shortcuts();
    render_main_menu_bar();
      dockspace_begin();
      render_settings();

    ImGui::Begin("Viewport");
    RomViewport::render(document ? &*document : nullptr, viewportSettings, viewportState);
    ImGui::End();

#if _DEBUG
    if (showDemo)
      ImGui::ShowDemoWindow(&showDemo);
#endif

    Events events{};
    events.exit = exitRequested;
    events.showDemo = showDemo;
    if (document)
      events.setWindowTitle += " - " + document->path.filename().string();
    return events;
  }
}
