'use client';

import { useState, useEffect } from 'react';
import {
  Droplets,
  Clock,
  Calendar,
  RefreshCw,
  Sliders,
  CheckCircle2,
  Cpu,
  Sparkles,
  Zap,
  Timer
} from 'lucide-react';

export default function HistoryDashboard() {
  const [status, setStatus] = useState(null);
  const [loading, setLoading] = useState(true);
  const [refreshing, setRefreshing] = useState(false);
  const [durationSec, setDurationSec] = useState(45);
  const [savingDuration, setSavingDuration] = useState(false);
  const [saveMessage, setSaveMessage] = useState('');

  const fetchStatus = async () => {
    try {
      setRefreshing(true);
      const res = await fetch('/api/status', { cache: 'no-store' });
      if (res.ok) {
        const data = await res.json();
        setStatus(data);
        if (data.config?.durationSec && !savingDuration) {
          setDurationSec(data.config.durationSec);
        }
      }
    } catch (err) {
      console.error('Erro ao carregar histórico:', err);
    } finally {
      setLoading(false);
      setTimeout(() => setRefreshing(false), 500);
    }
  };

  useEffect(() => {
    fetchStatus();
    const interval = setInterval(fetchStatus, 3000);
    return () => clearInterval(interval);
  }, []);

  const handleUpdateDuration = async (sec) => {
    setDurationSec(sec);
    setSavingDuration(true);
    setSaveMessage('');
    try {
      const res = await fetch('/api/config', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ durationSec: sec })
      });
      if (res.ok) {
        setSaveMessage(`Tempo de rega atualizado para ${sec}s!`);
        setTimeout(() => setSaveMessage(''), 3500);
        fetchStatus();
      }
    } catch (err) {
      console.error('Erro ao atualizar duração:', err);
    } finally {
      setSavingDuration(false);
    }
  };

  const history = status?.history || [];
  const totalRegas = history.length;
  const ultimaRega = history[0] ? history[0].rtcTime : 'Aguardando primeira rega';
  const duracaoAtual = status?.config?.durationSec || durationSec || 45;

  return (
    <div className="container">
      {/* Header */}
      <header className="header">
        <div className="logo-section">
          <div className="logo-icon">
            <Droplets className="w-6 h-6 text-white" />
          </div>
          <div className="logo-title">
            <h1>Painel de Irrigação Automatizada</h1>
            <p>Controle Dinâmico e Histórico em Tempo Real (ESP32 + RTC DS3231)</p>
          </div>
        </div>

        <div className="status-badge">
          <span className="pulse-dot"></span>
          <span>RTC DS3231 Autônomo (Grade 11 Regas)</span>
        </div>
      </header>

      {/* Cards de Resumo */}
      <div className="stats-grid">
        <div className="stat-card">
          <div className="stat-icon">
            <Droplets size={26} />
          </div>
          <div className="stat-info">
            <div className="value">{totalRegas}</div>
            <div className="label">Total de Regas Efetuadas</div>
          </div>
        </div>

        <div className="stat-card">
          <div className="stat-icon">
            <Clock size={26} />
          </div>
          <div className="stat-info">
            <div className="value" style={{ fontSize: '1.15rem' }}>{ultimaRega}</div>
            <div className="label">Último Horário Registrado (RTC)</div>
          </div>
        </div>

        <div className="stat-card">
          <div className="stat-icon">
            <Timer size={26} />
          </div>
          <div className="stat-info">
            <div className="value">{duracaoAtual}s</div>
            <div className="label">Tempo Dinâmico por Rega</div>
          </div>
        </div>
      </div>

      {/* Card de Configuração do Tempo de Rega Dinâmico */}
      <div className="table-card" style={{ marginBottom: '1.5rem', background: 'linear-gradient(135deg, rgba(30, 41, 59, 0.7), rgba(15, 23, 42, 0.8))' }}>
        <div className="table-header-bar" style={{ flexWrap: 'wrap', gap: '1rem' }}>
          <div className="table-title">
            <Sliders className="text-emerald-400" size={22} />
            <h2>Configuração Dinâmica do Tempo de Rega</h2>
            {saveMessage && (
              <span className="badge-count" style={{ background: '#059669', color: 'white', fontWeight: 600 }}>
                {saveMessage}
              </span>
            )}
          </div>
        </div>

        <div style={{ padding: '1.25rem 1.5rem' }}>
          <p style={{ color: '#94a3b8', fontSize: '0.9rem', marginBottom: '1rem' }}>
            Selecione o tempo que a bomba permanecerá ligada em cada uma das 11 regas diurnas agendadas. O ESP32 sincroniza esse valor automaticamente a cada ativação.
          </p>

          <div style={{ display: 'flex', gap: '0.75rem', flexWrap: 'wrap', alignItems: 'center' }}>
            {[15, 20, 30, 45, 60, 90, 120].map((sec) => (
              <button
                key={sec}
                onClick={() => handleUpdateDuration(sec)}
                disabled={savingDuration}
                style={{
                  padding: '0.6rem 1.2rem',
                  borderRadius: '8px',
                  fontWeight: 600,
                  fontSize: '0.95rem',
                  cursor: 'pointer',
                  border: duracaoAtual === sec ? '2px solid #10b981' : '1px solid #334155',
                  background: duracaoAtual === sec ? 'rgba(16, 185, 129, 0.2)' : '#1e293b',
                  color: duracaoAtual === sec ? '#34d399' : '#cbd5e1',
                  transition: 'all 0.2s ease',
                  boxShadow: duracaoAtual === sec ? '0 0 12px rgba(16, 185, 129, 0.3)' : 'none'
                }}
              >
                {sec} segundos {duracaoAtual === sec ? '✓' : ''}
              </button>
            ))}
          </div>
        </div>
      </div>

      {/* Tabela de Histórico de Irrigação */}
      <div className="table-card">
        <div className="table-header-bar">
          <div className="table-title">
            <Calendar className="text-emerald-400" size={22} />
            <h2>Registro das Regas Efetuadas</h2>
            <span className="badge-count">{totalRegas} Ativações</span>
          </div>

          <div className="action-buttons">
            <button className="btn btn-secondary" onClick={fetchStatus} disabled={refreshing}>
              <RefreshCw size={16} className={refreshing ? 'animate-spin' : ''} />
              Atualizar Tabela
            </button>
          </div>
        </div>

        <div className="table-wrapper">
          <table className="custom-table">
            <thead>
              <tr>
                <th>#</th>
                <th>Horário do RTC DS3231</th>
                <th>Duração da Rega</th>
                <th>Origem do Disparo</th>
                <th>Chaveamento</th>
                <th>Registro no Servidor</th>
              </tr>
            </thead>
            <tbody>
              {history.length === 0 ? (
                <tr>
                  <td colSpan={6}>
                    <div className="empty-state">
                      <Clock />
                      <p>Nenhuma rega registrada no histórico ainda.</p>
                      <span style={{ fontSize: '0.85rem' }}>
                        O ESP32 registrará automaticamente cada acionamento nos 11 horários diurnos (07:30 às 18:00).
                      </span>
                    </div>
                  </td>
                </tr>
              ) : (
                history.map((item, idx) => (
                  <tr key={item.id || idx}>
                    <td className="row-id">#{history.length - idx}</td>
                    <td className="time-col">
                      <Clock size={15} />
                      <span>{item.rtcTime || 'Desconhecido'}</span>
                    </td>
                    <td>
                      <span className="badge-duration">
                        💦 {item.durationSec || 45}s
                      </span>
                    </td>
                    <td>
                      <span className="badge-source">
                        {item.source || 'RTC Agendado'}
                      </span>
                    </td>
                    <td>
                      <span style={{ color: '#10b981', fontWeight: 600, fontSize: '0.85rem' }}>
                        ⚡ MOSFET PWM (GPIO 4)
                      </span>
                    </td>
                    <td className="server-time-col">
                      {item.serverTimestamp ? new Date(item.serverTimestamp).toLocaleString('pt-BR') : '—'}
                    </td>
                  </tr>
                ))
              )}
            </tbody>
          </table>
        </div>
      </div>
    </div>
  );
}
