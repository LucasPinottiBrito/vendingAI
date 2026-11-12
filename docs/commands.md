# Comandos de bancada v0.3

Console USB Serial/JTAG, 115200 baud.

| Comando | Efeito |
|---|---|
| status | Estado, motor selecionado, modo timed, tempo configurado em ms, canais bloqueados e motivo da última parada. |
| sensor | Máscara de feixes interrompidos, IR1 no bit 0. |
| motor N | Um acionamento por MotorRunMs (1000 ms provisórios), sem exigir fim de curso nem queda. Pode dispensar fisicamente. |
| vend N | Uma tentativa com confirmação de queda na barreira comum. |
| recover N | Liberar motor bloqueado após inspeção e reposicionamento pelo operador. |
| wifi, mqtt, heartbeat, help | Diagnóstico de conexão, status e ajuda. |

N varia de 1 a 16 e o canal deve estar instalado. A máscara de bloqueio sobrevive a reinícios. O fim de curso não é lido. Ajustar `MotorRunMs` em `src/config.hpp` e recompilar para mudar o tempo; esse tempo não comprova uma revolução. `vend N` mantém a confirmação óptica de queda e a janela de observação após desligar o motor.
