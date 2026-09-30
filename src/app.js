/* ==========================================================================
   app.js — AutoClicker frontend logic.
   Talks to the Rust backend through the Tauri global API (no bundler needed).
   ========================================================================== */

(function () {
  "use strict";

  const $ = (sel) => document.querySelector(sel);
  const $$ = (sel) => Array.from(document.querySelectorAll(sel));

  /* ---------- Tauri bridge ---------- */
  const T = window.__TAURI__ || null;
  const hasTauri = !!T;

  function invoke(cmd, args) {
    if (!hasTauri) return Promise.resolve(null);
    return T.core.invoke(cmd, args || {});
  }
  function listen(event, cb) {
    if (!hasTauri) return Promise.resolve(() => {});
    return T.event.listen(event, (e) => cb(e.payload));
  }
  function currentWindow() {
    return hasTauri ? T.window.getCurrentWindow() : null;
  }

  /* ---------- State ---------- */
  let points = [];          // [{ x, y, interval }]
  let running = false;
  let profiles = [];
  let hotkeyVk = 0x75;      // F6
  let picking = false;

  /* ---------- Enum mapping ---------- */
  // Rust serde uses camelCase for enum variants.
  const LIMIT_TO_JS = { byCount: "count", byTime: "time", unlimited: "unlimited" };
  const LIMIT_TO_RS = { count: "byCount", time: "byTime", unlimited: "unlimited" };

  /* ---------- Hotkey helpers ---------- */
  function keyEventToVk(e) {
    const k = e.key;
    if (/^F([1-9]|1[0-9]|2[0-4])$/.test(k)) {
      const n = parseInt(k.slice(1), 10);
      return 0x6f + n; // F1 = 0x70
    }
    if (k.length === 1) {
      const c = k.toUpperCase().charCodeAt(0);
      if ((c >= 0x30 && c <= 0x39) || (c >= 0x41 && c <= 0x5a)) return c;
    }
    return 0;
  }

  function vkToName(vk) {
    if (vk >= 0x70 && vk <= 0x87) return "F" + (vk - 0x6f);
    if ((vk >= 0x30 && vk <= 0x39) || (vk >= 0x41 && vk <= 0x5a)) {
      return String.fromCharCode(vk);
    }
    return "?";
  }

  /* ---------- Settings collection ---------- */
  function collectSettings() {
    const freq = $('input[name="freq"]:checked').value;
    const pos = $('input[name="pos"]:checked').value;
    const limit = $('input[name="limit"]:checked').value;

    return {
      useRandom: freq === "random",
      intervalFixed: Math.max(1, parseInt($("#interval-fixed").value, 10) || 1),
      intervalOffset: Math.max(0, parseInt($("#interval-offset").value, 10) || 0),

      posMode: pos, // fixed | cursor | sequence | random
      useAreaRandom: $("#area-random").checked,
      areaRadius: Math.max(0, parseInt($("#area-radius").value, 10) || 0),
      points: points.map((p) => ({ x: p.x, y: p.y, interval: p.interval })),

      limitMode: LIMIT_TO_RS[limit] || "unlimited",
      clickCountLimit: Math.max(0, parseInt($("#limit-count").value, 10) || 0),
      clickDurationSec: Math.max(0, parseInt($("#limit-time").value, 10) || 0),
      enableRest: $("#rest-enable").checked,
      restTime: Math.max(1, parseInt($("#rest-time").value, 10) || 1),
      restDuration: Math.max(1, parseInt($("#rest-duration").value, 10) || 1),

      button: $("#mouse-button").value, // left|right|middle|wheelUp|wheelDown
      clickType: $("#click-type").value, // single|double
    };
  }

  function applySettings(s) {
    if (!s) return;
    setRadio("freq", s.useRandom ? "random" : "fixed");
    $("#interval-fixed").value = s.intervalFixed;
    $("#interval-offset").value = s.intervalOffset;

    setRadio("pos", s.posMode || "cursor");
    $("#area-random").checked = !!s.useAreaRandom;
    $("#area-radius").value = s.areaRadius;

    points = (s.points || []).map((p) => ({ x: p.x, y: p.y, interval: p.interval }));
    renderPoints();

    setRadio("limit", LIMIT_TO_JS[s.limitMode] || "unlimited");
    $("#limit-count").value = s.clickCountLimit;
    $("#limit-time").value = s.clickDurationSec;
    $("#rest-enable").checked = !!s.enableRest;
    $("#rest-time").value = s.restTime;
    $("#rest-duration").value = s.restDuration || 60;

    $("#mouse-button").value = s.button || "left";
    $("#click-type").value = s.clickType || "single";
  }

  function setRadio(name, value) {
    const el = $('input[name="' + name + '"][value="' + value + '"]');
    if (el) el.checked = true;
  }

  /* ---------- Point list ---------- */
  function renderPoints() {
    const list = $("#point-list");
    const empty = $("#point-empty");
    $("#point-count").textContent = String(points.length);

    // remove existing rows
    Array.from(list.querySelectorAll(".point-item")).forEach((n) => n.remove());

    if (points.length === 0) {
      empty.hidden = false;
      return;
    }
    empty.hidden = true;

    points.forEach((p, i) => {
      const row = document.createElement("div");
      row.className = "point-item";

      const idx = document.createElement("span");
      idx.className = "point-index";
      idx.textContent = String(i + 1);

      const coord = document.createElement("span");
      coord.className = "point-coord";
      coord.textContent = "(" + p.x + ", " + p.y + ")";

      const interval = document.createElement("input");
      interval.type = "number";
      interval.className = "point-interval";
      interval.value = String(p.interval || 0);
      interval.min = "0";
      interval.title = "该点独立间隔 (ms)，0 表示使用全局";
      interval.addEventListener("change", () => {
        p.interval = Math.max(0, parseInt(interval.value, 10) || 0);
      });

      const del = document.createElement("button");
      del.className = "point-del";
      del.textContent = "×";
      del.title = "删除";
      del.addEventListener("click", () => {
        points.splice(i, 1);
        renderPoints();
      });

      row.appendChild(idx);
      row.appendChild(coord);
      row.appendChild(interval);
      row.appendChild(del);
      list.appendChild(row);
    });
  }

  /* ---------- Profiles ---------- */
  function renderProfiles() {
    const sel = $("#profile-select");
    sel.innerHTML = "";
    const placeholder = document.createElement("option");
    placeholder.value = "";
    placeholder.textContent = "— 选择方案 —";
    sel.appendChild(placeholder);

    profiles.forEach((p) => {
      const opt = document.createElement("option");
      opt.value = p.name;
      opt.textContent = p.name;
      sel.appendChild(opt);
    });
  }

  async function refreshProfiles() {
    const cfg = await invoke("load_config");
    if (!cfg) return;
    profiles = cfg.profiles || [];
    renderProfiles();
    if (typeof cfg.hotkey === "number" && cfg.hotkey > 0) {
      hotkeyVk = cfg.hotkey;
      $("#hotkey-value").textContent = vkToName(hotkeyVk);
    }
  }

  /* ---------- Run control ---------- */
  async function start() {
    const settings = collectSettings();
    await invoke("start_clicking", { settings });
    setRunning(true);
  }

  async function stop() {
    await invoke("stop_clicking");
    setRunning(false);
  }

  function setRunning(on) {
    running = on;
    const btn = $("#btn-toggle");
    btn.classList.toggle("running", on);
    $("#toggle-text").textContent = on ? "暂停" : "开始";
    $("#toggle-icon").style.borderRadius = on ? "1px" : "";
    $("#status-dot").classList.toggle("on", on);
    if (!on) {
      $("#status-text").textContent = "已停止";
      $("#status-dot").classList.remove("rest");
    } else {
      $("#status-text").textContent = "正在点击…";
    }
  }

  /* ---------- Status ---------- */
  function updateStatus(st) {
    if (!st) return;
    if (st.running) {
      setRunning(true);
      $("#status-text").textContent = st.resting ? "休息中" : "正在点击…";
      $("#status-dot").classList.toggle("rest", !!st.resting);
      $("#status-dot").classList.toggle("on", !st.resting);
    } else {
      setRunning(false);
    }
    $("#status-interval").textContent =
      "间隔 " + (st.lastInterval > 0 ? st.lastInterval + " ms" : "—");
    $("#status-remaining").textContent =
      "剩余 " + (st.remaining && st.remaining !== "-" ? st.remaining : "—");

    const pct = st.progress >= 0 ? Math.min(100, Math.max(0, st.progress)) : 0;
    $("#progress-fill").style.width = pct + "%";
  }

  /* ---------- Events ---------- */
  function bind() {
    // window controls
    $("#btn-min").addEventListener("click", () => {
      const w = currentWindow();
      if (w) w.minimize();
    });
    $("#btn-close").addEventListener("click", () => {
      const w = currentWindow();
      if (w) w.close();
    });

    // run control
    $("#btn-toggle").addEventListener("click", () => {
      if (running) stop();
      else start();
    });
    $("#btn-stop").addEventListener("click", () => stop());

    // picking
    $("#btn-pick").addEventListener("click", async () => {
      if (picking) return;
      picking = true;
      $("#pick-hint").hidden = false;
      await invoke("pick_position");
    });

    $("#btn-clear").addEventListener("click", () => {
      points = [];
      renderPoints();
    });

    // profiles
    $("#btn-save").addEventListener("click", async () => {
      const name = $("#profile-name").value.trim();
      if (!name) return;
      const settings = collectSettings();
      const existing = profiles.findIndex((p) => p.name === name);
      if (existing >= 0) profiles[existing] = { name, settings };
      else profiles.push({ name, settings });
      await invoke("save_profiles", { profiles });
      await refreshProfiles();
      $("#profile-select").value = name;
    });

    $("#btn-delete").addEventListener("click", async () => {
      const name = $("#profile-select").value;
      if (!name) return;
      profiles = await invoke("delete_profile", { name }) || [];
      renderProfiles();
      $("#profile-name").value = "";
    });

    $("#profile-select").addEventListener("change", () => {
      const name = $("#profile-select").value;
      const p = profiles.find((x) => x.name === name);
      if (!p) return;
      applySettings(p.settings);
      $("#profile-name").value = p.name;
    });

    // hotkey modal
    $("#btn-hotkey").addEventListener("click", openHotkeyModal);
    $("#btn-cancel-hotkey").addEventListener("click", closeHotkeyModal);
    $("#btn-confirm-hotkey").addEventListener("click", async () => {
      if (pendingVk > 0) {
        hotkeyVk = pendingVk;
        await invoke("set_hotkey", { vk: hotkeyVk });
        $("#hotkey-value").textContent = vkToName(hotkeyVk);
      }
      closeHotkeyModal();
    });
  }

  /* ---------- Hotkey modal ---------- */
  let pendingVk = 0;

  function openHotkeyModal() {
    pendingVk = 0;
    $("#modal-key").textContent = "…";
    $("#btn-confirm-hotkey").disabled = true;
    $("#modal").hidden = false;
  }

  function closeHotkeyModal() {
    $("#modal").hidden = true;
  }

  document.addEventListener("keydown", (e) => {
    if ($("#modal").hidden) return;
    if (e.key === "Escape") {
      closeHotkeyModal();
      return;
    }
    const vk = keyEventToVk(e);
    if (vk > 0) {
      pendingVk = vk;
      $("#modal-key").textContent = vkToName(vk);
      $("#btn-confirm-hotkey").disabled = false;
    }
  });

  /* ---------- Backend events ---------- */
  async function bindBackendEvents() {
    await listen("engine://status", updateStatus);

    await listen("hotkey://toggle", () => {
      if (running) stop();
      else start();
    });

    await listen("picked-position", (payload) => {
      picking = false;
      $("#pick-hint").hidden = true;
      if (!payload || !payload.ok) return;
      points.push({
        x: payload.x,
        y: payload.y,
        interval: parseInt($("#interval-fixed").value, 10) || 0,
      });
      renderPoints();

      // Choosing a coordinate implies fixed-position mode.
      setRadio("pos", "fixed");
    });
  }

  /* ---------- Init ---------- */
  async function init() {
    bind();
    renderPoints();
    await bindBackendEvents();
    await refreshProfiles();
    $("#hotkey-value").textContent = vkToName(hotkeyVk);
  }

  if (document.readyState === "loading") {
    document.addEventListener("DOMContentLoaded", init);
  } else {
    init();
  }
})();
