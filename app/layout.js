import './globals.css';

export const metadata = {
  title: 'ESP32 - Painel de Irrigação Automatizada',
  description: 'Controle de rega e monitoramento em tempo real dos sensores do ESP32 via Vercel',
};

export default function RootLayout({ children }) {
  return (
    <html lang="pt-BR">
      <body>{children}</body>
    </html>
  );
}
