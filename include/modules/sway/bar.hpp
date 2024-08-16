#pragma once
#include <atomic>
#include <string>

#include "bar_config.hpp"
#include "modules/sway/ipc/client.hpp"
#include "util/SafeSignal.hpp"
#include "util/json.hpp"

namespace waybar {

class BarInstance;

namespace modules::sway {

/*
 * Supported subset of i3/sway IPC barconfig object
 */
struct swaybar_config {
  std::string id;
  std::string mode;
  std::string hidden_state;
  std::string position;

  std::vector<std::string> outputs;
};

/**
 * swaybar IPC client
 */
class BarIpcClient {
 public:
  BarIpcClient(waybar::BarInstance& bar);

 private:
  using ModifierReset = BarConfig::ModifierReset;

  void onInitialConfig(const struct Ipc::ipc_response& res);
  void onIpcEvent(const struct Ipc::ipc_response&);
  void onCmd(const struct Ipc::ipc_response&);
  void onConfigUpdate(const swaybar_config& config);
  void onVisibilityUpdate(bool visible_by_modifier);
  void onModeUpdate(bool visible_by_modifier);
  void onUrgencyUpdate(bool visible_by_urgency);
  void update();

  BarInstance& bar_;
  util::JsonParser parser_;
  Ipc ipc_;

  swaybar_config bar_config_;
  ModifierReset modifier_reset_;
  bool visible_by_mode_ = false;
  bool visible_by_modifier_ = false;
  bool visible_by_urgency_ = false;
  std::atomic<bool> modifier_no_action_ = false;

  SafeSignal<bool> signal_mode_;
  SafeSignal<bool> signal_visible_;
  SafeSignal<bool> signal_urgency_;
  SafeSignal<swaybar_config> signal_config_;
};

}  // namespace modules::sway
}  // namespace waybar
