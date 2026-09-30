# 🤖 Braço Robótico - Pick and Place por Cor

Este repositório contém o código de um braço robótico educacional (Prática de Laboratório 1 - 2026) focado em pegar e colocar objetos (tampas) classificando-os por cor. O sistema é capaz de identificar as cores **Vermelho, Azul e Preto**, ajustando seu alcance e rotação de acordo com a posição de cada cor na bancada. 

O grande diferencial deste projeto é o foco na **movimentação suave**, utilizando a biblioteca `VarSpeedServo` para criar rampas de aceleração e desaceleração, evitando tremores e garantindo que o braço não tombe devido à inércia dos motores.

## 🛠️ Hardware e Conexões

**Componentes Principais:**
*   Placa Arduino (ex: Uno, Mega, Leonardo)
*   4x Servomotores (Base, Ombro, Cotovelo, Garra)
*   1x Sensor de Cor Adafruit TCS34725

**Pinagem:**
*   **Base:** Pino Digital 9
*   **Ombro:** Pino Digital 10
*   **Cotovelo:** Pino Digital 11
*   **Garra:** Pino Digital 6
*   **Sensor TCS34725:** Pinos I2C (SDA -> A4 / SCL -> A5 / VCC -> 3.3V)

## 📚 Dependências

Para rodar os códigos deste repositório, você precisará instalar as seguintes bibliotecas na IDE do Arduino:
*   `Adafruit_TCS34725` - Para leitura do sensor de cor.
*   `VarSpeedServo` - Para o controle fluido dos servomotores.
*   `Wire.h` e `Servo.h` (Nativas do Arduino).

## 📁 Estrutura do Repositório

O projeto é dividido em quatro arquivos principais que acompanham todo o fluxo de desenvolvimento, desde testes básicos até o ciclo final automatizado:

1.  **`teste_rapido_angulos_servos.ino`**: Um script simples usando a biblioteca nativa `Servo.h` para varrer um servomotor conectado ao pino 9 de 0° a 180°. Ideal para validar a montagem eletrônica.
2.  **`calibrador_cores.ino`**: Ferramenta que coleta 50 amostras de leitura do sensor TCS34725 para as cores Vermelho, Azul e Preto. No final, ele gera e sugere automaticamente a lógica de *thresholds* e limites no formato do código principal.
3.  **`calibracao_braco.ino`**: Ferramenta de calibração interativa. Através do Monitor Serial (9600 baud), você pode enviar comandos como `+` (aumenta 1°), `++` (aumenta 5°) e `s` (salvar) para ajustar milimetricamente as 5 posições chave do braço (Inicial, Pegar, Alto, Esquerda, Soltar).
4.  **`braco_robotico_varspeed_6.ino`**: O código de produção (versão 6). Ele integra a leitura de cores calibrada e as posições de servo definidas para executar a rotina *Pick and Place* com movimentos suaves através da função `moverSuave()`. O ciclo inclui etapas de segurança, como recolher o cotovelo e levantar o ombro antes de giros bruscos da base para evitar colisões.

## 🚀 Como Usar (Passo a Passo)

Para colocar o braço robótico em funcionamento na sua bancada, siga esta ordem estrita:

1.  **Validação Física:** Faça o upload de `teste_rapido_angulos_servos.ino` para testar as portas e garantir que os motores têm energia suficiente para operar.
2.  **Calibração de Cor:** Faça o upload de `calibrador_cores.ino`.
    *   Abra o Serial Monitor (9600 baud).
    *   Posicione as tampas sob o sensor uma a uma (Vermelho, Azul e Preto) confirmando com qualquer tecla.
    *   Copie o código gerado ao final do processo.
3.  **Calibração de Movimento:** Faça o upload de `calibracao_braco.ino`.
    *   Abra o Serial Monitor (9600 baud).
    *   Use o teclado para selecionar motores (`1` a `4`) e defina os ângulos exatos para cada passo do robô. 
    *   Digite `p` para imprimir a lista final de ângulos calibrados.
4.  **Operação Final:** Abra o `braco_robotico_varspeed_6.ino`.
    *   Cole os dados de cor (do Passo 2) na função `lerCor()`.
    *   Cole os ângulos calibrados (do Passo 3) nas variáveis `const int` no início do código.
    *   Faça o upload e veja seu robô classificar as peças automaticamente!
