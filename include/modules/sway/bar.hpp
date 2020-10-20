#pragma once
#include <string>

#include "bar_config.hpp"
#include "modules/sway/ipc/client.hpp"
#include "util/SafeSignal.hpp"
#include "util/json.hpp"

namespace waybar {

class Client;

namespace modules::sway {
/**
 * swaybar IPC client
 */
class BarIpcClient {
 public:
  BarIpcClient(waybar::Client& client, const std::string& bar_id, bool get_initial_config = true);

 private:
  void onInitialConfig(const struct Ipc::ipc_response& res);
  void onIpcEvent(const struct Ipc::ipc_response&);
  void onConfigUpdate(bar_config config);
  void onVisibilityUpdate(bool visible_by_modifier);

  const Client&     client_;
  const std::string bar_id_;
  util::JsonParser  parser_;
  Ipc               ipc_;

  SafeSignal<bool>       signal_visible_;
  SafeSignal<bar_config> signal_config_;
};

}  // namespace modules::sway
}  // namespace waybar
