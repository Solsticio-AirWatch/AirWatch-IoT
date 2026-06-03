from flask import Flask, jsonify, request
from flask_cors import CORS
from datetime import datetime

app = Flask(__name__)
CORS(app)

leitura_atual = {
    "aqi": 0,
    "status": "AGUARDANDO",
    "timestamp": "--"
}

historico = []

@app.route("/")
def dashboard():
    status_cor = {
        "BOM": "#10B981",
        "MODERADO": "#F59E0B",
        "RUIM": "#EF4444",
        "AGUARDANDO": "#475569"
    }
    cor = status_cor.get(leitura_atual["status"], "#475569")

    html = f"""
<!DOCTYPE html>
<html lang="pt-BR">
<head>
  <meta charset="UTF-8">
  <meta http-equiv="refresh" content="5">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>AirWatch</title>
  <style>
    *, *::before, *::after {{ box-sizing: border-box; margin: 0; padding: 0; }}
    body {{
      font-family: Arial, sans-serif;
      background: #0B1320;
      color: #F1F5F9;
      min-height: 100vh;
      padding: 24px 16px;
    }}
    header {{
      display: flex;
      flex-direction: column;
      align-items: center;
      gap: 4px;
      margin-bottom: 32px;
    }}
    .logo-title {{
      font-size: 2rem;
      font-weight: 700;
      letter-spacing: 2px;
    }}
    .logo-title span.air   {{ color: #F1F5F9; }}
    .logo-title span.watch {{ color: #10B981; }}
    .tagline {{
      font-size: 0.65rem;
      letter-spacing: 3px;
      color: #22D3EE;
      text-transform: uppercase;
    }}
    .divider {{
      width: 100%;
      max-width: 480px;
      height: 1px;
      background: linear-gradient(90deg, transparent, #0EA5E9, #10B981, transparent);
      margin: 12px auto 0;
    }}
    .grid {{
      display: grid;
      grid-template-columns: 1fr 1fr;
      gap: 16px;
      max-width: 480px;
      margin: 0 auto 16px;
    }}
    .card {{
      background: #0F1F35;
      border: 1px solid #1E3A5F;
      border-radius: 16px;
      padding: 20px 16px;
      position: relative;
      overflow: hidden;
    }}
    .card::before {{
      content: '';
      position: absolute;
      top: 0; left: 0; right: 0;
      height: 2px;
      background: linear-gradient(90deg, #0EA5E9, #10B981);
    }}
    .card.full {{ grid-column: 1 / -1; }}
    .card-label {{
      font-size: 0.7rem;
      letter-spacing: 2px;
      text-transform: uppercase;
      color: #22D3EE;
      margin-bottom: 10px;
      font-weight: 600;
    }}
    .aqi-value {{
      font-size: 3rem;
      font-weight: 700;
      line-height: 1;
      margin-bottom: 8px;
      color: {cor};
    }}
    .pill {{
      display: inline-flex;
      align-items: center;
      gap: 6px;
      padding: 4px 12px;
      border-radius: 999px;
      font-size: 0.72rem;
      font-weight: 700;
      letter-spacing: 1px;
      text-transform: uppercase;
      color: {cor};
      border: 1px solid {cor};
    }}
    .dot {{
      width: 7px; height: 7px;
      border-radius: 50%;
      background: {cor};
      box-shadow: 0 0 6px {cor};
      display: inline-block;
    }}
    .info-value {{
      font-size: 1.1rem;
      font-weight: 600;
      color: #CBD5E1;
      margin-top: 4px;
    }}
    .info-sub {{
      font-size: 0.72rem;
      color: #475569;
      margin-top: 6px;
    }}
    .status-row {{
      display: flex;
      align-items: center;
      justify-content: space-between;
      padding: 8px 0;
      border-bottom: 1px solid #1E3A5F;
      font-size: 0.8rem;
    }}
    .status-row:last-child {{ border-bottom: none; }}
    .status-row .key {{ color: #64748B; }}
    .status-row .val {{ color: #22D3EE; font-weight: 600; }}
    footer {{
      text-align: center;
      color: #1E3A5F;
      font-size: 0.65rem;
      letter-spacing: 2px;
      text-transform: uppercase;
      margin-top: 24px;
    }}
  </style>
</head>
<body>
  <header>
    <div class="logo-title">
      <span class="air">Air</span><span class="watch">Watch</span>
    </div>
    <div class="tagline">Monitoring the Air. Protecting the Future.</div>
    <div class="divider"></div>
  </header>
  <div class="grid">
    <div class="card full">
      <div class="card-label">Índice de Qualidade do Ar (AQI)</div>
      <div class="aqi-value">{leitura_atual["aqi"]}</div>
      <span class="pill"><span class="dot"></span>{leitura_atual["status"]}</span>
    </div>
    <div class="card">
      <div class="card-label">Última Leitura</div>
      <div class="info-value">{leitura_atual["timestamp"]}</div>
      <div class="info-sub">tempo de operação</div>
    </div>
    <div class="card">
      <div class="card-label">Total de Leituras</div>
      <div class="info-value">{len(historico)}</div>
      <div class="info-sub">registros na sessão</div>
    </div>
    <div class="card full">
      <div class="card-label">Status do Sistema</div>
      <div class="status-row">
        <span class="key">Dispositivo</span>
        <span class="val">AirWatch-ESP32</span>
      </div>
      <div class="status-row">
        <span class="key">API</span>
        <span class="val">ONLINE</span>
      </div>
    </div>
  </div>
  <footer>Atualiza a cada 5s &nbsp;·&nbsp; Solstício &nbsp;·&nbsp; FIAP 2026</footer>
</body>
</html>
"""
    return html

@app.route("/leitura", methods=["POST"])
def receber_leitura():
    dados = request.get_json()
    if not dados:
        return jsonify({"erro": "payload inválido"}), 400

    leitura_atual["aqi"]       = dados.get("aqi", 0)
    leitura_atual["status"]    = dados.get("status", "AGUARDANDO")
    leitura_atual["timestamp"] = dados.get("timestamp", "--")

    historico.append({
        "aqi":       leitura_atual["aqi"],
        "status":    leitura_atual["status"],
        "timestamp": leitura_atual["timestamp"]
    })

    if len(historico) > 10:
        historico.pop(0)

    return jsonify({"mensagem": "leitura registrada"}), 201

@app.route("/leitura/atual", methods=["GET"])
def get_leitura_atual():
    return jsonify(leitura_atual)

@app.route("/historico", methods=["GET"])
def get_historico():
    return jsonify({
        "leituras": historico,
        "total": len(historico)
    })

@app.route("/status", methods=["GET"])
def get_status():
    return jsonify({
        "dispositivo": "AirWatch-ESP32",
        "api": "online",
        "leituras": len(historico)
    })

if __name__ == "__main__":
    app.run(debug=True)
