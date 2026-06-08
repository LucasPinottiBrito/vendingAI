# Especificação Técnica — Firmware ESP32-S3 Vending Machine v0.2 MQTT

**Projeto:** Máquina de Venda Inteligente com Arquitetura IoT e Monitoramento Remoto em Tempo Real  
**Firmware:** ESP32-S3 Vending Machine  
**Versão:** 0.2  
**Status:** Especificação para integração MQTT com backend  
**Hardware alvo:** ESP32-S3 N16R8  
**Linguagem:** C++  
**Framework:** ESP-IDF via PlatformIO  

---

## 1. Objetivo da versão v0.2

A versão v0.2 do firmware deve transformar o protótipo local da ESP32-S3 em uma máquina conectada à plataforma web por MQTT, mantendo apenas 2 motores físicos e 2 sensores IR break beam, mas tratando esses dois módulos como uma vending machine operacional completa para o MVP.

A versão atual já realiza:

- conexão Wi-Fi;
- indicação de status de conexão Wi-Fi;
- testes de motores;
- testes de sensores IR;
- execução local de venda, girando o motor até detectar queda no sensor IR correspondente.

A versão v0.2 deve adicionar:

- reconexão Wi-Fi automática;
- conexão ao broker MQTT;
- reconexão MQTT automática;
- assinatura do tópico de ações da máquina;
- recebimento de comandos de dispensação enviados pelo backend;
- validação mínima dos comandos recebidos;
- execução física da venda com 2 motores;
- retentativa automática em caso de não detecção;
- publicação de eventos operacionais;
- publicação de heartbeat periódico;
- publicação de status da máquina;
- publicação de logs operacionais úteis ao backend;
- proteção contra comando duplicado.

A ESP32-S3 continua sendo apenas o executor físico da venda. O backend continua sendo a fonte da verdade para regras comerciais.

---

## 2. Escopo da versão v0.2

### 2.1 Dentro do escopo

A firmware deve implementar:

- Wi-Fi com credenciais fixas no código ou em arquivo de configuração local.
- Reconexão Wi-Fi em caso de queda.
- MQTT usando broker HiveMQ público ou broker configurável.
- MQTT sem TLS obrigatório no MVP.
- MQTT com usuário e senha, se configurado.
- Subscribe em `vending/{machine_id}/actions`.
- Publish em `vending/{machine_id}/events`.
- Publish em `vending/{machine_id}/status`.
- Heartbeat a cada 30 segundos.
- Estado interno da máquina.
- Processamento do comando `DISPENSE`.
- Controle de 2 motores 28BYJ-48 com ULN2003.
- Leitura de 2 sensores IR break beam.
- Mapeamento local entre `motor_id` e GPIOs.
- Mapeamento local entre `sensor_column_id` e GPIOs.
- Retentativa de dispensação.
- Timeout por tentativa.
- Publicação de sucesso ou falha.
- Logs via serial.
- Eventos de log para backend, quando relevante.
- Comandos seriais de debug mantidos.

### 2.2 Fora do escopo

A firmware não deve implementar:

- autenticação de usuário;
- validação de saldo;
- validação de Pix;
- validação de pagamento;
- cálculo de preço;
- consulta direta ao banco de dados;
- atualização direta de estoque;
- criação de venda;
- criação de usuário;
- decisão de estorno;
- compra sem cadastro;
- carrinho com múltiplos produtos;
- OTA update;
- TLS obrigatório;
- HMAC obrigatório;
- controle dos 6 motores;
- lógica ativa do reed switch;
- interface gráfica local;
- requisições HTTP ao backend.

---

## 3. Princípio arquitetural

A comunicação operacional entre backend e ESP32-S3 deve ser exclusivamente MQTT.

O backend publica comandos.  
A ESP32-S3 consome comandos.  
A ESP32-S3 publica eventos e status.  
O backend consome eventos e status.

A ESP32-S3 não publica no tópico `actions`.

O backend é responsável por:

- autenticar usuário;
- validar saldo;
- validar produto;
- validar slot;
- validar estoque;
- debitar saldo;
- reservar estoque;
- criar venda;
- criar comando físico;
- publicar comando MQTT;
- processar eventos da ESP32-S3;
- finalizar venda;
- baixar estoque definitivo;
- liberar reserva;
- realizar estorno.

A ESP32-S3 é responsável por:

- manter conexão Wi-Fi;
- manter conexão MQTT;
- receber comandos autorizados;
- validar se o comando pertence à máquina;
- validar se o comando é fisicamente executável;
- acionar motor;
- monitorar sensor;
- publicar eventos operacionais;
- publicar status;
- manter logs locais.

---

## 4. Broker MQTT

Configuração esperada:

```txt
Broker: HiveMQ público ou broker MQTT configurável
Protocolo: MQTT
Porta sem TLS: 1883
TLS: não obrigatório no MVP
Autenticação: usuário e senha, se configurado
QoS comandos: 1, quando suportado pela biblioteca
QoS eventos críticos: 1, quando suportado pela biblioteca
Retained commands: não usar
```

A versão acadêmica pode usar MQTT sem TLS. Em uma versão comercial, a comunicação deve evoluir para TLS, credenciais individuais por máquina, rotação de credenciais, proteção contra replay e validação forte de payloads.

---

## 5. Configuração do firmware

O arquivo de configuração deve conter, no mínimo:

```cpp
#pragma once

#include <cstdint>

constexpr const char* FIRMWARE_VERSION = "0.2.0";

// Machine
constexpr int MACHINE_ID = 1;
constexpr const char* MACHINE_CODE = "vending-univap-01";

// Wi-Fi
constexpr const char* WIFI_SSID = "SEU_WIFI";
constexpr const char* WIFI_PASSWORD = "SUA_SENHA";
constexpr uint32_t WIFI_CONNECT_TIMEOUT_MS = 15000;
constexpr uint32_t WIFI_RECONNECT_INTERVAL_MS = 5000;

// MQTT
constexpr const char* MQTT_HOST = "broker.hivemq.com";
constexpr int MQTT_PORT = 1883;
constexpr const char* MQTT_USERNAME = "";
constexpr const char* MQTT_PASSWORD = "";
constexpr uint32_t MQTT_RECONNECT_INTERVAL_MS = 5000;
constexpr uint32_t MQTT_HEARTBEAT_INTERVAL_MS = 30000;

// Topics
constexpr const char* MQTT_TOPIC_ACTIONS_FORMAT = "vending/%d/actions";
constexpr const char* MQTT_TOPIC_EVENTS_FORMAT  = "vending/%d/events";
constexpr const char* MQTT_TOPIC_STATUS_FORMAT  = "vending/%d/status";

// Motor timing
constexpr uint32_t DEFAULT_STEP_DELAY_MS = 4;
constexpr uint32_t TEST_MOTOR_STEPS = 512;
constexpr uint32_t DEFAULT_ATTEMPTS_ALLOWED = 2;
constexpr uint32_t DEFAULT_TIMEOUT_MS_PER_ATTEMPT = 10000;

// GPIO - Motor 1
constexpr int MOTOR1_IN1 = 4;
constexpr int MOTOR1_IN2 = 5;
constexpr int MOTOR1_IN3 = 6;
constexpr int MOTOR1_IN4 = 7;

// GPIO - Motor 2
constexpr int MOTOR2_IN1 = 15;
constexpr int MOTOR2_IN2 = 16;
constexpr int MOTOR2_IN3 = 17;
constexpr int MOTOR2_IN4 = 18;

// GPIO - Sensors
constexpr int IR_SENSOR_1 = 10;
constexpr int IR_SENSOR_2 = 11;
constexpr int REED_SWITCH_RESERVED = 12;

constexpr bool IR_SENSOR_ACTIVE_LOW = true;
```

---

## 6. Tópicos MQTT

### 6.1 Tópico de ações

```txt
vending/{machine_id}/actions
```

Publicado por: backend.  
Consumido por: ESP32-S3.

Exemplo:

```txt
vending/1/actions
```

Uso:

- comando de dispensação;
- comandos futuros de manutenção/configuração.

A ESP32-S3 não deve publicar nesse tópico.

### 6.2 Tópico de eventos

```txt
vending/{machine_id}/events
```

Publicado por: ESP32-S3.  
Consumido por: backend.

Exemplo:

```txt
vending/1/events
```

Uso:

- início de dispensação;
- sensor ativado;
- retentativa;
- sucesso;
- falha;
- erro de motor;
- erro de máquina;
- logs operacionais relevantes.

### 6.3 Tópico de status

```txt
vending/{machine_id}/status
```

Publicado por: ESP32-S3.  
Consumido por: backend.

Exemplo:

```txt
vending/1/status
```

Uso:

- heartbeat;
- estado atual da máquina;
- versão do firmware;
- IP local;
- estado de conexão Wi-Fi/MQTT.

---

## 7. Estados internos do firmware

A firmware deve usar uma máquina de estados simples:

```cpp
enum class MachineState {
    BOOTING,
    WIFI_CONNECTING,
    MQTT_CONNECTING,
    READY,
    VENDING,
    ERROR
};
```

Descrição:

- `BOOTING`: firmware inicializando.
- `WIFI_CONNECTING`: tentativa de conexão Wi-Fi.
- `MQTT_CONNECTING`: tentativa de conexão MQTT.
- `READY`: máquina conectada e pronta para receber comandos.
- `VENDING`: comando de dispensação em execução.
- `ERROR`: erro operacional geral.

Estados de slot:

```cpp
enum class SlotState {
    IDLE,
    RUNNING,
    SUCCESS,
    FAILED,
    DISABLED
};
```

---

## 8. Mapeamento local de slots, motores e sensores

O backend envia identificadores de domínio e identificadores físicos. A firmware deve resolver apenas os identificadores físicos necessários para executar a ação.

Na v0.2, a máquina possui 2 motores e 2 sensores:

| Slot físico | motor_id | sensor_column_id | Descrição |
|---|---:|---:|---|
| A1 | 1 | 1 | Motor 1, sensor IR coluna 1 |
| B1 | 2 | 2 | Motor 2, sensor IR coluna 2 |

O backend pode enviar `slot_id` e `slot_code`, mas o firmware deve acionar com base no `motor_id` e confirmar queda com base no `sensor_column_id`.

Regras:

- `motor_id = 1` resolve para GPIOs do motor 1.
- `motor_id = 2` resolve para GPIOs do motor 2.
- `sensor_column_id = 1` resolve para IR sensor 1.
- `sensor_column_id = 2` resolve para IR sensor 2.
- Qualquer outro motor ou sensor deve ser rejeitado com erro.

---

## 9. Payload do comando DISPENSE

Tópico:

```txt
vending/{machine_id}/actions
```

Payload:

```json
{
  "type": "DISPENSE",
  "command_id": 123,
  "sale_id": 456,
  "machine_id": 1,
  "product_id": 10,
  "slot_id": 3,
  "slot_code": "A1",
  "motor_id": 1,
  "sensor_column_id": 1,
  "quantity": 1,
  "attempts_allowed": 2,
  "timeout_ms_per_attempt": 10000,
  "issued_at": "2026-06-08T12:00:00Z"
}
```

### 9.1 Campos

| Campo | Tipo | Obrigatório | Descrição |
|---|---|---:|---|
| `type` | string | sim | Deve ser `DISPENSE` |
| `command_id` | int/string | sim | ID do comando físico no backend |
| `sale_id` | int/string | sim | ID da venda associada |
| `machine_id` | int/string | sim | ID técnico da máquina |
| `product_id` | int/string | sim | ID do produto vendido |
| `slot_id` | int/string | sim | ID lógico do slot no backend |
| `slot_code` | string | sim | Código visual do slot, por exemplo `A1` |
| `motor_id` | int | sim | Motor físico a acionar |
| `sensor_column_id` | int | sim | Sensor físico da coluna |
| `quantity` | int | sim | No MVP deve ser `1` |
| `attempts_allowed` | int | sim | No MVP normalmente `2` |
| `timeout_ms_per_attempt` | int | sim | Timeout de cada tentativa |
| `issued_at` | string | sim | Data/hora de emissão do comando |

---

## 10. Validações do comando no firmware

Ao receber uma mensagem MQTT no tópico de ações, a ESP32-S3 deve validar:

1. Se o payload é JSON válido.
2. Se `type == "DISPENSE"`.
3. Se `machine_id` corresponde ao `MACHINE_ID` configurado.
4. Se a máquina está em estado `READY`.
5. Se não existe venda em andamento.
6. Se `motor_id` é conhecido localmente.
7. Se `sensor_column_id` é conhecido localmente.
8. Se `quantity == 1`.
9. Se `attempts_allowed > 0`.
10. Se `timeout_ms_per_attempt > 0`.
11. Se o comando não é duplicado.

Falhas de validação devem gerar `MACHINE_ERROR` ou `DISPENSE_FAILED`, conforme o caso.

Regras práticas:

- JSON inválido: publicar `MACHINE_ERROR` com reason `INVALID_JSON`.
- Tipo desconhecido: publicar `MACHINE_ERROR` com reason `UNKNOWN_COMMAND_TYPE`.
- Máquina diferente: ignorar ou publicar `MACHINE_ERROR` com reason `MACHINE_ID_MISMATCH`, se for seguro.
- Máquina ocupada: publicar `DISPENSE_FAILED` com reason `MACHINE_BUSY`.
- Motor desconhecido: publicar `DISPENSE_FAILED` com reason `UNKNOWN_MOTOR_ID`.
- Sensor desconhecido: publicar `DISPENSE_FAILED` com reason `UNKNOWN_SENSOR_COLUMN_ID`.
- Quantidade diferente de 1: publicar `DISPENSE_FAILED` com reason `UNSUPPORTED_QUANTITY`.

---

## 11. Fluxo de execução do comando

Ao receber um comando `DISPENSE` válido:

1. Salvar dados do comando atual em memória.
2. Alterar `MachineState` para `VENDING`.
3. Publicar `DISPENSE_STARTED` com `attempt = 1`.
4. Acionar o motor correspondente lentamente.
5. Verificar o sensor correspondente após cada passo ou pequeno grupo de passos.
6. Se o sensor detectar passagem:
   - parar motor;
   - desenergizar bobinas;
   - publicar `SENSOR_TRIGGERED`;
   - publicar `DISPENSE_SUCCESS`;
   - salvar `command_id` como último comando concluído;
   - alterar estado para `READY`.
7. Se atingir timeout sem detecção:
   - parar motor;
   - desenergizar bobinas;
   - se ainda houver tentativa disponível, publicar `DISPENSE_RETRY` e repetir;
   - se não houver tentativa disponível, publicar `DISPENSE_FAILED`;
   - salvar `command_id` como último comando concluído;
   - alterar estado para `READY`.
8. Se ocorrer erro físico:
   - parar motor;
   - desenergizar bobinas;
   - publicar `MOTOR_ERROR` ou `MACHINE_ERROR`;
   - salvar `command_id` se aplicável;
   - alterar estado para `ERROR` ou `READY`, conforme gravidade.

---

## 12. Eventos publicados pela ESP32-S3

Todos os eventos devem ser publicados em:

```txt
vending/{machine_id}/events
```

Todos os eventos devem conter, sempre que aplicável:

```json
{
  "type": "EVENT_TYPE",
  "command_id": 123,
  "sale_id": 456,
  "machine_id": 1,
  "slot_id": 3,
  "slot_code": "A1",
  "motor_id": 1,
  "sensor_column_id": 1,
  "timestamp": "2026-06-08T12:00:03Z"
}
```

Como a ESP32-S3 pode não ter relógio real sincronizado no início, o campo `timestamp` pode usar:

- horário ISO real se houver SNTP implementado;
- `uptime_ms` como campo adicional;
- timestamp aproximado, se disponível.

Recomendação: incluir sempre `uptime_ms` nos eventos para debug.

---

## 13. Evento DISPENSE_STARTED

Publicado quando a ESP32-S3 inicia a execução física do comando.

```json
{
  "type": "DISPENSE_STARTED",
  "command_id": 123,
  "sale_id": 456,
  "machine_id": 1,
  "slot_id": 3,
  "slot_code": "A1",
  "motor_id": 1,
  "sensor_column_id": 1,
  "attempt": 1,
  "uptime_ms": 123456
}
```

Efeito esperado no backend:

- registrar evento;
- considerar comando recebido/iniciado;
- alterar venda para `DISPENSING`.

---

## 14. Evento SENSOR_TRIGGERED

Publicado quando o sensor da coluna detecta a passagem do produto.

```json
{
  "type": "SENSOR_TRIGGERED",
  "command_id": 123,
  "sale_id": 456,
  "machine_id": 1,
  "slot_id": 3,
  "slot_code": "A1",
  "motor_id": 1,
  "sensor_column_id": 1,
  "attempt": 1,
  "uptime_ms": 127000
}
```

Esse evento é informativo. O backend deve aguardar `DISPENSE_SUCCESS` para finalizar a venda com sucesso.

---

## 15. Evento DISPENSE_RETRY

Publicado quando uma tentativa falha por timeout e a ESP32-S3 iniciará nova tentativa.

```json
{
  "type": "DISPENSE_RETRY",
  "command_id": 123,
  "sale_id": 456,
  "machine_id": 1,
  "slot_id": 3,
  "slot_code": "A1",
  "motor_id": 1,
  "sensor_column_id": 1,
  "attempt": 2,
  "reason": "PRODUCT_NOT_DETECTED",
  "uptime_ms": 135000
}
```

O backend deve registrar o evento, mas não deve estornar nesse momento.

---

## 16. Evento DISPENSE_SUCCESS

Publicado quando o produto é detectado pelo sensor.

```json
{
  "type": "DISPENSE_SUCCESS",
  "command_id": 123,
  "sale_id": 456,
  "machine_id": 1,
  "slot_id": 3,
  "slot_code": "A1",
  "motor_id": 1,
  "sensor_column_id": 1,
  "attempt": 1,
  "uptime_ms": 128000
}
```

Efeito esperado no backend:

- marcar comando como `SUCCESS`;
- marcar venda como `DISPENSED`;
- baixar estoque definitivamente;
- registrar log da operação.

---

## 17. Evento DISPENSE_FAILED

Publicado quando nenhuma tentativa detecta a queda do produto ou quando o comando não pode ser executado fisicamente.

```json
{
  "type": "DISPENSE_FAILED",
  "command_id": 123,
  "sale_id": 456,
  "machine_id": 1,
  "slot_id": 3,
  "slot_code": "A1",
  "motor_id": 1,
  "sensor_column_id": 1,
  "attempts_executed": 2,
  "reason": "PRODUCT_NOT_DETECTED",
  "uptime_ms": 150000
}
```

Motivos padronizados:

```txt
PRODUCT_NOT_DETECTED
MACHINE_BUSY
UNKNOWN_MOTOR_ID
UNKNOWN_SENSOR_COLUMN_ID
UNSUPPORTED_QUANTITY
INVALID_COMMAND
COMMAND_DUPLICATED
MOTOR_TIMEOUT
INTERNAL_ERROR
```

Efeito esperado no backend:

- marcar comando como `FAILED`;
- marcar venda como `FAILED`;
- liberar reserva de estoque;
- estornar saldo;
- marcar venda como `REFUNDED`;
- registrar log da operação.

---

## 18. Evento MOTOR_ERROR

Publicado quando ocorre erro físico relacionado ao motor.

```json
{
  "type": "MOTOR_ERROR",
  "command_id": 123,
  "sale_id": 456,
  "machine_id": 1,
  "slot_id": 3,
  "slot_code": "A1",
  "motor_id": 1,
  "reason": "MOTOR_TIMEOUT",
  "uptime_ms": 150000
}
```

O backend deve tratar esse evento como falha operacional quando estiver associado a uma venda.

---

## 19. Evento MACHINE_ERROR

Publicado quando a ESP32-S3 detecta erro geral.

```json
{
  "type": "MACHINE_ERROR",
  "machine_id": 1,
  "status": "ERROR",
  "reason": "INVALID_COMMAND",
  "details": "Unknown slot_id or malformed payload",
  "uptime_ms": 150000
}
```

Motivos padronizados:

```txt
INVALID_JSON
UNKNOWN_COMMAND_TYPE
MACHINE_ID_MISMATCH
INVALID_COMMAND
MQTT_DISCONNECTED
WIFI_DISCONNECTED
INTERNAL_ERROR
```

---

## 20. Evento MACHINE_LOG

Para logs operacionais que não representam necessariamente falha ou transição de venda, a ESP32-S3 pode publicar:

```json
{
  "type": "MACHINE_LOG",
  "machine_id": 1,
  "level": "INFO",
  "message": "MQTT connected and subscribed to actions topic",
  "current_state": "READY",
  "uptime_ms": 10000
}
```

Níveis permitidos:

```txt
DEBUG
INFO
WARN
ERROR
```

Regras:

- não publicar logs excessivos a cada loop;
- publicar logs úteis de boot, conexão, reconexão, comandos inválidos e erros;
- continuar imprimindo logs locais via serial.

---

## 21. Heartbeat e status

A ESP32-S3 deve publicar heartbeat periódico em:

```txt
vending/{machine_id}/status
```

Periodicidade:

```txt
30 segundos
```

Payload:

```json
{
  "type": "HEARTBEAT",
  "machine_id": 1,
  "status": "ONLINE",
  "current_state": "READY",
  "firmware_version": "0.2.0",
  "ip_address": "192.168.0.120",
  "wifi_connected": true,
  "mqtt_connected": true,
  "uptime_ms": 60000
}
```

Estados possíveis no campo `status`:

```txt
ONLINE
DISPENSING
ERROR
```

A ESP32-S3 não publica `OFFLINE`, pois, se estiver offline, provavelmente não conseguirá publicar. O backend deve inferir `OFFLINE` pela ausência de heartbeat.

Regra esperada no backend:

```txt
Se não houver heartbeat por dois períodos consecutivos, considerar máquina offline.
```

Com heartbeat de 30 segundos, a máquina deve ser considerada offline após aproximadamente 60 segundos sem heartbeat. O backend pode usar margem de 65 a 75 segundos.

---

## 22. Reconexão Wi-Fi

A versão atual conecta ao Wi-Fi, mas não reconecta automaticamente em caso de queda. A v0.2 deve corrigir isso.

Requisitos:

- detectar evento de desconexão Wi-Fi;
- alterar estado para `WIFI_CONNECTING` quando aplicável;
- tentar reconectar periodicamente;
- não travar a execução em loop bloqueante infinito;
- ao reconectar Wi-Fi, tentar reconectar MQTT;
- publicar log local e, quando possível, `MACHINE_LOG` após reconexão.

Comportamento esperado:

```text
Wi-Fi conectado → MQTT conectado → READY
Wi-Fi desconectou → MQTT cai → estado WIFI_CONNECTING
Wi-Fi reconectou → estado MQTT_CONNECTING
MQTT reconectou → resubscribe actions → READY
```

---

## 23. Reconexão MQTT

A firmware deve manter a conexão MQTT ativa.

Requisitos:

- detectar desconexão MQTT;
- tentar reconectar a cada `MQTT_RECONNECT_INTERVAL_MS`;
- após reconectar, assinar novamente `vending/{machine_id}/actions`;
- publicar heartbeat apenas quando MQTT estiver conectado;
- não executar comandos se MQTT estiver desconectado;
- não perder estabilidade dos comandos seriais de debug.

---

## 24. Idempotência e comandos duplicados

A ESP32-S3 deve evitar executar fisicamente o mesmo comando mais de uma vez.

Regra mínima:

```txt
Se command_id recebido == último command_id concluído, não acionar motor novamente.
```

Comportamento recomendado:

- manter `last_completed_command_id` em memória RAM;
- ao receber comando duplicado, publicar `MACHINE_LOG` ou `DISPENSE_FAILED` com reason `COMMAND_DUPLICATED` sem acionar motor;
- não usar NVS obrigatoriamente na v0.2;
- aceitar que a proteção se perca após reboot no MVP.

---

## 25. Relação com estados do backend

A firmware não altera diretamente estados comerciais. Ela apenas publica eventos que o backend processa.

Estados da venda no backend:

```txt
CREATED
AUTHORIZED
DISPENSING
DISPENSED
FAILED
REFUNDED
```

Fluxo de sucesso:

```txt
CREATED → AUTHORIZED → DISPENSING → DISPENSED
```

Fluxo de falha:

```txt
CREATED → AUTHORIZED → DISPENSING → FAILED → REFUNDED
```

Estados do comando físico no backend:

```txt
PENDING
PUBLISHED
ACKED
SUCCESS
FAILED
EXPIRED
```

Na v0.2, a ESP32-S3 não precisa publicar ACK explícito. O backend pode considerar `DISPENSE_STARTED` como evidência de recebimento/início.

---

## 26. Comandos seriais mantidos

A v0.2 deve manter comandos seriais para debug local:

| Comando | Descrição |
|---|---|
| `help` | Lista comandos disponíveis |
| `status` | Mostra estado geral da máquina |
| `sensor` | Mostra leitura dos sensores IR |
| `motor 1` | Teste curto do motor 1 |
| `motor 2` | Teste curto do motor 2 |
| `vend 1` | Executa venda local no slot/motor 1 |
| `vend 2` | Executa venda local no slot/motor 2 |
| `wifi` | Mostra estado Wi-Fi |
| `mqtt` | Mostra estado MQTT e tópicos |
| `heartbeat` | Publica heartbeat manualmente |

Regras:

- comandos locais não devem publicar eventos de venda como se fossem venda real do backend, a menos que o comando seja explicitamente de teste MQTT;
- testes locais devem ser claramente logados como `LOCAL_TEST`;
- comando serial não deve interromper uma venda MQTT em andamento.

---

## 27. Estrutura de código recomendada

A estrutura atual deve ser preservada e expandida sem reescrita desnecessária.

Estrutura recomendada:

```text
firmware/
├── platformio.ini
├── README.md
├── src/
│   ├── main.cpp
│   ├── config.hpp
│   ├── wifi/
│   │   ├── WifiManager.hpp
│   │   └── WifiManager.cpp
│   ├── mqtt/
│   │   ├── MqttManager.hpp
│   │   ├── MqttManager.cpp
│   │   ├── MqttTopics.hpp
│   │   ├── MqttPayloads.hpp
│   │   └── MqttPayloads.cpp
│   ├── motor/
│   │   ├── StepperMotor.hpp
│   │   └── StepperMotor.cpp
│   ├── sensor/
│   │   ├── IrBreakBeamSensor.hpp
│   │   └── IrBreakBeamSensor.cpp
│   ├── vending/
│   │   ├── VendingTypes.hpp
│   │   ├── VendingSlot.hpp
│   │   ├── VendingSlot.cpp
│   │   ├── VendingController.hpp
│   │   └── VendingController.cpp
│   └── serial/
│       ├── CommandHandler.hpp
│       └── CommandHandler.cpp
└── docs/
    ├── firmware-mqtt-v0.2.md
    ├── mqtt-contract.md
    ├── wiring.md
    └── commands.md
```

### 27.1 MqttManager

Responsabilidades:

- configurar cliente MQTT;
- conectar ao broker;
- reconectar em caso de queda;
- assinar tópico de ações;
- publicar eventos;
- publicar status;
- encaminhar payload recebido para o `VendingController` ou handler específico.

### 27.2 MqttPayloads

Responsabilidades:

- parsear JSON de comando;
- validar campos básicos;
- montar payloads JSON de eventos;
- montar payloads JSON de heartbeat;
- evitar duplicação de strings espalhadas pelo projeto.

### 27.3 VendingController

Responsabilidades adicionais na v0.2:

- receber comando estruturado vindo do MQTT;
- validar se está livre;
- resolver motor e sensor;
- executar tentativa/retry;
- chamar callbacks ou métodos do MQTT para publicar eventos;
- manter último `command_id` concluído.

---

## 28. Bibliotecas e componentes ESP-IDF

A implementação deve usar ESP-IDF, sem Arduino.

Componentes esperados:

- `esp_wifi` para Wi-Fi;
- `mqtt_client` ou componente equivalente do ESP-IDF para MQTT;
- `driver/gpio` para GPIO;
- `esp_timer` ou FreeRTOS tick para temporização;
- `esp_log` para logs;
- `cJSON` ou biblioteca JSON compatível disponível no ESP-IDF.

Não usar:

- `WiFi.h` do Arduino;
- `PubSubClient` do Arduino;
- `ArduinoJson` se o projeto estiver estritamente ESP-IDF sem Arduino;
- delays bloqueantes longos que prejudiquem conexão/reconexão.

---

## 29. Segurança operacional

A firmware deve garantir:

- nunca acionar dois motores simultaneamente;
- nunca executar comando se já estiver em `VENDING`;
- sempre aplicar timeout por tentativa;
- sempre desenergizar bobinas após sucesso, falha ou erro;
- validar motor antes de acionar;
- validar sensor antes de executar;
- ignorar comandos de outra máquina;
- não confiar em payload incompleto;
- não reiniciar por comando malformado;
- publicar falhas de forma clara;
- manter logs via serial;
- reconectar Wi-Fi e MQTT sem travar a máquina.

---

## 30. Critérios de aceite da v0.2

A v0.2 será considerada funcional quando:

1. O projeto compilar no PlatformIO usando ESP-IDF.
2. A ESP32-S3 conectar ao Wi-Fi.
3. Se o Wi-Fi cair, a ESP32-S3 tentar reconectar automaticamente.
4. A ESP32-S3 conectar ao broker MQTT.
5. Se o MQTT cair, a ESP32-S3 tentar reconectar automaticamente.
6. A ESP32-S3 assinar `vending/{machine_id}/actions`.
7. A ESP32-S3 publicar heartbeat em `vending/{machine_id}/status` a cada 30 segundos.
8. O backend receber heartbeat e conseguir marcar máquina como online.
9. A ESP32-S3 receber comando `DISPENSE` pelo tópico de ações.
10. A ESP32-S3 validar `machine_id`.
11. A ESP32-S3 validar `motor_id` e `sensor_column_id`.
12. A ESP32-S3 publicar `DISPENSE_STARTED`.
13. A ESP32-S3 acionar o motor correto.
14. A ESP32-S3 ler o sensor correto.
15. Ao detectar queda, publicar `SENSOR_TRIGGERED`.
16. Ao detectar queda, publicar `DISPENSE_SUCCESS`.
17. Se não detectar queda na primeira tentativa, publicar `DISPENSE_RETRY`.
18. Se não detectar queda após a segunda tentativa, publicar `DISPENSE_FAILED`.
19. Após sucesso, falha ou erro, desenergizar as bobinas do motor.
20. Um comando duplicado não pode acionar o motor novamente.
21. Comandos seriais locais de teste devem continuar funcionando.
22. O firmware não deve implementar nenhuma regra comercial do backend.

---

## 31. Prompt recomendado para implementação no Codex

Use este prompt após a documentação ser salva no repositório como `docs/firmware-mqtt-v0.2.md`.

```text
Você está trabalhando no firmware da ESP32-S3 do projeto acadêmico de vending machine.

Antes de alterar código, leia obrigatoriamente:

- AGENTS.md
- GEMINI.md
- docs/firmware-mqtt-v0.2.md
- docs/mqtt-contract.md, se existir
- README.md
- código atual em firmware/src

Contexto atual:

O projeto já conecta ao Wi-Fi, indica status Wi-Fi, testa motores, testa sensores IR e executa venda local girando o motor até detecção no sensor IR correspondente.

Nova tarefa:

Implementar a versão v0.2 da firmware com integração MQTT ao backend, mantendo somente 2 motores e 2 sensores IR.

Requisitos principais:

- Manter ESP-IDF puro em C++.
- Não usar Arduino.
- Não reescrever o projeto do zero.
- Não quebrar os testes locais já existentes.
- Adicionar reconexão Wi-Fi automática.
- Adicionar conexão MQTT.
- Adicionar reconexão MQTT automática.
- Assinar `vending/{machine_id}/actions`.
- Publicar heartbeat em `vending/{machine_id}/status` a cada 30 segundos.
- Processar comando `DISPENSE` recebido via MQTT.
- Validar JSON, type, machine_id, motor_id, sensor_column_id, quantity, attempts_allowed e timeout_ms_per_attempt.
- Executar o motor correto.
- Monitorar o sensor correto.
- Fazer até 2 tentativas por padrão.
- Publicar `DISPENSE_STARTED`, `SENSOR_TRIGGERED`, `DISPENSE_RETRY`, `DISPENSE_SUCCESS`, `DISPENSE_FAILED`, `MOTOR_ERROR`, `MACHINE_ERROR` e `MACHINE_LOG` conforme necessário.
- Evitar execução duplicada do mesmo `command_id`.
- Nunca executar dois motores ao mesmo tempo.
- Sempre desenergizar bobinas após sucesso, falha ou erro.
- Manter comandos seriais de debug.
- Atualizar README e documentação.

Contrato MQTT esperado:

Actions topic:
`vending/{machine_id}/actions`

Events topic:
`vending/{machine_id}/events`

Status topic:
`vending/{machine_id}/status`

Payload de comando:

{
  "type": "DISPENSE",
  "command_id": 123,
  "sale_id": 456,
  "machine_id": 1,
  "product_id": 10,
  "slot_id": 3,
  "slot_code": "A1",
  "motor_id": 1,
  "sensor_column_id": 1,
  "quantity": 1,
  "attempts_allowed": 2,
  "timeout_ms_per_attempt": 10000,
  "issued_at": "2026-06-08T12:00:00Z"
}

Primeiro, analise o código atual e proponha um plano de alteração com arquivos a criar/modificar. Depois implemente incrementalmente.

Ao final, informe:

1. arquivos criados/modificados;
2. como configurar Wi-Fi/MQTT;
3. como testar pelo monitor serial;
4. como testar publicando comando MQTT;
5. exemplos de payload MQTT;
6. limitações restantes.
```
