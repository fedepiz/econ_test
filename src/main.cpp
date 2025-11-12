// Raylib
#include "arena.h"
#include "imgui_internal.h"
#include "raylib.h"
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

using namespace arena;

struct Gui {
  bool window{false};
  ImFont* font{nullptr};
};

static inline void DrawGui(Gui& gui, const simulation::Sim& sim,
                           simulation::EntityId selected_id) {
  using namespace simulation;

  rlImGuiBegin();
  ImGui::PushFont(gui.font, 24.0f);

  if (selected_id.IsValid()) {

    struct Object {
      const char* name{nullptr};
    };
        
    Object obj;
    switch (selected_id.kind) {
      case EntityIdKind::Location: {
          const auto* location = (Location*)selected_id.handle;
          obj.name = location->name;
        break;
      }
      default:
        assert(false);
    }
    
    ImGui::Begin("Selected Entity");
    ImGui::BeginTable("overview_table", 2);
    ImGui::TableNextRow();
    ImGui::TableNextColumn();
    ImGui::Text("Name");
    ImGui::TableNextColumn();
    ImGui::Text("%s", obj.name);
    ImGui::EndTable();
    ImGui::End();
  }

  if (gui.window) {
    ImGui::Begin("Test", &gui.window, ImGuiWindowFlags_NoCollapse);
    ImGui::Text("Hello!");

    ImGui::Text("FPS: %d", GetFPS());

    if (ImGui::Button("Click me!")) {
      gui.window = false;
    }

    ImGui::End();
  }

  ImGui::PopFont();
  rlImGuiEnd();
}

struct Board {
  Camera2D camera;
};

void BoardInit(Board& board) {
  board.camera = {0};
  board.camera.offset = {GetScreenWidth() / 2.0f, GetScreenHeight() / 2.0f};
  board.camera.target = {0.0f, 0.0f};
  board.camera.rotation = 0.0f;
  board.camera.zoom = 1.0f;
}

Color ToRay(simulation::RGB color) {
  return Color{.r = color.r, .g = color.g, .b = color.b, .a = 255};
}

static inline void Draw(Arena& arena, Board& board, const simulation::Sim& sim,
    simulation::EntityId& selected_id) {
  f32 scale = 40.0;

  const auto items = simulation::ViewMapItems(sim, arena);

  struct ClickBox {
    Rectangle bounds;
    simulation::EntityId id;
  };
  auto click_boxes = Stream<ClickBox>(&arena);

  {
    BeginMode2D(board.camera);
    auto iter = items.Iterate();

    while (const auto* item = iter.Next()) {
      auto size = item->size * scale;
      auto bounds = Rectangle{.x = item->coords.x * scale - size / 2.0f,
          .y = item->coords.y * scale - size / 2.0f,
          .width = size / 2.0f,
          .height = size / 2.0f};
      auto color = ToRay(item->color);
      bool is_selected = item->id == selected_id;
      DrawRectanglePro(bounds, {0.0, 0.0}, 0.0, color);

      // Borders
      Color border_color = BLACK;
      if (is_selected) {
        border_color = YELLOW;
      }
      DrawRectangleLinesEx(bounds, 4.0, border_color);

      click_boxes.Push({ bounds, item->id });
    }

    EndMode2D();
  }

  if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && !ImGui::GetIO().WantCaptureMouse) {
    auto screen_pos = GetMousePosition();
    auto world_pos = GetScreenToWorld2D(screen_pos, board.camera);

    auto iter = click_boxes.Iterate();
    auto found_id = simulation::EntityId::Null();
    while (auto* item = iter.Next()) {
      if (CheckCollisionPointRec(world_pos, item->bounds)) {
        found_id = item->id;
      }
    }
    selected_id = found_id;
  }
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
  Arena arena;

  SetConfigFlags(FLAG_VSYNC_HINT);
  InitWindow(1600, 900, "Econ Test");
  rlImGuiSetup(true);

  Gui gui;
  SetupGui(gui);

  SetTargetFPS(GetMonitorRefreshRate(GetCurrentMonitor()));

  simulation::Sim sim;
  simulation::Init(sim);

  Board board;
  BoardInit(board);

  simulation::EntityId selected_id;

  while (!WindowShouldClose()) {
    arena.Reset();

    if (!selected_id.IsValid()) {
      selected_id = simulation::EntityId::Null();
    }

    // Input
    if (IsKeyPressed(KEY_ESCAPE)) {
      break;
    }

    // Draw
    //----------------------------------------------------------------------------------
    BeginDrawing();
    ClearBackground(GRAY);

    Draw(arena, board, sim, selected_id);

    DrawGui(gui, sim, selected_id);

    EndDrawing();

    // Simulate
  }

  rlImGuiShutdown();
  CloseWindow();
  return 0;
}
