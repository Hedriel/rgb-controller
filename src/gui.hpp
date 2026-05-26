#pragma once

#include "src/rgb_controller_hid.hpp"
#include "src/rgb_controller_layout.hpp"

#include <memory>
#include <mutex>
#include <string>

#include <gtkmm.h>

namespace rgb_controller {

class RgbControllerGui : public Gtk::Window {
public:
    RgbControllerGui();
    ~RgbControllerGui() override;

    bool is_hid_ready() const { return hid_ != nullptr; }

private:
    void build_ui();
    void connect_signals();

    void send_static_frame();
    void on_color_changed();
    void on_brightness_changed();

    void update_status(const std::string& msg);

    void save_config();
    void load_config();
    std::string config_path() const;

    bool send_frame_safe(const LedFrame& frame);

    Gtk::Box main_box_{Gtk::ORIENTATION_VERTICAL, 10};

    Gtk::Widget* color_chooser_widget_{nullptr};
    Gtk::Scale brightness_scale_;
    Glib::RefPtr<Gtk::Adjustment> brightness_adj_;

    Gtk::Label status_label_;
    Gtk::Label status_text_;
    Gtk::Box status_box_{Gtk::ORIENTATION_HORIZONTAL, 6};

    std::unique_ptr<RgbControllerHid> hid_;
    std::mutex hid_mutex_;

    Gdk::RGBA current_rgba_;
    Gdk::RGBA confirmed_rgba_;
};

} // namespace rgb_controller
