(()=> {
  "use strict";
  const $ = id => document.getElementById(id);
  const backend = window.__JUCE__?.backend;
  const pending = new Map();
  let nextPromiseId = 0;
  let requestInFlight = false;

  const pageMeta = {
    play: ["PLAY", "Performance", "Your voice, translated into musical expression."],
    midi: ["MIDI", "MIDI routing", "Manage the connection between NOVOX and your instrument."],
    audio: ["AUDIO", "Audio environment", "Input signal, pitch detection and runtime health."],
    presets: ["PRESETS", "Your presets", "Your preferred vocal instrument setups, stored locally."]
  };

  function showPage(name) {
    const selected = pageMeta[name] ? name : "play";
    document.querySelectorAll("[data-view]").forEach(button => {
      const active = button.dataset.view === selected;
      button.classList.toggle("active", active);
      button.setAttribute("aria-current", active ? "page" : "false");
    });
    document.querySelectorAll("[data-page]").forEach(page => {
      page.classList.toggle("active", page.dataset.page === selected);
    });
    $("breadcrumb-current").textContent = pageMeta[selected][0];
    $("page-title").textContent = pageMeta[selected][1];
    $("page-subtitle").textContent = pageMeta[selected][2];
  }

  document.querySelectorAll("[data-view]").forEach(button => {
    button.addEventListener("click", () => showPage(button.dataset.view));
  });

  function invokeNative(name, params = []) {
    if (!backend) return Promise.reject(new Error("Native bridge unavailable"));
    return new Promise((resolve, reject) => {
      const promiseId = nextPromiseId++;
      pending.set(promiseId, { resolve, reject });
      backend.emitEvent("__juce__invoke", { name, params, resultId: promiseId });
    });
  }

  function requestStatus() {
    return invokeNative("getNovoxStatus");
  }

  async function loadMidiOutputs() {
    const result = await invokeNative("getNovoxMidiOutputs");
    const select = $("midi-output-select");
    if (!select) return;
    select.replaceChildren(new Option("No MIDI output", ""));
    for (let i = 0; i < Number(result.count || 0); i++) {
      const name = result["item" + i];
      if (name) select.add(new Option(name, name));
    }
    select.value = result.selected || "";
  }

  async function runMidiCommand(name, params = [], successMessage = "Settings saved.") {
    const label = $("midi-command-status");
    if (label) label.textContent = "Applying…";
    try {
      const success = await invokeNative(name, params);
      if (success !== true) throw new Error("The native engine rejected this setting.");
      if (label) label.textContent = successMessage;
      await loadMidiOutputs();
      await poll();
    } catch (error) {
      if (label) label.textContent = "Could not apply setting. Check the MIDI device and try again.";
    }
  }

  backend?.addEventListener("__juce__complete", payload => {
    const item = pending.get(payload.promiseId);
    if (!item) return;
    pending.delete(payload.promiseId);
    item.resolve(payload.result);
  });

  $("midi-output-select")?.addEventListener("change", event => {
    runMidiCommand("setNovoxMidiOutput", [event.target.value], "MIDI output updated.");
  });
  $("refresh-midi")?.addEventListener("click", async () => {
    const label = $("midi-command-status");
    if (label) label.textContent = "Searching for MIDI outputs…";
    try {
      const success = await invokeNative("refreshNovoxMidiOutputs");
      if (success !== true) throw new Error("Refresh failed");
      await loadMidiOutputs();
      await poll();
      if (label) label.textContent = "MIDI outputs refreshed.";
    } catch (_) {
      if (label) label.textContent = "Could not refresh MIDI outputs.";
    }
  });
  $("midi-channel-select")?.addEventListener("change", event => {
    runMidiCommand("setNovoxMidiChannel", [Number(event.target.value)], "MIDI channel updated.");
  });
  $("pitch-bend-toggle")?.addEventListener("click", event => {
    const button = event.currentTarget;
    const enabled = button.getAttribute("aria-pressed") !== "true";
    runMidiCommand("setNovoxPitchBendEnabled", [enabled], "Pitch Bend setting updated.");
  });
  $("bend-range-select")?.addEventListener("change", event => {
    runMidiCommand("setNovoxPitchBendRange", [Number(event.target.value)], "Pitch Bend range updated.");
  });

  const number = (value, decimals = 0) => Number.isFinite(Number(value))
    ? Number(value).toFixed(decimals) : "—";
  const setText = (id, value) => { const node = $(id); if (node) node.textContent = value; };
  const setBar = (id, value, max = 100) => {
    const node = $(id);
    if (node) node.style.width = Math.max(0, Math.min(100, Number(value) / max * 100)) + "%";
  };

  function render(status) {
    const voiced = Number(status.frequencyHz) > 0;
    const confidence = Math.max(0, Math.min(1, Number(status.confidence) || 0));
    const connected = Boolean(status.midiConnected);
    const audioAvailable = Boolean(status.audioAvailable);
    const gain = Number(status.inputGain) || 1;
    const sensitivity = Number(status.pitchSensitivity) || 0;
    const smoothing = Number(status.pitchSmoothing) || 0;
    const bendRange = Number(status.pitchBendRange) || 2;
    const midiName = connected && status.midiOutputName ? status.midiOutputName : "No output selected";
    const inputName = status.audioInputName || "No input device";

    setText("note", voiced ? (status.note || "--") : "--");
    setText("frequency", voiced ? number(status.frequencyHz, 1) + " Hz" : "Waiting for signal");
    setText("confidence", Math.round(confidence * 100));
    setBar("meter-fill", confidence * 100);
    setText("midi-state", connected ? "Connected" : "Not connected");
    setText("midi-device", connected ? midiName : "Choose an output in MIDI settings");
    $("midi-dot").classList.toggle("connected", connected);
    setText("midi-channel", String(status.midiChannel || 1).padStart(2, "0"));
    setText("bend-range", "±" + bendRange + " semitones");
    setText("input-level", number((Number(status.inputRms) || 0) * 100, 1) + "%");
    setText("input-device-short", audioAvailable ? inputName : "No audio input available");
    setText("listening-label", voiced ? "VOICE DETECTED" : "LISTENING");
    const detectedPitch = String(status.note || "").replace(/[0-9-]/g, "").replace("♯", "#").replace("♭", "b");
    document.querySelectorAll(".wheel-note").forEach(node => node.classList.toggle("active", voiced && node.dataset.pitch === detectedPitch));
    $("footer-audio-led")?.classList.toggle("ready", audioAvailable);
    if ($("footer-audio-state")) $("footer-audio-state").textContent = audioAvailable ? "ACTIVE" : "WAITING";
    $("footer-midi-led")?.classList.toggle("ready", connected);
    if ($("footer-midi-state")) $("footer-midi-state").textContent = connected ? "CONNECTED" : "DISCONNECTED";

    $("engine-dot").classList.add("ready");
    setText("engine-state", "Native engine online");
    $("header-live-pulse").classList.add("ready");
    setText("connection-label", "Native engine connected");

    setText("midi-output-name", midiName);
    setText("midi-output-description", connected
      ? "NOVOX is connected to this MIDI destination."
      : "Select a MIDI destination from the existing MIDI settings tab.");
    setText("midi-output-state", connected ? "CONNECTED" : "DISCONNECTED");
    setText("midi-channel-detail", String(status.midiChannel || 1).padStart(2, "0"));
    setText("bend-state", status.pitchBendEnabled ? "Enabled" : "Disabled");
    setText("bend-range-detail", "±" + bendRange + " semitones");
    const outputSelect = $("midi-output-select");
    if (outputSelect && document.activeElement !== outputSelect) outputSelect.value = status.midiOutputName || "";
    const channelSelect = $("midi-channel-select");
    if (channelSelect && document.activeElement !== channelSelect) channelSelect.value = String(status.midiChannel || 1);
    const bendToggle = $("pitch-bend-toggle");
    if (bendToggle) {
      const enabled = Boolean(status.pitchBendEnabled);
      bendToggle.setAttribute("aria-pressed", String(enabled));
      bendToggle.textContent = enabled ? "ON" : "OFF";
    }
    const rangeSelect = $("bend-range-select");
    if (rangeSelect && document.activeElement !== rangeSelect) rangeSelect.value = String(bendRange);

    setText("audio-device-name", inputName);
    setText("audio-device-state", audioAvailable ? "ACTIVE" : "UNAVAILABLE");
    $("audio-device-state").classList.toggle("available", audioAvailable);
    setText("audio-channels", audioAvailable ? String(status.audioInputChannels || 0) : "—");
    setText("sample-rate", audioAvailable ? (number(Number(status.sampleRate) / 1000, 1) + " kHz") : "—");
    setText("buffer-size", audioAvailable ? number(status.bufferSize) + " samples" : "—");
    setText("input-gain", number(gain * 100) + "%");
    setBar("gain-fill", gain * 100, 400);
    setText("sensitivity", number(sensitivity) + "%");
    setBar("sensitivity-fill", sensitivity);
    setText("smoothing", number(smoothing) + "%");
    setBar("smoothing-fill", smoothing);
    setText("dropped-audio", number(status.droppedAudioSamples));
    setText("dropped-midi", number(status.droppedMidiEvents));
    setText("input-rms", number(status.inputRms, 3));
  }

  async function poll() {
    if (requestInFlight) return;
    requestInFlight = true;
    try {
      render(await requestStatus());
    } catch (_) {
      $("engine-dot").classList.remove("ready");
      setText("engine-state", "Bridge unavailable");
      setText("connection-label", "Engine bridge unavailable");
    } finally {
      requestInFlight = false;
    }
  }

  showPage("play");
  backend?.emitEvent("novoxUiReady", { version: 3 });
  loadMidiOutputs().catch(() => {});
  poll();
  window.setInterval(poll, 250);
})();