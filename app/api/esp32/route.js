import { NextResponse } from 'next/server';
import { getStateAsync, recordWateringEvent } from '@/lib/state';

export const dynamic = 'force-dynamic';

// GET /api/esp32
export async function GET() {
  const state = await getStateAsync();
  return NextResponse.json({
    waterRequested: false,
    serverTime: new Date().toISOString()
  });
}

// POST /api/esp32 -> ESP32 envia a confirmação de rega e nível da bateria
export async function POST(request) {
  try {
    const body = await request.json();
    console.log('[API ESP32] POST recebido:', JSON.stringify(body));

    if (body.waterCompleted || body.durationSec) {
      const history = await recordWateringEvent({
        rtcTime: body.rtcTime || body.timestamp,
        durationSec: body.durationSec || 15,
        source: body.source || 'RTC Agendado',
        batteryVoltage: body.batteryVoltage || 4.14,
        batteryPct: body.batteryPct || 94
      });

      return NextResponse.json({
        success: true,
        message: 'Evento gravado no histórico com telemetria da bateria!',
        totalEvents: history.length
      });
    }

    const state = await getStateAsync();
    return NextResponse.json({ success: true, state });
  } catch (error) {
    console.error('[API ESP32] Erro:', error.message);
    return NextResponse.json(
      { success: false, error: 'JSON inválido' },
      { status: 400 }
    );
  }
}
