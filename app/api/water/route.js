import { NextResponse } from 'next/server';
import { requestWatering, clearWateringRequest, clearHistory } from '@/lib/state';

export const dynamic = 'force-dynamic';

export async function POST(request) {
  try {
    const body = await request.json();
    
    if (body.action === 'clear_history') {
      await clearHistory();
      return NextResponse.json({ success: true, message: 'Histórico limpo com sucesso.' });
    }

    if (body.action === 'stop') {
      const state = await clearWateringRequest();
      return NextResponse.json({ success: true, state });
    }

    const durationSec = Number(body.durationSec) || 15;
    const state = await requestWatering(durationSec);
    return NextResponse.json({ success: true, state });
  } catch (error) {
    return NextResponse.json(
      { success: false, error: error.message },
      { status: 400 }
    );
  }
}
