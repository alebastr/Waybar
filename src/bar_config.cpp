#include "bar_config.hpp"

#include <spdlog/spdlog.h>

#include <stdexcept>

namespace {
using namespace waybar;
const std::map<std::string_view, bar_layer> layer_str = {
    {"bottom", bar_layer::BOTTOM},
    {"top", bar_layer::TOP},
    {"overlay", bar_layer::OVERLAY},
};

const std::map<std::string_view, bar_mode> mode_str = {
    {"dock", bar_mode::DOCK},
    {"hide", bar_mode::HIDE},
    {"invisible", bar_mode::INVISIBLE},
    {"overlay", bar_mode::OVERLAY},
};

const std::map<std::string_view, bar_position> position_str = {
    {"top", bar_position::TOP},
    {"right", bar_position::RIGHT},
    {"bottom", bar_position::BOTTOM},
    {"left", bar_position::LEFT},
};

const std::string_view UNDEFINED_STR = "<undefined>";

class EnumValueError : public std::logic_error {
 private:
  template <typename EnumType>
  static std::string values_to_str(const std::map<std::string_view, EnumType> &values) {
    std::string acc;
    for (auto [key, value] : values) {
      if (!acc.empty()) {
        acc += ", ";
      }
      acc += key;
    }
    return acc;
  }

 public:
  template <typename EnumType>
  EnumValueError(const std::string_view &enum_name, const std::string_view &value,
                 const std::map<std::string_view, EnumType> &accepted)
      : std::logic_error{
            fmt::format("Invalid value \"{}\" specified for enum {}. Accepted values: {}", value,
                        enum_name, values_to_str(accepted))} {}
};

}  // namespace

namespace waybar {

void from_str(const std::string_view &str, bar_layer &dst) {
  const auto &it = layer_str.find(str);
  if (it == layer_str.end()) {
    throw EnumValueError{"bar_layer", str, layer_str};
  }
  dst = it->second;
}

void from_json(const Json::Value &src, bar_layer &dst) { from_str(src.asString(), dst); }

void from_str(const std::string_view &str, bar_mode &dst) {
  const auto &it = mode_str.find(str);
  if (it == mode_str.end()) {
    throw EnumValueError{"bar_mode", str, mode_str};
  }
  dst = it->second;
}

void from_json(const Json::Value &src, bar_mode &dst) { from_str(src.asString(), dst); }

void from_str(const std::string_view &str, bar_position &dst) {
  const auto &it = position_str.find(str);
  if (it == position_str.end()) {
    throw EnumValueError{"bar_position", str, position_str};
  }
  dst = it->second;
}

void from_json(const Json::Value &src, bar_position &dst) { from_str(src.asString(), dst); }

namespace detail {

const std::string_view &to_str(bar_layer val) {
  for (const auto &[key, value] : layer_str) {
    if (value == val) {
      return key;
    }
  }
  return UNDEFINED_STR;
}

const std::string_view &to_str(bar_mode val) {
  for (const auto &[key, value] : mode_str) {
    if (value == val) {
      return key;
    }
  }
  return UNDEFINED_STR;
}

const std::string_view &to_str(bar_position val) {
  for (const auto &[key, value] : position_str) {
    if (value == val) {
      return key;
    }
  }
  return UNDEFINED_STR;
}

}  // namespace detail

void from_str(const std::string &str, bar_margins &result) {
  std::istringstream       iss(str);
  std::vector<std::string> values{std::istream_iterator<std::string>(iss), {}};
  try {
    if (values.size() == 1) {
      auto gaps = std::stoi(values[0], nullptr, 10);
      result = {gaps, gaps, gaps, gaps};
    }
    if (values.size() == 2) {
      auto vertical_margins = std::stoi(values[0], nullptr, 10);
      auto horizontal_margins = std::stoi(values[1], nullptr, 10);
      result = {vertical_margins, horizontal_margins, vertical_margins, horizontal_margins};
    }
    if (values.size() == 3) {
      auto horizontal_margins = std::stoi(values[1], nullptr, 10);
      result.top = std::stoi(values[0], nullptr, 10);
      result.right = horizontal_margins;
      result.bottom = std::stoi(values[2], nullptr, 10);
      result.left = horizontal_margins;
    }
    if (values.size() >= 4) {
      result.top = std::stoi(values[0], nullptr, 10);
      result.right = std::stoi(values[1], nullptr, 10);
      result.bottom = std::stoi(values[2], nullptr, 10);
      result.left = std::stoi(values[3], nullptr, 10);
    }
  } catch (...) {
    spdlog::warn("Invalid margins: {}", str);
  }
}

void from_json(const Json::Value &src, bar_margins &dst) {
  if (src.isObject()) {
    dst.top = src["top"].isInt() ? src["top"].asInt() : 0;
    dst.right = src["right"].isInt() ? src["right"].asInt() : 0;
    dst.bottom = src["bottom"].isInt() ? src["bottom"].asInt() : 0;
    dst.left = src["left"].isInt() ? src["left"].asInt() : 0;
  } else if (src.isInt()) {
    int value = src.asInt();
    dst.top = value;
    dst.right = value;
    dst.bottom = value;
    dst.left = value;
  } else if (src.isString()) {
    from_str(src.asString(), dst);
  }
}

void from_json(const Json::Value &src, bar_config &dst) {
  if (!src.isObject()) {
    return;
  }

  if (auto height = src["height"]; height.isUInt()) {
    dst.height = height.asUInt();
  }

  if (auto width = src["width"]; width.isUInt()) {
    dst.width = width.asUInt();
  }

  if (auto layer = src["layer"]; layer.isString()) {
    from_json(layer, dst.layer);
  }

  if (auto mode = src["mode"]; mode.isString()) {
    from_json(src["mode"], dst.mode);
  }

  if (auto position = src["position"]; position.isString()) {
    from_json(position, dst.position);
  }

  if (src["margin-top"].isInt() || src["margin-right"].isInt() || src["margin-bottom"].isInt() ||
      src["margin-left"].isInt()) {
    dst.margins = {
        src["margin-top"].isInt() ? src["margin-top"].asInt() : 0,
        src["margin-right"].isInt() ? src["margin-right"].asInt() : 0,
        src["margin-bottom"].isInt() ? src["margin-bottom"].asInt() : 0,
        src["margin-left"].isInt() ? src["margin-left"].asInt() : 0,
    };
  } else if (src.isMember("margin")) {
    from_json(src["margin"], dst.margins);
  }
}

void from_json(const Json::Value &src, ipc_bar_config &dst) {
  if (!src.isObject()) {
    return;
  }

  // ipc messages should not change layer
  dst.layer = bar_layer::NONE;

  if (auto height = src["bar_height"]; height.isUInt()) {
    dst.height = height.asUInt();
  }
  if (auto hidden = src["hidden_state"]; hidden.isString()) {
    dst.hidden_state = hidden.asString() != "show";
  }

  from_json(src["mode"], dst.mode);
  from_json(src["position"], dst.position);
  from_json(src["gaps"], dst.margins);
}

}  // namespace waybar
