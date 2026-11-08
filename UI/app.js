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

  function requestStatus() {
    if (!backend) return Promise.reject(new Error("Native bridge unavailable"));
    return new Promise((resolve, reject) => {
      const promiseId = nextPromiseId++;
      pending.set(promiseId, { resolve, reject });
      backend.emitEvent("__juce__invoke", {
        name: "getNovoxStatus",
        params: [],
        resultId: promiseId
      });
    });
  }

  backend?.addEventListener("__juce__complete", payload => {
    const item = pending.get(payload.promiseId);
    if (!item) return;
    pending.delete(payload.promiseId);
    item.resolve(payload.result);
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
  backend?.emitEvent("novoxUiReady", { version: 2 });
  poll();
  window.setInterval(poll, 250);
})();