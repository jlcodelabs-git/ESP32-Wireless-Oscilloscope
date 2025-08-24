# ESP32 Wireless Oscilloscope — User Manual

A comprehensive guide to the ESP32 Wireless Oscilloscope web application: connection, controls, display modes, triggering, frequency detection, spectrum view, audio, and exporting data.

This manual is meant to be copied into your GitHub repository as-is.


## Table of Contents
- Overview
- Requirements
- Quick Start
- Connection and Transport
- UI Overview
  - Info Display
  - Time/Div and Horizontal Position
  - Plot Display
  - Channel Controls (Ch1 / Ch2)
  - Trigger
  - Math / Measurements
  - Frequency Detection
  - Cursors & Export
  - Persistence (Trails)
  - Audio
  - Spectrum / FFT
- Tips & Gestures
- Persistence & Saved Settings
- HTTPS, LAN, and WSS Proxy
- Troubleshooting
- Credits


## Overview
The ESP32 Wireless Oscilloscope is a two-channel (Ch1, Ch2) web oscilloscope UI that connects to an ESP32 device streaming data over WebSocket. It renders live waveforms with adjustable timebase and volts/div, supports edge and pulse width triggers, provides frequency detection and measurements, and offers spectrum (FFT) visualization and audio monitoring of detected frequencies.

- Two channels with per-channel display and processing controls
- Time-domain and X-Y (Lissajous) display modes
- Trigger: Edge and Pulse Width, with hysteresis and hold-off
- Frequency detection: Zero Crossing, Peak Detection, Autocorrelation
- Spectrum view with linear and dB scales
- Measurement tools: Cursors, RMS/AVG
- Audio synthesis mapped to detected/channel frequencies
- PNG/CSV export


## Requirements
- ESP32 firmware that streams samples via WebSocket
- A modern browser (Chromium, Firefox, Safari)
- For Audio: browser must allow/resume AudioContext after user interaction


## Quick Start
1) Open the page at /esp32-oscilloscope
2) In Connection Settings:
   - Enter your ESP32 endpoint (examples below)
   - Click Connect, then Start to begin streaming
3) Adjust Time/Div and Volts/Div to bring signals into view
4) Enable Trigger to stabilize repeating signals, if desired
5) Use Frequency Detection to view live frequency/period and amplitude
6) Optional: Enable Spectrum, Audio, or Persistence trails
7) Export PNG/CSV from Cursors & Export


## Connection and Transport
Enter an ESP32 endpoint in the Connection Settings input. Accepted formats:
- ws://host:port/path
- wss://host:port/path
- http(s)://host:port/path (auto-converted to ws(s)://)
- host or host:port (scheme inferred; default port 81 if omitted)

Notes:
- Default port is 81 when not specified.
- On HTTPS sites, browsers block ws:// connections to private/loopback hosts. See “HTTPS, LAN, and WSS Proxy”.
- The app can auto-rewrite a private ws:// endpoint to a WSS proxy if configured.


## UI Overview
The main UI is organized into:
- Connection Settings: endpoint, connect/disconnect, start/stop, sample rate presets
- Global Controls: Time/Div, Horizontal Position, Plot Display, Trigger, Math/Measurements, Frequency, Cursors & Export, Persistence, Audio
- Per-Channel Controls for Ch1 and Ch2
- Chart area (time-domain or spectrum), with optional X-Y mode

### Info Display
- Samples: Total buffered samples
- Rate: Current sample rate (Hz) reported/used by the device
- C (Ch1) and | (Ch2): Latest corrected instantaneous values (V)
- F: Live frequency readouts per channel (when Frequency Detection is enabled)
- Amp: Live peak-to-peak amplitude estimates (Vpp)


### Time/Div and Horizontal Position
- Time/Div presets: 10 µs/div up to 5 s/div (10 divisions total)
- Window: Total visible time window = Time/Div × 10
- Horizontal Position (Delay): percent pre-trigger, 0–90%
- Mouse gestures: Ctrl/Cmd+Scroll to zoom Time/Div; Scroll to pan horizontally (Shift = larger steps)


### Plot Display
- Clamp near 0 V to flatline: Visual-only clamping for cleaner display near zero
- Legacy (raw) plotting: Plots raw arrays directly with basic scaling — useful for quick checks
- X-Y mode (Ch1 vs Ch2): Draws a single trace with X from Ch1 and Y from Ch2 (Lissajous)
- Hold: Freeze display updates


### Channel Controls (Ch1 / Ch2)
Each channel has independent controls:
- Enabled: Toggle channel visibility and processing
- Color: Trace color swatch
- VOLTS/DIV: Vertical scale per division — typical steps are 0.1, 0.2, 0.5, 1, 2, 5 V/div
- Vertical POSITION: Shifts the channel trace up/down in display units (V)
- Intensity: Trace opacity
- Trace Thickness: 1–6 px
- Coupling:
  - DC: Use signal as-is
  - AC: Remove DC component (mean) from the displayed/processed segment
  - GND: Force 0 V (useful for reference)
- Probe: 1×, 10×, 100× — scales displayed voltage accordingly
- Invert: Multiply by −1
- Median smoothing: Visual median filter for noise reduction (window set globally)
- Zero (GND)/Clear Offset: Estimate and set DC offset with probe shorted to GND; or reset to 0

Notes:
- Visualization vs. Measurement: Some clamping/smoothing are visual only. Measurements (RMS/AVG/frequency) use “no clamp” processed data to remain accurate.


### Trigger
Stabilize repeating signals using a trigger. Works with both Edge and Pulse Width modes.

Common controls:
- Enabled: Turns trigger processing on/off
- Mode: Auto, Normal, Single
  - Auto: If no trigger found, displays the most recent window
  - Normal: Displays only when a trigger is found
  - Single: Arms once; displays a single triggered frame (Arm button to re-arm)
- Source: Auto, Ch1, Ch2, EXT* (ext not implemented; falls back to Ch1)
- Horizontal Position (global): Pre-trigger samples as % of window
- Force: Force a capture immediately (useful in Single mode)

Edge Trigger:
- Slope: Rising or Falling
- Level: Voltage threshold (V)
- Hysteresis: Small voltage band around level to avoid chatter and noise-triggering

Pulse Width Trigger:
- Polarity: Positive (rise→fall) or Negative (fall→rise)
- Condition:
  - Width > T
  - Width < T
  - Between T and T_max
- T (ms), and for “Between,” T_max (ms)
- Edge reference: Leading (pulse start) or Trailing (pulse end)
- Edge Level/Hysteresis: Use the same level band to find rise/fall edges
- Hold-off (ms): Minimum time between valid triggers (wall-clock based)

Single Shot UX:
- Arm: arms the single shot
- Captured indicator shows the time of capture and briefly emphasizes the state


### Math / Measurements
- Math: Off, Ch1 − Ch2
- Automatic measurements (per channel): RMS, AVG displayed in the section


### Frequency Detection
Modes:
- Zero Crossing: Finds zero-cross periods (robust for clean periodic signals)
- Peak Detection: Finds local maxima above a dynamic threshold
- Autocorrelation: Finds periodicities in noisy signals (default)

Parameters:
- Min Freq (Hz), Max Freq (Hz)
- Amplitude gate (V): If amplitude below this threshold, frequency is reported as 0 Hz

Behavior:
- Detection windows are time-based and mapped to sample counts dynamically
- Optional smoothing during detection to reduce noise sensitivity
- In HOPPING device mode (if reported by firmware), the UI uses the device-reported frequency for Audio to ensure tight sync


### Cursors & Export
- Vertical Cursors (Cursor A/B, %): Two time cursors — the UI displays Δt and f ≈ 1/Δt
- Horizontal Voltage Cursors (Ch1 scale): Two voltage cursors V1/V2; ΔV is displayed
- Save PNG: Exports the current chart as an image
- Export CSV: Exports the current chart data (labels and datasets) for offline analysis


### Persistence (Trails)
- Enable: Show previous frames as trails (persistence effect)
- Trail Depth: 1–6; older trails fade progressively


### Audio
- Enable/Disable: Start/stop WebAudio oscillators
- Channel: Off, Ch1, Ch2, Both
- Waveform: sine, square, sawtooth, triangle
- Volume: 0–100

Behavior:
- Frequencies map from detected values (or device-reported HOPPING frequency)
- Gains are gated low when no valid frequency is present (mute state)
- AudioContext resumes upon user interaction per browser autoplay policies


### Spectrum / FFT
- Enable Spectrum View: Switch to frequency-domain view
- Channel: Analyze Ch1 or Ch2
- NFFT: 512, 1024, 2048 (window size is last N samples or NFFT, whichever is smaller; rounded down to power-of-two)
- Scale: Linear or dB (20·log10)
- Window: Rect or Hann

Notes:
- Frequencies span 0..(fs/2)
- Magnitude is normalized by N
- DC component is removed before windowing


## Tips & Gestures
- Ctrl/Cmd + Scroll: Zoom Time/Div
- Scroll: Pan Horizontal Position (Delay)
- Shift: Larger steps for panning
- Single-shot: Arm then Force if needed to grab a specific moment


## Persistence & Saved Settings
Most controls are saved to localStorage and restored on next visit, including:
- All plotting, display, trigger, frequency detection, persistence, audio, spectrum, and cursor settings
- Per-channel colors, scales, positions, offsets, and visual toggles
- Connection endpoint (ws/wss URL or host[:port])


## HTTPS, LAN, and WSS Proxy
When the page is served over HTTPS, browsers block ws:// connections to private/loopback addresses (mixed content + Private Network Access constraints). Options:
- Develop locally over HTTP
- Provide a WSS reverse-proxy (TLS termination) and set NEXT_PUBLIC_ESP32_WSS_PROXY to a wss:// endpoint that forwards to your ESP32 ws:// device
- If configured, the app will rewrite private ws:// endpoints to use the proxy automatically


## Troubleshooting
- Can’t connect:
  - Verify the endpoint (host, port, path)
  - On HTTPS, use WSS or configure a WSS proxy
  - Ensure ESP32 firmware is running and serving WebSocket
- No data / flatline:
  - Click Start to begin capture
  - Check VOLTS/DIV and POSITION
  - Try DC coupling; verify probe setting
- Trigger doesn’t lock:
  - Adjust Level and Slope, or switch to Auto mode first
  - Use Hysteresis to avoid noise-triggering
  - Consider Pulse Width trigger for pulsed signals
- Frequency shows “-- Hz” or 0 Hz:
  - Increase amplitude gate if noise is high, or decrease if signal is small
  - Adjust Min/Max Freq range
  - Try Autocorrelation for noisy signals
- Audio silent:
  - Ensure Audio is enabled and volume is above 0
  - Interact with the page to allow AudioContext to resume
  - Verify a valid detected/device frequency exists
- Spectrum empty:
  - Ensure enough samples are present (window size)
  - Check Channel selection and NFFT


## Credits
- Web UI built with Next.js + React + TypeScript and Chart.js
- Uses WebAudio for audio synthesis and a radix-2 FFT for spectrum view
- ESP32 firmware: https://github.com/jlcodelabs-git/ESP32-Wireless-Oscilloscope


---
If you have suggestions or want to report issues, please open an issue in the repository.

