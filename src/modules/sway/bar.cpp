#include <fmt/ostream.h>
#include <spdlog/spdlog.h>

#include "bar_config.hpp"
#include "client.hpp"
#include "modules/sway/bar.hpp"
#include "modules/sway/ipc/ipc.hpp"

namespace waybar::modules::sway {

BarIpcClient::BarIpcClient(waybar::Client& client, const std::string& bar_id,
                           bool get_initial_config)
    : client_{client}, bar_id_{bar_id} {
  if (get_initial_config) {
    sigc::connection handle =
        ipc_.signal_cmd.connect(sigc::mem_fun(*this, &BarIpcClient::onInitialConfig));
    ipc_.sendCmd(IPC_GET_BAR_CONFIG, bar_id);

    handle.disconnect();
  }

  signal_config_.connect(sigc::mem_fun(*this, &BarIpcClient::onConfigUpdate));
  signal_visible_.connect(sigc::mem_fun(*this, &BarIpcClient::onVisibilityUpdate));

  ipc_.subscribe(R"(["bar_state_update", "barconfig_update"])");
  ipc_.signal_event.connect(sigc::mem_fun(*this, &BarIpcClient::onIpcEvent));
  // Launch worker
  ipc_.setWorker([this] {
    try {
      ipc_.handleEvent();
    } catch (const std::exception& e) {
      spdlog::error("BarIpcClient::handleEvent {}", e.what());
    }
  });
}

void BarIpcClient::onInitialConfig(const struct Ipc::ipc_response& res) {
  try {
    auto payload = parser_.parse(res.payload);

    ipc_bar_config config;
    from_json(payload, config);
    spdlog::info("swaybar ipc: initial config: {}", config);
  } catch (const std::exception& e) {
    spdlog::error("BarIpcClient::onInitialConfig {}", e.what());
  }
}

void BarIpcClient::onIpcEvent(const struct Ipc::ipc_response& res) {
  try {
    auto payload = parser_.parse(res.payload);
    if (auto id = payload["id"]; id.isString() && id.asString() != bar_id_) {
      spdlog::trace("swaybar ipc: ignore event for {}", id.asString());
      return;
    }
    if (payload.isMember("visible_by_modifier")) {
      // visibility change for hidden bar
      signal_visible_(payload["visible_by_modifier"].asBool());
    } else {
      // configuration update
      ipc_bar_config config;
      from_json(payload, config);
      signal_config_(config);
    }
  } catch (const std::exception& e) {
    spdlog::error("BarIpcClient::onEvent {}", e.what());
  }
}

void BarIpcClient::onConfigUpdate(bar_config config) {
  spdlog::info("config update: {}", config);
  // TODO: pass config to bars
}

void BarIpcClient::onVisibilityUpdate(bool visible_by_modifier) {
  spdlog::trace("visiblity update: {}", visible_by_modifier);
  for (auto& bar : client_.bars) {
    bar->setVisible(visible_by_modifier);
  }
}

}  // namespace waybar::modules::sway
