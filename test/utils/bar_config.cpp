#include "bar_config.hpp"

#include "util/json.hpp"

#if __has_include(<catch2/catch_test_macros.hpp>)
#include <catch2/catch_test_macros.hpp>
#else
#include <catch2/catch.hpp>
#endif

using namespace waybar;

TEST_CASE("Defaults", "[config]") {
  util::JsonParser parser;
  Json::Value json = parser.parse("{}");
  BarConfig conf{json};

  REQUIRE(conf.mode == std::nullopt);
  REQUIRE(conf.position == std::nullopt);
  REQUIRE(conf.name == std::nullopt);
  REQUIRE(conf.width == 0);
  REQUIRE(conf.height == 0);
}

TEST_CASE("Custom modes", "[config]") {
  util::JsonParser parser;
  Json::Value json = parser.parse(R"(
    {
      "mode": "custom",
      "modes": {
        "custom": {
          "layer": "overlay",
          "exclusive": false,
          "passthrough": true
        },
        "invisible": {
          "layer": "bottom",
          "exclusive": false,
          "passthrough": false,
          "visible": true
        }
      }
    }
  )");

  BarConfig conf{json};

  REQUIRE(conf.mode == "custom");

  REQUIRE(conf.modes[BarConfig::MODE_DEFAULT].layer == BarLayer::BOTTOM);

  REQUIRE(conf.modes[BarConfig::MODE_INVISIBLE].layer == BarLayer::BOTTOM);
  REQUIRE_FALSE(conf.modes[BarConfig::MODE_INVISIBLE].exclusive);
  REQUIRE_FALSE(conf.modes[BarConfig::MODE_INVISIBLE].passthrough);
  REQUIRE(conf.modes[BarConfig::MODE_INVISIBLE].visible);

  REQUIRE(conf.modes["custom"].layer == BarLayer::OVERLAY);
  REQUIRE_FALSE(conf.modes["custom"].exclusive);
  REQUIRE(conf.modes["custom"].passthrough);
}

TEST_CASE("isModuleEnabled", "[config]") {
  util::JsonParser parser;
  Json::Value json = parser.parse(R"(
    {
      "outputs": ["eDP-1", "HDMI-0"],
      "modules-left": ["sway/workspaces"],
      "modules-center": ["sway/window#hash"],
      "modules-right": ["group/hardware", "clock"],
      "group/hardware": {
        "orientation": "inherit",
        "modules": [
          "cpu",
          "memory",
          "battery"
        ]
      }
    }
  )");

  BarConfig conf{json};

  REQUIRE(conf.isModuleEnabled("sway/workspaces"));
  REQUIRE(conf.isModuleEnabled("sway/window"));
  REQUIRE(conf.isModuleEnabled("sway/window#hash"));
  REQUIRE(conf.isModuleEnabled("battery"));
  REQUIRE_FALSE(conf.isModuleEnabled("sway/workspace"));
  REQUIRE_FALSE(conf.isModuleEnabled("sway/workspaces#hash"));
  REQUIRE_FALSE(conf.isModuleEnabled("sway/language"));
}

TEST_CASE("outputs", "[config]") {
  util::JsonParser parser;

  SECTION("string") {
    Json::Value json = parser.parse(R"(
    {
      "output": "*"
    }
    )");
    BarConfig conf{json};

    REQUIRE(conf.outputs.size() == 1);
    REQUIRE(conf.outputs.at(0) == "*");
  }

  SECTION("array") {
    Json::Value json = parser.parse(R"(
    {
      "output": ["!HDMI-0", "*"]
    }
    )");
    BarConfig conf{json};

    REQUIRE(conf.outputs.size() == 2);
    REQUIRE(conf.outputs.at(0) == "!HDMI-0");
    REQUIRE(conf.outputs.at(1) == "*");
  }
}

TEST_CASE("format-alt-click", "[config]") {
  util::JsonParser parser;
  Json::Value json = parser.parse(R"(
    {
      "modules-center": ["test1", "test2", "group/group"],
      "test1": {
        "format-alt": "{}",
      },
      "test2": {
        "format-alt": "{}",
        "format-alt-click": "click-right"
      },
      "test3": {
        "format-alt": "{}",
        "format-alt-click": "click-right"
      },
      "group/group": {
        "orientation": "inherit",
        "modules": [
          "test3"
        ]
      },
    }
  )");

  BarConfig conf{json};

  /* Default */
  REQUIRE(conf.json()["test1"]["format-alt-click"].asUInt() == 1U);
  /* Normal parsing */
  REQUIRE(conf.json()["test2"]["format-alt-click"].asUInt() == 3U);
  /* Module within a group */
  REQUIRE(conf.json()["test3"]["format-alt-click"].asUInt() == 3U);
}

TEST_CASE("Margins", "[config]") {
  util::JsonParser parser;

  SECTION("individual properties") {
    Json::Value json = parser.parse(R"(
      {
        "margin-top": 1,
        "margin-right": 2,
        "margin-bottom": 3,
        "margin-left": 4
      }
    )");

    BarConfig conf{json};
    REQUIRE(conf.margins.top == 1);
    REQUIRE(conf.margins.right == 2);
    REQUIRE(conf.margins.bottom == 3);
    REQUIRE(conf.margins.left == 4);
  }

  SECTION("number") {
    Json::Value json = parser.parse(R"(
      {
        "margin": 1
      }
    )");

    BarConfig conf{json};
    REQUIRE(conf.margins.top == 1);
    REQUIRE(conf.margins.right == 1);
    REQUIRE(conf.margins.bottom == 1);
    REQUIRE(conf.margins.left == 1);
  }

  SECTION("string") {
    Json::Value json = parser.parse(R"(
      {
        "margin": "1"
      }
    )");

    BarConfig conf{json};
    REQUIRE(conf.margins.top == 1);
    REQUIRE(conf.margins.right == 1);
    REQUIRE(conf.margins.bottom == 1);
    REQUIRE(conf.margins.left == 1);
  }

  SECTION("string with 2 values") {
    Json::Value json = parser.parse(R"(
      {
        "margin": "1 2"
      }
    )");

    BarConfig conf{json};
    REQUIRE(conf.margins.top == 1);
    REQUIRE(conf.margins.right == 2);
    REQUIRE(conf.margins.bottom == 1);
    REQUIRE(conf.margins.left == 2);
  }

  SECTION("string with 4 values") {
    Json::Value json = parser.parse(R"(
      {
        "margin": "1 2 3 4"
      }
    )");

    BarConfig conf{json};
    REQUIRE(conf.margins.top == 1);
    REQUIRE(conf.margins.right == 2);
    REQUIRE(conf.margins.bottom == 3);
    REQUIRE(conf.margins.left == 4);
  }
}
