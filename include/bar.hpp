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

class Bar;
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

class BarInstance : public sigc::trackable {
 public:
  BarInstance(Glib::RefPtr<Gtk::Application> app, const Json::Value &json);
  ~BarInstance();

  Glib::RefPtr<Gtk::Application> app;
  const BarConfig config;

  bool isOutputEnabled(struct waybar_output *output) const;

  void onOutputAdded(struct waybar_output *output);
  void onOutputRemoved(struct waybar_output *output);

  void handleSignal(int signal);
  void setMode(const std::string &mode);
  void setOutputs(const std::vector<std::string> &outputs);
  void setPosition(const std::string &pos);
  void setVisible(bool value);
  void toggle();

  const std::string &mode() { return mode_; }
  sigc::signal<void, const std::string &> signal_mode;

  const std::vector<std::string> &outputs() { return outputs_; }

  Gtk::PositionType position() { return position_; }
  sigc::signal<void, Gtk::PositionType> signal_position;

  std::list<Bar> surfaces;

#ifdef HAVE_SWAY
  std::string bar_id;
#endif

 private:
  bool visible_;
  std::string mode_;
  std::vector<std::string> outputs_;
  Gtk::PositionType position_;

#ifdef HAVE_SWAY
  using BarIpcClient = modules::sway::BarIpcClient;
  std::unique_ptr<BarIpcClient> ipc_client_;
#endif
};

class Bar : public sigc::trackable {
 public:
  Bar(struct waybar_output *w_output, const BarConfig &w_config, BarInstance &w_inst);
  Bar(const Bar &) = delete;
  ~Bar();

  void handleSignal(int);

  const BarConfig &config;
  BarInstance &inst;

  struct waybar_output *output;
  struct wl_surface *surface;
  bool visible = true;
  Gtk::Window window;
  Gtk::PositionType position;
  Gtk::Orientation orientation;

  int x_global;
  int y_global;

 private:
  void onMap(GdkEventAny *);
  auto setupWidgets() -> void;
  void getModules(const Factory &, const std::string &, waybar::Group *);
  void setMode(const BarMode &mode);
  void setPassThrough(bool passthrough);
  void onConfigure(GdkEventConfigure *ev);
  void configureGlobalOffset(int width, int height);
  void onOutputGeometryChanged();

  void onModeChange(const std::string &mode);
  void onPositionChange(Gtk::PositionType position);

  std::string last_mode_{BarConfig::MODE_DEFAULT};
  Gtk::PositionType last_position_{Gtk::PositionType::POS_TOP};

  uint32_t width_, height_;
  bool passthrough_;

  Gtk::Box left_;
  Gtk::Box center_;
  Gtk::Box right_;
  Gtk::Box box_;
  std::vector<std::shared_ptr<waybar::AModule>> modules_left_;
  std::vector<std::shared_ptr<waybar::AModule>> modules_center_;
  std::vector<std::shared_ptr<waybar::AModule>> modules_right_;
  std::vector<std::shared_ptr<waybar::AModule>> modules_all_;
};

}  // namespace waybar
