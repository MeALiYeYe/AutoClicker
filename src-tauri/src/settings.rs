//! settings.rs — Serializable settings structures shared between the
//! Rust backend and the web frontend (JSON via serde).

use serde::{Deserialize, Serialize};

/// Where the click should land.
#[derive(Serialize, Deserialize, Clone, Copy, Debug, PartialEq, Eq)]
#[serde(rename_all = "camelCase")]
pub enum PosMode {
    /// Use the first stored coordinate.
    Fixed,
    /// Use wherever the cursor currently is.
    Cursor,
    /// Cycle through stored coordinates in order.
    Sequence,
    /// Pick a random stored coordinate each time.
    Random,
}

/// Which mouse button to synthesize.
#[derive(Serialize, Deserialize, Clone, Copy, Debug, PartialEq, Eq)]
#[serde(rename_all = "camelCase")]
pub enum MouseButton {
    Left,
    Right,
    Middle,
    WheelUp,
    WheelDown,
}

/// Single or double click.
#[derive(Serialize, Deserialize, Clone, Copy, Debug, PartialEq, Eq)]
#[serde(rename_all = "camelCase")]
pub enum ClickType {
    Single,
    Double,
}

/// How the run should be bounded.
#[derive(Serialize, Deserialize, Clone, Copy, Debug, PartialEq, Eq)]
#[serde(rename_all = "camelCase")]
pub enum LimitMode {
    Unlimited,
    ByCount,
    ByTime,
}

impl Default for PosMode {
    fn default() -> Self {
        PosMode::Cursor
    }
}
impl Default for MouseButton {
    fn default() -> Self {
        MouseButton::Left
    }
}
impl Default for ClickType {
    fn default() -> Self {
        ClickType::Single
    }
}
impl Default for LimitMode {
    fn default() -> Self {
        LimitMode::Unlimited
    }
}

/// A stored click coordinate with its own interval override.
#[derive(Serialize, Deserialize, Clone, Copy, Debug, PartialEq, Eq)]
pub struct ClickPoint {
    pub x: i32,
    pub y: i32,
    /// Per-point interval in milliseconds (0 = use global interval).
    pub interval: u32,
}

/// Everything the click engine needs to run.
#[derive(Serialize, Deserialize, Clone, Debug, PartialEq)]
#[serde(rename_all = "camelCase", default)]
pub struct ClickSettings {
    /// Randomize the interval around `interval_fixed`.
    pub use_random: bool,
    pub interval_fixed: u32,
    pub interval_offset: u32,

    pub pos_mode: PosMode,
    /// Jitter the landing point within `area_radius` pixels.
    pub use_area_random: bool,
    pub area_radius: u32,
    pub points: Vec<ClickPoint>,

    pub limit_mode: LimitMode,
    pub click_count_limit: u32,
    pub click_duration_sec: u32,
    /// Anti-addiction: pause periodically.
    pub enable_rest: bool,
    /// Work time before a rest, in seconds.
    pub rest_time: u32,
    /// How long each rest lasts, in seconds.
    pub rest_duration: u32,

    pub button: MouseButton,
    pub click_type: ClickType,
}

impl Default for ClickSettings {
    fn default() -> Self {
        Self {
            use_random: false,
            interval_fixed: 1000,
            interval_offset: 200,
            pos_mode: PosMode::Cursor,
            use_area_random: false,
            area_radius: 20,
            points: Vec::new(),
            limit_mode: LimitMode::Unlimited,
            click_count_limit: 1000,
            click_duration_sec: 3600,
            enable_rest: false,
            rest_time: 600,
            rest_duration: 60,
            button: MouseButton::Left,
            click_type: ClickType::Single,
        }
    }
}

/// A named, persisted configuration.
#[derive(Serialize, Deserialize, Clone, Debug)]
pub struct SettingsProfile {
    pub name: String,
    pub settings: ClickSettings,
}

/// Live status pushed to the frontend.
#[derive(Serialize, Clone, Debug)]
#[serde(rename_all = "camelCase")]
pub struct EngineStatus {
    pub running: bool,
    pub click_count: u64,
    /// Remaining count or seconds as text; "-" when unlimited.
    pub remaining: String,
    pub last_interval: u32,
    /// 0..100, or -1 when unbounded.
    pub progress: i32,
    pub resting: bool,
}
