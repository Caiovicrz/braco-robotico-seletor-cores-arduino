/*
 * =====================================================
 *  BRAÇO ROBÓTICO - PEGAR E COLOCAR POR COR
 *  Prática de Laboratório 1 - 2026
 *  Biblioteca: VarSpeedServo (movimento suave nativo)
 *  v5: thresholds recalibrados + movimento mais fluido
 *  v6: recalibração 2 (corrige confusão AZUL x PRETO)
 * =====================================================
 *
 *  Sequência do ciclo:
 *    1.  Base vai para a tampa (base + cotovelo por cor)
 *    2.  Abre garra
 *    3.  Desce e estende para pegar a tampa
 *    4.  Fecha garra → pega tampa
 *    5.  Recolhe cotovelo (evita tombar ao girar)
 *    6.  Sobe ombro alto
 *    7.  Gira base para a esquerda
 *    8.  Desce até altura de soltura
 *    9.  Abre garra → solta tampa
 *   10.  Recolhe cotovelo antes de voltar (evita derrubar caixa)
 *   11.  Sobe ombro alto novamente
 *   12.  Gira base para frente
 *   13.  Retorna à posição inicial
 *
 *  TAMPAS EM TRIÂNGULO:
 *    Cada cor tem seu próprio BASE e COTOV_PEGAR,
 *    calibrados individualmente pois as tampas
 *    não ficam em linha reta na bancada.
 * =====================================================
 */

#include <Wire.h>
#include <Adafruit_TCS34725.h>
#include <VarSpeedServo.h>

// ── Pinos ─────────────────────────────────────────────
#define PIN_BASE   9
#define PIN_OMBRO  10
#define PIN_COTOV  11
#define PIN_GARRA  6

// ── Instâncias VarSpeedServo ──────────────────────────
VarSpeedServo servoBase;
VarSpeedServo servoOmbro;
VarSpeedServo servoCotov;
VarSpeedServo servoGarra;

Adafruit_TCS34725 tcs = Adafruit_TCS34725(TCS34725_INTEGRATIONTIME_50MS, TCS34725_GAIN_4X);

// ── Velocidades ───────────────────────────────────────
// Valores menores = mais lento e suave (1–255)
#define VEL_BASE        25   // era 25 → menos tremor na rotação
#define VEL_BASE_LENTO  20   // rampa de saída/chegada da base
#define VEL_OMBRO       20   // era 20 → sobe/desce mais controlado
#define VEL_OMBRO_LENTO  15   // rampa ombro
#define VEL_COTOV       25   // era 25
#define VEL_COTOV_LENTO 15   // rampa cotovelo
#define VEL_GARRA       60   // era 60 → garra fecha sem chacoalhar

// ── Posição de repouso ────────────────────────────────
const int BASE_INICIAL   = 90;
const int OMBRO_INICIAL  = 80;
const int COTOV_INICIAL  = 60;
const int GARRA_ABERTA   = 65;
const int GARRA_FECHADA  = 100;

// ── Posição de pega — OMBRO é igual para todas as cores
const int OMBRO_PEGAR    = 60;

// ── Posições de pega POR COR ─────────────────────────
// As tampas formam um triângulo na bancada, então cada
// cor precisa de base e alcance (cotovelo) específicos.
// Use o arquivo de calibração para ajustar estes valores.
//
//                  [PRETO]
//               /
//   robô ──            [VERMELHO]
//               \
//                  [AZUL]
//
const int BASE_VERMELHO  = 70;   // ← CALIBRE: ângulo da base para o vermelho
const int COTOV_VERMELHO = 110;   // ← CALIBRE: alcance para o vermelho

const int BASE_PRETO     = 90;   // ← CALIBRE: ângulo da base para o preto
const int COTOV_PRETO    = 120;   // ← CALIBRE: alcance maior (tampa mais longe)

const int BASE_AZUL      = 112;   // ← CALIBRE: ângulo da base para o azul
const int COTOV_AZUL     = 107;   // ← CALIBRE: alcance para o azul

// ── Transporte ────────────────────────────────────────
const int COTOV_RECOLHER = 60;    // recolhe ANTES de girar (evita tombar)
const int OMBRO_ALTO     = 165;
const int COTOV_ALTO     = 110;

// ── Destino: esquerda ─────────────────────────────────
const int BASE_ESQUERDA  = 45;
const int OMBRO_SOLTAR   = 150;
const int COTOV_SOLTAR   = 85;

// ── Base intermediária de segurança na volta ──────────
// Após soltar, o braço passa por BASE_FRENTE antes de
// ir para BASE_INICIAL, evitando varrer a caixa no caminho.
const int BASE_FRENTE    = 110;

// ── Pausa entre passos (ms) ───────────────────────────
#define ESPERA       400   // era 500 — servo já parou, pausa menor
#define ESPERA_CURTA 200   // entre micro-passos da rampa

// =====================================================
//  SETUP
// =====================================================
void setup() {
  Serial.begin(9600);
  Serial.println(F("=== BRAÇO ROBÓTICO - SENSOR DE COR ==="));

  servoBase.attach(PIN_BASE);
  servoOmbro.attach(PIN_OMBRO);
  servoCotov.attach(PIN_COTOV);
  servoGarra.attach(PIN_GARRA);

  posicaoInicial(true);
  Serial.println(F("Posição inicial OK."));

  if (!tcs.begin()) {
    Serial.println(F("ERRO: TCS34725 não encontrado!"));
    while (1);
  }
  Serial.println(F("Sensor OK. Aproxime a tampa..."));
  Serial.println(F("--------------------------------------"));
}

// =====================================================
//  LOOP PRINCIPAL
// =====================================================
void loop() {
  String cor = lerCor();

  if (cor == "INDEFINIDA") {
    Serial.println(F("Cor não identificada. Tente novamente."));
    delay(1000);
    return;
  }

  Serial.print(F("Cor detectada: "));
  Serial.println(cor);

  // Seleciona base e cotovelo específicos da cor
  int baseCorr  = BASE_VERMELHO;
  int cotovCorr = COTOV_VERMELHO;

  if (cor == "PRETO") {
    baseCorr  = BASE_PRETO;
    cotovCorr = COTOV_PRETO;
  } else if (cor == "AZUL") {
    baseCorr  = BASE_AZUL;
    cotovCorr = COTOV_AZUL;
  }

  cicloPickAndPlace(baseCorr, cotovCorr);

  Serial.println(F("--------------------------------------"));
  Serial.println(F("Ciclo completo! Próxima leitura em 5s..."));
  delay(5000);
}

// =====================================================
//  CICLO COMPLETO
//  baseCorr  → ângulo da base para a tampa desta cor
//  cotovCorr → alcance do cotovelo para esta cor
// =====================================================
void cicloPickAndPlace(int baseCorr, int cotovCorr) {

  // ── [1] Base vai para a posição da tampa desta cor ─
  Serial.println(F("[1] Posicionando base para a tampa..."));
  moverSuave(servoBase, baseCorr, VEL_BASE_LENTO, VEL_BASE);
  delay(ESPERA);

  // ── [2] Abrir garra ────────────────────────────────
  Serial.println(F("[2] Abrindo garra..."));
  servoGarra.write(GARRA_ABERTA, VEL_GARRA, true);
  delay(ESPERA);

  // ── [3] Descer e esticar para pegar ────────────────
  Serial.println(F("[3] Descendo para pegar..."));
  servoOmbro.write(OMBRO_PEGAR, VEL_OMBRO, false);
  servoCotov.write(cotovCorr,   VEL_COTOV, false);  // alcance da cor
  aguardarServos();
  delay(ESPERA);

  // ── [4] Fechar garra ───────────────────────────────
  Serial.println(F("[4] Fechando garra..."));
  servoGarra.write(GARRA_FECHADA, VEL_GARRA, true);
  delay(ESPERA);

  // ── [5] Recolher cotovelo antes de girar ───────────
  // Braço esticado ao girar → desequilíbrio → robô tomba.
  // Recolher primeiro centraliza o peso sobre a base.
  Serial.println(F("[5] Recolhendo cotovelo (segurança)..."));
  servoCotov.write(COTOV_RECOLHER, VEL_COTOV, true);
  delay(ESPERA);

  
    // ── [7] Subir ombro alto (cotovelo já recolhido) ───
  Serial.println(F("[7] Subindo para transporte..."));
  servoOmbro.write(OMBRO_ALTO, VEL_OMBRO, false); 
  aguardarServos();
  delay(ESPERA);
  
  
    // ── [6] Girar para a esquerda ──────────────────────
  Serial.println(F("[6] Girando para a esquerda..."));
  moverSuave(servoBase, BASE_ESQUERDA, VEL_BASE_LENTO, VEL_BASE);
  delay(ESPERA);


  

  // ── [8] Subir ombro alto (cotovelo já recolhido) ───
  Serial.println(F("[8] esticando para entrega..."));
  servoCotov.write(COTOV_ALTO, VEL_COTOV, false);
  aguardarServos();
  delay(ESPERA);


  // ── [9] Abrir garra ────────────────────────────────
  Serial.println(F("[9] Soltando tampa..."));
  servoGarra.write(GARRA_ABERTA, VEL_GARRA, true);
  delay(ESPERA);

  // ── [10] Recolher ANTES de sair da área da caixa ───
  // Sem este passo o braço varre a caixa ao girar de volta.
  Serial.println(F("[10] Recolhendo (evita derrubar caixa)..."));
  servoCotov.write(COTOV_RECOLHER, VEL_COTOV, true);  // cotovelo
  servoOmbro.write(OMBRO_ALTO,     VEL_OMBRO, true);  // ombro
  aguardarServos();
  delay(ESPERA);

  // ── [11] Girar base para posição central (frente) ──
  Serial.println(F("[11] Girando base para frente..."));
  moverSuave(servoBase, BASE_FRENTE, VEL_BASE_LENTO, VEL_BASE);
  delay(ESPERA);

  // ── [12] Voltar à posição inicial ──────────────────
  Serial.println(F("[12] Voltando à posição inicial..."));
  posicaoInicial(true);
}

// =====================================================
//  POSIÇÃO INICIAL
// =====================================================
void posicaoInicial(bool waitForIt) {
  // Recolhe cotovelo e sobe ombro ANTES de girar a base
  servoCotov.write(COTOV_INICIAL, VEL_COTOV, false);
  servoOmbro.write(OMBRO_INICIAL, VEL_OMBRO, false);
  servoGarra.write(GARRA_ABERTA,  VEL_GARRA, false);
  aguardarServos();

  // Base com rampa para não chacoalhar ao chegar na posição
  moverSuave(servoBase, BASE_INICIAL, VEL_BASE_LENTO, VEL_BASE);
  if (waitForIt) delay(ESPERA);
}

// =====================================================
//  AGUARDA TODOS OS SERVOS TERMINAREM
// =====================================================
void aguardarServos() {
  delay(100);
  int prevBase, prevOmbro, prevCotov, prevGarra;
  do {
    prevBase  = servoBase.read();
    prevOmbro = servoOmbro.read();
    prevCotov = servoCotov.read();
    prevGarra = servoGarra.read();
    delay(60);
  } while (
    abs(servoBase.read()  - prevBase)  > 1 ||
    abs(servoOmbro.read() - prevOmbro) > 1 ||
    abs(servoCotov.read() - prevCotov) > 1 ||
    abs(servoGarra.read() - prevGarra) > 1
  );
}

// =====================================================
//  MOVIMENTO COM RAMPA (aceleração + desaceleração)
//  Divide o percurso em 3 fases:
//    1. 25% do trajeto na velocidade lenta (arranca)
//    2. 50% na velocidade normal (cruzeiro)
//    3. 25% na velocidade lenta de novo (freia)
//  Isso elimina o tremor por inércia no início e fim.
// =====================================================
void moverSuave(VarSpeedServo &servo, int destino,
                int velLenta, int velNormal) {
  int origem = servo.read();
  int total  = destino - origem;
  if (total == 0) return;

  int p1 = origem  + total * 25 / 100;  // 25%
  int p2 = origem  + total * 75 / 100;  // 75%

  servo.write(p1,      velLenta,  true);  // arranca devagar
  servo.write(p2,      velNormal, true);  // velocidade de cruzeiro
  servo.write(destino, velLenta,  true);  // freia antes de parar
  delay(ESPERA_CURTA);
}

// =====================================================
//  LÊ E CLASSIFICA A COR (TCS34725)
//  Thresholds recalibrados (2ª calibração — corrige
//  confusão entre AZUL e PRETO sob a luz do local).
//  Dados do calibrador (50 amostras por cor):
//    VERMELHO → C: 1636–2126 | R dominante (razão > 3.60, r > 1207)
//    AZUL     → C: 266–770   | G e B > R, r < 154
//    PRETO    → C: 287–396   | pouca luz refletida (c < 435)
// =====================================================
String lerCor() {
  uint16_t r, g, b, c;
  tcs.getRawData(&r, &g, &b, &c);

  if (c == 0) return "INDEFINIDA";

  Serial.print(F("R: "));   Serial.print(r);
  Serial.print(F(" | G: ")); Serial.print(g);
  Serial.print(F(" | B: ")); Serial.print(b);
  Serial.print(F(" | C: ")); Serial.println(c);

  // VERMELHO: canal R claramente dominante sobre G e B
  // (verificado antes do PRETO pois o vermelho tem C alto e não conflita)
  float razaoRG = (float)r / (g > 0 ? g : 1);
  float razaoRB = (float)r / (b > 0 ? b : 1);
  if (razaoRG > 3.60 && razaoRB > 3.60 && r > 1207) return "VERMELHO";

  // AZUL: G e B maiores que R — checado ANTES do PRETO porque os
  // ranges de C se sobrepõem (AZUL: 266-770 / PRETO: 287-396).
  // O que diferencia de fato é a relação entre os canais, não o C.
  if (g > r && b > r && r < 154 && c >= 239) return "AZUL";

  // PRETO: pouca luz refletida (limiar calibrado = 435)
  if (c < 435) return "PRETO";

  return "INDEFINIDA";
}
