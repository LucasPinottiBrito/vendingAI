# Ligações v0.3 para bancada

| ESP32-S3 | Sinal | Direção |
|---|---|---|
| GPIO4, 5, 6 | DATA, CLOCK, LATCH dos dois 595 | saídas |
| GPIO1 (J3.4) | ENABLE pelo NPN, HIGH habilita | saída |
| GPIO15, 16, 17 | Antiga interface dos 165, sem uso nesta versão | não configurados pelo controlador |
| GPIO2, 42, 41, 40 (J3.5 a J3.8, nessa ordem) | IR1, IR2, IR3, IR4 | entradas digitais ativas em LOW |

Vista superior, antena em cima e USB embaixo: J3 é a barra direita. Esta pinagem usa JTAG por USB (padrão da ESP32-S3); não usar JTAG externo nos GPIO40–42 com os sensores conectados. Na migração, mover o fio que vai a R33 e as quatro entradas IR; manter o pino EN/RST da placa livre.

Os quatro GPIOs IR são uma escolha provisória para receptores com saída compatível com 3,3 V. Para coletor aberto, prever pull-up externo a 3,3 V. Conferir modelo, polaridade, alimentação e cabeamento antes de ligar. Cada emissor aponta para seu receptor do outro lado do espaço de queda.

A tabela Channels em config.hpp assume ordem crescente QA..QH por banco e habilita motores 1, 2 e 3 para a demonstração. Confirmar instalações e ordem física antes da carga. Verificar GPIO1 → transistor → OE_N nos dois 595 e associação QA..QH a OUT01..16.

**Acionamento por tempo, sem fim de curso:** manter o estágio original com MOSFET N no negativo. Positivo do motor em +12 V; negativo do motor no drain do IRLZ44N; source no GND da fonte e da lógica. Não ligar o negativo do motor diretamente ao GND, pois isso contorna o MOSFET. Manter flyback com cátodo em +12 V e ânodo no drain/negativo do motor.

O terceiro fio do motor (retorno medido em até 12 V) fica individualmente isolado, sem conexão com ESP32, 165, filtro, GND ou outra alimentação. Não montar a adaptação high-side anteriormente proposta apenas para viabilizar feedback: ela deixou de ser necessária para este modo. Os 165 e filtros de feedback podem ficar fora da montagem. Se já estiverem alimentados, manter entradas em níveis definidos, sem os retornos dos motores conectados.

Detalhes: [revisão elétrica vigente por tempo](../../electronic-circuit/Controle_motor_por_tempo.md).
