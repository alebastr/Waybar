#pragma once

#include <fmt/format.h>
#include <json/value.h>

#include <cstdint>
#include <map>
#include <optional>
#include <string_view>

namespace waybar {

enum class bar_layer : uint8_t { NONE, BOTTOM, TOP, OVERLAY };

enum class bar_mode : uint8_t {
  DOCK,
  HIDE,
  INVISIBLE,
  OVERLAY,
};

enum class bar_position : uint8_t {
  TOP,
  RIGHT,
  BOTTOM,
  LEFT,
};

struct bar_margins {
  int top = 0;
  int right = 0;
  int bottom = 0;
  int left = 0;
};

struct bar_config {
  uint32_t     height = 0;
  uint32_t     width = 0;
  bar_layer    layer = bar_layer::BOTTOM;
  bar_mode     mode = bar_mode::DOCK;
  bar_position position = bar_position::TOP;
  /*
   * i3/sway bar with mode `hide` can be displayed temporarily by pressing a modifier and issuing
   * `bar_state_update` event or permanently by setting `hidden_state` to show.
   */
  bool hidden_state = true;

  struct bar_margins margins;

  bool is_vertical() const {
    return (position == bar_position::LEFT || position == bar_position::RIGHT);
  }
  /**
   * The bar is configured to be displayed over the applications without creating exclusive zone.
   */
  bool is_overlay() const { return (mode == bar_mode::OVERLAY || mode == bar_mode::HIDE); }
  /**
   * The bar is hidden according to the sway configuration.
   * Only hidden bars could receive `visible_by_modifier` events.
   */
  bool is_hidden() const {
    return (mode == bar_mode::INVISIBLE || (mode == bar_mode::HIDE && hidden_state));
  }
};

// need a separate JSON deserializer for IPC responses
struct ipc_bar_config : public bar_config {};

/**
 * Deserialize bar layer from string
 */
void from_str(const std::string_view &src, bar_layer &dst);
void from_json(const Json::Value &src, bar_layer &dst);

/**
 * Deserialize bar mode from string
 */
void from_str(const std::string_view &src, bar_mode &dst);
void from_json(const Json::Value &src, bar_mode &dst);

/**
 * Deserialize bar position from string
 */
void from_str(const std::string_view &src, bar_position &dst);
void from_json(const Json::Value &src, bar_position &dst);

/**
 * Deserialize margins from string in one of following formats:
 * <all> | <vertical> <horizontal> | <top> <horizontal> <bottom> | <top> <right> <bottom> <left>
 * NOTE: the format is not compatible with sway `bar <bar_id> gaps ...`
 */
void from_str(const std::string &str, bar_margins &dst);

/**
 * Deserialize margins from JSON value (string | object { top, right, bottom, left })
 */
void from_json(const Json::Value &str, bar_margins &dst);

/**
 * Deserialize bar configuration from waybar json format
 */
void from_json(const Json::Value &str, bar_config &dst);

/**
 * Deserialize bar configuration from i3/sway barconfig_update json object
 */
void from_json(const Json::Value &src, ipc_bar_config &dst);

namespace detail {
const std::string_view &to_str(bar_layer val);
const std::string_view &to_str(bar_mode val);
const std::string_view &to_str(bar_position val);

}  // namespace detail

}  // namespace waybar

template <>
struct fmt::formatter<waybar::bar_layer> : fmt::formatter<std::string_view> {
  template <typename FormatContext>
  auto format(waybar::bar_layer val, FormatContext &ctx) {
    return fmt::formatter<std::string_view>::format(waybar::detail::to_str(val), ctx);
  }
};

template <>
struct fmt::formatter<waybar::bar_mode> : fmt::formatter<std::string_view> {
  template <typename FormatContext>
  auto format(waybar::bar_mode val, FormatContext &ctx) {
    return fmt::formatter<std::string_view>::format(waybar::detail::to_str(val), ctx);
  }
};

template <>
struct fmt::formatter<waybar::bar_position> : fmt::formatter<std::string_view> {
  template <typename FormatContext>
  auto format(waybar::bar_position val, FormatContext &ctx) {
    return fmt::formatter<std::string_view>::format(waybar::detail::to_str(val), ctx);
  }
};

template <>
struct fmt::formatter<waybar::bar_config> : fmt::formatter<std::string> {
  template <typename FormatContext>
  auto format(const waybar::bar_config &val, FormatContext &ctx) {
    return format_to(
        ctx.out(),
        "mode: {}, layer: {}, position: {}, size: {}x{}, margins: {} {} {} {}, hidden: {}",
        val.mode,
        val.layer,
        val.position,
        val.width,
        val.height,
        val.margins.top,
        val.margins.right,
        val.margins.bottom,
        val.margins.left,
        val.is_hidden());
  }
};

template <>
struct fmt::formatter<waybar::ipc_bar_config> : fmt::formatter<waybar::bar_config> {};
