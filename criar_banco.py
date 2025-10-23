import sqlite3
import uvicorn
from fastapi import FastAPI, HTTPException, Request
from pydantic import BaseModel
from typing import List, Optional
from datetime import datetime

# --- Configurações ---
DB_FILE = "robo_explorador.db"
app = FastAPI(
    title="API Robô Explorador",
    description="Backend para receber e consultar dados do ESP32."
)

# --- Modelo de Dados (Pydantic) ---
# Define a estrutura de dados que o ESP32 deve enviar via POST [cite: 356]
class LeituraSensores(BaseModel):
    temperatura_c: float
    umidade_pct: float
    luminosidade: int
    presenca: int
    probabilidade_vida: float

# --- Funções do Banco de Dados ---
def get_db_connection():
    """Cria e retorna uma conexão com o banco de dados SQLite."""
    conn = sqlite3.connect(DB_FILE)
    conn.row_factory = sqlite3.Row # Permite acessar os resultados por nome da coluna
    return conn

# --- ENDPOINTS DA API ---

@app.get("/")
def read_root():
    """Endpoint raiz para verificar se a API está online."""
    return {"status": "API do Robô Explorador está online!"}

@app.post("/leituras")
async def criar_leitura(leitura: LeituraSensores):
    """
    Endpoint para RECEBER dados (HTTP POST) do ESP32 e salvar no banco.
    """
    # Gera o timestamp no momento do recebimento
    timestamp_atual = datetime.now().isoformat()
    
    sql = """
    INSERT INTO leituras (timestamp, temperatura_c, umidade_pct, luminosidade, presenca, probabilidade_vida)
    VALUES (?, ?, ?, ?, ?, ?)
    """
    
    try:
        conn = get_db_connection()
        cursor = conn.cursor()
        
        cursor.execute(sql, (
            timestamp_atual,
            leitura.temperatura_c,
            leitura.umidade_pct,
            leitura.luminosidade,
            leitura.presenca,
            leitura.probabilidade_vida
        ))
        conn.commit()
    
    except sqlite3.Error as e:
        print(f"Erro no banco de dados: {e}")
        raise HTTPException(status_code=500, detail=f"Erro ao inserir dados: {e}")
    
    finally:
        if conn:
            conn.close()
            
    print(f"Dados recebidos e salvos: {leitura.model_dump()}")
    return {"status": "Dados salvos com sucesso!", "dados_salvos": leitura, "timestamp": timestamp_atual}

@app.get("/leituras")
async def get_ultimas_leituras():
    """
    Endpoint para CONSULTAR (HTTP GET) as últimas 100 leituras do banco[cite: 385].
    """
    
    sql = "SELECT * FROM leituras ORDER BY id DESC LIMIT 100"
    
    leituras_lista = []
    try:
        conn = get_db_connection()
        cursor = conn.cursor()
        cursor.execute(sql)
        
        for row in cursor.fetchall():
            leituras_lista.append(dict(row))
            
    except sqlite3.Error as e:
        print(f"Erro no banco de dados: {e}")
        raise HTTPException(status_code=500, detail=f"Erro ao consultar dados: {e}")
    
    finally:
        if conn:
            conn.close()
            
    print(f"Consulta GET realizada. Retornando {len(leituras_lista)} leituras.")
    return {"leituras": leituras_lista}

# --- Execução do Servidor ---
if __name__ == "__main__":
    """
    Inicia o servidor Uvicorn.
    Use --host 0.0.0.0 para tornar o servidor acessível na sua rede local (para o ESP32).
    """
    print("Iniciando servidor FastAPI em http://0.0.0.0:5000")
    print("O ESP32 deve enviar dados para http://[IP_DO_SEU_PC]:5000/leituras")
    uvicorn.run("api:app", host="0.0.0.0", port=5000, reload=True)
