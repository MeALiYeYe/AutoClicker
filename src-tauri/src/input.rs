//! input.rs — Synthesize mouse input via the Win32 `SendInput` API.

use crate::settings::{ClickType, MouseButton};
use windows::Win32::Foundation::POINT;
use windows::Win32::UI::Input::KeyboardAndMouse::{
    GetCursorPos, SendInput, INPUT, INPUT_0, INPUT_MOUSE, MOUSEINPUT, MOUSEEVENTF_ABSOLUTE,
    MOUSEEVENTF_LEFTDOWN, MOUSEEVENTF_LEFTUP, MOUSEEVENTF_MIDDLEDOWN, MOUSEEVENTF_MIDDLEUP,
    MOUSEEVENTF_MOVE, MOUSEEVENTF_RIGHTDOWN, MOUSEEVENTF_RIGHTUP, MOUSEEVENTF_WHEEL,
};
use windows::Win32::UI::WindowsAndMessaging::{GetSystemMetrics, SM_CXSCREEN, SM_CYSCREEN};

/// One wheel notch, as defined by Win32.
const WHEEL_DELTA: i32 = 120;

fn screen_size() -> (i32, i32) {
    unsafe {
        let w = GetSystemMetrics(SM_CXSCREEN);
        let h = GetSystemMetrics(SM_CYSCREEN);
        (if w < 1 { 1920 } else { w }, if h < 1 { 1080 } else { h })
    }
}

/// `SendInput` expects absolute coordinates normalized to 0..65535.
fn to_absolute(x: i32, y: i32) -> (i32, i32) {
    let (sw, sh) = screen_size();
    ((x * 65535) / sw, (y * 65535) / sh)
}

unsafe fn send_mouse(dx: i32, dy: i32, data: i32, flags: windows::Win32::UI::Input::KeyboardAndMouse::MOUSE_EVENT_FLAGS) {
    let input = INPUT {
        r#type: INPUT_MOUSE,
        Anonymous: INPUT_0 {
            mi: MOUSEINPUT {
                dx,
                dy,
                mouseData: data,
                dwFlags: flags,
                time: 0,
                dwExtraInfo: 0,
            },
        },
    };
    SendInput(&[input], std::mem::size_of::<INPUT>() as i32);
}

/// Move the cursor to an absolute screen position.
pub fn move_to(x: i32, y: i32) {
    unsafe {
        let (ax, ay) = to_absolute(x, y);
        send_mouse(ax, ay, 0, MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE);
    }
}

unsafe fn press(button: MouseButton) {
    match button {
        MouseButton::Left => send_mouse(0, 0, 0, MOUSEEVENTF_LEFTDOWN),
        MouseButton::Right => send_mouse(0, 0, 0, MOUSEEVENTF_RIGHTDOWN),
        MouseButton::Middle => send_mouse(0, 0, 0, MOUSEEVENTF_MIDDLEDOWN),
        _ => {}
    }
}

unsafe fn release(button: MouseButton) {
    match button {
        MouseButton::Left => send_mouse(0, 0, 0, MOUSEEVENTF_LEFTUP),
        MouseButton::Right => send_mouse(0, 0, 0, MOUSEEVENTF_RIGHTUP),
        MouseButton::Middle => send_mouse(0, 0, 0, MOUSEEVENTF_MIDDLEUP),
        _ => {}
    }
}

/// Click at the given screen coordinate.
pub fn click_at(x: i32, y: i32, button: MouseButton, kind: ClickType) {
    unsafe {
        move_to(x, y);
        // Give the system a moment to process the move before clicking.
        std::thread::sleep(std::time::Duration::from_millis(2));

        if matches!(button, MouseButton::WheelUp | MouseButton::WheelDown) {
            let delta = if button == MouseButton::WheelUp {
                WHEEL_DELTA
            } else {
                -WHEEL_DELTA
            };
            send_mouse(0, 0, delta, MOUSEEVENTF_WHEEL);
            return;
        }

        let times = if kind == ClickType::Double { 2 } else { 1 };
        for _ in 0..times {
            press(button);
            std::thread::sleep(std::time::Duration::from_millis(5));
            release(button);
            if times == 2 {
                std::thread::sleep(std::time::Duration::from_millis(20));
            }
        }
    }
}

/// Click at the cursor's current position without moving it.
pub fn click_current(button: MouseButton, kind: ClickType) {
    let (x, y) = cursor_pos();
    unsafe {
        if matches!(button, MouseButton::WheelUp | MouseButton::WheelDown) {
            let delta = if button == MouseButton::WheelUp {
                WHEEL_DELTA
            } else {
                -WHEEL_DELTA
            };
            send_mouse(0, 0, delta, MOUSEEVENTF_WHEEL);
            return;
        }
        let times = if kind == ClickType::Double { 2 } else { 1 };
        for _ in 0..times {
            press(button);
            std::thread::sleep(std::time::Duration::from_millis(5));
            release(button);
            if times == 2 {
                std::thread::sleep(std::time::Duration::from_millis(20));
            }
        }
    }
    let _ = (x, y);
}

/// Current cursor position in screen pixels.
pub fn cursor_pos() -> (i32, i32) {
    unsafe {
        let mut p = POINT::default();
        // Ignore the return value: `POINT` stays zeroed on failure, and the
        // windows-rs binding shape (BOOL vs Result) varies by crate version.
        let _ = GetCursorPos(&mut p);
        (p.x, p.y)
    }
}
