import { NextResponse } from 'next/server';
import { getStateAsync, recordWateringEvent } from '@/lib/state';

export const dynamic = 'force-dynamic';

// GET /api/esp32 -> ESP32 consulta se há acionamento manual pendente
export async function GET() {
  const state = await getStateAsync();
  return NextResponse.json({
    waterRequested: state.waterRequested,
    durationSec: state.durationSec,
    serverTime: new Date().toISOString()
  });
}

// POST /api/esp32 -> ESP32 envia a confirmação e horário da rega realizada
export async function POST(request) {
  try {
    const body = await request.json();
    console.log('[API ESP32] POST recebido:', JSON.stringify(body));

    if (body.waterCompleted || body.durationSec) {
      const history = await recordWateringEvent({
        rtcTime: body.rtcTime || body.timestamp,
        durationSec: body.durationSec || 15,
        source: body.source || (body.cycle ? `Ciclo #${body.cycle}` : 'RTC Agendado')
      });

      return NextResponse.json({
        success: true,
        message: 'Evento de irrigação gravado com sucesso no histórico!',
        totalEvents: history.length,
        waterRequested: false
      });
    }

    const state = await getStateAsync();
    return NextResponse.json({
      success: true,
      waterRequested: state.waterRequested,
      durationSec: state.durationSec
    });
  } catch (error) {
    console.error('[API ESP32] Erro no processamento:', error.message);
    return NextResponse.json(
      { success: false, error: 'Formato de JSON inválido' },
      { status: 400 }
    );
  }
}
