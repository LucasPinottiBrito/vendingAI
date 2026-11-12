# Arquitetura v0.3

MotorController inicializa os GPIOs antes da conexão Wi-Fi e transmite 16 bits com latch. Desde 10/11/2026 cada acionamento termina pelo tempo `MotorRunMs` (1000 ms provisórios), sem ler ou configurar os GPIOs da antiga cadeia de feedback. ENABLE cai antes da limpeza do latch. O prazo efetivo é o menor entre `MotorRunMs`, o timeout da tentativa e `VendTimeoutMs` (10 s). Se o timeout truncar o tempo configurado, retorna `Timeout`; se forem iguais, retorna `Complete`. O laço consulta o relógio monotônico a cada tick; o desligamento pode ocorrer depois do prazo conforme a latência de escalonamento. Não se trata de corte independente por hardware.

DropDetector usa quatro receptores e interrupções nas duas bordas para memorizar a máscara de passagem e a duração do último pulso completo por feixe. Zero na duração indica ausência de pulso completo registrado. A qualificação óptica e a filtragem final dependem da bancada.

VendingController mantém um mutex em toda a operação, inclusive entre retentativas. Antes de energizar grava em NVS o canal em posição incerta. Após acionamento concluído por tempo libera o bloqueio, sem comprovar posição ou revolução mecânica. Em timeout (`MOTOR_TIMEOUT`) ou reinício durante o acionamento, o bloqueio persiste até recover N. Falha de persistência coloca a máquina em ERROR. A janela de queda de 750 ms ocorre com potência desligada e também vale após timeout. Uma queda encerra as tentativas mesmo com falha de acionamento; a detecção IR durante o giro é memorizada e não antecipa o término do tempo configurado.

Um período concluído sem queda resulta em `PRODUCT_NOT_DETECTED`, com retentativa se autorizada pelo comando e com barreira livre. Não há detecção de eixo travado ou confirmação de volta completa. O tempo deve ser ajustado ao mecanismo e à carga para reduzir erro de posicionamento e múltiplas dispensações.

Serial e MQTT usam o mesmo controlador. MQTT correlaciona os eventos pelo command_id, sale_id e slot_id, aciona pelo motor_id e usa sensor_column_id 1. Há deduplicação do último comando em RAM. Reentregas de comandos após reboot ou após outro comando não estão cobertas por esse histórico.
