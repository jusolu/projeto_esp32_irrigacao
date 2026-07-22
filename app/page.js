'use client';

import { useState, useEffect } from 'react';
import {
  Droplets,
  Thermometer,
  Wind,
  BatteryCharging,
  Power,
  RefreshCw,
  Clock,
  Activity,
  CheckCircle2,
  AlertCircle
} from 'lucide-react';

export default function Dashboard() {
  const [status, setStatus] = useState(null);
  const [loading, setLoading] = useState(true);
  const [actionLoading, setActionLoading] = useState(false);
  const [selectedDuration, setSelectedDuration] = useState(10);

  // Fetch status da API
  const fetchStatus = async () => {
    try {
      const res = await fetch('/api/status');
      if (res.ok) {
        const data = await res.json();
        setStatus(data);
      }
    } catch (err) {
      console.error('Erro ao buscar status:', err);
    } finally {
      setLoading(false);
    }
  };

  useEffect(() => {
    fetchStatus();
    const interval = setInterval(fetchStatus, 2500); // Polling a cada 2.5s
    return () => clearInterval(interval);
  }, []);

  // Alternar rega (Ativar / Cancelar)
  const handleToggleWater = async () => {
    if (!status) return;
    setActionLoading(true);
    try {
      const isWatering = status.waterRequested;
      const res = await fetch('/api/water', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({
          action: isWatering ? 'stop' : 'start',
          durationSec: selectedDuration
        })
      });
      if (res.ok) {
        await fetchStatus();
      }
    } catch (err) {
      console.error('Erro ao enviar ação de rega:', err);
    } finally {
      setActionLoading(false);
    }
  };

  // Formatação de data/hora
  const formatTime = (isoString) => {
    if (!isoString) return 'Sem registro';
    const date = new Date(isoString);
    return date.toLocaleTimeString('pt-BR', { hour: '2-digit', minute: '2-digit', second: '2-digit' });
  };

  const telemetry = status?.telemetry || {
    soilMoisture: 0,
    temperature: 0,
    humidity: 0,
    batteryVoltage: 0,
    updatedAt: null
  };

  // Cálculo percentual bateria (considerando célula 18650 entre 3.2V e 4.2V)
  const calculateBatteryPercent = (volts) => {
    if (!volts) return 0;
    const minV = 3.2;
    const maxV = 4.2;
    const pct = Math.round(((volts - minV) / (maxV - minV)) * 100);
    return Math.max(0, Math.min(100, pct));
  };

  const batteryPct = calculateBatteryPercent(telemetry.batteryVoltage);

  return (
    <div className="container">
      {/* Header */}
      <header className="header">
        <div className="header-title">
          <Droplets className="w-8 h-8 text-emerald-400" style={{ color: '#10B981' }} size={32} />
          <div>
            <h1>Irrigação ESP32</h1>
            <p style={{ color: 'var(--text-secondary)', fontSize: '0.85rem' }}>
              Servidor Serverless Vercel & Realtime Control
            </p>
          </div>
        </div>

        <div style={{ display: 'flex', gap: '0.75rem', alignItems: 'center' }}>
          {status?.waterRequested ? (
            <span className="badge badge-active">
              <Activity size={14} /> Rega Solicitada ({status.durationSec}s)
            </span>
          ) : (
            <span className="badge badge-online">
              <CheckCircle2 size={14} /> Sistema Pronto
            </span>
          )}

          <button
            onClick={fetchStatus}
            style={{
              background: 'rgba(255,255,255,0.05)',
              border: '1px solid var(--panel-border)',
              color: 'var(--text-secondary)',
              padding: '0.4rem',
              borderRadius: '0.5rem',
              cursor: 'pointer'
            }}
            title="Atualizar agora"
          >
            <RefreshCw size={16} className={loading ? 'animate-spin' : ''} />
          </button>
        </div>
      </header>

      {/* Main Grid Layout */}
      <div className="grid">
        {/* Painel de Controle Principal (Regar Agora) */}
        <div className="card col-4 action-box">
          <div className="card-title" style={{ marginBottom: '1.5rem' }}>
            <Power size={18} /> Ação Manual
          </div>

          <button
            onClick={handleToggleWater}
            disabled={actionLoading}
            className={`water-btn ${status?.waterRequested ? 'active' : ''}`}
          >
            <Droplets size={36} />
            <span>{status?.waterRequested ? 'CANCELAR' : 'REGAR AGORA'}</span>
          </button>

          <p style={{ fontSize: '0.85rem', color: 'var(--text-secondary)', marginBottom: '0.75rem' }}>
            {status?.waterRequested
              ? 'Aguardando ESP32 conectar e ligar a bomba d’água...'
              : 'Clique para solicitar a rega do vaso'}
          </p>

          <div className="duration-selector">
            <span style={{ fontSize: '0.8rem', color: 'var(--text-secondary)' }}>Duração:</span>
            {[5, 10, 15, 30].map((sec) => (
              <button
                key={sec}
                className={`duration-btn ${selectedDuration === sec ? 'selected' : ''}`}
                onClick={() => setSelectedDuration(sec)}
              >
                {sec}s
              </button>
            ))}
          </div>
        </div>

        {/* Métrica 1: Umidade do Solo */}
        <div className="card col-4">
          <div className="card-header">
            <span className="card-title">
              <Droplets size={18} style={{ color: 'var(--accent-emerald)' }} />
              Umidade do Solo (Capacitivo v1.2)
            </span>
          </div>

          <div className="metric-value">
            {telemetry.soilMoisture}
            <span className="metric-unit">%</span>
          </div>

          <div className="progress-bg">
            <div
              className="progress-fill fill-emerald"
              style={{ width: `${Math.min(100, Math.max(0, telemetry.soilMoisture))}%` }}
            ></div>
          </div>

          <div className="metric-footer">
            {telemetry.soilMoisture < 30 ? (
              <span style={{ color: 'var(--accent-rose)', display: 'flex', alignItems: 'center', gap: '0.25rem' }}>
                <AlertCircle size={14} /> Solo Seco — Irrigação Recomendada
              </span>
            ) : telemetry.soilMoisture > 75 ? (
              <span style={{ color: 'var(--accent-cyan)' }}>Solo Úmido — Boa hidratação</span>
            ) : (
              <span style={{ color: 'var(--accent-emerald)' }}>Umidade Ideal para o Vaso</span>
            )}
          </div>
        </div>

        {/* Métrica 2: Temperatura e Umidade DHT */}
        <div className="card col-4">
          <div className="card-header">
            <span className="card-title">
              <Thermometer size={18} style={{ color: 'var(--accent-amber)' }} />
              Ambiente (DHT11/DHT22)
            </span>
          </div>

          <div style={{ display: 'flex', justifyBetween: 'space-between', gap: '1.5rem' }}>
            <div>
              <p style={{ fontSize: '0.8rem', color: 'var(--text-secondary)' }}>Temperatura</p>
              <div className="metric-value" style={{ fontSize: '1.8rem' }}>
                {telemetry.temperature}
                <span className="metric-unit">°C</span>
              </div>
            </div>

            <div>
              <p style={{ fontSize: '0.8rem', color: 'var(--text-secondary)' }}>Umidade do Ar</p>
              <div className="metric-value" style={{ fontSize: '1.8rem' }}>
                {telemetry.humidity}
                <span className="metric-unit">%</span>
              </div>
            </div>
          </div>

          <div className="metric-footer" style={{ display: 'flex', alignItems: 'center', gap: '0.3rem' }}>
            <Clock size={14} />
            Última atualização: {formatTime(telemetry.updatedAt)}
          </div>
        </div>

        {/* Métrica 3: Bateria & Painel Solar */}
        <div className="card col-4">
          <div className="card-header">
            <span className="card-title">
              <BatteryCharging size={18} style={{ color: 'var(--accent-cyan)' }} />
              Energia Solar / Bateria 18650
            </span>
          </div>

          <div className="metric-value">
            {telemetry.batteryVoltage}
            <span className="metric-unit">V ({batteryPct}%)</span>
          </div>

          <div className="progress-bg">
            <div
              className="progress-fill fill-cyan"
              style={{ width: `${batteryPct}%` }}
            ></div>
          </div>

          <div className="metric-footer">
            Alimentação via Painel Solar + Shield 4 Baterias 18650
          </div>
        </div>

        {/* Histórico de Logs */}
        <div className="card col-8">
          <div className="card-header">
            <span className="card-title">
              <Activity size={18} /> Log de Atividades do Sistema
            </span>
          </div>

          <div className="log-container">
            {status?.logs?.length > 0 ? (
              status.logs.map((log) => (
                <div key={log.id} className="log-item">
                  <span className="log-time">[{formatTime(log.timestamp)}]</span>
                  <span className={`log-${log.type}`}>{log.message}</span>
                </div>
              ))
            ) : (
              <p style={{ color: 'var(--text-secondary)' }}>Nenhum log registrado ainda.</p>
            )}
          </div>
        </div>

        {/* Guia de Integração com o ESP32 */}
        <div className="card col-12">
          <div className="card-header">
            <span className="card-title">
              🚀 Como Conectar o seu ESP32 neste Servidor Vercel
            </span>
          </div>

          <p style={{ fontSize: '0.9rem', color: 'var(--text-secondary)' }}>
            Seu projeto no Vercel fornece os seguintes endpoints HTTPS automáticos para comunicação:
          </p>

          <div className="guide-box">
            <p><strong>1. Envio de Telemetria e Checagem de Rega (ESP32):</strong></p>
            <p><code>POST https://projeto-esp32-irrigacao.vercel.app/api/esp32</code></p>
            <p style={{ marginTop: '0.5rem' }}><strong>Payload JSON enviado pelo ESP32:</strong></p>
            <code>{'{"soilMoisture": 45, "temperature": 25.4, "humidity": 60, "batteryVoltage": 4.1}'}</code>
            <p style={{ marginTop: '0.5rem' }}><strong>Resposta JSON do Vercel para o ESP32:</strong></p>
            <code>{'{"success": true, "waterRequested": true, "durationSec": 10}'}</code>
          </div>
        </div>
      </div>
    </div>
  );
}
