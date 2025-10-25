from flask import Flask, request, jsonify
import sqlite3
from datetime import datetime

# Correção: __name__ (dois underscores de cada lado)
app = Flask(__name__)

# ---- Função para conectar no banco ---- #
def conectar_banco():
    """Conecta ao banco de dados SQLite."""
    # O banco será criado na mesma pasta do script
    return sqlite3.connect("robo_explorador.db") 


# ---- Criar tabela se não existir ---- #
def criar_tabela():
    """Verifica e cria a tabela 'leituras' se ela não existir."""
    conn = conectar_banco()
    cursor = conn.cursor()
    
    # SQL para criar a tabela conforme o roteiro 
    cursor.execute("""
        CREATE TABLE IF NOT EXISTS leituras (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            timestamp TEXT,
            temperatura_c REAL,
            umidade_pct REAL,
            luminosidade INTEGER,
            presenca INTEGER,
            probabilidade_vida REAL
        )
    """)
    conn.commit()
    conn.close()

# Executa a criação da tabela assim que o servidor inicia
criar_tabela()


# ---- API: Receber dados do ESP32 via POST ---- #
@app.route("/leituras", methods=["POST"])
def adicionar_leitura():
    """
    Endpoint para RECEBER dados (HTTP POST) do ESP32 e salvar no banco.
    
    """
    dados = request.get_json()
    
    # Pega o timestamp enviado pelo ESP32, ou gera um novo se não for enviado
    timestamp = dados.get("timestamp", datetime.now().isoformat())

    conn = conectar_banco()
    cursor = conn.cursor()
    cursor.execute("""
        INSERT INTO leituras (timestamp, temperatura_c, umidade_pct, luminosidade, presenca, probabilidade_vida)
        VALUES (?, ?, ?, ?, ?, ?)
    """, (
        timestamp,
        dados["temperatura_c"],
        dados["umidade_pct"],
        dados["luminosidade"],
        dados["presenca"],
        dados["probabilidade_vida"]
    ))
    conn.commit()
    conn.close()
    
    print(f"Dados recebidos e salvos: {dados}")
    return jsonify({"status": "sucesso", "dados_recebidos": dados}), 201


# ---- API: Consultar últimas 100 leituras ---- #
@app.route("/leituras", methods=["GET"])
def listar_leituras():
    """
    Endpoint para CONSULTAR (HTTP GET) as últimas 100 leituras do banco.
    
    """
    conn = conectar_banco()
    # Retorna os dados como dicionários (mais fácil de converter para JSON)
    conn.row_factory = sqlite3.Row 
    cursor = conn.cursor()
    
    cursor.execute("SELECT * FROM leituras ORDER BY id DESC LIMIT 100")
    
    # Converte os resultados (sqlite3.Row) para uma lista de dicionários
    dados_formatados = [dict(row) for row in cursor.fetchall()]
    
    conn.close()
    
    print(f"Consulta GET realizada. Retornando {len(dados_formatados)} leituras.")
    return jsonify(dados_formatados)


# ---- Rodar o servidor ---- #
# Correção: __name__ e __main__ (dois underscores de cada lado)
if __name__ == "__main__":
    print("Iniciando servidor Flask em http://0.0.0.0:5000")
    print("O ESP32 deve enviar dados para http://[IP_DO_SEU_PC]:5000/leituras")
    print("Use CTRL+C para parar o servidor.")
    app.run(host="0.0.0.0", port=5000, debug=True)
