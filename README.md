# Smart Industrial Machine Monitoring & Predictive Maintenance
### On-Device 128-Point FFT Spectral Analysis, Unsupervised Anomaly Detection, and Real-Time SSE Telemetry

[![Platform: ESP32](https://img.shields.io/badge/Platform-ESP32%20Xtensa%20Dual--Core-blue.svg)](https://www.espressif.com/)
[![Sensors: MPU6050 | DS18B20 | DHT22](https://img.shields.io/badge/Sensors-MPU6050%20%7C%20DS18B20%20%7C%20DHT22-green.svg)](firmware/esp32_fft_machine_monitoring.ino)
[![Edge DSP: 128-pt FFT](https://img.shields.io/badge/Edge%20DSP-128--point%20Radix--2%20FFT-orange.svg)](firmware/esp32_fft_machine_monitoring.ino)
[![Backend: FastAPI | scikit-learn](https://img.shields.io/badge/Backend-FastAPI%20%7C%20Isolation%20Forest-purple.svg)](backend/app.py)
[![License: MIT](https://img.shields.io/badge/License-MIT-darkgreen.svg)](LICENSE)

An end-to-end Industrial Internet of Things (IIoT) predictive maintenance and machine condition monitoring system designed for rotating factory machinery (CNC milling spindles, lathes, centrifugal pumps, induction motors, compressors, and gearboxes).

The architecture computes **128-point Fast Fourier Transforms (FFT) directly on the edge microcontroller**, extracting dominant vibration frequencies and peak spectral energies while reducing wireless transmission bandwidth by $>90\%$. Extracted telemetry is streamed to a high-throughput **FastAPI backend** running an **Isolation Forest anomaly detection engine** and rendered on a reactive 3D/spectral digital twin dashboard via Server-Sent Events (SSE).

---

## 1. System Architecture & Bandwidth Optimization

```
                                  Edge Sensing & DSP (ESP32)
                        ┌───────────────────────────────────────────────┐
                        │ MPU-6050 (1 kHz Accelerometer Sampling)       │
                        │                    │                          │
                        │                    ▼                          │
                        │ 128-Point Fixed-Point Real FFT                │
                        │                    │                          │
                        │                    ▼                          │
                        │ Dominant Peak & Energy Extraction             │
                        │ DS18B20 (Surface Temp) + DHT22 (Ambient)      │
                        └────────────────────┬──────────────────────────┘
                                             │ JSON Telemetry Packet (20 Bytes)
                                             │ [>90% Wireless Bandwidth Reduction]
                                             ▼
                                  Telemetry Ingestion & ML (FastAPI)
                        ┌───────────────────────────────────────────────┐
                        │ Unsupervised Isolation Forest Anomaly Engine  │
                        │ Features: [f_dom, P_mag, T_surf, T_amb, H_amb]│
                        │ Evaluates Machine Condition (NORMAL/WARN/CRIT)│
                        └────────────────────┬──────────────────────────┘
                                             │ Server-Sent Events (SSE) Stream
                                             ▼
                                  Reactive Digital Twin Dashboard
                        ┌───────────────────────────────────────────────┐
                        │ HTML5 / CSS3 / ES6 Spectral Web Visualizer    │
                        │ Live 6-Machine Telemetry & Anomaly Gauges     │
                        └───────────────────────────────────────────────┘
```

---

## 2. Mathematical Formulations

### A. Edge Vibration Fast Fourier Transform (FFT)
Accelerations are sampled along the primary vibration axis at $f_s = 1000\text{ Hz}$ across $N = 128$ points:

$$\text{Frequency Resolution } \Delta f = \frac{f_s}{N} = \frac{1000\text{ Hz}}{128} = 7.8125\text{ Hz / bin}$$

$$\text{Nyquist Frequency } f_{\text{Nyquist}} = \frac{f_s}{2} = 500\text{ Hz}$$

For each 128-sample window $x[n]$, the Discrete Fourier Transform converts time-domain acceleration into spectral coefficients:

$$X[k] = \sum_{n=0}^{N-1} x[n] \cdot e^{-j 2\pi k n / N}, \quad k = 0, 1, \dots, \frac{N}{2}-1$$

$$|X[k]| = \sqrt{\text{Re}(X[k])^2 + \text{Im}(X[k])^2}$$

$$\text{Dominant Peak Frequency } f_{\text{dom}} = k_{\text{max}} \cdot \Delta f, \quad \text{where } k_{\text{max}} = \arg\max_{k \ge 1} |X[k]|$$

$$\text{Vibration RMS Magnitude } = \sqrt{\frac{1}{N} \sum_{n=0}^{N-1} x[n]^2}$$

### B. Unsupervised Isolation Forest Anomaly Detection
Mechanical degradation (bearing wear, rotor unbalance, shaft misalignment, gear pitting) manifests as simultaneous multi-parameter drift. The Isolation Forest isolates anomalies by randomly partitioning feature space:

$$s(x, n) = 2^{-\frac{E(h(x))}{c(n)}}$$

Where:
- $h(x)$: Path length of sample $x$ in an isolation tree
- $E(h(x))$: Average path length across an ensemble of 100 isolation trees
- $c(n) = 2 \ln(n - 1) + 0.5772156649 - \frac{2(n-1)}{n}$: Average path length of unsuccessful searches in a Binary Search Tree (BST)

Condition thresholds:
- $s(x) < 0.35 \implies \text{NORMAL condition (Nominal healthy baseline)}$
- $0.35 \le s(x) < 0.60 \implies \text{WARNING (Early bearing or thermal degradation)}$
- $s(x) \ge 0.60 \implies \text{CRITICAL (Imminent mechanical failure alarm)}$

---

## 3. Hardware Interfacing & Pin Assignments

| Sensor / Module | Physical Parameter | Protocol / Interface | ESP32 Pin Assignment | Operating Voltage |
| :--- | :--- | :---: | :---: | :---: |
| **MPU-6050** | Tri-Axial Vibration Acceleration | I²C Fast-Mode | `SDA: GPIO 21`, `SCL: GPIO 22` | 3.3V |
| **DS18B20** | Machine Surface Temperature | Dallas 1-Wire | `DATA: GPIO 4` ($4.7\text{k}\Omega$ pull-up) | 3.3V / 5.0V |
| **DHT22 (AM2302)**| Ambient Temperature & Humidity | Single-Wire Proprietary | `DATA: GPIO 15` | 3.3V / 5.0V |
| **ESP32-WROOM-32**| Processing & Wi-Fi Client | Central Controller | -- | 3.3V / 5.0V |

---

## 4. Directory Layout

```
Smart-Industrial-Machine-Monitoring/
├── README.md                           # Master architectural specification
├── .gitignore                          # Build and environment exclusions
├── firmware/
│   ├── esp32_fft_machine_monitoring.ino# 128-point on-device FFT firmware
│   └── platformio.ini                  # PlatformIO build configuration
├── backend/
│   ├── app.py                          # FastAPI SSE stream & Isolation Forest engine
│   └── requirements.txt                # Python dependencies
├── frontend/
│   └── index.html                      # Real-time industrial digital twin dashboard
└── docs/
    └── architecture_spec.md            # Hardware wiring & telemetry documentation
```

---

## 5. Execution Quick Start

### 1. Flash Firmware (PlatformIO)
```bash
cd firmware
pio run -t upload
pio device monitor -b 115200
```

### 2. Launch FastAPI Telemetry Backend
```bash
cd backend
pip install -r requirements.txt
uvicorn app:app --host 0.0.0.0 --port 8000 --reload
```

### 3. Open Digital Twin Dashboard
Launch [`frontend/index.html`](frontend/index.html) in any modern web browser. The dashboard connects to the live Server-Sent Events stream (`http://localhost:8000/api/telemetry/stream`), displaying live telemetry and spectral waterfalls.

---

## 6. Authors & License

- **Lead Engineer**: Hari Kruthik SK (Reg. 23BEC0358)  
- **Affiliation**: School of Electronics Engineering (SENSE), Vellore Institute of Technology (VIT), Vellore  
- **License**: Released under the [MIT License](LICENSE).
