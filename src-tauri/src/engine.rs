//! engine.rs — Background clicking thread with interval, position and
//! limit handling. Reports live status back to the frontend via a callback.

use crate::input;
use crate::settings::{ClickSettings, EngineStatus, LimitMode, PosMode};
use parking_lot::Mutex;
use rand::Rng;
use std::sync::atomic::{AtomicBool, Ordering};
use std::sync::Arc;
use std::thread::JoinHandle;
use std::time::{Duration, Instant};

type StatusCallback = Box<dyn Fn(EngineStatus) + Send + Sync>;

struct Inner {
    settings: Mutex<ClickSettings>,
    /// Set to true to terminate the worker thread.
    stop: AtomicBool,
    /// Set to true to temporarily suspend clicking.
    paused: AtomicBool,
    on_status: Mutex<Option<StatusCallback>>,
}

/// Thread-safe handle to the clicking worker.
#[derive(Clone)]
pub struct Engine {
    inner: Arc<Inner>,
    /// Worker handle, guarded so start/stop are serialized.
    worker: Arc<Mutex<Option<JoinHandle<()>>>>,
}

impl Engine {
    pub fn new() -> Self {
        Self {
            inner: Arc::new(Inner {
                settings: Mutex::new(ClickSettings::default()),
                stop: AtomicBool::new(false),
                paused: AtomicBool::new(false),
                on_status: Mutex::new(None),
            }),
            worker: Arc::new(Mutex::new(None)),
        }
    }

    /// Install the callback used to push status to the frontend.
    pub fn set_status_callback<F>(&self, cb: F)
    where
        F: Fn(EngineStatus) + Send + Sync + 'static,
    {
        *self.inner.on_status.lock() = Some(Box::new(cb));
    }

    fn emit(&self, status: EngineStatus) {
        if let Some(cb) = self.inner.on_status.lock().as_ref() {
            cb(status);
        }
    }

    pub fn is_running(&self) -> bool {
        self.worker.lock().is_some()
    }

    pub fn is_paused(&self) -> bool {
        self.inner.paused.load(Ordering::Relaxed)
    }

    pub fn settings(&self) -> ClickSettings {
        self.inner.settings.lock().clone()
    }

    /// Apply new settings (used while running to update behaviour).
    pub fn update_settings(&self, s: ClickSettings) {
        *self.inner.settings.lock() = s;
    }

    /// Begin clicking. Restarts the worker if one is already running.
    pub fn start(&self, settings: ClickSettings) {
        self.stop();
        *self.inner.settings.lock() = settings;
        self.inner.stop.store(false, Ordering::Relaxed);
        self.inner.paused.store(false, Ordering::Relaxed);

        let inner = self.inner.clone();
        let mut guard = self.worker.lock();
        *guard = Some(std::thread::spawn(move || run_loop(inner)));
    }

    /// Temporarily suspend clicking (worker stays alive).
    pub fn pause(&self) {
        self.inner.paused.store(true, Ordering::Relaxed);
    }

    /// Resume after `pause`.
    pub fn resume(&self) {
        self.inner.paused.store(false, Ordering::Relaxed);
    }

    /// Terminate the worker and reset counters.
    pub fn stop(&self) {
        self.inner.stop.store(true, Ordering::Relaxed);
        if let Some(handle) = self.worker.lock().take() {
            let _ = handle.join();
        }
        self.inner.paused.store(false, Ordering::Relaxed);
        self.emit(EngineStatus {
            running: false,
            click_count: 0,
            remaining: "-".into(),
            last_interval: 0,
            progress: -1,
            resting: false,
        });
    }
}

impl Default for Engine {
    fn default() -> Self {
        Self::new()
    }
}

/// Sleep in small slices so `stop` is honoured quickly.
fn sleep_checked(total: Duration, stop: &AtomicBool) {
    const SLICE: Duration = Duration::from_millis(40);
    let mut left = total;
    while left > Duration::ZERO {
        if stop.load(Ordering::Relaxed) {
            return;
        }
        let step = if left < SLICE { left } else { SLICE };
        std::thread::sleep(step);
        left -= step;
    }
}

/// Resolve where (and how often) to click for this iteration.
fn resolve_target(
    s: &ClickSettings,
    seq: &mut usize,
    rng: &mut impl Rng,
) -> (Option<(i32, i32)>, u32) {
    // Base interval, optionally randomized around the fixed value.
    let base = if s.use_random && s.interval_offset > 0 {
        let off = s.interval_offset;
        let lo = s.interval_fixed.saturating_sub(off).max(5);
        let hi = s.interval_fixed.saturating_add(off).max(lo + 1);
        rng.gen_range(lo..=hi)
    } else {
        s.interval_fixed.max(5)
    };

    if s.points.is_empty() || matches!(s.pos_mode, PosMode::Cursor) {
        return (None, base);
    }

    let (index, point_interval) = match s.pos_mode {
        PosMode::Fixed => (0usize, s.points[0].interval),
        PosMode::Sequence => {
            let i = *seq % s.points.len();
            *seq += 1;
            (i, s.points[i].interval)
        }
        PosMode::Random => {
            let i = rng.gen_range(0..s.points.len());
            (i, s.points[i].interval)
        }
        PosMode::Cursor => unreachable!(),
    };

    let interval = if point_interval > 0 { point_interval } else { base };

    let mut x = s.points[index].x;
    let mut y = s.points[index].y;
    if s.use_area_random && s.area_radius > 0 {
        let r = s.area_radius as i32;
        x += rng.gen_range(-r..=r);
        y += rng.gen_range(-r..=r);
    }

    (Some((x, y)), interval)
}

fn run_loop(inner: Arc<Inner>) {
    let mut rng = rand::thread_rng();
    let mut seq: usize = 0;
    let mut count: u64 = 0;
    let started = Instant::now();
    let mut work_start = Instant::now();
    let mut last_emit = Instant::now();

    loop {
        if inner.stop.load(Ordering::Relaxed) {
            break;
        }

        if inner.paused.load(Ordering::Relaxed) {
            std::thread::sleep(Duration::from_millis(80));
            continue;
        }

        let settings = inner.settings.lock().clone();

        // Anti-addiction rest period.
        if settings.enable_rest
            && settings.rest_time > 0
            && work_start.elapsed() >= Duration::from_secs(settings.rest_time as u64)
        {
            let rest_secs = settings.rest_duration.max(1);
            throttled_emit(
                &inner,
                &mut last_emit,
                EngineStatus {
                    running: true,
                    click_count: count,
                    remaining: format!("休息 {}s", rest_secs),
                    last_interval: 0,
                    progress: -1,
                    resting: true,
                },
            );
            sleep_checked(Duration::from_secs(rest_secs as u64), &inner.stop);
            work_start = Instant::now();
            continue;
        }

        let (pos, interval) = resolve_target(&settings, &mut seq, &mut rng);

        match pos {
            Some((x, y)) => input::click_at(x, y, settings.button, settings.click_type),
            None => input::click_current(settings.button, settings.click_type),
        }

        count += 1;

        // Progress accounting.
        let (progress, remaining, done) = match settings.limit_mode {
            LimitMode::ByCount if settings.click_count_limit > 0 => {
                let pct = (count as f64 / settings.click_count_limit as f64 * 100.0) as i32;
                (
                    pct.min(100),
                    format!(
                        "{} 次",
                        settings.click_count_limit.saturating_sub(count as u32)
                    ),
                    count >= settings.click_count_limit as u64,
                )
            }
            LimitMode::ByTime if settings.click_duration_sec > 0 => {
                let total = settings.click_duration_sec as f64;
                let el = started.elapsed().as_secs_f64();
                let pct = ((el / total) * 100.0) as i32;
                let left =
                    settings.click_duration_sec as i64 - started.elapsed().as_secs() as i64;
                (
                    pct.min(100),
                    format!("{}s", if left < 0 { 0 } else { left }),
                    el >= total,
                )
            }
            _ => (-1, "-".to_string(), false),
        };

        throttled_emit(
            &inner,
            &mut last_emit,
            EngineStatus {
                running: true,
                click_count: count,
                remaining,
                last_interval: interval,
                progress,
                resting: false,
            },
        );

        if done {
            inner.stop.store(true, Ordering::Relaxed);
            if let Some(cb) = inner.on_status.lock().as_ref() {
                cb(EngineStatus {
                    running: false,
                    click_count: count,
                    remaining: "0".into(),
                    last_interval: interval,
                    progress: 100,
                    resting: false,
                });
            }
            break;
        }

        sleep_checked(Duration::from_millis(interval as u64), &inner.stop);
    }
}

/// Push a status update, throttled so very fast intervals don't flood the webview.
fn throttled_emit(inner: &Arc<Inner>, last_emit: &mut Instant, status: EngineStatus) {
    let important = status.resting || !status.running;
    if !important && last_emit.elapsed() < Duration::from_millis(80) {
        return;
    }
    *last_emit = Instant::now();
    if let Some(cb) = inner.on_status.lock().as_ref() {
        cb(status);
    }
}
