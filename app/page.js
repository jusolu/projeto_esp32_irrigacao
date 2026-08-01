'use client';

import { useState, useEffect } from 'react';
import {
  Droplets,
  Clock,
  Calendar,
  Zap,
  RefreshCw,
  Trash2,
  Play,
  Sun,
  ShieldCheck,
  CheckCircle2
} from 'lucide-react';

export default function IrrigationDashboard() {
  const [status, setStatus] = useState(null);
  const [loading, setLoading] = useState(true);
  const [actionLoading, setActionLoading] = useState(false);
  const [selectedDuration, setSelectedDuration] = useState(15);
  const [refreshing, setRefreshing] = useState(false);

  // Fetch status e histórico completo da API
  const fetchStatus = async () => {
    try {
      setRefreshing(true);
      const res = await fetch('/api/status', { cache: 'no-store' });
      if (res.ok) {
        const data = await res.json();
        setStatus(data);
      }
    } catch (err) {
      console.error('Erro ao carregar dados:', err);
    } finally {
      setLoading(false);
      setTimeout(() => setRefreshing(false), 500);
    }
  };

  useEffect(() => {
    fetchStatus();
    const interval = setInterval(fetchStatus, 3000); // Polling a cada 3s
    return () => clearInterval(interval);
  }, []);

  // Solicita rega manual de teste
  const handleTriggerWatering = async () => {
    setActionLoading(true);
    try {
      const res = await fetch('/api/water', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({
          action: 'start',
          durationSec: selectedDuration
        })
      });
      if (res.ok) {
        await fetchStatus();
      }
    } catch (err) {
      console.error('Erro ao acionar rega:', err);
    } finally {
      setActionLoading(false);
    }
  };

  // Limpa o histórico de regas
  const handleClearHistory = async () => {
    if (!confirm('Deseja realmente limpar todo o histórico de irrigação?')) return;
    try {
      await fetch('/api/water', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ action: 'clear_history' })
      });
      await fetchStatus();
    } catch (err) {
      console.error('Erro ao limpar histórico:', err);
    }
  };

  const history = status?.history || [];
  const totalRegas = history.length;
  const ultimaRega = history[0] ? history[0].rtcTime : 'Nenhuma rega registrada';

  return (
    <div className="container">
      {/* Header */}
      <header className="header">
        <div className="logo-section">
          <div className="logo-icon">
            <Droplets className="w-6 h-6 text-white" />
          </div>
          <div className="logo-title">
            <h1>Registro de Irrigação Automatizada</h1>
            <p>Monitoramento Solar & Módulo RTC DS3231 (ESP32)</p>
          </div>
        </div>

        <div className="status-badge">
          <span className="pulse-dot"></span>
          <span>ESP32 Solar Online (Canal 11)</span>
        </div>
      </header>

      {/* Cards de Estatísticas */}
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
            <div className="value" style={{ fontSize: '1.2rem' }}>{ultimaRega}</div>
            <div className="label">Último Horário Ativado (RTC)</div>
          </div>
        </div>

        <div className="stat-card">
          <div className="stat-icon">
            <Sun size={26} />
          </div>
          <div className="stat-info">
            <div className="value">06:00 - 20:00</div>
            <div className="label">Janela Diurna (A cada 2h)</div>
          </div>
        </div>
      </div>

      {/* Tabela de Histórico de Irrigação */}
      <div className="table-card">
        <div className="table-header-bar">
          <div className="table-title">
            <Calendar className="text-emerald-400" size={22} />
            <h2>Histórico de Ativações da Bomba</h2>
            <span className="badge-count">{totalRegas} Registros</span>
          </div>

          <div className="action-buttons">
            <button className="btn btn-secondary" onClick={fetchStatus} disabled={refreshing}>
              <RefreshCw size={16} className={refreshing ? 'animate-spin' : ''} />
              Atualizar
            </button>
            {history.length > 0 && (
              <button className="btn btn-danger" onClick={handleClearHistory}>
                <Trash2 size={16} />
                Limpar Histórico
              </button>
            )}
          </div>
        </div>

        <div className="table-wrapper">
          <table className="custom-table">
            <thead>
              <tr>
                <th>#</th>
                <th>Horário Ativado no RTC</th>
                <th>Duração da Rega</th>
                <th>Modo de Ativação</th>
                <th>Registro do Servidor</th>
              </tr>
            </thead>
            <tbody>
              {history.length === 0 ? (
                <tr>
                  <td colSpan={5}>
                    <div className="empty-state">
                      <Clock />
                      <p>Nenhuma rega registrada ainda.</p>
                      <span style={{ fontSize: '0.85rem' }}>
                        As regas automáticas do RTC (06h às 20h) e manuais aparecerão nesta tabela.
                      </span>
                    </div>
                  </td>
                </tr>
              ) : (
                history.map((item, index) => (
                  <tr key={item.id || index}>
                    <td style={{ fontWeight: '600', color: '#94a3b8' }}>#{totalRegas - index}</td>
                    <td style={{ fontWeight: '600', color: '#fff' }}>
                      <span style={{ display: 'inline-flex', alignItems: 'center', gap: '6px' }}>
                        <Clock size={15} className="text-emerald-400" />
                        {item.rtcTime}
                      </span>
                    </td>
                    <td>
                      <span className="badge-duration">
                        <Zap size={14} />
                        {item.durationSec} Segundos
                      </span>
                    </td>
                    <td>
                      <span className="badge-source">
                        {item.source || 'RTC Agendado'}
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

        {/* Barra de Teste Manual */}
        <div className="control-bar">
          <div className="select-group">
            <span style={{ fontSize: '0.9rem', color: '#94a3b8' }}>Testar Irrigação Manual:</span>
            <select
              className="select-input"
              value={selectedDuration}
              onChange={(e) => setSelectedDuration(Number(e.target.value))}
            >
              <option value={5}>5 Segundos</option>
              <option value={10}>10 Segundos</option>
              <option value={15}>15 Segundos</option>
              <option value={30}>30 Segundos</option>
            </select>
          </div>

          <button
            className="btn btn-primary"
            onClick={handleTriggerWatering}
            disabled={actionLoading || status?.waterRequested}
          >
            <Play size={16} />
            {status?.waterRequested ? 'Solicitação Pendente...' : 'Acionar Rega de Teste Agora'}
          </button>
        </div>
      </div>
    </div>
  );
}
