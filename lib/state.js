// Manager de Estado e Histórico de Irrigação no Upstash Redis
const REDIS_URL = process.env.UPSTASH_REDIS_REST_URL || 'https://on-phoenix-191786.upstash.io';
const REDIS_TOKEN = process.env.UPSTASH_REDIS_REST_TOKEN || 'gQAAAAAAAu0qAAIgcDI3YTIyODQzNTQ2NWE0MjdhODExMDQ4NDVhYWQyNmExYw';

const KEY_WATER   = 'esp32:water';       
const KEY_HISTORY = 'esp32:history_list';

async function redisPipeline(commands) {
  try {
    const res = await fetch(`${REDIS_URL}/pipeline`, {
      method: 'POST',
      headers: {
        Authorization: `Bearer ${REDIS_TOKEN}`,
        'Content-Type': 'application/json'
      },
      body: JSON.stringify(commands),
      cache: 'no-store'
    });
    return await res.json();
  } catch (err) {
    console.error('[REDIS ERROR] Pipeline falhou:', err);
    return [];
  }
}

async function redisGet(key) {
  try {
    const res = await fetch(`${REDIS_URL}/get/${key}`, {
      headers: { Authorization: `Bearer ${REDIS_TOKEN}` },
      cache: 'no-store'
    });
    const data = await res.json();
    return data.result;
  } catch (err) {
    return null;
  }
}

async function redisGetJson(key) {
  const raw = await redisGet(key);
  if (!raw) return null;
  try { return JSON.parse(raw); } catch { return null; }
}

export async function getStateAsync() {
  const [water, history] = await Promise.all([
    redisGet(KEY_WATER),
    redisGetJson(KEY_HISTORY)
  ]);

  return {
    waterRequested: water === 'true',
    history: Array.isArray(history) ? history : []
  };
}

export async function recordWateringEvent(eventData) {
  const state = await getStateAsync();
  const currentHistory = state.history || [];

  const newEntry = {
    id: Date.now(),
    rtcTime: eventData.rtcTime || new Date().toLocaleString('pt-BR'),
    durationSec: Number(eventData.durationSec) || 15,
    source: eventData.source || 'RTC Agendado',
    batteryVoltage: Number(eventData.batteryVoltage) || 4.14,
    batteryPct: Number(eventData.batteryPct) || 94,
    serverTimestamp: new Date().toISOString()
  };

  const updatedHistory = [newEntry, ...currentHistory].slice(0, 50);

  await redisPipeline([
    ['SET', KEY_HISTORY, JSON.stringify(updatedHistory)],
    ['SET', KEY_WATER, 'false']
  ]);

  return updatedHistory;
}

export async function clearHistory() {
  await redisPipeline([['SET', KEY_HISTORY, JSON.stringify([])]]);
  return [];
}
