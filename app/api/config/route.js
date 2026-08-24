import { NextResponse } from 'next/server';
import { getConfigAsync, setConfigAsync } from '@/lib/state';

export const dynamic = 'force-dynamic';

export async function GET() {
  const config = await getConfigAsync();
  return NextResponse.json(config);
}

export async function POST(request) {
  try {
    const body = await request.json();
    const durationSec = Number(body.durationSec);
    if (!durationSec || durationSec < 5 || durationSec > 300) {
      return NextResponse.json(
        { success: false, error: 'Duração deve ser entre 5 e 300 segundos.' },
        { status: 400 }
      );
    }

    const updated = await setConfigAsync({ durationSec });
    return NextResponse.json({ success: true, config: updated });
  } catch (error) {
    return NextResponse.json(
      { success: false, error: error.message },
      { status: 400 }
    );
  }
}
