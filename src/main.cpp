// Raylib
#include "arena.h"
#include "raylib.h"
#include <chrono>
#include <iostream>
#include <raylib.h>
// Imgui
#include <imgui.h>
#include <imgui_impl_raylib.h>
#include <rlImGui.h>
// Simulation
#include <core.h>
#include <simulation.h>
// Other
#include <sstream>

struct Gui {
  bool window{false};
  ImFont* font{nullptr};
};

static inline void DrawGui(Gui& gui) {
  rlImGuiBegin();
  ImGui::PushFont(gui.font, 24.0f);

  if (gui.window) {
    std::stringstream ss;
    ImGui::Begin("Test", &gui.window, ImGuiWindowFlags_NoCollapse);
    ImGui::Text("Hello!");

    ImGui::Text("FPS: %d", GetFPS());

    if (ImGui::Button("Click me!")) {
      // gui.window = false;
    }
    ImGui::End();
  }

  ImGui::PopFont();
  rlImGuiEnd();
}

struct HSV {
  f32 h{0.0};
  f32 s{0.0};
  f32 v{0.0};

  HSV IncreaseValue(f32 amt) {
    auto res = *this;
    res.v += amt;
    return res;
  }
};

ImColor ToImgui(const HSV& hsv) { return ImColor::HSV(hsv.h, hsv.s, hsv.v); }

static inline void SetupGui(Gui& gui) {
  gui.window = true;

  auto& io = ImGui::GetIO();
  gui.font = io.Fonts->AddFontFromFileTTF("../assets/fonts/default.ttf");
  auto& style = ImGui::GetStyle();
  style.WindowRounding = 8.0f;
  style.FrameRounding = 8.0f;

  HSV base = {0.1, 0.2, 0.4};
  style.Colors[ImGuiCol_Button] = ToImgui(base);
  style.Colors[ImGuiCol_ButtonHovered] = ToImgui(base.IncreaseValue(0.2));
  style.Colors[ImGuiCol_ButtonActive] = ToImgui(base.IncreaseValue(0.4));
  style.Colors[ImGuiCol_TitleBg] = ToImgui(base);
  style.Colors[ImGuiCol_TitleBgActive] = ToImgui(base);
}


int main() {
  SetConfigFlags(FLAG_VSYNC_HINT);
  InitWindow(1600, 900, "Econ Test");
  rlImGuiSetup(true);

  Gui gui;
  SetupGui(gui);

  SetTargetFPS(GetMonitorRefreshRate(GetCurrentMonitor()));

  while (!WindowShouldClose()) {
    // Input
    if (IsKeyPressed(KEY_ESCAPE)) {
      break;
    }

    // Draw
    //----------------------------------------------------------------------------------
    BeginDrawing();
    ClearBackground(GRAY);

    DrawGui(gui);

    EndDrawing();
  }

  rlImGuiShutdown();
  CloseWindow();
  return 0;
}

// int main() {
//   auto* sim = simulation::Init();

//   {
//     simulation::TickRequest tick_req {
//       .advance_time = true,
//     };
//     simulation::Tick(*sim, tick_req);
//   }

//   simulation::DeInit(sim);

//   return 0;
// }
