'use client';

import { useState, useEffect } from 'react';
import {
  Droplets,
  Clock,
  Calendar,
  RefreshCw,
  Timer,
  Zap,
  CheckCircle2
} from 'lucide-react';

export default function HistoryDashboard() {
  const [status, setStatus] = useState(null);
  const [loading, setLoading] = useState(true);
  const [refreshing, setRefreshing] = useState(false);

  const fetchStatus = async () => {
    try {
      setRefreshing(true);
      const res = await fetch('/api/status', { cache: 'no-store' });
      if (res.ok) {
        const data = await res.json();
        setStatus(data);
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

  const history = status?.history || [];
  const totalRegas = history.length;
  const ultimaRega = history[0] ? history[0].rtcTime : 'Aguardando primeira rega';

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
            <p>Monitoramento e Histórico em Tempo Real (ESP32 + RTC DS3231)</p>
          </div>
        </div>

        <div className="status-badge">
          <span className="pulse-dot"></span>
          <span>Grade Diurna Oficial (13 Regas: 06:30 às 19:30 - 60s)</span>
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
            <div className="value">60s</div>
            <div className="label">Duração Fixa por Rega</div>
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
                        O ESP32 executará automaticamente 60 segundos de rega nos 13 horários diurnos (06:30 às 19:30).
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
                        💦 {item.durationSec || 60}s
                      </span>
                    </td>
                    <td>
                      <span className="badge-source">
                        {item.source || 'RTC Agendado'}
                      </span>
                    </td>
                    <td>
                      <span style={{ color: '#10b981', fontWeight: 600, fontSize: '0.85rem' }}>
                        ⚡ Relé Digital (GPIO 4)
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
