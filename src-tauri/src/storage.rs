//! storage.rs — Persist profiles and the hotkey as JSON under %APPDATA%.

use crate::settings::SettingsProfile;
use serde::{Deserialize, Serialize};
use std::fs;
use std::path::PathBuf;

use crate::hotkey::DEFAULT_VK;

/// Everything we persist.
#[derive(Serialize, Deserialize, Default, Clone, Debug)]
pub struct AppConfig {
    pub profiles: Vec<SettingsProfile>,
    pub hotkey: u32,
}

fn app_dir() -> PathBuf {
    let base = std::env::var("APPDATA").unwrap_or_else(|_| ".".to_string());
    PathBuf::from(base).join("AutoClicker")
}

fn config_path() -> PathBuf {
    app_dir().join("config.json")
}

/// Read the config, returning defaults on any failure.
pub fn load_config() -> AppConfig {
    let path = config_path();
    let mut cfg = match fs::read_to_string(&path) {
        Ok(text) => serde_json::from_str::<AppConfig>(&text).unwrap_or_default(),
        Err(_) => AppConfig::default(),
    };
    if cfg.hotkey == 0 {
        cfg.hotkey = DEFAULT_VK;
    }
    cfg
}

/// Write the config, creating the directory if needed.
pub fn save_config(cfg: &AppConfig) -> Result<(), String> {
    let dir = app_dir();
    if !dir.exists() {
        fs::create_dir_all(&dir).map_err(|e| e.to_string())?;
    }
    let text = serde_json::to_string_pretty(cfg).map_err(|e| e.to_string())?;
    fs::write(config_path(), text).map_err(|e| e.to_string())
}

/// Replace the stored profile list.
pub fn save_profiles(profiles: &[SettingsProfile]) -> Result<(), String> {
    let mut cfg = load_config();
    cfg.profiles = profiles.to_vec();
    save_config(&cfg)
}

/// Persist the hotkey virtual-key code.
pub fn save_hotkey(vk: u32) -> Result<(), String> {
    let mut cfg = load_config();
    cfg.hotkey = vk;
    save_config(&cfg)
}

/// Remove a profile by name.
pub fn delete_profile(name: &str) -> Result<Vec<SettingsProfile>, String> {
    let mut cfg = load_config();
    cfg.profiles.retain(|p| p.name != name);
    save_config(&cfg)?;
    Ok(cfg.profiles)
}
