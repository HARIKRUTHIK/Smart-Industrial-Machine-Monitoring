# =============================================================================
# FastAPI Backend & Isolation Forest Anomaly Detection Engine
# Smart Industrial Machine Monitoring & Predictive Maintenance
# =============================================================================

import time
import asyncio
from typing import Dict, Any
from fastapi import FastAPI, Request
from fastapi.middleware.cors import CORSMiddleware
from sse_starlette.sse import EventSourceResponse
from pydantic import BaseModel
import numpy as np
from sklearn.ensemble import IsolationForest

app = FastAPI(title="Industrial Predictive Maintenance API", version="1.0.0")

app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)

# Unsupervised Anomaly Detection Model (Pre-fitted on nominal healthy baseline)
# Features: [dominant_freq, peak_mag, surface_temp, ambient_temp, ambient_humidity]
nominal_data = np.array([
    [24.5, 0.12, 42.0, 26.5, 55.0],
    [25.0, 0.14, 43.1, 27.0, 54.0],
    [24.8, 0.11, 41.8, 26.2, 56.2],
    [25.2, 0.15, 43.5, 27.3, 53.8],
    [24.9, 0.13, 42.6, 26.8, 55.4]
])
anomaly_detector = IsolationForest(contamination=0.05, random_state=42)
anomaly_detector.fit(nominal_data)

# In-memory latest telemetry state for 6 monitored industrial machines
machine_states: Dict[str, Dict[str, Any]] = {
    f"machine_{i}": {
        "machine_id": f"machine_{i}",
        "type": ["CNC Milling", "Lathe", "Centrifugal Pump", "Induction Motor", "Compressor", "Gearbox"][i],
        "dominant_freq": 25.0,
        "peak_mag": 0.13,
        "surface_temp": 42.5,
        "ambient_temp": 26.8,
        "ambient_humidity": 55.0,
        "anomaly_status": "NORMAL",
        "anomaly_score": 0.12,
        "last_updated": time.time()
    } for i in range(6)
}

class TelemetryPayload(BaseModel):
    machine_id: str
    dominant_freq: float
    peak_mag: float
    surface_temp: float
    ambient_temp: float
    ambient_humidity: float

@app.post("/api/telemetry")
async def ingest_telemetry(payload: TelemetryPayload):
    features = np.array([[
        payload.dominant_freq,
        payload.peak_mag,
        payload.surface_temp,
        payload.ambient_temp,
        payload.ambient_humidity
    ]])

    # Isolation Forest: -1 = Anomaly, 1 = Normal
    prediction = anomaly_detector.predict(features)[0]
    score = float(-anomaly_detector.score_samples(features)[0]) # Higher score = more anomalous

    status = "WARNING - ANOMALY DETECTED" if prediction == -1 else "NORMAL"

    machine_states[payload.machine_id] = {
        "machine_id": payload.machine_id,
        "type": machine_states.get(payload.machine_id, {}).get("type", "Industrial Unit"),
        "dominant_freq": payload.dominant_freq,
        "peak_mag": payload.peak_mag,
        "surface_temp": payload.surface_temp,
        "ambient_temp": payload.ambient_temp,
        "ambient_humidity": payload.ambient_humidity,
        "anomaly_status": status,
        "anomaly_score": round(score, 3),
        "last_updated": time.time()
    }

    return {"status": "success", "machine_status": status, "anomaly_score": score}

@app.get("/api/stream")
async def stream_telemetry(request: Request):
    """Server-Sent Events (SSE) streaming real-time digital twin updates."""
    async def event_generator():
        while True:
            if await request.is_disconnected():
                break
            yield {
                "event": "machine_update",
                "data": machine_states
            }
            await asyncio.sleep(1.0)
    return EventSourceResponse(event_generator())

if __name__ == "__main__":
    import uvicorn
    uvicorn.run(app, host="0.0.0.0", port=8000)
