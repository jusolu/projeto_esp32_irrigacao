// Shared global state for Vercel Serverless environment
// Uses globalThis to survive hot reload & function warm starts

if (!globalThis._esp32State) {
  globalThis._esp32State = {
    waterRequested: false,
    durationSec: 10,
    lastWateredAt: null,
    telemetry: {
      soilMoisture: 42,      // Valor em % (0 = seco, 100 = molhado)
      temperature: 24.5,     // °C
      humidity: 65.0,        // %
      batteryVoltage: 3.95,  // Volts (Célula 18650)
      rawAnalog: 2150,       // Leitura ADC do sensor capacitivo
      deviceConnected: true,
      updatedAt: new Date().toISOString()
    },
    logs: [
      {
        id: 1,
        timestamp: new Date().toISOString(),
        type: 'system',
        message: 'Sistema de Irrigação Automatizado inicializado no Vercel.'
      }
    ]
  };
}

export function getState() {
  return globalThis._esp32State;
}

export function requestWatering(duration = 10) {
  const state = globalThis._esp32State;
  state.waterRequested = true;
  state.durationSec = Number(duration) || 10;
  
  addLog('user', `Rega manual solicitada via Dashboard (${state.durationSec} segundos).`);
  return state;
}

export function clearWateringRequest() {
  const state = globalThis._esp32State;
  state.waterRequested = false;
  state.lastWateredAt = new Date().toISOString();
  addLog('esp32', `Rega concluída com sucesso pelo ESP32.`);
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
  
  // Manter no máximo 30 logs recentes
  if (state.logs.length > 30) {
    state.logs = state.logs.slice(0, 30);
  }
}
