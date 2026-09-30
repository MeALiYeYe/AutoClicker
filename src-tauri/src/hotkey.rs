//! hotkey.rs — Global hotkey registration.
//!
//! Uses a message-only window plus `RegisterHotKey`. A dedicated thread runs
//! the message pump and also listens for hotkey-change requests, so the hotkey
//! can be rebound at runtime without restarting the app.

use std::sync::atomic::{AtomicU32, Ordering};
use std::sync::mpsc::{channel, Receiver, Sender};
use std::sync::Arc;
use std::time::Duration;
use windows::core::w;
use windows::Win32::Foundation::{HINSTANCE, HMODULE, HWND, LPARAM, LRESULT, WPARAM};
use windows::Win32::System::LibraryLoader::GetModuleHandleW;
use windows::Win32::UI::Input::KeyboardAndMouse::{RegisterHotKey, UnregisterHotKey, MOD_NOREPEAT};
use windows::Win32::UI::WindowsAndMessaging::{
    CreateWindowExW, DefWindowProcW, DispatchMessageW, PeekMessageW, RegisterClassW,
    TranslateMessage, HWND_MESSAGE, MSG, PM_REMOVE, WINDOW_EX_STYLE, WINDOW_STYLE, WM_HOTKEY,
    WNDCLASSW,
};

const CLASS_NAME: &windows::core::PCWSTR = w!("AutoClickerHotkeyWindow");
const HOTKEY_ID: i32 = 1;

/// Default hotkey: F6.
pub const DEFAULT_VK: u32 = 0x75;

type Trigger = Arc<dyn Fn() + Send + Sync>;

/// Handle for controlling the global hotkey.
pub struct HotkeyManager {
    tx: Sender<u32>,
    current: Arc<AtomicU32>,
}

impl HotkeyManager {
    /// Spawn the hotkey thread and register `initial_vk`.
    pub fn new(initial_vk: u32, on_trigger: Trigger) -> Self {
        let (tx, rx) = channel::<u32>();
        let current = Arc::new(AtomicU32::new(initial_vk));
        let current2 = current.clone();
        std::thread::spawn(move || hotkey_thread(initial_vk, rx, current2, on_trigger));
        Self { tx, current }
    }

    /// Rebind the hotkey. Takes effect on the next pump iteration.
    pub fn set_vk(&self, vk: u32) {
        self.current.store(vk, Ordering::Relaxed);
        let _ = self.tx.send(vk);
    }

    pub fn get_vk(&self) -> u32 {
        self.current.load(Ordering::Relaxed)
    }
}

unsafe extern "system" fn wnd_proc(
    hwnd: HWND,
    msg: u32,
    wparam: WPARAM,
    lparam: LPARAM,
) -> LRESULT {
    DefWindowProcW(hwnd, msg, wparam, lparam)
}

fn hotkey_thread(initial_vk: u32, rx: Receiver<u32>, current: Arc<AtomicU32>, on_trigger: Trigger) {
    unsafe {
        let hmodule = GetModuleHandleW(None).unwrap_or_default();

        let mut wc = WNDCLASSW::default();
        wc.lpfnWndProc = Some(wnd_proc);
        wc.hInstance = HINSTANCE(hmodule.0);
        wc.lpszClassName = *CLASS_NAME;
        RegisterClassW(&wc);

        let hwnd = CreateWindowExW(
            WINDOW_EX_STYLE::default(),
            *CLASS_NAME,
            None,
            WINDOW_STYLE::default(),
            0,
            0,
            0,
            0,
            HWND_MESSAGE,
            None,
            hmodule,
            None,
        );

        if hwnd.0.is_null() {
            // Cannot create the hidden window; nothing more we can do.
            return;
        }

        let mut active: Option<u32> = None;

        loop {
            // Apply any pending hotkey change.
            if let Ok(new_vk) = rx.try_recv() {
                if active.take().is_some() {
                    // Ignore the result: binding shape (BOOL vs Result) varies.
                    let _ = UnregisterHotKey(hwnd, HOTKEY_ID);
                }
                if new_vk != 0 {
                    let _ = RegisterHotKey(hwnd, HOTKEY_ID, MOD_NOREPEAT, new_vk);
                    active = Some(new_vk);
                }
            } else if active.is_none() && initial_vk != 0 {
                // Initial registration.
                let _ = RegisterHotKey(hwnd, HOTKEY_ID, MOD_NOREPEAT, initial_vk);
                active = Some(initial_vk);
            }

            // Drain the message queue.
            let mut msg = MSG::default();
            while PeekMessageW(&mut msg, None, 0, 0, PM_REMOVE).as_bool() {
                if msg.message == WM_HOTKEY {
                    on_trigger();
                } else {
                    let _ = TranslateMessage(&msg);
                    DispatchMessageW(&msg);
                }
            }

            // Keep `current` authoritative in case a registration failed.
            let _ = current.load(Ordering::Relaxed);

            std::thread::sleep(Duration::from_millis(20));
        }
    }
}

/// Suppress "unused import" noise for HMODULE on some feature combinations.
#[allow(dead_code)]
fn _assert_types(_: HMODULE) {}
