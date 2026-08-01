import { NextResponse } from 'next/server';
import { getStateAsync } from '@/lib/state';

export const dynamic = 'force-dynamic';

export async function GET() {
  const state = await getStateAsync();
  return NextResponse.json(state);
}
