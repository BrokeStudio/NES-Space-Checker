#include <filesystem>
#include "imgui.h"
#include "imgui_internal.h" // for dockspace

#include "App/App.hpp"
#include "Core/Logger/Logger.hpp"
#include "UI/Dialog/FileDialog.hpp"

namespace App
{
  bool exit = false;
  bool showDemo = false;

  Logger log;

  /*
  8888b.   dP"Yb   dP""b8 88  dP .dP"Y8 88""Yb    db     dP""b8 888888
   8I  Yb dP   Yb dP   `" 88odP  `Ybo." 88__dP   dPYb   dP   `" 88__
   8I  dY Yb   dP Yb      88"Yb  o.`Y8b 88"""   dP__Yb  Yb      88""
  8888Y"   YbodP   YboodP 88  Yb 8bodP' 88     dP""""Yb  YboodP 888888
  */

  static void dockspace_begin()
  {
    ImGuiViewport *viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->Pos);
    ImGui::SetNextWindowSize(viewport->Size);
    ImGui::SetNextWindowViewport(viewport->ID);

    ImGuiWindowFlags hostFlags =
        ImGuiWindowFlags_NoDocking |
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoBringToFrontOnFocus |
        ImGuiWindowFlags_NoNavFocus;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

    ImGui::Begin("##DockHost", nullptr, hostFlags);

    ImGui::PopStyleVar(3);

    const ImGuiID dockspaceId = ImGui::GetID("MainDockSpace");
    ImGui::DockSpace(dockspaceId, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_NoTabBar);

    static bool layoutInitialized = false;
    if (!layoutInitialized)
    {
      layoutInitialized = true;

      ImGui::DockBuilderRemoveNode(dockspaceId);
      ImGui::DockBuilderAddNode(dockspaceId, ImGuiDockNodeFlags_DockSpace);
      ImGui::DockBuilderSetNodeSize(dockspaceId, viewport->Size);

      ImGuiID mainDock = dockspaceId;
      ImGuiID leftDock = 0;
      // ImGuiID rightDock = 0;
      // ImGuiID bottomDock = 0;

      leftDock = ImGui::DockBuilderSplitNode(mainDock, ImGuiDir_Left, 0.15f, nullptr, &mainDock);
      // rightDock = ImGui::DockBuilderSplitNode(mainDock, ImGuiDir_Right, 0.25f, nullptr, &mainDock);
      // bottomDock = ImGui::DockBuilderSplitNode(mainDock, ImGuiDir_Down, 0.25f, nullptr, &mainDock);
      // if (ImGuiDockNode *node = ImGui::DockBuilderGetNode(bottomDock))
      //   node->LocalFlags |= ImGuiDockNodeFlags_NoTabBar;

      ImGui::DockBuilderDockWindow("Left", leftDock);
      ImGui::DockBuilderDockWindow("Viewport", mainDock);
      // ImGui::DockBuilderDockWindow("Inspector", rightDock);
      // ImGui::DockBuilderDockWindow("Bottom", bottomDock);
      ImGui::DockBuilderFinish(dockspaceId);
    }

    ImGui::End();
  }

  /**
   * @brief called from main.cpp when a file is dropped in the app
   *
   * @param selectedPath
   */
  void on_file_drop(const std::string &selectedPath)
  {
  }

  // /**
  //  * @brief close the currently opened manifest
  //  *
  //  */
  // void manifest_close()
  // {
  //   if (state.manifest.isOpen)
  //   {
  //     state = {};
  //     state.manifest.clear();
  //   }
  // }

  // /**
  //  * @brief save manifest
  //  *
  //  */
  // void manifest_save()
  // {
  //   if (!state.manifest.isOpen)
  //   {
  //     state.log.addf(LogLevel::Error, "No manifest currently opened");
  //     return;
  //   }

  //   if (!state.manifest.save())
  //   {
  //     state.log.addf(LogLevel::Error, "%s", state.manifest.lastError.c_str());
  //   }
  //   else
  //   {
  //     state.log.addf(LogLevel::Info, "Manifest '%s' successfully saved", state.manifest.path.filename().string().c_str());
  //   }
  // }

  // /**
  //  * @brief save manifest as...
  //  *
  //  * @param selectedPath
  //  * @param selectedFilename
  //  */
  // void manifest_save_as(const std::string &selectedPath, const std::string &selectedFilename)
  // {
  //   if (!state.manifest.isOpen)
  //   {
  //     state.log.addf(LogLevel::Error, "No manifest currently opened");
  //     return;
  //   }

  //   if (!state.manifest.save_as(std::filesystem::path(selectedPath)))
  //   {
  //     state.log.addf(LogLevel::Error, "%s", state.manifest.lastError.c_str());
  //   }
  //   else
  //   {
  //     state.log.addf(LogLevel::Info, "Manifest successfully saved as '%s'", state.manifest.path.filename().string().c_str());
  //   }
  // }

  // /**
  //  * @brief
  //  *
  //  */
  // void manifest_save_dialog()
  // {
  //   Dialog::fileExt = ".json";
  //   Dialog::callback = manifest_save_as;
  //   Dialog::showFileSave = true;
  // }

  /*
  .dP"Y8 88  88  dP"Yb  88""Yb 888888  dP""b8 88   88 888888 .dP"Y8
  `Ybo." 88  88 dP   Yb 88__dP   88   dP   `" 88   88   88   `Ybo."
  o.`Y8b 888888 Yb   dP 88"Yb    88   Yb      Y8   8P   88   o.`Y8b
  8bodP' 88  88  YbodP  88  Yb   88    YboodP `YbodP'   88   8bodP'
  */

  /**
   * @brief handle shortcuts
   *
   */
  // void handle_shortcuts(State &state)
  // {
  //   ImGuiIO &io = ImGui::GetIO();
  //   // if (!io.KeyCtrl || io.WantTextInput)
  //   // {
  //   //   return;
  //   // }

  //   // Ctrl+N
  //   if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_N, false))
  //   {
  //     return manifest_create();
  //   }

  //   // Ctrl+O
  //   if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_O, false))
  //   {
  //     return manifest_open_dialog();
  //   }

  //   // Ctrl+F4
  //   if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_F4, false))
  //   {
  //     return manifest_close();
  //   }

  //   // Ctrl+Shift+S
  //   if (io.KeyCtrl && io.KeyShift && ImGui::IsKeyPressed(ImGuiKey_S, false))
  //   {
  //     if (state.manifest.isOpen)
  //     {
  //       return manifest_save_dialog();
  //     }
  //   }

  //   // Ctrl+S
  //   if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_S, false))
  //   {
  //     if (state.manifest.isOpen)
  //     {
  //       return manifest_save();
  //     }
  //   }

  //   // Ctrl+Q
  //   if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Q, false))
  //   {
  //     exit = true;
  //     return;
  //   }

  //   // Ctrl+Z
  //   if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Z, false))
  //   {
  //     if (state.manifest.isOpen && state.undo.can_undo())
  //     {
  //       state.undo_one();
  //       return;
  //     }
  //   }

  //   // Ctrl+Y
  //   if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Y, false))
  //   {
  //     if (state.manifest.isOpen && state.undo.can_redo())
  //     {
  //       state.redo_one();
  //       return;
  //     }
  //   }
  // }

  /*
  8b    d8 888888 88b 88 88   88     88""Yb    db    88""Yb
  88b  d88 88__   88Yb88 88   88     88__dP   dPYb   88__dP
  88YbdP88 88""   88 Y88 Y8   8P     88""Yb  dP__Yb  88"Yb
  88 YY 88 888888 88  Y8 `YbodP'     88oodP dP""""Yb 88  Yb
  */

  /**
   * @brief render main menu bar
   *
   */
  //   void render_main_menu_bar()
  //   {
  //     if (!ImGui::BeginMainMenuBar())
  //     {
  //       return;
  //     }

  //     if (ImGui::BeginMenu("File"))
  //     {
  //       if (ImGui::MenuItem("New", "Ctrl+N"))
  //       {
  //         manifest_create();
  //       }

  //       if (ImGui::MenuItem("Open...", "Ctrl+O"))
  //       {
  //         manifest_open_dialog();
  //       }

  //       bool manifestIsOpen = state.manifest.isOpen;
  //       if (!manifestIsOpen)
  //       {
  //         ImGui::BeginDisabled();
  //       }

  //       if (ImGui::MenuItem("Save", "Ctrl+S"))
  //       {
  //         if (!state.manifest.save())
  //           ImGui::Text("Error: %s", state.manifest.lastError.c_str());
  //       }

  //       if (ImGui::MenuItem("Save As...", "Ctrl+Shift+S"))
  //       {
  //         manifest_save_dialog();
  //       }

  //       if (ImGui::MenuItem("Close", "Ctrl+F4"))
  //       {
  //         manifest_close();
  //       }

  //       if (!manifestIsOpen)
  //       {
  //         ImGui::EndDisabled();
  //       }

  //       if (ImGui::MenuItem("Exit", "Ctrl+Q"))
  //       {
  //         exit = true;
  //       }

  //       ImGui::EndMenu();
  //     }

  //     if (ImGui::BeginMenu("Edit"))
  //     {
  //       bool manifestIsOpen = state.manifest.isOpen;
  //       if (!manifestIsOpen)
  //       {
  //         ImGui::BeginDisabled();
  //       }

  //       const bool canUndo = manifestIsOpen && state.undo.can_undo();
  //       const bool canRedo = manifestIsOpen && state.undo.can_redo();

  //       if (!canUndo)
  //       {
  //         ImGui::BeginDisabled();
  //       }
  //       if (ImGui::MenuItem("Undo", "Ctrl+Z"))
  //       {
  //         state.undo_one();
  //       }
  //       if (!canUndo)
  //       {
  //         ImGui::EndDisabled();
  //       }

  //       if (!canRedo)
  //       {
  //         ImGui::BeginDisabled();
  //       }
  //       if (ImGui::MenuItem("Redo", "Ctrl+Y"))
  //       {
  //         state.redo_one();
  //       }
  //       if (!canRedo)
  //       {
  //         ImGui::EndDisabled();
  //       }

  //       if (!manifestIsOpen)
  //       {
  //         ImGui::EndDisabled();
  //       }

  //       ImGui::EndMenu();
  //     }

  //     if (ImGui::BeginMenu("Settings"))
  //     {
  // #if _DEBUG
  //       ImGui::Checkbox("Show ImGui Demo", &showDemo);
  // #endif
  //       ImGui::EndMenu();
  //     }

  //     ImGui::EndMainMenuBar();
  //   }

  /*
  88""Yb 888888 88b 88 8888b.  888888 88""Yb
  88__dP 88__   88Yb88  8I  Yb 88__   88__dP
  88"Yb  88""   88 Y88  8I  dY 88""   88"Yb
  88  Yb 888888 88  Y8 8888Y"  888888 88  Yb
  */

  /**
   * @brief render the app
   *
   * @return Events simple object to pass messages to caller (usually main.cpp)
   */
  Events render()
  {

    // app and global stuff
    Dialog::render();
    // handle_shortcuts(state);
    // render_main_menu_bar();
    dockspace_begin();

    // Dockspace left panel
    ImGui::Begin("Left");

    // display settings/options
    ImGui::SeparatorText("Settings");

    ImGui::End();

    // Dockspace main/center panel
    ImGui::Begin("Viewport");

    ImGui::End();

    // app events to return to caller (main.cpp)
    Events appEvents{};
    appEvents.exit = exit;
    appEvents.showDemo = showDemo;
    // if (state.manifest.isOpen)
    // {
    //   if (state.manifest.path.empty())
    //     appEvents.set_window_title += " - unamed.json";
    //   else
    //     appEvents.set_window_title += " - " + state.manifest.path.filename().string();
    // }

    // if (state.manifest.isDirty)
    //   appEvents.set_window_title += " (*)";

    return appEvents;
  }
}
