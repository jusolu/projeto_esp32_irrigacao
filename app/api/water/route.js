import { NextResponse } from 'next/server';
import { requestWatering, clearWateringRequest, getStateAsync } from '@/lib/state';

export const dynamic = 'force-dynamic';

export async function POST(request) {
  try {
    const body = await request.json();
    console.log('[WATER] POST recebido:', JSON.stringify(body));

    let state;
    if (body.action === 'stop') {
      state = await clearWateringRequest();
      return NextResponse.json({ success: true, message: 'Rega cancelada.', state });
    }

    const duration = body.durationSec || body.duration || 10;
    state = await requestWatering(duration);
    console.log('[WATER] waterRequested agora:', state.waterRequested);

    return NextResponse.json({
      success: true,
      message: `Comando de rega enviado (${duration}s)!`,
      state
    });
  } catch (error) {
    console.error('[WATER] Erro:', error.message);
    return NextResponse.json(
      { success: false, error: 'Erro ao processar requisição de rega' },
      { status: 400 }
    );
  }
}
