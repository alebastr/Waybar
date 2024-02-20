#pragma once

#include <gtkmm/enums.h>
#include <json/json.h>

#include <optional>
#include <string>
#include <vector>

namespace waybar {

enum class BarLayer : uint8_t {
  BOTTOM,
  TOP,
  OVERLAY,
};

struct BarMargins {
  int top = 0;
  int right = 0;
  int bottom = 0;
  int left = 0;
};

struct BarMode {
  std::optional<BarLayer> layer;
  bool exclusive;
  bool passthrough;
  bool visible;
};

void FromStr(std::string_view s, BarLayer& l);
std::string ToStr(BarLayer l);

void FromStr(std::string_view s, Gtk::PositionType& pos);
std::string ToStr(Gtk::PositionType pos);

inline Gtk::Orientation ToOrientation(Gtk::PositionType pos) {
  return (pos == Gtk::POS_LEFT || pos == Gtk::POS_RIGHT) ? Gtk::ORIENTATION_VERTICAL
                                                         : Gtk::ORIENTATION_HORIZONTAL;
}

void FromJson(const Json::Value& j, BarMode& m);

/* Parsed initial configuration for the bar */
class BarConfig {
 public:
  using ModeMap = std::map<std::string, struct BarMode>;

  static const ModeMap PRESET_MODES;
  static const std::string MODE_DEFAULT;
  static const std::string MODE_INVISIBLE;

  BarConfig(Json::Value json);

  const Json::Value& json() const { return json_; }

  /* Check if the module with specified type is added to the bar */
  bool isModuleEnabled(const std::string& ref) const;
  /* Get module configuration object */
  const Json::Value& getModuleConfig(const std::string& ref) const { return json_[ref]; }
  /*
   * Get module names for specified list or group.
   * Guaranteed to return Json::arrayValue.
   */
  const Json::Value& getModuleList(const std::string& ref) const;

  std::vector<std::string> outputs;
  std::optional<Gtk::PositionType> position;
  std::optional<std::string> name;

  BarMargins margins;
  uint32_t width = 0, height = 0;
  std::optional<int> spacing;

  std::optional<std::string> mode;
  /* Copy initial set of modes to allow customization */
  ModeMap modes = PRESET_MODES;

  bool fixed_center = true;
  bool start_hidden = false;
  bool reload_styles = false;

#ifdef HAVE_SWAY
  bool ipc = false;
  std::optional<std::string> bar_id;

  enum class ModifierReset : uint8_t {
    PRESS,
    RELEASE,
  } modifier_reset = ModifierReset::PRESS;
#endif
 private:
  void setupAltFormatKeyForModuleList(const Json::Value& modules);

  Json::Value json_;
};

}  // namespace waybar
