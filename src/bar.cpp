#include "bar.hpp"

#include <gtk-layer-shell.h>
#include <spdlog/spdlog.h>

#include <type_traits>

#include "client.hpp"
#include "factory.hpp"
#include "group.hpp"

#ifdef HAVE_SWAY
#include "modules/sway/bar.hpp"
#endif

namespace waybar {
static constexpr const char* MIN_HEIGHT_MSG =
    "Requested height: {} is less than the minimum height: {} required by the modules";

static constexpr const char* MIN_WIDTH_MSG =
    "Requested width: {} is less than the minimum width: {} required by the modules";

static constexpr const char* BAR_SIZE_MSG = "Bar configured (width: {}, height: {}) for output: {}";

const std::string_view DEFAULT_BAR_ID = "bar-0";

BarInstance::BarInstance(Glib::RefPtr<Gtk::Application> app, const Json::Value& json)
    : app{std::move(app)}, config{json} {
  mode_ = config.mode.value_or(BarConfig::MODE_DEFAULT);
  position_ = config.position.value_or(Gtk::POS_TOP);
  visible_ = !config.start_hidden;

#if HAVE_SWAY
  if (config.ipc) {
    bar_id = config.bar_id.value_or(Client::inst()->bar_id);
    if (bar_id.empty()) {
      bar_id = DEFAULT_BAR_ID;
    }

    try {
      ipc_client_ = std::make_unique<BarIpcClient>(*this);
    } catch (const std::exception& exc) {
      spdlog::warn("Failed to open bar ipc connection: {}", exc.what());
    }
  }
#endif
}

/* Need to define it here because of forward declared members */
BarInstance::~BarInstance() = default;

bool BarInstance::isOutputEnabled(struct waybar_output* output) const {
  if (config.outputs.empty()) {
    return true;
  }

  for (const auto& pattern : config.outputs) {
    if (pattern.starts_with('!') &&
        (pattern.substr(1) == output->name || pattern.substr(1) == output->identifier)) {
      return false;
    }

    if (pattern.starts_with('*') || pattern == output->name || pattern == output->identifier)
      return true;
  }

  return false;
}

void BarInstance::onOutputAdded(struct waybar_output* out) {
  if (isOutputEnabled(out) && !std::any_of(surfaces.begin(), surfaces.end(),
                                           [out](const auto& bar) { return bar.output == out; })) {
    surfaces.emplace_back(out, config, *this);
  }
}

void BarInstance::onOutputRemoved(struct waybar_output* out) {
  for (auto it = surfaces.begin(); it != surfaces.end();) {
    if (it->output == out) {
      it->window.hide();
      app->remove_window(it->window);
      it = surfaces.erase(it);
      spdlog::info("Bar removed from output: {}", out->name);
    } else {
      ++it;
    }
  }
}

void BarInstance::handleSignal(int signal) {
  std::for_each(surfaces.begin(), surfaces.end(),
                [&](auto& surface) { surface.handleSignal(signal); });
}

void BarInstance::setMode(const std::string& mode) {
  if (!config.modes.contains(mode)) {
    spdlog::warn("Invalid mode {}", mode);
  } else if (mode != mode_) {
    signal_mode.emit(mode_ = mode);
  }
}

void BarInstance::setPosition(const std::string& pos) {
  if (config.position) {
    /* The bar position was explicitly specified in the config */
    return;
  }

  Gtk::PositionType new_position = position_;
  FromStr(pos, new_position);
  if (new_position == position_) {
    return;
  }

  if (ToOrientation(new_position) != ToOrientation(position_)) {
    /* Orientation change should be properly signaled to all the containers in the current window.
     * As we don't do that now, let's reject the update.
     */
    spdlog::warn("Invalid position update: {} -> {}; refusing to change bar orientation",
                 ToStr(position_), ToStr(new_position));
    return;
  }
  spdlog::debug("Bar position updated: {} -> {}", ToStr(position_), ToStr(new_position));

  signal_position.emit(position_ = new_position);
}

void BarInstance::setVisible(bool value) {
  visible_ = value;
  setMode(visible_ ? config.mode.value_or(BarConfig::MODE_DEFAULT) : BarConfig::MODE_INVISIBLE);
}

void BarInstance::toggle() { setVisible(!visible_); }

};  // namespace waybar

waybar::Bar::Bar(struct waybar_output* w_output, const BarConfig& w_config, BarInstance& w_inst)
    : config(w_config),
      inst(w_inst),
      output(w_output),
      window{Gtk::WindowType::WINDOW_TOPLEVEL},
      position(config.position.value_or(Gtk::POS_TOP)),
      orientation(ToOrientation(inst.position())),
      x_global(0),
      y_global(0),
      left_(orientation, 0),
      center_(orientation, 0),
      right_(orientation, 0),
      box_(orientation, 0) {
  window.set_title("waybar");
  window.set_name("waybar");
  window.set_decorated(false);
  window.get_style_context()->add_class(output->name);

  if (config.name) {
    window.get_style_context()->add_class(config.name.value());
  }

  window.get_style_context()->add_class(ToStr(position));

  left_.get_style_context()->add_class("modules-left");
  center_.get_style_context()->add_class("modules-center");
  right_.get_style_context()->add_class("modules-right");

  if (config.spacing) {
    left_.set_spacing(config.spacing.value());
    center_.set_spacing(config.spacing.value());
    right_.set_spacing(config.spacing.value());
  }

  height_ = config.height;
  width_ = config.width;

  window.signal_configure_event().connect_notify(sigc::mem_fun(*this, &Bar::onConfigure));
  output->monitor->property_geometry().signal_changed().connect(
      sigc::mem_fun(*this, &Bar::onOutputGeometryChanged));

  // this has to be executed before GtkWindow.realize
  auto* gtk_window = window.gobj();
  gtk_layer_init_for_window(gtk_window);
  gtk_layer_set_keyboard_mode(gtk_window, GTK_LAYER_SHELL_KEYBOARD_MODE_NONE);
  gtk_layer_set_monitor(gtk_window, output->monitor->gobj());
  gtk_layer_set_namespace(gtk_window, "waybar");

  gtk_layer_set_margin(gtk_window, GTK_LAYER_SHELL_EDGE_LEFT, config.margins.left);
  gtk_layer_set_margin(gtk_window, GTK_LAYER_SHELL_EDGE_RIGHT, config.margins.right);
  gtk_layer_set_margin(gtk_window, GTK_LAYER_SHELL_EDGE_TOP, config.margins.top);
  gtk_layer_set_margin(gtk_window, GTK_LAYER_SHELL_EDGE_BOTTOM, config.margins.bottom);

  window.set_size_request(width_, height_);

  // Position needs to be set after calculating the height due to the
  // GTK layer shell anchors logic relying on the dimensions of the bar.
  onPositionChange(inst.position());
  inst.signal_position.connect(sigc::mem_fun(*this, &Bar::onPositionChange));

  onModeChange(inst.mode());
  inst.signal_mode.connect(sigc::mem_fun(*this, &Bar::onModeChange));

  window.signal_map_event().connect_notify(sigc::mem_fun(*this, &Bar::onMap));

  setupWidgets();
  window.show_all();

  if (spdlog::should_log(spdlog::level::debug)) {
    // Unfortunately, this function isn't in the C++ bindings, so we have to call the C version.
    char* gtk_tree = gtk_style_context_to_string(
        window.get_style_context()->gobj(),
        (GtkStyleContextPrintFlags)(GTK_STYLE_CONTEXT_PRINT_RECURSE |
                                    GTK_STYLE_CONTEXT_PRINT_SHOW_STYLE));
    spdlog::debug("GTK widget tree:\n{}", gtk_tree);
    g_free(gtk_tree);
  }
}

/* Need to define it here because of forward declared members */
waybar::Bar::~Bar() = default;

void waybar::Bar::onModeChange(const std::string& mode) {
  using namespace std::literals::string_literals;

  auto style = window.get_style_context();
  /* remove styles added by previous setMode calls */
  style->remove_class("mode-"s + last_mode_);

  auto it = config.modes.find(mode);
  if (it != config.modes.end()) {
    last_mode_ = mode;
    style->add_class("mode-"s + last_mode_);
    setMode(it->second);
  } else {
    spdlog::warn("Unknown mode \"{}\" requested", mode);
    last_mode_ = BarConfig::MODE_DEFAULT;
    style->add_class("mode-"s + last_mode_);
    setMode(config.modes.at(BarConfig::MODE_DEFAULT));
  }
}

void waybar::Bar::setMode(const struct BarMode& mode) {
  auto* gtk_window = window.gobj();

  if (mode.layer == BarLayer::BOTTOM) {
    gtk_layer_set_layer(gtk_window, GTK_LAYER_SHELL_LAYER_BOTTOM);
  } else if (mode.layer == BarLayer::TOP) {
    gtk_layer_set_layer(gtk_window, GTK_LAYER_SHELL_LAYER_TOP);
  } else if (mode.layer == BarLayer::OVERLAY) {
    gtk_layer_set_layer(gtk_window, GTK_LAYER_SHELL_LAYER_OVERLAY);
  }

  if (mode.exclusive) {
    gtk_layer_auto_exclusive_zone_enable(gtk_window);
  } else {
    gtk_layer_set_exclusive_zone(gtk_window, 0);
  }

  setPassThrough(passthrough_ = mode.passthrough);

  if (mode.visible) {
    window.get_style_context()->remove_class("hidden");
    window.set_opacity(1);
  } else {
    window.get_style_context()->add_class("hidden");
    window.set_opacity(0);
  }
}

void waybar::Bar::setPassThrough(bool passthrough) {
  auto gdk_window = window.get_window();
  if (gdk_window) {
    Cairo::RefPtr<Cairo::Region> region;
    if (passthrough) {
      region = Cairo::Region::create();
    }
    gdk_window->input_shape_combine_region(region, 0, 0);
  }
}

void waybar::Bar::onPositionChange(Gtk::PositionType position) {
  std::array<gboolean, GTK_LAYER_SHELL_EDGE_ENTRY_NUMBER> anchors;
  anchors.fill(TRUE);

  switch (position) {
    case Gtk::POS_LEFT:
      anchors[GTK_LAYER_SHELL_EDGE_RIGHT] = FALSE;
      break;
    case Gtk::POS_RIGHT:
      anchors[GTK_LAYER_SHELL_EDGE_LEFT] = FALSE;
      break;
    case Gtk::POS_BOTTOM:
      anchors[GTK_LAYER_SHELL_EDGE_TOP] = FALSE;
      break;
    default: /* Gtk::POS_TOP */
      anchors[GTK_LAYER_SHELL_EDGE_BOTTOM] = FALSE;
      break;
  };
  // Disable anchoring for other edges too if the width
  // or the height has been set to a value other than 'auto'
  // otherwise the bar will use all space
  auto orientation = ToOrientation(position);
  if (orientation == Gtk::ORIENTATION_VERTICAL && config.height > 1) {
    anchors[GTK_LAYER_SHELL_EDGE_TOP] = FALSE;
    anchors[GTK_LAYER_SHELL_EDGE_BOTTOM] = FALSE;
  } else if (orientation == Gtk::ORIENTATION_HORIZONTAL && config.width > 1) {
    anchors[GTK_LAYER_SHELL_EDGE_LEFT] = FALSE;
    anchors[GTK_LAYER_SHELL_EDGE_RIGHT] = FALSE;
  }

  auto style = window.get_style_context();
  style->remove_class(ToStr(last_position_));
  last_position_ = position;
  style->add_class(ToStr(last_position_));

  for (auto edge : {GTK_LAYER_SHELL_EDGE_LEFT, GTK_LAYER_SHELL_EDGE_RIGHT, GTK_LAYER_SHELL_EDGE_TOP,
                    GTK_LAYER_SHELL_EDGE_BOTTOM}) {
    gtk_layer_set_anchor(window.gobj(), edge, anchors[edge]);
  }
}

void waybar::Bar::onMap(GdkEventAny* /*unused*/) {
  /*
   * Obtain a pointer to the custom layer surface for modules that require it (idle_inhibitor).
   */
  auto* gdk_window = window.get_window()->gobj();
  surface = gdk_wayland_window_get_wl_surface(gdk_window);
  configureGlobalOffset(gdk_window_get_width(gdk_window), gdk_window_get_height(gdk_window));

  setPassThrough(passthrough_);
}

void waybar::Bar::handleSignal(int signal) {
  for (auto& module : modules_all_) {
    module->refresh(signal);
  }
}

void waybar::Bar::getModules(const Factory& factory, const std::string& pos,
                             waybar::Group* group = nullptr) {
  for (const auto& name : config.getModuleList(pos)) {
    try {
      auto ref = name.asString();
      AModule* module;

      if (ref.compare(0, 6, "group/") == 0 && ref.size() > 6) {
        auto hash_pos = ref.find('#');
        auto id_name = ref.substr(6, hash_pos - 6);
        auto class_name = hash_pos != std::string::npos ? ref.substr(hash_pos + 1) : "";

        auto vertical = (group != nullptr ? group->getBox().get_orientation()
                                          : box_.get_orientation()) == Gtk::ORIENTATION_VERTICAL;

        auto* group_module = new waybar::Group(id_name, class_name, config.json()[ref], vertical);
        getModules(factory, ref, group_module);
        module = group_module;
      } else {
        module = factory.makeModule(ref, pos);
      }

      std::shared_ptr<AModule> module_sp(module);
      modules_all_.emplace_back(module_sp);
      if (group != nullptr) {
        group->addWidget(*module);
      } else {
        if (pos == "modules-left") {
          modules_left_.emplace_back(module_sp);
        }
        if (pos == "modules-center") {
          modules_center_.emplace_back(module_sp);
        }
        if (pos == "modules-right") {
          modules_right_.emplace_back(module_sp);
        }
      }
      module->dp.connect([module, ref] {
        try {
          module->update();
        } catch (const std::exception& e) {
          spdlog::error("{}: {}", ref, e.what());
        }
      });
    } catch (const std::exception& e) {
      spdlog::warn("module {}: {}", name.asString(), e.what());
    }
  }
}

auto waybar::Bar::setupWidgets() -> void {
  window.add(box_);
  box_.pack_start(left_, false, false);
  if (config.fixed_center) {
    box_.set_center_widget(center_);
  } else {
    box_.pack_start(center_, true, false);
  }
  box_.pack_end(right_, false, false);

  Factory factory(*this);
  getModules(factory, "modules-left");
  getModules(factory, "modules-center");
  getModules(factory, "modules-right");
  for (auto const& module : modules_left_) {
    left_.pack_start(*module, false, false);
  }
  for (auto const& module : modules_center_) {
    center_.pack_start(*module, false, false);
  }
  std::reverse(modules_right_.begin(), modules_right_.end());
  for (auto const& module : modules_right_) {
    right_.pack_end(*module, false, false);
  }
}

void waybar::Bar::onConfigure(GdkEventConfigure* ev) {
  /*
   * GTK wants new size for the window.
   * Actual resizing and management of the exclusve zone is handled within the gtk-layer-shell
   * code. This event handler only updates stored size of the window and prints some warnings.
   *
   * Note: forced resizing to a window smaller than required by GTK would not work with
   * gtk-layer-shell.
   */
  if (orientation == Gtk::ORIENTATION_VERTICAL) {
    if (width_ > 1 && ev->width > static_cast<int>(width_)) {
      spdlog::warn(MIN_WIDTH_MSG, width_, ev->width);
    }
  } else {
    if (height_ > 1 && ev->height > static_cast<int>(height_)) {
      spdlog::warn(MIN_HEIGHT_MSG, height_, ev->height);
    }
  }
  width_ = ev->width;
  height_ = ev->height;

  configureGlobalOffset(ev->width, ev->height);
  spdlog::info(BAR_SIZE_MSG, ev->width, ev->height, output->name);
}

void waybar::Bar::configureGlobalOffset(int width, int height) {
  auto monitor_geometry = *output->monitor->property_geometry().get_value().gobj();
  const auto& margins = config.margins;
  int x;
  int y;
  switch (position) {
    case Gtk::POS_BOTTOM:
      if (width + margins.left + margins.right >= monitor_geometry.width)
        x = margins.left;
      else
        x = (monitor_geometry.width - width) / 2;
      y = monitor_geometry.height - height - margins.bottom;
      break;
    case Gtk::POS_LEFT:
      x = margins.left;
      if (height + margins.top + margins.bottom >= monitor_geometry.height)
        y = margins.top;
      else
        y = (monitor_geometry.height - height) / 2;
      break;
    case Gtk::POS_RIGHT:
      x = monitor_geometry.width - width - margins.right;
      if (height + margins.top + margins.bottom >= monitor_geometry.height)
        y = margins.top;
      else
        y = (monitor_geometry.height - height) / 2;
      break;
    default: /* Gtk::POS_TOP */
      if (width + margins.left + margins.right >= monitor_geometry.width)
        x = margins.left;
      else
        x = (monitor_geometry.width - width) / 2;
      y = margins.top;
      break;
  }

  x_global = x + monitor_geometry.x;
  y_global = y + monitor_geometry.y;
}

void waybar::Bar::onOutputGeometryChanged() {
  configureGlobalOffset(window.get_width(), window.get_height());
}
