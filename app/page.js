'use client';

import { useState, useEffect } from 'react';
import {
  Droplets,
  Clock,
  Calendar,
  RefreshCw,
  Sun,
  ShieldCheck,
  Award
} from 'lucide-react';

export default function HistoryDashboard() {
  const [status, setStatus] = useState(null);
  const [loading, setLoading] = useState(true);
  const [refreshing, setRefreshing] = useState(false);

  // Fetch histórico de irrigação da API
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
    const interval = setInterval(fetchStatus, 3000); // Atualiza a cada 3 segundos
    return () => clearInterval(interval);
  }, []);

  const history = status?.history || [];
  const totalRegas = history.length;
  const ultimaRega = history[0] ? history[0].rtcTime : 'Aguardando primeira rega';

  return (
    <div className="container">
      {/* Header Limpo */}
      <header className="header">
        <div className="logo-section">
          <div className="logo-icon">
            <Droplets className="w-6 h-6 text-white" />
          </div>
          <div className="logo-title">
            <h1>Histórico de Irrigação Automatizada</h1>
            <p>Registro das regas solares executadas pelo ESP32 + Módulo RTC</p>
          </div>
        </div>

        <div className="status-badge">
          <span className="pulse-dot"></span>
          <span>ESP32 Solar Online</span>
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
            <Sun size={26} />
          </div>
          <div className="stat-info">
            <div className="value">06:00 - 20:00</div>
            <div className="label">Janela Diurna (A cada 2 Horas)</div>
          </div>
        </div>
      </div>

      {/* Tabela Puramente de Histórico de Irrigação */}
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
                <th>Horário Ativado no RTC</th>
                <th>Duração da Rega</th>
                <th>Origem do Disparo</th>
                <th>Registro do Servidor</th>
              </tr>
            </thead>
            <tbody>
              {history.length === 0 ? (
                <tr>
                  <td colSpan={5}>
                    <div className="empty-state">
                      <Clock />
                      <p>Nenhuma rega registrada no histórico ainda.</p>
                      <span style={{ fontSize: '0.85rem' }}>
                        O ESP32 registrará automaticamente cada acionamento a cada 2h (das 06:00 às 20:00).
                      </span>
                    </div>
                  </td>
                </tr>
              ) : (
                history.map((item, index) => (
                  <tr key={item.id || index}>
                    <td style={{ fontWeight: '600', color: '#94a3b8' }}>#{totalRegas - index}</td>
                    <td style={{ fontWeight: '600', color: '#fff' }}>
                      <span style={{ display: 'inline-flex', alignItems: 'center', gap: '8px' }}>
                        <Clock size={16} className="text-emerald-400" />
                        {item.rtcTime}
                      </span>
                    </td>
                    <td>
                      <span className="badge-duration">
                        💧 {item.durationSec} Segundos
                      </span>
                    </td>
                    <td>
                      <span className="badge-source">
                        {item.source || 'RTC 2 Horas Diurno'}
                      </span>
                    </td>
                    <td style={{ color: '#94a3b8', fontSize: '0.85rem' }}>
                      {new Date(item.serverTimestamp).toLocaleTimeString('pt-BR')}
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
