// Manager de Estado e Histórico de Irrigação no Upstash Redis
const REDIS_URL = process.env.UPSTASH_REDIS_REST_URL || 'https://on-phoenix-191786.upstash.io';
const REDIS_TOKEN = process.env.UPSTASH_REDIS_REST_TOKEN || 'gQAAAAAAAu0qAAIgcDI3YTIyODQzNTQ2NWE0MjdhODExMDQ4NDVhYWQyNmExYw';

const KEY_WATER   = 'esp32:water';       // "true" ou "false"
const KEY_DUR     = 'esp32:duration';    // duração solicitada em segundos
const KEY_HISTORY = 'esp32:history_list';// JSON array de histórico de regas

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
    console.error('[REDIS ERROR] Get falhou:', err);
    return null;
  }
}

async function redisGetJson(key) {
  const raw = await redisGet(key);
  if (!raw) return null;
  try { return JSON.parse(raw); } catch { return null; }
}

// Retorna o estado completo incluindo a lista de histórico de regas
export async function getStateAsync() {
  const [water, dur, history] = await Promise.all([
    redisGet(KEY_WATER),
    redisGet(KEY_DUR),
    redisGetJson(KEY_HISTORY)
  ]);

  return {
    waterRequested: water === 'true',
    durationSec: dur ? Number(dur) : 15,
    history: Array.isArray(history) ? history : []
  };
}

// Adiciona um novo registro de rega no histórico
export async function recordWateringEvent(eventData) {
  const state = await getStateAsync();
  const currentHistory = state.history || [];

  const newEntry = {
    id: Date.now(),
    rtcTime: eventData.rtcTime || new Date().toLocaleString('pt-BR'),
    durationSec: Number(eventData.durationSec) || 15,
    source: eventData.source || 'RTC Agendado',
    serverTimestamp: new Date().toISOString()
  };

  // Mantém os últimos 50 registros no histórico
  const updatedHistory = [newEntry, ...currentHistory].slice(0, 50);

  await redisPipeline([
    ['SET', KEY_HISTORY, JSON.stringify(updatedHistory)],
    ['SET', KEY_WATER, 'false'] // Limpa a flag de requisição manual se houver
  ]);

  return updatedHistory;
}

// Solicita uma rega manual pelo Dashboard
export async function requestWatering(duration = 15) {
  const dur = Number(duration) || 15;
  await redisPipeline([
    ['SET', KEY_WATER, 'true'],
    ['SET', KEY_DUR, String(dur)]
  ]);
  return await getStateAsync();
}

// Cancela ou reseta requisições de rega
export async function clearWateringRequest() {
  await redisPipeline([['SET', KEY_WATER, 'false']]);
  return await getStateAsync();
}

// Limpa todo o histórico de regas
export async function clearHistory() {
  await redisPipeline([['SET', KEY_HISTORY, JSON.stringify([])]]);
  return [];
}
