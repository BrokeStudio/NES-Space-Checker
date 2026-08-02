#include <algorithm>
#include <cmath>
#include <string>

#include <SDL.h>
#include <SDL_opengl.h>

#include "imgui.h"
#include "imgui_impl_opengl2.h"
#include "imgui_impl_sdl2.h"

#include "App/App.hpp"

// FontAwesome
#include "IconsFontAwesome6.h"
#include "fa_regular_400.h"
#include "fa_solid_900.h"

void setup_imgui_style_for_viewports()
{
  ImGui::StyleColorsDark();

  ImGuiIO &io = ImGui::GetIO();
  if ((io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) == 0)
  {
    return;
  }

  ImGuiStyle &style = ImGui::GetStyle();
  style.WindowRounding = 0.0f;
  style.Colors[ImGuiCol_WindowBg].w = 1.0f;
}

float sanitize_dpi_scale(const float scaleValue)
{
  if (scaleValue <= 0.0f || !std::isfinite(scaleValue))
  {
    return 1.0f;
  }
  return scaleValue;
}

void apply_imgui_dpi_scale(const float scaleValue, float &currentScale)
{
  const float sanitizedScale = sanitize_dpi_scale(scaleValue);
  if (currentScale > 0.0f && std::abs(sanitizedScale - currentScale) < 0.001f)
  {
    return;
  }

  ImGuiStyle &style = ImGui::GetStyle();

  // Scale all Dear ImGui geometry once and only rescale by ratio on display changes.
  if (currentScale <= 0.0f)
  {
    style.ScaleAllSizes(sanitizedScale);
  }
  else
  {
    style.ScaleAllSizes(sanitizedScale / currentScale);
  }

  // Keep text scale synchronized with monitor content scale.
  style.FontScaleDpi = sanitizedScale;
  currentScale = sanitizedScale;
}

int main(int, char **)
{
#ifdef _WIN32
#ifdef SDL_HINT_WINDOWS_DPI_AWARENESS
  SDL_SetHint(SDL_HINT_WINDOWS_DPI_AWARENESS, "permonitorv2");
#endif
#endif

  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0)
  {
    return -1;
  }

#ifdef SDL_HINT_IME_SHOW_UI
  SDL_SetHint(SDL_HINT_IME_SHOW_UI, "1");
#endif

  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
  SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
  SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);

  constexpr int windowWidth = 1024;
  constexpr int windowHeight = 720;

  SDL_Window *window = SDL_CreateWindow(
      "NES Space Checker",
      SDL_WINDOWPOS_CENTERED,
      SDL_WINDOWPOS_CENTERED,
      windowWidth,
      windowHeight,
      static_cast<SDL_WindowFlags>(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI));
  if (window == nullptr)
  {
    SDL_Quit();
    return -1;
  }
  SDL_SetWindowMinimumSize(window, windowWidth, windowHeight);

  SDL_GLContext glContext = SDL_GL_CreateContext(window);
  SDL_GL_MakeCurrent(window, glContext);
  SDL_GL_SetSwapInterval(1);

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
  io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
  io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
  io.IniFilename = nullptr;

  setup_imgui_style_for_viewports();
  float currentUiScale = 0.0f;
  apply_imgui_dpi_scale(ImGui_ImplSDL2_GetContentScaleForWindow(window), currentUiScale);

  ImGui_ImplSDL2_InitForOpenGL(window, glContext);
  ImGui_ImplOpenGL2_Init();

  io.Fonts->AddFontDefault();
  float baseFontSize = 18.0f;

  // add FontAwesome fonts
  float iconFontSize = baseFontSize * 2.0f / 3.0f; // FontAwesome fonts need to have their sizes reduced by 2.0f/3.0f in order to align correctly
  static const ImWchar icons_ranges[] = {ICON_MIN_FA, ICON_MAX_16_FA, 0};
  ImFontConfig icons_config;
  icons_config.MergeMode = true;
  icons_config.PixelSnapH = true;
  icons_config.GlyphMinAdvanceX = iconFontSize;
  io.Fonts->AddFontFromMemoryCompressedBase85TTF(fa_regular_400_compressed_data_base85, iconFontSize, &icons_config, icons_ranges);
  io.Fonts->AddFontFromMemoryCompressedBase85TTF(fa_solid_900_compressed_data_base85, iconFontSize, &icons_config, icons_ranges);

  std::string lastTitle;
  bool done = false;
  while (!done)
  {
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
      ImGui_ImplSDL2_ProcessEvent(&event);
      if (event.type == SDL_QUIT)
      {
        done = true;
      }
      if (event.type == SDL_WINDOWEVENT &&
          event.window.event == SDL_WINDOWEVENT_CLOSE &&
          event.window.windowID == SDL_GetWindowID(window))
      {
        done = true;
      }
#ifdef SDL_WINDOWEVENT_DISPLAY_CHANGED
      if (event.type == SDL_WINDOWEVENT &&
          event.window.event == SDL_WINDOWEVENT_DISPLAY_CHANGED &&
          event.window.windowID == SDL_GetWindowID(window))
      {
        apply_imgui_dpi_scale(ImGui_ImplSDL2_GetContentScaleForWindow(window), currentUiScale);
      }
#endif
      if (event.type == SDL_DROPFILE && event.drop.file != nullptr)
      {
        App::on_file_drop(event.drop.file);
        SDL_free(event.drop.file);
      }
    }

    ImGui_ImplOpenGL2_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();

    // render app
    App::Events appEvents = App::render();
    done |= appEvents.exit;
    if (appEvents.setWindowTitle != lastTitle)
    {
      lastTitle = appEvents.setWindowTitle;
      SDL_SetWindowTitle(window, appEvents.setWindowTitle.c_str());
    }

    // #if _DEBUG
    //     if (appEvents.showDemo)
    //       ImGui::ShowDemoWindow();
    // #endif

    if (done)
    {
      break;
    }

    ImGui::Render();

    glViewport(0, 0, static_cast<int>(io.DisplaySize.x), static_cast<int>(io.DisplaySize.y));
    glClearColor(0.11f, 0.12f, 0.13f, 1.00f);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());
    App::process_pending_export();

    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
      SDL_Window *backupCurrentWindow = SDL_GL_GetCurrentWindow();
      SDL_GLContext backupCurrentContext = SDL_GL_GetCurrentContext();
      ImGui::UpdatePlatformWindows();
      ImGui::RenderPlatformWindowsDefault();
      SDL_GL_MakeCurrent(backupCurrentWindow, backupCurrentContext);
    }

    SDL_GL_SwapWindow(window);
  }

  ImGui_ImplOpenGL2_Shutdown();
  ImGui_ImplSDL2_Shutdown();
  ImGui::DestroyContext();

  SDL_GL_DeleteContext(glContext);
  SDL_DestroyWindow(window);
  SDL_Quit();

  return 0;
}
