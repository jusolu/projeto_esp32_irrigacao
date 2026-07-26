import { NextResponse } from 'next/server';
import { requestWatering, clearWateringRequest, getStateAsync, syncToRemote } from '@/lib/state';

export async function POST(request) {
  try {
    const body = await request.json();
    
    if (body.action === 'stop') {
      clearWateringRequest();
      await syncToRemote();
      const state = await getStateAsync();
      return NextResponse.json({ success: true, message: 'Rega cancelada.', state });
    }
    
    const duration = body.durationSec || body.duration || 10;
    requestWatering(duration);
    await syncToRemote();
    const state = await getStateAsync();
    
    return NextResponse.json({
      success: true,
      message: `Comando de rega enviado com sucesso (${duration}s)!`,
      state
    });
  } catch (error) {
    return NextResponse.json(
      { success: false, error: 'Erro ao processar requisição de rega' },
      { status: 400 }
    );
  }
}
