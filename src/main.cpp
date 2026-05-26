#include "src/gui.hpp"

#include <hidapi/hidapi.h>

#include <cstdlib>
#include <iostream>

int main(int argc, char** argv) {
    if (hid_init() != 0) {
        std::cerr << "hid_init failed\n";
        return 1;
    }

    auto app = Gtk::Application::create(argc, argv, "org.rgb-controller.gui");

    rgb_controller::RgbControllerGui window;

    if (!window.is_hid_ready()) {
        // The error dialog has already been shown.
        // Exit after the user closes the window.
        int ret = app->run(window);
        hid_exit();
        return ret;
    }

    int ret = app->run(window);
    hid_exit();
    return ret;
}
