import { NextResponse } from 'next/server';
import { getStateAsync, getConfigAsync, recordWateringEvent } from '@/lib/state';

export const dynamic = 'force-dynamic';

// GET /api/esp32 -> Retorna a configuração atual e status para o ESP32
export async function GET() {
  const config = await getConfigAsync();
  return NextResponse.json({
    targetDurationSec: config.durationSec || 45,
    serverTime: new Date().toISOString()
  });
}

// POST /api/esp32 -> ESP32 envia a confirmação de rega e recebe a duração dinâmica
export async function POST(request) {
  try {
    const body = await request.json();
    console.log('[API ESP32] POST recebido:', JSON.stringify(body));

    const config = await getConfigAsync();

    if (body.waterCompleted || body.durationSec) {
      const history = await recordWateringEvent({
        rtcTime: body.rtcTime || body.timestamp,
        durationSec: body.durationSec || config.durationSec || 45,
        source: body.source || 'RTC Agendado'
      });

      return NextResponse.json({
        success: true,
        message: 'Evento gravado no histórico com sucesso!',
        targetDurationSec: config.durationSec || 45,
        totalEvents: history.length
      });
    }

    const state = await getStateAsync();
    return NextResponse.json({ 
      success: true, 
      targetDurationSec: config.durationSec || 45,
      state 
    });
  } catch (error) {
    console.error('[API ESP32] Erro:', error.message);
    return NextResponse.json(
      { success: false, error: 'JSON inválido' },
      { status: 400 }
    );
  }
}
