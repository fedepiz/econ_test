#include "imgui.h"
#include <imgui_impl_raylib.h>
#include <raylib.h>
#include <rlImGui.h>

#include <iostream>
#include <simulation.h>

int main() {
  InitWindow(1600, 900, "Econ Test");
  rlImGuiSetup(true);

  SetTargetFPS(60);

  bool window = true;

  while (!WindowShouldClose()) {
    // Input
    if (IsKeyPressed(KEY_ESCAPE)) {
      break;
    }

    // Draw
    //----------------------------------------------------------------------------------
    BeginDrawing();
    ClearBackground(GRAY);

    rlImGuiBegin();
    ImGui::PushFont(nullptr, 30.0f);

    if (window) {
      ImGui::Begin("Test", &window, ImGuiWindowFlags_NoCollapse);
      ImGui::Text("Hello!");
      if (ImGui::Button("Click me!")) {
        window = false;
        std::cout << "Clicked! " << window << std::endl;
      }
      ImGui::End();
    }

    ImGui::PopFont();
    rlImGuiEnd();


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
