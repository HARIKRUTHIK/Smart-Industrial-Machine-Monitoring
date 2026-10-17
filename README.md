# Smart Industrial Machine Monitoring & Predictive Maintenance

[![Platform: ESP32](https://img.shields.io/badge/Platform-ESP32-blue.svg)](https://www.espressif.com/)
[![Sensors: MPU6050 | DS18B20 | DHT22](https://img.shields.io/badge/Sensors-Vibration%20%7C%20Temp%20%7C%20Humidity-green.svg)]()
[![DSP: Edge FFT](https://img.shields.io/badge/Edge%20DSP-128--pt%20FFT%20(%3E90%25%20bandwidth%20reduction)-purple.svg)]()
[![Backend: FastAPI | Isolation Forest](https://img.shields.io/badge/ML-Isolation%20Forest%20Anomaly%20Detection-orange.svg)]()
[![Twin: React 3D SSE](https://img.shields.io/badge/Digital%20Twin-React%203D%20%7C%20SSE-61dafb.svg)]()

A complete end-to-end edge-computing industrial telemetry and predictive maintenance infrastructure. Replicated and enhanced based on the open industrial monitoring paradigm ([`Ankitnotnani/Smart-Industrial-Machine-Monitoring`](https://github.com/Ankitnotnani/Smart-Industrial-Machine-Monitoring)).

The system pairs **on-device ESP32 FFT vibration analysis** with multi-sensor environmental condition monitoring (surface temperature via DS18B20 1-Wire, ambient temperature/humidity via DHT22), streaming compressed spectral features to a **FastAPI backend** running **Isolation Forest unsupervised anomaly detection**, synchronized with an interactive **3D React Digital Twin** of 6 industrial machine types via Server-Sent Events (SSE).

---

## 1. System Architecture

```
                 Industrial Machine Unit (CNC / Lathe / Motor)
                       │                 │                 │
             Vibration │   Surface Temp  │    Ambient Env  │
                       ▼                 ▼                 ▼
                    MPU6050           DS18B20            DHT22
                  (6-Axis IMU)      (1-Wire Bus)      (Single Bus)
                       │                 │                 │
                       └───────────────┬─┴─────────────────┘
                                       │
                                       ▼
                       ┌───────────────────────────────┐
                       │       ESP32 Edge Node         │
                       │ 128-Point FFT Processing      │
                       │ Extract Dominant Freq & Mag   │
                       └───────────────┬───────────────┘
                                       │
                      Compressed JSON  │ (>90% payload reduction)
                      HTTP POST        ▼
                       ┌───────────────────────────────┐
                       │     FastAPI Cloud/Edge API    │
                       │  Isolation Forest ML Engine   │
                       └───────────────┬───────────────┘
                                       │
                       Server-Sent     │ Real-time telemetry
                       Events (SSE)    ▼
                       ┌───────────────────────────────┐
                       │ 3D React Digital Twin Client  │
                       │ Live Status of 6 Machines     │
                       └───────────────────────────────┘
```

---

## 2. On-Device Vibration Signal Processing

### Bandwidth Bottleneck & Edge Solution
Continuously streaming uncompressed 3-axis vibration samples at $1\text{ kHz}$ generates over $6000\text{ bytes/second}$ per machine node, choking factory Wi-Fi networks.

Instead, the ESP32 performs on-device spectral analysis using a 128-point FFT with Hamming windowing:
1. Buffers 128 vibration acceleration samples.
2. Applies a Hamming window to minimize spectral leakage.
3. Computes the real Fast Fourier Transform (FFT).
4. Extracts only the **dominant frequency (Hz)** and **peak magnitude ($g$)**.

This reduces network transmission overhead by **$>90\%$**, turning high-frequency vibration signals into a 5-variable structured telemetry payload.

---

## 3. Unsupervised Anomaly Detection (Isolation Forest)

Industrial equipment failures are rare and diverse (bearing wear, imbalance, shaft misalignment), making traditional supervised classification impractical.

The backend employs an **Isolation Forest** unsupervised learning model:
- Trained on baseline healthy nominal operating cycles.
- Evaluates multi-dimensional parameter vectors:
  $$X = [f_{\text{dominant}}, M_{\text{peak}}, T_{\text{surface}}, T_{\text{ambient}}, H_{\text{ambient}}]$$
- Isolates anomalous data points deeper in the partition trees, producing real-time anomaly severity scores ($0 \to 1$) without requiring prior fault labels.

---

## 4. Hardware Wiring & Setup

| Sensor / Module | Interface | ESP32 GPIO | Measurement |
| :--- | :---: | :---: | :--- |
| **MPU6050** | I²C Serial Data (SDA) | `GPIO 21` | High-frequency mechanical vibration acceleration |
| **MPU6050** | I²C Serial Clock (SCL) | `GPIO 22` | Synchronous clock line |
| **DS18B20** | 1-Wire Digital Bus | `GPIO 4` | Machine chassis / casing contact surface temperature |
| **DHT22** | Digital Single-Wire | `GPIO 15` | Factory ambient temperature & relative humidity |

---

## 5. How to Run

### 1. Start the FastAPI Anomaly Engine
```bash
cd backend
pip install -r requirements.txt # fastapi, uvicorn, sse-starlette, scikit-learn, numpy
python app.py
# Server listening on http://0.0.0.0:8000
```

### 2. Flash ESP32 Firmware
- Open [`firmware/esp32_fft_machine_monitoring.ino`](firmware/esp32_fft_machine_monitoring.ino) in Arduino IDE.
- Required Libraries: `arduinoFFT`, `OneWire`, `DallasTemperature`, `DHT sensor library`.
- Update Wi-Fi SSID, Password, and target API URL.
- Flash to ESP32 at **115200 baud**.

---

## 6. Repository File Structure

```text
Smart-Industrial-Machine-Monitoring/
├── README.md
├── .gitignore
│
├── firmware/
│   └── esp32_fft_machine_monitoring.ino   # Edge 128-pt FFT and sensor acquisition firmware
│
├── backend/
│   └── app.py                             # FastAPI anomaly detection & SSE streaming server
│
└── docs/
    └── architecture_spec.md               # Detailed digital twin telemetry schema
```
