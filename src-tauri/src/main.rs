// Hide the console window on release builds.
#![cfg_attr(not(debug_assertions), windows_subsystem = "windows")]

fn main() {
    autoclicker_lib::run()
}
