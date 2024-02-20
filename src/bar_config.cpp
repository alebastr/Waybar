#include "bar_config.hpp"

#include <spdlog/spdlog.h>

namespace waybar {
const BarConfig::ModeMap BarConfig::PRESET_MODES = {  //
    {"default",
     {// Special mode to hold the global bar configuration
      .layer = BarLayer::BOTTOM,
      .exclusive = true,
      .passthrough = false,
      .visible = true}},
    {"dock",
     {// Modes supported by the sway config; see man sway-bar(5)
      .layer = BarLayer::BOTTOM,
      .exclusive = true,
      .passthrough = false,
      .visible = true}},
    {"hide",
     {//
      .layer = BarLayer::TOP,
      .exclusive = false,
      .passthrough = false,
      .visible = true}},
    {"invisible",
     {//
      .layer = std::nullopt,
      .exclusive = false,
      .passthrough = true,
      .visible = false}},
    {"overlay",
     {//
      .layer = BarLayer::TOP,
      .exclusive = false,
      .passthrough = true,
      .visible = true}}};

const std::string BarConfig::MODE_DEFAULT = "default";
const std::string BarConfig::MODE_INVISIBLE = "invisible";

/* Deserializer for enum BarLayer */
void FromStr(std::string_view s, BarLayer& l) {
  if (s == "bottom") {
    l = BarLayer::BOTTOM;
  } else if (s == "top") {
    l = BarLayer::TOP;
  } else if (s == "overlay") {
    l = BarLayer::OVERLAY;
  }
}

std::string ToStr(BarLayer l) {
  using namespace std::literals;
  switch (l) {
    case BarLayer::TOP:
      return "top"s;
    case BarLayer::OVERLAY:
      return "overlay"s;
    default: /* BarLayer::BOTTOM */
      return "bottom"s;
  }
}

template <typename T>
void FromStr(std::string_view s, std::optional<T>& v) {
  FromStr(s, v.emplace());
}

/* Deserializer for struct BarMode */
void FromJson(const Json::Value& j, BarMode& m) {
  if (j.isObject()) {
    if (const auto& v = j["layer"]; v.isString()) {
      FromStr(v.asString(), m.layer);
    }
    if (const auto& v = j["exclusive"]; v.isBool()) {
      m.exclusive = v.asBool();
    }
    if (const auto& v = j["passthrough"]; v.isBool()) {
      m.passthrough = v.asBool();
    }
    if (const auto& v = j["visible"]; v.isBool()) {
      m.visible = v.asBool();
    }
  }
}

/* Deserializer for enum Gtk::PositionType */
void FromStr(std::string_view s, Gtk::PositionType& pos) {
  if (s == "left") {
    pos = Gtk::POS_LEFT;
  } else if (s == "right") {
    pos = Gtk::POS_RIGHT;
  } else if (s == "top") {
    pos = Gtk::POS_TOP;
  } else if (s == "bottom") {
    pos = Gtk::POS_BOTTOM;
  }
}

std::string ToStr(Gtk::PositionType pos) {
  using namespace std::literals;

  switch (pos) {
    case Gtk::POS_LEFT:
      return "left"s;
    case Gtk::POS_RIGHT:
      return "right"s;
    case Gtk::POS_BOTTOM:
      return "bottom"s;
    default: /* Gtk::POS_TOP */
      return "top"s;
  }
}

/* Deserializer for JSON Object -> map<string compatible type, Value>
 * Assumes that all the values in the object are deserializable to the same type.
 */
template <typename Key, typename Value,
          typename = std::enable_if_t<std::is_convertible_v<std::string, Key>>>
void FromJson(const Json::Value& j, std::map<Key, Value>& m) {
  if (j.isObject()) {
    for (auto it = j.begin(); it != j.end(); ++it) {
      FromJson(*it, m[it.key().asString()]);
    }
  }
}

void FromJson(const Json::Value& j, BarMargins& out) {
  if (j["margin-top"].isInt() || j["margin-right"].isInt() || j["margin-bottom"].isInt() ||
      j["margin-left"].isInt()) {
    out = {
        j["margin-top"].isInt() ? j["margin-top"].asInt() : 0,
        j["margin-right"].isInt() ? j["margin-right"].asInt() : 0,
        j["margin-bottom"].isInt() ? j["margin-bottom"].asInt() : 0,
        j["margin-left"].isInt() ? j["margin-left"].asInt() : 0,
    };
  } else if (j["margin"].isString()) {
    std::istringstream iss(j["margin"].asString());
    std::vector<std::string> margins{std::istream_iterator<std::string>(iss), {}};
    try {
      if (margins.size() == 1) {
        auto gaps = std::stoi(margins[0], nullptr, 10);
        out = {.top = gaps, .right = gaps, .bottom = gaps, .left = gaps};
      }
      if (margins.size() == 2) {
        auto vertical_margins = std::stoi(margins[0], nullptr, 10);
        auto horizontal_margins = std::stoi(margins[1], nullptr, 10);
        out = {.top = vertical_margins,
               .right = horizontal_margins,
               .bottom = vertical_margins,
               .left = horizontal_margins};
      }
      if (margins.size() == 3) {
        auto horizontal_margins = std::stoi(margins[1], nullptr, 10);
        out = {.top = std::stoi(margins[0], nullptr, 10),
               .right = horizontal_margins,
               .bottom = std::stoi(margins[2], nullptr, 10),
               .left = horizontal_margins};
      }
      if (margins.size() == 4) {
        out = {.top = std::stoi(margins[0], nullptr, 10),
               .right = std::stoi(margins[1], nullptr, 10),
               .bottom = std::stoi(margins[2], nullptr, 10),
               .left = std::stoi(margins[3], nullptr, 10)};
      }
    } catch (...) {
      spdlog::warn("Invalid margins: {}", j["margin"].asString());
    }
  } else if (j["margin"].isInt()) {
    auto gaps = j.get("margin", 0).asInt();
    out = {.top = gaps, .right = gaps, .bottom = gaps, .left = gaps};
  }
}

void FromJson(const Json::Value& j, BarConfig& config) {
#define get_value(dst, key, type)                 \
  if (const auto& val = j[key]; val.is##type()) { \
    dst = val.as##type();                         \
  }

/* Non-empty string */
#define get_str(dst, key)                                                    \
  if (const auto& val = j[key]; val.isString() && !val.asString().empty()) { \
    dst = val.asString();                                                    \
  }

  {
    const auto& outputs = j["output"];
    if (outputs.isArray()) {
      config.outputs.reserve(outputs.size());
      for (auto const& output : outputs) {
        config.outputs.push_back(output.asString());
      }
    } else if (outputs.isString()) {
      config.outputs.push_back(outputs.asString());
    }
  }

  get_str(config.name, "name");
  get_str(config.mode, "mode");

  get_value(config.spacing, "spacing", Int);
  get_value(config.width, "width", UInt);
  get_value(config.height, "height", UInt);

  get_value(config.fixed_center, "fixed-center", Bool);
  get_value(config.start_hidden, "start_hidden", Bool);
  get_value(config.reload_styles, "reload_style_on_change", Bool);

#ifdef HAVE_SWAY
  get_value(config.ipc, "ipc", Bool);
  get_str(config.bar_id, "id");

  if (const auto& val = j["modifier-reset"]; val.isString()) {
    if (val == "press") {
      config.modifier_reset = BarConfig::ModifierReset::PRESS;
    } else if (val == "release") {
      config.modifier_reset = BarConfig::ModifierReset::RELEASE;
    }
  }

#endif

  if (const auto& val = j["position"]; val.isString() && !val.asString().empty()) {
    Gtk::PositionType pos;
    FromStr(val.asString(), pos);
    config.position = pos;
  }

  /* Read custom modes if available */
  if (auto modes = j.get("modes", {}); modes.isObject()) {
    FromJson(modes, config.modes);
  }
  /* Update "default" mode with the global bar options */
  FromJson(j, config.modes[BarConfig::MODE_DEFAULT]);

  FromJson(j, config.margins);

#undef get_value
#undef get_str
}

// Converting string to button code rn as to avoid doing it later
void setupAltFormatKeyForModule(Json::Value& module) {
  if (module.isObject() && module.isMember("format-alt")) {
    if (module.isMember("format-alt-click")) {
      Json::Value& click = module["format-alt-click"];
      if (click.isString()) {
        if (click == "click-right") {
          module["format-alt-click"] = 3U;
        } else if (click == "click-middle") {
          module["format-alt-click"] = 2U;
        } else if (click == "click-backward") {
          module["format-alt-click"] = 8U;
        } else if (click == "click-forward") {
          module["format-alt-click"] = 9U;
        } else {
          module["format-alt-click"] = 1U;  // default click-left
        }
      } else {
        module["format-alt-click"] = 1U;
      }
    } else {
      module["format-alt-click"] = 1U;
    }
  }
}

BarConfig::BarConfig(Json::Value json) : json_(std::move(json)) {
  FromJson(json_, *this);

  for (const auto& section : {"modules-left", "modules-center", "modules-right"}) {
    setupAltFormatKeyForModuleList(getModuleList(section));
  }
}

bool BarConfig::isModuleEnabled(const std::string& ref) const {
  auto check = [this, ref](const auto& list, auto& check) -> bool {
    for (const auto& module : list) {
      auto name = module.asString();
      if (name == ref || name.substr(0, name.find('#')) == ref) {
        return true;
      }

      if (name.starts_with("group/") && check(getModuleList(name), check)) {
        return true;
      }
    }

    return false;
  };

  for (const auto& section : {"modules-left", "modules-center", "modules-right"}) {
    if (check(getModuleList(section), check)) {
      return true;
    }
  }

  return false;
}

const Json::Value& BarConfig::getModuleList(const std::string& ref) const {
  static Json::Value emptyArray = Json::arrayValue;

  if (ref.starts_with("modules-") && json_[ref].isArray()) {
    return json_[ref];
  }

  if (ref.starts_with("group/") && json_[ref]["modules"].isArray()) {
    return json_[ref]["modules"];
  }

  return emptyArray;
}

void BarConfig::setupAltFormatKeyForModuleList(const Json::Value& modules) {
  for (const auto& module_name : modules) {
    if (!module_name.isString()) {
      continue;
    }

    auto ref = module_name.asString();
    if (ref.starts_with("group/")) {
      setupAltFormatKeyForModuleList(getModuleList(ref));
    } else {
      setupAltFormatKeyForModule(json_[ref]);
    }
  }
}
}  // namespace waybar
