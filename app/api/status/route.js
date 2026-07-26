import { NextResponse } from 'next/server';
import { getStateAsync } from '@/lib/state';

export async function GET() {
  const state = await getStateAsync();
  return NextResponse.json(state, {
    headers: {
      'Cache-Control': 'no-store, max-age=0'
    }
  });
}
