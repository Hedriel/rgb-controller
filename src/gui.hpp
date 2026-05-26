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

    void on_static_param_changed();
    void send_static_frame();
    bool on_static_refresh_tick();
    void on_color_pick();
    void update_color_swatch();

    void on_test_leds();
    bool on_test_leds_tick();

    void update_status(const std::string& msg);

    void save_config();
    void load_config();
    std::string config_path() const;

    bool send_frame_safe(const LedFrame& frame);
    bool send_brightness_safe(uint8_t brightness);

    Gtk::Box main_box_{Gtk::ORIENTATION_VERTICAL, 10};

    Gtk::Frame static_frame_;
    Gtk::Grid static_grid_;
    Gtk::Button color_pick_button_;
    Gtk::Scale brightness_scale_;
    Glib::RefPtr<Gtk::Adjustment> brightness_adj_;
    Gtk::Button test_leds_button_;

    Gtk::Label status_label_;
    Gtk::Label status_text_;
    Gtk::Box status_box_{Gtk::ORIENTATION_HORIZONTAL, 6};

    std::unique_ptr<RgbControllerHid> hid_;
    std::mutex hid_mutex_;

    sigc::connection static_refresh_conn_;
    sigc::connection test_tick_conn_;
    int test_led_index_{0};

    Gdk::RGBA current_rgba_;
    Gdk::RGBA confirmed_rgba_;
};

} // namespace rgb_controller
