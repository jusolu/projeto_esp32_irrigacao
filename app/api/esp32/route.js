import { NextResponse } from 'next/server';
import { getStateAsync, updateTelemetry, clearWateringRequest } from '@/lib/state';

export const dynamic = 'force-dynamic';

// GET /api/esp32 -> O ESP32 consulta se deve regar
export async function GET() {
  const state = await getStateAsync();
  return NextResponse.json({
    waterRequested: state.waterRequested,
    durationSec: state.durationSec,
    serverTime: new Date().toISOString()
  });
}

// POST /api/esp32 -> ESP32 envia telemetria e recebe instrução de rega
export async function POST(request) {
  try {
    const body = await request.json();
    console.log('[ESP32] POST recebido:', JSON.stringify(body));

    let state;
    if (body.waterCompleted) {
      console.log('[ESP32] Rega concluída - limpando request');
      state = await clearWateringRequest();
    } else {
      state = await updateTelemetry({
        soilMoisture: body.soilMoisture,
        temperature: body.temperature,
        humidity: body.humidity,
        batteryVoltage: body.batteryVoltage,
        rawAnalog: body.rawAnalog
      });
    }

    console.log('[ESP32] Respondendo waterRequested:', state.waterRequested);
    return NextResponse.json({
      success: true,
      waterRequested: state.waterRequested,
      durationSec: state.durationSec,
      timestamp: new Date().toISOString()
    });
  } catch (error) {
    console.error('[ESP32] Erro:', error.message);
    return NextResponse.json(
      { success: false, error: 'Formato de JSON inválido' },
      { status: 400 }
    );
  }
}
