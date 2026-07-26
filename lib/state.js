// State manager com Upstash Redis
// waterRequested e durationSec ficam em chaves simples separadas para evitar race conditions
// O estado completo (telemetria, logs) fica em chave separada

const REDIS_URL = process.env.UPSTASH_REDIS_REST_URL || 'https://on-phoenix-191786.upstash.io';
const REDIS_TOKEN = process.env.UPSTASH_REDIS_REST_TOKEN || 'gQAAAAAAAu0qAAIgcDI3YTIyODQzNTQ2NWE0MjdhODExMDQ4NDVhYWQyNmExYw';

// Chaves separadas no Redis
const KEY_WATER = 'esp32:water';      // "true" ou "false"
const KEY_DUR   = 'esp32:duration';   // número em string
const KEY_TELEM = 'esp32:telemetry';  // JSON de telemetria
const KEY_LOGS  = 'esp32:logs';       // JSON de logs

async function redisPipeline(commands) {
  const res = await fetch(`${REDIS_URL}/pipeline`, {
    method: 'POST',
    headers: {
      Authorization: `Bearer ${REDIS_TOKEN}`,
      'Content-Type': 'application/json'
    },
    body: JSON.stringify(commands),
    cache: 'no-store'
  });
  return res.json();
}

async function redisGet(key) {
  const res = await fetch(`${REDIS_URL}/get/${key}`, {
    headers: { Authorization: `Bearer ${REDIS_TOKEN}` },
    cache: 'no-store'
  });
  const data = await res.json();
  return data.result;
}

async function redisSet(key, value) {
  await redisPipeline([['SET', key, String(value)]]);
}

async function redisSetJson(key, value) {
  await redisPipeline([['SET', key, JSON.stringify(value)]]);
}

async function redisGetJson(key) {
  const raw = await redisGet(key);
  if (!raw) return null;
  try { return JSON.parse(raw); } catch { return null; }
}

// =============================================
// API PÚBLICA
// =============================================

export async function getStateAsync() {
  const [water, dur, telem, logs] = await Promise.all([
    redisGet(KEY_WATER),
    redisGet(KEY_DUR),
    redisGetJson(KEY_TELEM),
    redisGetJson(KEY_LOGS)
  ]);

  return {
    waterRequested: water === 'true',
    durationSec: dur ? Number(dur) : 10,
    telemetry: telem || {
      soilMoisture: 50, temperature: 25, humidity: 60,
      batteryVoltage: 4.1, rawAnalog: 2150, deviceConnected: false, updatedAt: null
    },
    logs: logs || [{ id: 1, timestamp: new Date().toISOString(), type: 'system', message: 'Sistema iniciado.' }],
    lastWateredAt: null
  };
}

export function getState() {
  return {
    waterRequested: false, durationSec: 10,
    telemetry: { soilMoisture: 50, temperature: 25, humidity: 60, batteryVoltage: 4.1, deviceConnected: false, updatedAt: null },
    logs: []
  };
}

export async function requestWatering(duration = 10) {
  const dur = Number(duration) || 10;
  console.log(`[REDIS] SET ${KEY_WATER}=true dur=${dur}`);
  // Atualiza as duas chaves atomicamente
  await redisPipeline([
    ['SET', KEY_WATER, 'true'],
    ['SET', KEY_DUR, String(dur)]
  ]);
  const state = await getStateAsync();
  await _appendLog(state, 'user', `Rega solicitada (${dur}s).`);
  return state;
}

export async function clearWateringRequest() {
  console.log(`[REDIS] SET ${KEY_WATER}=false`);
  await redisSet(KEY_WATER, 'false');
  const state = await getStateAsync();
  await _appendLog(state, 'esp32', 'Rega concluída pelo ESP32.');
  return state;
}

export async function updateTelemetry(data) {
  const telem = (await redisGetJson(KEY_TELEM)) || {};
  if (data.soilMoisture !== undefined) telem.soilMoisture = Number(data.soilMoisture);
  if (data.temperature !== undefined) telem.temperature = Number(data.temperature);
  if (data.humidity !== undefined) telem.humidity = Number(data.humidity);
  if (data.batteryVoltage !== undefined) telem.batteryVoltage = Number(data.batteryVoltage);
  if (data.rawAnalog !== undefined) telem.rawAnalog = Number(data.rawAnalog);
  telem.deviceConnected = true;
  telem.updatedAt = new Date().toISOString();
  await redisSetJson(KEY_TELEM, telem);
  const state = await getStateAsync();
  await _appendLog(state, 'esp32', `Telemetria: Solo ${telem.soilMoisture}%, Temp ${telem.temperature}°C`);
  return state;
}

async function _appendLog(state, type, message) {
  const logs = state.logs || [];
  logs.unshift({ id: Date.now() + Math.random(), timestamp: new Date().toISOString(), type, message });
  if (logs.length > 30) logs.length = 30;
  await redisSetJson(KEY_LOGS, logs);
}

// Retrocompatibilidade
export function addLog() {}
export async function syncToRemote() {}
export async function fetchFromRemote() {}
