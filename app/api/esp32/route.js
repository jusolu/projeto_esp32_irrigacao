import { NextResponse } from 'next/server';
import { getStateAsync, updateTelemetry, clearWateringRequest, syncToRemote } from '@/lib/state';

// GET /api/esp32 -> O ESP32 pode consultar se precisa acionar a bomba
export async function GET() {
  const state = await getStateAsync();
  return NextResponse.json({
    waterRequested: state.waterRequested,
    durationSec: state.durationSec,
    serverTime: new Date().toISOString()
  });
}

// POST /api/esp32 -> O ESP32 envia dados dos sensores e lê a instrução de rega
export async function POST(request) {
  try {
    const body = await request.json();
    
    // Se o ESP32 informou que terminou uma rega
    if (body.waterCompleted) {
      clearWateringRequest();
    }
    
    // Atualiza telemetria dos sensores enviados pelo ESP32
    updateTelemetry({
      soilMoisture: body.soilMoisture,
      temperature: body.temperature,
      humidity: body.humidity,
      batteryVoltage: body.batteryVoltage,
      rawAnalog: body.rawAnalog
    });
    
    await syncToRemote();
    const state = await getStateAsync();
    
    // Resposta para o ESP32 informando se deve regar agora
    return NextResponse.json({
      success: true,
      waterRequested: state.waterRequested,
      durationSec: state.durationSec,
      timestamp: new Date().toISOString()
    });
  } catch (error) {
    return NextResponse.json(
      { success: false, error: 'Formato de JSON inválido' },
      { status: 400 }
    );
  }
}
