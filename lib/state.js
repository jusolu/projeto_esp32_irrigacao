// Persistent State Manager for Vercel Serverless & Local Dev
// Uses npoint.io remote JSON bin to synchronize state across Vercel Lambdas

const NPOINT_URL = 'https://api.npoint.io/512b9bf227ca7ffaa3c3';

if (!globalThis._esp32State) {
  globalThis._esp32State = {
    waterRequested: false,
    durationSec: 10,
    lastWateredAt: null,
    telemetry: {
      soilMoisture: 50,      // Valor em % (0 = seco, 100 = molhado)
      temperature: 25.0,     // °C
      humidity: 60.0,        // %
      batteryVoltage: 4.1,   // Volts (Célula 18650)
      rawAnalog: 2150,       // Leitura ADC do sensor capacitivo
      deviceConnected: true,
      updatedAt: new Date().toISOString()
    },
    logs: [
      {
        id: 1,
        timestamp: new Date().toISOString(),
        type: 'system',
        message: 'Sistema de Irrigação Automatizado inicializado no Vercel (npoint persistence active).'
      }
    ]
  };
}

export async function syncToRemote() {
  try {
    await fetch(NPOINT_URL, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify(globalThis._esp32State)
    });
  } catch (err) {
    console.error('Erro ao sincronizar com npoint:', err);
  }
}

export async function fetchFromRemote() {
  try {
    const res = await fetch(NPOINT_URL, { cache: 'no-store' });
    if (res.ok) {
      const data = await res.json();
      if (data && typeof data.waterRequested === 'boolean') {
        globalThis._esp32State = { ...globalThis._esp32State, ...data };
      }
    }
  } catch (err) {
    console.error('Erro ao buscar de npoint:', err);
  }
}

export async function getStateAsync() {
  await fetchFromRemote();
  return globalThis._esp32State;
}

export function getState() {
  fetchFromRemote().catch(() => {});
  return globalThis._esp32State;
}

export function requestWatering(duration = 10) {
  const state = globalThis._esp32State;
  state.waterRequested = true;
  state.durationSec = Number(duration) || 10;
  
  addLog('user', `Rega manual solicitada via Dashboard (${state.durationSec} segundos).`);
  syncToRemote().catch(() => {});
  return state;
}

export function clearWateringRequest() {
  const state = globalThis._esp32State;
  state.waterRequested = false;
  state.lastWateredAt = new Date().toISOString();
  addLog('esp32', `Rega concluída com sucesso pelo ESP32.`);
  syncToRemote().catch(() => {});
  return state;
}

export function updateTelemetry(data) {
  const state = globalThis._esp32State;
  
  if (data.soilMoisture !== undefined) state.telemetry.soilMoisture = Number(data.soilMoisture);
  if (data.temperature !== undefined) state.telemetry.temperature = Number(data.temperature);
  if (data.humidity !== undefined) state.telemetry.humidity = Number(data.humidity);
  if (data.batteryVoltage !== undefined) state.telemetry.batteryVoltage = Number(data.batteryVoltage);
  if (data.rawAnalog !== undefined) state.telemetry.rawAnalog = Number(data.rawAnalog);
  
  state.telemetry.deviceConnected = true;
  state.telemetry.updatedAt = new Date().toISOString();
  
  addLog('esp32', `Telemetria atualizada: Solo ${state.telemetry.soilMoisture}%, Temp ${state.telemetry.temperature}°C, Bat ${state.telemetry.batteryVoltage}V`);
  syncToRemote().catch(() => {});
  return state;
}

export function addLog(type, message) {
  const state = globalThis._esp32State;
  state.logs.unshift({
    id: Date.now() + Math.random(),
    timestamp: new Date().toISOString(),
    type,
    message
  });
  
  if (state.logs.length > 30) {
    state.logs = state.logs.slice(0, 30);
  }
}
