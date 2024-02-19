#pragma once

#include <gdkmm/monitor.h>
#include <glibmm/refptr.h>
#include <gtkmm/box.h>
#include <gtkmm/cssprovider.h>
#include <gtkmm/main.h>
#include <gtkmm/window.h>
#include <json/json.h>

#include <memory>
#include <optional>
#include <vector>

#include "AModule.hpp"
#include "bar_config.hpp"
#include "group.hpp"
#include "xdg-output-unstable-v1-client-protocol.h"

namespace waybar {

class Factory;
struct waybar_output {
  Glib::RefPtr<Gdk::Monitor> monitor;
  std::string name;
  std::string identifier;

  std::unique_ptr<struct zxdg_output_v1, decltype(&zxdg_output_v1_destroy)> xdg_output = {
      nullptr, &zxdg_output_v1_destroy};
};

#ifdef HAVE_SWAY
namespace modules::sway {
class BarIpcClient;
}
#endif  // HAVE_SWAY

class Bar {
 public:
  Bar(struct waybar_output *w_output, const Json::Value &);
  Bar(const Bar &) = delete;
  ~Bar();

  void setMode(const std::string &mode);
  void setVisible(bool value);
  void toggle();
  void handleSignal(int);

  const BarConfig config;

  struct waybar_output *output;
  struct wl_surface *surface;
  bool visible = true;
  Gtk::Window window;
  Gtk::PositionType position;
  Gtk::Orientation orientation;

  int x_global;
  int y_global;

#ifdef HAVE_SWAY
  std::string bar_id;
#endif

 private:
  void onMap(GdkEventAny *);
  auto setupWidgets() -> void;
  void getModules(const Factory &, const std::string &, waybar::Group *);
  void setMode(const BarMode &mode);
  void setPassThrough(bool passthrough);
  void setPosition(Gtk::PositionType position);
  void onConfigure(GdkEventConfigure *ev);
  void configureGlobalOffset(int width, int height);
  void onOutputGeometryChanged();

  std::string last_mode_{BarConfig::MODE_DEFAULT};

  uint32_t width_, height_;
  bool passthrough_;

  Gtk::Box left_;
  Gtk::Box center_;
  Gtk::Box right_;
  Gtk::Box box_;
  std::vector<std::shared_ptr<waybar::AModule>> modules_left_;
  std::vector<std::shared_ptr<waybar::AModule>> modules_center_;
  std::vector<std::shared_ptr<waybar::AModule>> modules_right_;
#ifdef HAVE_SWAY
  using BarIpcClient = modules::sway::BarIpcClient;
  std::unique_ptr<BarIpcClient> _ipc_client;
#endif
  std::vector<std::shared_ptr<waybar::AModule>> modules_all_;
};

}  // namespace waybar
