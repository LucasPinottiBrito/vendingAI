# Firmware ESP32-S3 v0.3

Controlador DC de 16 canais com SN74AHCT595, acionamento por tempo e barreira comum de quatro feixes IR. Ambiente PlatformIO com ESP-IDF. A configuração inicial habilita canais 1 a 3. O fim de curso e os SN74HC165 não são usados.

`MotorRunMs` em `src/config.hpp` define o tempo ligado por tentativa: **1000 ms provisórios**, sem garantia de uma volta completa. `VendTimeoutMs` limita a energização a no máximo 10 s; um timeout solicitado menor que `MotorRunMs` interrompe a tentativa com `MOTOR_TIMEOUT`. O encerramento ocorre na próxima atualização do laço de controle, sujeito ao escalonamento do FreeRTOS. O tempo encerrado não confirma entrega: `vend` e MQTT continuam exigindo detecção pelos sensores IR. O firmware não foi gravado na placa por esta alteração.

- [Pinagem e pré-requisitos de bancada](docs/wiring.md)
- [Fluxo e recuperação](docs/architecture.md)
- [Console serial](docs/commands.md)
- [Contrato MQTT](docs/mqtt-contract.md)
- [Relatório de desenvolvimento e protocolo experimental](../documents/Analise_firmware_acionamento_registradores.md)

Dentro de firmware, executar `pio run -e esp32-s3-devkitc-1` para compilar. Para os testes de lógica executar `python test/host/run.py` com g++ ou em um Developer Command Prompt do Visual Studio.
