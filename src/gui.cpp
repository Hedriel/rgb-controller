#include "src/gui.hpp"

#include <chrono>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sys/stat.h>
#include <sys/types.h>

namespace rgb_controller {

std::string RgbControllerGui::config_path() const {
    const char* xdg = std::getenv("XDG_CONFIG_HOME");
    std::string base = xdg ? std::string(xdg) : std::string(std::getenv("HOME")) + "/.config";
    return base + "/rgb-controller/config.ini";
}

void RgbControllerGui::load_config() {
    std::string path = config_path();
    std::ifstream file(path);
    if (!file.is_open()) {
        return;
    }

    std::string line;
    while (std::getline(file, line)) {
        auto start = line.find_first_not_of(" \t");
        if (start == std::string::npos) continue;
        if (line[start] == '#') continue;

        auto eq = line.find('=', start);
        if (eq == std::string::npos) continue;

        std::string key = line.substr(start, eq - start);
        while (!key.empty() && (key.back() == ' ' || key.back() == '\t')) {
            key.pop_back();
        }

        if (key != "static_color") continue;

        std::string value = line.substr(eq + 1);
        auto vs = value.find_first_not_of(" \t");
        auto ve = value.find_last_not_of(" \t");
        if (vs != std::string::npos && ve != std::string::npos) {
            value = value.substr(vs, ve - vs + 1);
        }

        if (value.length() == 6) {
            try {
                current_rgba_.set("#" + value);
                confirmed_rgba_ = current_rgba_;
            } catch (...) {
            }
        }
        break;
    }
}

void RgbControllerGui::save_config() {
    std::string path = config_path();

    auto slash = path.rfind('/');
    if (slash != std::string::npos) {
        std::string dir = path.substr(0, slash);
        mkdir(dir.c_str(), 0755);
    }

    std::vector<std::string> lines;
    {
        std::ifstream infile(path);
        std::string line;
        while (std::getline(infile, line)) {
            lines.push_back(line);
        }
    }

    char hexbuf[8];
    std::snprintf(hexbuf, sizeof(hexbuf), "%02X%02X%02X",
                  static_cast<int>(confirmed_rgba_.get_red()   * 255.0 + 0.5),
                  static_cast<int>(confirmed_rgba_.get_green() * 255.0 + 0.5),
                  static_cast<int>(confirmed_rgba_.get_blue()  * 255.0 + 0.5));
    std::string color_line = std::string("static_color=") + hexbuf;

    bool found = false;
    for (auto& l : lines) {
        auto start = l.find_first_not_of(" \t");
        if (start != std::string::npos &&
            l.substr(start, 12) == "static_color" &&
            (l.size() <= start + 12 || l[start + 12] == '=' || l[start + 12] == ' ')) {
            l = color_line;
            found = true;
            break;
        }
    }
    if (!found) {
        lines.push_back(color_line);
    }

    std::ofstream outfile(path);
    for (const auto& l : lines) {
        outfile << l << "\n";
    }
}

RgbControllerGui::RgbControllerGui() {
    set_title("rgb-controller");
    set_default_size(480, 300);
    set_border_width(12);

    current_rgba_.set("#FFFFFF");
    confirmed_rgba_ = current_rgba_;

    load_config();

    build_ui();
    connect_signals();

    update_color_swatch();

    try {
        hid_ = std::make_unique<RgbControllerHid>(0x1A86, 0xFE07, 255);
        update_status("Device connected — Ready");
    } catch (const std::exception& e) {
        update_status("Device error: " + std::string(e.what()));
        Gtk::MessageDialog dialog(*this,
            "Failed to open RGB Controller HID device:\n\n" + std::string(e.what()) +
            "\n\nCheck USB connection and udev rules.",
            false, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_CLOSE);
        dialog.set_title("Device Error");
        dialog.run();
    }

    send_static_frame();
    show_all_children();
}

RgbControllerGui::~RgbControllerGui() {
}

void RgbControllerGui::build_ui() {
    add(main_box_);
    main_box_.set_margin_start(4);
    main_box_.set_margin_end(4);

    static_frame_.set_label("Color");
    static_grid_.set_row_spacing(8);
    static_grid_.set_column_spacing(10);
    static_grid_.set_margin_start(10);
    static_grid_.set_margin_end(10);
    static_grid_.set_margin_top(6);
    static_grid_.set_margin_bottom(8);

    auto* color_label = Gtk::make_managed<Gtk::Label>("Pick:");
    color_label->set_halign(Gtk::ALIGN_START);
    static_grid_.attach(*color_label, 0, 0, 1, 1);

    color_pick_button_.set_size_request(56, 36);
    static_grid_.attach(color_pick_button_, 1, 0, 1, 1);

    auto* bright_label_w = Gtk::make_managed<Gtk::Label>("Brightness:");
    bright_label_w->set_halign(Gtk::ALIGN_START);
    static_grid_.attach(*bright_label_w, 0, 1, 1, 1);

    brightness_adj_ = Gtk::Adjustment::create(255.0, 0.0, 255.0, 1.0, 10.0, 0.0);
    brightness_scale_.set_adjustment(brightness_adj_);
    brightness_scale_.set_digits(0);
    brightness_scale_.set_value_pos(Gtk::POS_RIGHT);
    brightness_scale_.set_hexpand(true);
    static_grid_.attach(brightness_scale_, 1, 1, 1, 1);

    test_leds_button_.set_label("Test LEDs");
    static_grid_.attach(test_leds_button_, 0, 2, 2, 1);

    static_frame_.add(static_grid_);
    main_box_.pack_start(static_frame_, Gtk::PACK_SHRINK);

    status_label_.set_markup("<b>Status:</b>");
    status_text_.set_text("Initializing…");
    status_text_.set_halign(Gtk::ALIGN_START);
    status_text_.set_hexpand(true);
    status_text_.set_ellipsize(Pango::ELLIPSIZE_END);
    status_box_.pack_start(status_label_, Gtk::PACK_SHRINK, 0);
    status_box_.pack_start(status_text_, Gtk::PACK_EXPAND_WIDGET, 0);
    main_box_.pack_start(status_box_, Gtk::PACK_SHRINK);
}

void RgbControllerGui::connect_signals() {
    color_pick_button_.signal_clicked().connect(
        sigc::mem_fun(*this, &RgbControllerGui::on_color_pick));

    brightness_adj_->signal_value_changed().connect([this]() {
        on_static_param_changed();
    });

    test_leds_button_.signal_clicked().connect(
        sigc::mem_fun(*this, &RgbControllerGui::on_test_leds));
}

void RgbControllerGui::on_color_pick() {
    Gdk::RGBA before = confirmed_rgba_;

    Gtk::ColorChooserDialog dialog("Pick Color");
    dialog.set_transient_for(*this);
    dialog.set_modal(true);
    dialog.set_rgba(before);

    dialog.property_rgba().signal_changed().connect(
        [this, &dialog]() {
            current_rgba_ = dialog.get_rgba();
            update_color_swatch();
            send_static_frame();
        });

    int result = dialog.run();
    dialog.hide();

    if (result == Gtk::RESPONSE_OK) {
        confirmed_rgba_ = dialog.get_rgba();
        current_rgba_   = confirmed_rgba_;
        save_config();
    } else {
        current_rgba_ = before;
    }

    update_color_swatch();
    send_static_frame();
}

void RgbControllerGui::update_color_swatch() {
    auto ctx = color_pick_button_.get_style_context();
    static Glib::RefPtr<Gtk::CssProvider> s_provider;
    if (!s_provider) {
        s_provider = Gtk::CssProvider::create();
        ctx->add_provider(s_provider, GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    }

    char css[128];
    std::snprintf(css, sizeof(css),
                  "* { background-color: rgb(%d,%d,%d); }",
                  static_cast<int>(current_rgba_.get_red()   * 255.0 + 0.5),
                  static_cast<int>(current_rgba_.get_green() * 255.0 + 0.5),
                  static_cast<int>(current_rgba_.get_blue()  * 255.0 + 0.5));
    s_provider->load_from_data(css);
}

void RgbControllerGui::on_static_param_changed() {
    auto b = static_cast<uint8_t>(brightness_adj_->get_value());
    send_brightness_safe(b);
    send_static_frame();
}

void RgbControllerGui::send_static_frame() {
    if (!hid_) return;

    uint8_t r = static_cast<uint8_t>(current_rgba_.get_red()   * 255.0 + 0.5);
    uint8_t g = static_cast<uint8_t>(current_rgba_.get_green() * 255.0 + 0.5);
    uint8_t b = static_cast<uint8_t>(current_rgba_.get_blue()  * 255.0 + 0.5);

    LedFrame frame{};
    frame.fill(ColorRgb{r, g, b});
    send_frame_safe(frame);
}

bool RgbControllerGui::on_static_refresh_tick() {
    if (test_tick_conn_.connected()) {
        return true;
    }
    send_static_frame();
    return true;
}

void RgbControllerGui::on_test_leds() {
    if (!hid_) return;

    test_tick_conn_.disconnect();

    test_led_index_ = 0;
    update_status("LED test running — watch the strip (LED 1/63)");

    test_tick_conn_ = Glib::signal_timeout().connect(
        sigc::mem_fun(*this, &RgbControllerGui::on_test_leds_tick),
        400);
}

bool RgbControllerGui::on_test_leds_tick() {
    if (test_led_index_ >= static_cast<int>(kLedCount)) {
        test_tick_conn_.disconnect();
        send_static_frame();
        update_status("LED test finished — 63 LEDs checked");
        return false;
    }

    LedFrame frame{};
    frame[static_cast<std::size_t>(test_led_index_)] = ColorRgb{255, 0, 0};
    send_frame_safe(frame);

    char buf[64];
    std::snprintf(buf, sizeof(buf), "LED test: LED %d / %zu  (watch the strip)",
                  test_led_index_ + 1, kLedCount);
    update_status(buf);

    ++test_led_index_;
    return true;
}

void RgbControllerGui::update_status(const std::string& msg) {
    auto msg_copy = msg;
    Glib::signal_idle().connect_once([this, msg_copy]() {
        status_text_.set_text(msg_copy);
    });
}

bool RgbControllerGui::send_frame_safe(const LedFrame& frame) {
    std::lock_guard<std::mutex> lock(hid_mutex_);
    if (!hid_) return false;
    return hid_->sendFrame(frame);
}

bool RgbControllerGui::send_brightness_safe(uint8_t brightness) {
    std::lock_guard<std::mutex> lock(hid_mutex_);
    if (!hid_) return false;
    return hid_->setBrightness(brightness);
}

} // namespace rgb_controller
