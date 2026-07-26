// State manager usando Upstash Redis para persistência real entre instâncias Vercel
// Redis HTTP REST API - sem TCP, funciona 100% em Serverless

const REDIS_URL = process.env.UPSTASH_REDIS_REST_URL || 'https://on-phoenix-191786.upstash.io';
const REDIS_TOKEN = process.env.UPSTASH_REDIS_REST_TOKEN || 'gQAAAAAAAu0qAAIgcDI3YTIyODQzNTQ2NWE0MjdhODExMDQ4NDVhYWQyNmExYw';
const STATE_KEY = 'esp32:state';

const DEFAULT_STATE = {
  waterRequested: false,
  durationSec: 10,
  lastWateredAt: null,
  lastUpdated: new Date().toISOString(),
  telemetry: {
    soilMoisture: 50,
    temperature: 25.0,
    humidity: 60.0,
    batteryVoltage: 4.1,
    rawAnalog: 2150,
    deviceConnected: false,
    updatedAt: null
  },
  logs: [
    {
      id: 1,
      timestamp: new Date().toISOString(),
      type: 'system',
      message: 'Sistema de Irrigação inicializado com Upstash Redis.'
    }
  ]
};

async function redisGet(key) {
  const res = await fetch(`${REDIS_URL}/get/${key}`, {
    headers: { Authorization: `Bearer ${REDIS_TOKEN}` },
    cache: 'no-store'
  });
  const data = await res.json();
  if (!data.result) return null;
  try {
    return JSON.parse(data.result);
  } catch {
    return null;
  }
}

async function redisSet(key, value) {
  const encoded = encodeURIComponent(JSON.stringify(value));
  await fetch(`${REDIS_URL}/set/${key}/${encoded}`, {
    headers: { Authorization: `Bearer ${REDIS_TOKEN}` },
    cache: 'no-store'
  });
}

export async function getStateAsync() {
  try {
    const state = await redisGet(STATE_KEY);
    if (state) return state;
  } catch (err) {
    console.error('[REDIS] Erro ao buscar estado:', err.message);
  }
  // Inicializa no Redis se não existir
  await redisSet(STATE_KEY, DEFAULT_STATE).catch(() => {});
  return { ...DEFAULT_STATE };
}

export function getState() {
  // Versão síncrona para compatibilidade - retorna DEFAULT, use getStateAsync quando possível
  return { ...DEFAULT_STATE };
}

export async function requestWatering(duration = 10) {
  const state = await getStateAsync();
  state.waterRequested = true;
  state.durationSec = Number(duration) || 10;
  state.lastUpdated = new Date().toISOString();
  addLogToState(state, 'user', `Rega solicitada (${state.durationSec}s).`);
  console.log(`[REDIS] SET waterRequested=true dur=${state.durationSec}s`);
  await redisSet(STATE_KEY, state);
  return state;
}

export async function clearWateringRequest() {
  const state = await getStateAsync();
  state.waterRequested = false;
  state.lastWateredAt = new Date().toISOString();
  state.lastUpdated = new Date().toISOString();
  addLogToState(state, 'esp32', `Rega concluída pelo ESP32.`);
  console.log(`[REDIS] SET waterRequested=false`);
  await redisSet(STATE_KEY, state);
  return state;
}

export async function updateTelemetry(data) {
  const state = await getStateAsync();
  if (data.soilMoisture !== undefined) state.telemetry.soilMoisture = Number(data.soilMoisture);
  if (data.temperature !== undefined) state.telemetry.temperature = Number(data.temperature);
  if (data.humidity !== undefined) state.telemetry.humidity = Number(data.humidity);
  if (data.batteryVoltage !== undefined) state.telemetry.batteryVoltage = Number(data.batteryVoltage);
  if (data.rawAnalog !== undefined) state.telemetry.rawAnalog = Number(data.rawAnalog);
  state.telemetry.deviceConnected = true;
  state.telemetry.updatedAt = new Date().toISOString();
  addLogToState(state, 'esp32', `Telemetria: Solo ${state.telemetry.soilMoisture}%, Temp ${state.telemetry.temperature}°C`);
  await redisSet(STATE_KEY, state);
  return state;
}

function addLogToState(state, type, message) {
  state.logs.unshift({
    id: Date.now() + Math.random(),
    timestamp: new Date().toISOString(),
    type,
    message
  });
  if (state.logs.length > 30) state.logs = state.logs.slice(0, 30);
}

// Retrocompatibilidade
export function addLog(type, message) {}
export async function syncToRemote() {}
export async function fetchFromRemote() {}
