#include <fstream>

#include <catch2/catch.hpp>
#include <json/json.h>

#include "bar_config.hpp"
#include "util/json.hpp"

using namespace waybar;

Json::Value operator""_json(const char* str, size_t size) {
  static util::JsonParser parser;
  return parser.parse(str);
}

template <typename ValueType>
auto get(const Json::Value& src) -> ValueType {
  ValueType dst;
  from_json(src, dst);
  return dst;
}

TEST_CASE("Format operators", "[bar_config]") {
  REQUIRE(fmt::format("{}", bar_layer::TOP) == "top");
  REQUIRE(fmt::format("{}", bar_mode::DOCK) == "dock");
  REQUIRE(fmt::format("{}", bar_position::BOTTOM) == "bottom");

  bar_config config;
  config.width = 100;
  config.height = 200;
  config.margins = {1, 2, 3, 4};
  REQUIRE(
      fmt::format("{}", config) ==
      "mode: dock, layer: bottom, position: top, size: 100x200, margins: 1 2 3 4, hidden: false");
}

TEST_CASE("Enum validation", "[bar_config]") {
  using Catch::Matchers::Contains;

  CHECK_THROWS(get<bar_layer>(1));
  CHECK_THROWS(get<bar_layer>(true));
  CHECK_THROWS_WITH(get<bar_layer>("invalid"),
                    Contains("Invalid value") && Contains("specified for enum bar_layer"));
  CHECK_NOTHROW(get<bar_layer>("top"));

  CHECK_THROWS_WITH(get<bar_mode>("invalid"),
                    Contains("Invalid value") && Contains("specified for enum bar_mode"));
  CHECK_NOTHROW(get<bar_mode>("dock"));

  CHECK_THROWS_WITH(get<bar_position>("invalid"),
                    Contains("Invalid value") && Contains("specified for enum bar_position"));
  CHECK_NOTHROW(get<bar_position>("top"));
}

TEST_CASE("Empty config", "[bar_config]") {
  auto config = get<bar_config>(R"({  })"_json);

  REQUIRE(config.mode == bar_mode::DOCK);
  REQUIRE(config.layer == bar_layer::BOTTOM);
  REQUIRE(config.position == bar_position::TOP);
  REQUIRE(config.hidden_state == true);
  REQUIRE(config.margins.top == 0);
  REQUIRE(config.margins.right == 0);
  REQUIRE(config.margins.bottom == 0);
  REQUIRE(config.margins.left == 0);
  REQUIRE(config.height == 0);
  REQUIRE(config.width == 0);
}

TEST_CASE("Vertical bar", "[bar_config]") {
  auto config = get<bar_config>(R"({
    "layer": "top",
    "position": "left",
    "height": 5,
    "width": 10
  })"_json);

  REQUIRE(config.mode == bar_mode::DOCK);
  REQUIRE(config.layer == bar_layer::TOP);
  REQUIRE(config.position == bar_position::LEFT);
  REQUIRE(config.hidden_state == true);
  REQUIRE(config.height == 5);
  REQUIRE(config.width == 10);

  REQUIRE(config.is_hidden() == false);
  REQUIRE(config.is_vertical() == true);
}

TEST_CASE("Margins", "[bar_config]") {
  auto config = get<bar_config>(R"({
    "margin": "1 2 3 4"
  })"_json);

  REQUIRE(config.margins.top == 1);
  REQUIRE(config.margins.right == 2);
  REQUIRE(config.margins.bottom == 3);
  REQUIRE(config.margins.left == 4);

  config = get<bar_config>(R"({
    "margin": "1 2 3"
  })"_json);

  REQUIRE(config.margins.top == 1);
  REQUIRE(config.margins.right == 2);
  REQUIRE(config.margins.bottom == 3);
  REQUIRE(config.margins.left == 2);

  config = get<bar_config>(R"({
    "margin": "1 2"
  })"_json);

  REQUIRE(config.margins.top == 1);
  REQUIRE(config.margins.right == 2);
  REQUIRE(config.margins.bottom == 1);
  REQUIRE(config.margins.left == 2);

  config = get<bar_config>(R"({
    "margin": "1"
  })"_json);

  REQUIRE(config.margins.top == 1);
  REQUIRE(config.margins.right == 1);
  REQUIRE(config.margins.bottom == 1);
  REQUIRE(config.margins.left == 1);

  config = get<bar_config>(R"({
    "margin-top": 1,
    "margin-right": 2,
    "margin-bottom": 3,
    "margin-left": 4
  })"_json);

  REQUIRE(config.margins.top == 1);
  REQUIRE(config.margins.right == 2);
  REQUIRE(config.margins.bottom == 3);
  REQUIRE(config.margins.left == 4);

  config = get<bar_config>(R"({
    "margin-top": 1,
    "margin-right": 2,
    "margin-left": 4
  })"_json);

  REQUIRE(config.margins.top == 1);
  REQUIRE(config.margins.right == 2);
  REQUIRE(config.margins.bottom == 0);
  REQUIRE(config.margins.left == 4);
}

TEST_CASE("Sway barconfig_update", "[bar_config]") {
  bar_config config = get<ipc_bar_config>(R"({
    "id": "bar-0",
    "mode": "dock",
    "hidden_state": "hide",
    "position": "top",
    "status_command": null,
    "font": "monospace 10",
    "gaps": {
      "top": 5,
      "right": 10,
      "bottom": 5,
      "left": 10
    },
    "bar_height": 20,
    "status_padding": 1,
    "status_edge_padding": 3,
    "wrap_scroll": false,
    "workspace_buttons": true,
    "strip_workspace_numbers": false,
    "strip_workspace_name": false,
    "binding_mode_indicator": true,
    "verbose": false,
    "pango_markup": false,
    "colors": { },
    "tray_padding": 2
  })"_json);
  REQUIRE(config.mode == bar_mode::DOCK);
  REQUIRE(config.hidden_state == true);
  REQUIRE(config.position == bar_position::TOP);
  REQUIRE(config.margins.top == 5);
  REQUIRE(config.margins.right == 10);
  REQUIRE(config.margins.bottom == 5);
  REQUIRE(config.margins.left == 10);
  REQUIRE(config.height == 20);
  REQUIRE(config.width == 0);

  REQUIRE(config.is_hidden() == false);
  REQUIRE(config.is_vertical() == false);
}

TEST_CASE("Sway barconfig_update with hidden bar", "[bar_config]") {
  bar_config config = get<ipc_bar_config>(R"({
    "id": "bar-0",
    "mode": "hide",
    "hidden_state": "hide",
    "position": "bottom",
    "status_command": null,
    "font": "monospace 10",
    "gaps": {
      "top": 5,
      "right": 10,
      "bottom": 5,
      "left": 10
    },
    "bar_height": 20
  })"_json);
  REQUIRE(config.mode == bar_mode::HIDE);
  REQUIRE(config.hidden_state == true);
  REQUIRE(config.position == bar_position::BOTTOM);
  REQUIRE(config.margins.top == 5);
  REQUIRE(config.margins.right == 10);
  REQUIRE(config.margins.bottom == 5);
  REQUIRE(config.margins.left == 10);
  REQUIRE(config.height == 20);
  REQUIRE(config.width == 0);

  REQUIRE(config.is_hidden() == true);
  REQUIRE(config.is_vertical() == false);
}

TEST_CASE("Check parsing of the default config", "[bar_config]") {
  // path is relative to the `test` directory of the project
  std::ifstream file("../resources/config");
  Json::Value   json;
  REQUIRE(file.is_open());
  REQUIRE_NOTHROW(file >> json);
  REQUIRE_NOTHROW(get<bar_config>(json));
}
