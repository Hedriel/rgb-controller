#include "src/gui.hpp"

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sys/stat.h>
#include <sys/types.h>

namespace rgb_controller {

static void show_all_recursive(GtkWidget* widget) {
    if (!widget) return;
    gtk_widget_show(widget);
    gtk_widget_set_no_show_all(widget, FALSE);
    if (GTK_IS_CONTAINER(widget)) {
        GList* children = gtk_container_get_children(GTK_CONTAINER(widget));
        for (GList* l = children; l; l = l->next) {
            show_all_recursive(GTK_WIDGET(l->data));
        }
        g_list_free(children);
    }
}

static void hide_editor_extras(GtkWidget* widget) {
    if (!widget || !GTK_IS_CONTAINER(widget)) return;
    GList* children = gtk_container_get_children(GTK_CONTAINER(widget));
    bool first_color_scale_kept = false;
    for (GList* l = children; l; l = l->next) {
        GtkWidget* child = GTK_WIDGET(l->data);
        const gchar* type_name = G_OBJECT_TYPE_NAME(child);
        if (type_name && g_str_equal(type_name, "GtkColorScale")) {
            if (!first_color_scale_kept) {
                gtk_widget_hide(child);
                first_color_scale_kept = true;
            }
        }
        if (type_name && (g_str_equal(type_name, "GtkEntry") ||
                          g_str_equal(type_name, "GtkColorSwatch") ||
                          g_str_equal(type_name, "GtkButton"))) {
            gtk_widget_hide(child);
        }
        if (GTK_IS_BOX(child)) {
            GList* grandchildren = gtk_container_get_children(GTK_CONTAINER(child));
            bool has_spinbutton = false;
            for (GList* g = grandchildren; g; g = g->next) {
                if (GTK_IS_GRID(g->data)) {
                    GList* grid_children = gtk_container_get_children(GTK_CONTAINER(g->data));
                    for (GList* gc = grid_children; gc; gc = gc->next) {
                        if (GTK_IS_SPIN_BUTTON(gc->data)) {
                            has_spinbutton = true;
                            break;
                        }
                    }
                    g_list_free(grid_children);
                }
            }
            g_list_free(grandchildren);
            if (has_spinbutton) {
                gtk_widget_hide(child);
            }
        }
        hide_editor_extras(child);
    }
    g_list_free(children);
}

static void show_editor_only(GtkWidget* widget) {
    if (!widget || !GTK_IS_CONTAINER(widget)) return;
    GList* children = gtk_container_get_children(GTK_CONTAINER(widget));
    int i = 0;
    for (GList* l = children; l; l = l->next, i++) {
        if (i == 0 && GTK_IS_BOX(l->data)) {
            gtk_widget_hide(GTK_WIDGET(l->data));
        }
        if (i == 1 && GTK_IS_BOX(l->data)) {
            show_all_recursive(GTK_WIDGET(l->data));
            hide_editor_extras(GTK_WIDGET(l->data));
        }
    }
    g_list_free(children);
}

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
    set_default_size(480, 400);
    set_border_width(12);

    current_rgba_.set("#FFFFFF");
    confirmed_rgba_ = current_rgba_;

    load_config();

    build_ui();
    connect_signals();

    try {
        hid_ = std::make_unique<RgbControllerHid>(0x1A86, 0xFE07, 255);
        update_status("Device connected — Ready");
        
        current_rgba_ = confirmed_rgba_;
        send_static_frame();
    } catch (const std::exception& e) {
        update_status("Device error: " + std::string(e.what()));
        Gtk::MessageDialog dialog(*this,
            "Failed to open RGB Controller HID device:\n\n" + std::string(e.what()) +
            "\n\nCheck USB connection and udev rules.",
            false, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_CLOSE);
        dialog.set_title("Device Error");
        dialog.run();
    }

    show_all_children();

    Glib::signal_idle().connect_once([this]() {
        show_editor_only(color_chooser_widget_->gobj());
    });
}

RgbControllerGui::~RgbControllerGui() {
    if (hid_) {
        send_static_frame();
    }
}

void RgbControllerGui::build_ui() {
    add(main_box_);
    main_box_.set_margin_start(4);
    main_box_.set_margin_end(4);

    color_chooser_widget_ = Glib::wrap(gtk_color_chooser_widget_new());
    color_chooser_widget_->set_hexpand(true);
    color_chooser_widget_->set_vexpand(true);
    
    GtkColorChooser* chooser = GTK_COLOR_CHOOSER(color_chooser_widget_->gobj());
    gtk_color_chooser_set_use_alpha(chooser, FALSE);
    
    GdkRGBA c_rgba;
    c_rgba.red = confirmed_rgba_.get_red();
    c_rgba.green = confirmed_rgba_.get_green();
    c_rgba.blue = confirmed_rgba_.get_blue();
    c_rgba.alpha = 1.0;
    gtk_color_chooser_set_rgba(chooser, &c_rgba);
    
    main_box_.pack_start(*color_chooser_widget_, Gtk::PACK_EXPAND_WIDGET);

    brightness_adj_ = Gtk::Adjustment::create(255.0, 0.0, 255.0, 1.0, 10.0, 0.0);
    brightness_scale_.set_adjustment(brightness_adj_);
    brightness_scale_.set_digits(0);
    brightness_scale_.set_value_pos(Gtk::POS_RIGHT);
    brightness_scale_.set_hexpand(true);
    
    auto* bright_label = Gtk::make_managed<Gtk::Label>("Brightness:");
    bright_label->set_halign(Gtk::ALIGN_START);
    
    auto* brightness_box = Gtk::make_managed<Gtk::Box>(Gtk::ORIENTATION_HORIZONTAL, 10);
    brightness_box->pack_start(*bright_label, Gtk::PACK_SHRINK);
    brightness_box->pack_start(brightness_scale_, Gtk::PACK_EXPAND_WIDGET);
    main_box_.pack_start(*brightness_box, Gtk::PACK_SHRINK);

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
    GtkColorChooser* chooser = GTK_COLOR_CHOOSER(color_chooser_widget_->gobj());
    g_signal_connect_swapped(chooser, "notify::rgba",
        G_CALLBACK(+[](RgbControllerGui* self) {
            self->on_color_changed();
        }), this);

    brightness_adj_->signal_value_changed().connect(
        sigc::mem_fun(*this, &RgbControllerGui::on_brightness_changed));
}

void RgbControllerGui::on_color_changed() {
    if (!color_chooser_widget_) return;
    
    GtkColorChooser* chooser = GTK_COLOR_CHOOSER(color_chooser_widget_->gobj());
    GdkRGBA c_rgba;
    gtk_color_chooser_get_rgba(chooser, &c_rgba);
    current_rgba_.set_rgba(c_rgba.red, c_rgba.green, c_rgba.blue, 1.0);
    confirmed_rgba_ = current_rgba_;
    save_config();
    send_static_frame();
}

void RgbControllerGui::on_brightness_changed() {
    send_static_frame();
}

void RgbControllerGui::send_static_frame() {
    if (!hid_) return;

    uint8_t brightness = static_cast<uint8_t>(brightness_adj_->get_value());
    double scale = brightness / 255.0;
    
    uint8_t r = static_cast<uint8_t>(current_rgba_.get_red()   * 255.0 * scale + 0.5);
    uint8_t g = static_cast<uint8_t>(current_rgba_.get_green() * 255.0 * scale + 0.5);
    uint8_t b = static_cast<uint8_t>(current_rgba_.get_blue()  * 255.0 * scale + 0.5);

    LedFrame frame{};
    frame.fill(ColorRgb{r, g, b});
    send_frame_safe(frame);
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

} // namespace rgb_controller
