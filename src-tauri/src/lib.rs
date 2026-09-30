//! lib.rs — Tauri application shell: commands, tray icon, hotkey wiring and
//! the low-level mouse hook used to pick a screen coordinate.

mod engine;
mod hotkey;
mod input;
mod settings;
mod storage;

use engine::Engine;
use hotkey::HotkeyManager;
use parking_lot::Mutex;
use settings::{ClickSettings, SettingsProfile};
use std::sync::{Arc, OnceLock};
use storage::AppConfig;
use tauri::{Emitter, Manager, State};
use windows::Win32::Foundation::{HHOOK, LPARAM, LRESULT, WPARAM};
use windows::Win32::UI::WindowsAndMessaging::{
    CallNextHookEx, SetWindowsHookExW, UnhookWindowsHookEx, MSLLHOOKSTRUCT, WH_MOUSE_LL,
    WM_LBUTTONDOWN,
};

struct AppState {
    engine: Engine,
    hotkey: Arc<HotkeyManager>,
    /// Guards against two concurrent pick operations.
    picking: Mutex<bool>,
}

// ---------------------------------------------------------------------------
//  Coordinate picking (low-level mouse hook)
// ---------------------------------------------------------------------------

#[derive(Default)]
struct PickState {
    active: bool,
    result: Option<(i32, i32)>,
}

static PICK: OnceLock<Mutex<PickState>> = OnceLock::new();

fn pick_state() -> &'static Mutex<PickState> {
    PICK.get_or_init(|| Mutex::new(PickState::default()))
}

unsafe extern "system" fn mouse_hook_proc(code: i32, wparam: WPARAM, lparam: LPARAM) -> LRESULT {
    if code >= 0 && wparam.0 as u32 == WM_LBUTTONDOWN {
        let hook = lparam.0 as *const MSLLHOOKSTRUCT;
        if !hook.is_null() {
            let pt = (*hook).pt;
            let mut st = pick_state().lock();
            if st.active && st.result.is_none() {
                st.result = Some((pt.x, pt.y));
                st.active = false;
            }
        }
    }
    CallNextHookEx(HHOOK::default(), code, wparam, lparam)
}

// ---------------------------------------------------------------------------
//  Commands
// ---------------------------------------------------------------------------

#[tauri::command]
fn load_config() -> AppConfig {
    storage::load_config()
}

#[tauri::command]
fn save_profiles(profiles: Vec<SettingsProfile>) -> Result<(), String> {
    storage::save_profiles(&profiles)
}

#[tauri::command]
fn delete_profile(name: String) -> Result<Vec<SettingsProfile>, String> {
    storage::delete_profile(&name)
}

#[tauri::command]
fn set_hotkey(vk: u32, state: State<'_, AppState>) -> Result<(), String> {
    state.hotkey.set_vk(vk);
    storage::save_hotkey(vk)
}

#[tauri::command]
fn get_hotkey(state: State<'_, AppState>) -> u32 {
    state.hotkey.get_vk()
}

#[tauri::command]
fn start_clicking(settings: ClickSettings, state: State<'_, AppState>) -> Result<(), String> {
    state.engine.start(settings);
    Ok(())
}

#[tauri::command]
fn stop_clicking(state: State<'_, AppState>) -> Result<(), String> {
    state.engine.stop();
    Ok(())
}

#[tauri::command]
fn is_running(state: State<'_, AppState>) -> bool {
    state.engine.is_running() && !state.engine.is_paused()
}

#[tauri::command]
fn get_cursor_pos() -> (i32, i32) {
    input::cursor_pos()
}

/// Capture the next left-click and report its screen coordinate.
#[tauri::command]
fn pick_position(app: tauri::AppHandle, state: State<'_, AppState>) -> Result<(), String> {
    {
        let mut busy = state.picking.lock();
        if *busy {
            return Ok(());
        }
        *busy = true;
    }

    {
        let mut st = pick_state().lock();
        st.active = true;
        st.result = None;
    }

    std::thread::spawn(move || {
        use windows::Win32::UI::WindowsAndMessaging::{
            DispatchMessageW, PeekMessageW, TranslateMessage, MSG, PM_REMOVE,
        };
        unsafe {
            let hook = match SetWindowsHookExW(WH_MOUSE_LL, Some(mouse_hook_proc), None, 0) {
                Ok(h) => h,
                Err(_) => {
                    let _ = app.emit("picked-position", serde_json::json!({ "ok": false }));
                    return;
                }
            };

            // Low-level hooks require a message pump on the installing thread.
            let mut msg = MSG::default();
            loop {
                while PeekMessageW(&mut msg, None, 0, 0, PM_REMOVE).as_bool() {
                    let _ = TranslateMessage(&msg);
                    DispatchMessageW(&msg);
                }

                let done = pick_state().lock().result;
                if let Some((x, y)) = done {
                    let _ = app.emit(
                        "picked-position",
                        serde_json::json!({ "ok": true, "x": x, "y": y }),
                    );
                    break;
                }
                std::thread::sleep(std::time::Duration::from_millis(16));
            }

            let _ = UnhookWindowsHookEx(hook);
        }
    });

    Ok(())
}

#[tauri::command]
fn show_main_window(app: tauri::AppHandle) -> Result<(), String> {
    if let Some(w) = app.get_webview_window("main") {
        w.show().map_err(|e| e.to_string())?;
        w.set_focus().map_err(|e| e.to_string())?;
    }
    Ok(())
}

#[tauri::command]
fn hide_main_window(app: tauri::AppHandle) -> Result<(), String> {
    if let Some(w) = app.get_webview_window("main") {
        w.hide().map_err(|e| e.to_string())?;
    }
    Ok(())
}

// ---------------------------------------------------------------------------
//  App bootstrap
// ---------------------------------------------------------------------------

#[cfg_attr(mobile, tauri::mobile_entry_point)]
pub fn run() {
    tauri::Builder::default()
        .setup(|app| {
            let handle = app.handle().clone();
            let cfg = storage::load_config();

            let engine = Engine::new();
            engine.set_status_callback(move |status| {
                let _ = handle.emit("engine://status", status);
            });

            // Global hotkey: emit an event and let the frontend decide,
            // since it owns the current settings.
            let toggle_handle = app.handle().clone();
            let hotkey = HotkeyManager::new(
                cfg.hotkey,
                Arc::new(move || {
                    let _ = toggle_handle.emit("hotkey://toggle", ());
                }),
            );

            app.manage(AppState {
                engine,
                hotkey: Arc::new(hotkey),
                picking: Mutex::new(false),
            });

            // ---- Tray icon ----
            build_tray(app)?;

            Ok(())
        })
        .invoke_handler(tauri::generate_handler![
            load_config,
            save_profiles,
            delete_profile,
            set_hotkey,
            get_hotkey,
            start_clicking,
            stop_clicking,
            is_running,
            get_cursor_pos,
            pick_position,
            show_main_window,
            hide_main_window
        ])
        .run(tauri::generate_context!())
        .expect("error while running AutoClicker");
}

fn build_tray(app: &tauri::App) -> Result<(), Box<dyn std::error::Error>> {
    use tauri::menu::{MenuBuilder, MenuItemBuilder};
    use tauri::tray::TrayIconBuilder;

    let show = MenuItemBuilder::with_id("show", "显示窗口").build(app)?;
    let toggle = MenuItemBuilder::with_id("toggle", "开始 / 暂停").build(app)?;
    let quit = MenuItemBuilder::with_id("quit", "退出").build(app)?;
    let menu = MenuBuilder::new(app)
        .items(&[&show, &toggle, &quit])
        .build()?;

    // `icon()` takes an owned Image, so clone the app's default icon.
    let mut tray = TrayIconBuilder::with_id("main-tray").menu(&menu).tooltip("AutoClicker");
    if let Some(ic) = app.default_window_icon() {
        tray = tray.icon(ic.clone());
    }

    tray.on_menu_event(|app, event| match event.id().as_ref() {
        "show" => {
            if let Some(w) = app.get_webview_window("main") {
                let _ = w.show();
                let _ = w.set_focus();
            }
        }
        "toggle" => {
            let _ = app.emit("hotkey://toggle", ());
        }
        "quit" => {
            if let Some(state) = app.try_state::<AppState>() {
                state.engine.stop();
            }
            app.exit(0);
        }
        _ => {}
    })
    .build(app)?;

    Ok(())
}
