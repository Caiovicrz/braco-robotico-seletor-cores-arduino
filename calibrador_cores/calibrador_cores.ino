/*
 * =====================================================
 *  CALIBRADOR DE COR - TCS34725
 *  Prática de Laboratório 1 - 2026
 * =====================================================
 *  Como usar:
 *    1. Abra o Serial Monitor a 9600 baud
 *    2. Aproxime o sensor da tampa VERMELHA e pressione
 *       qualquer tecla (ou envie "ok") para iniciar
 *    3. O código coleta 50 amostras e imprime os ranges
 *    4. Repita para AZUL e depois PRETO
 *    5. Copie os valores para o código principal
 *
 *  Hardware:
 *    TCS34725: SDA → A4 | SCL → A5 | VCC → 3.3V | GND → GND
 * =====================================================
 */

#include <Wire.h>
#include <Adafruit_TCS34725.h>

// Sensor com ganho x4 e integração 50ms (igual ao projeto principal)
Adafruit_TCS34725 tcs = Adafruit_TCS34725(TCS34725_INTEGRATIONTIME_50MS, TCS34725_GAIN_4X);

// ── Configuração ──────────────────────────────────────
#define NUM_AMOSTRAS  50    // amostras por cor
#define INTERVALO_MS  150   // intervalo entre leituras (ms)

// ── Estrutura para guardar estatísticas de cada cor ───
struct EstatisticasCor {
  const char* nome;
  uint16_t rMin, rMax;
  uint16_t gMin, gMax;
  uint16_t bMin, bMax;
  uint16_t cMin, cMax;
  float    rMedia, gMedia, bMedia, cMedia;
};

// Três cores para calibrar (ordem: VERMELHO → AZUL → PRETO)
EstatisticasCor cores[3] = {
  { "VERMELHO", 65535, 0, 65535, 0, 65535, 0, 65535, 0, 0, 0, 0, 0 },
  { "AZUL",     65535, 0, 65535, 0, 65535, 0, 65535, 0, 0, 0, 0, 0 },
  { "PRETO",    65535, 0, 65535, 0, 65535, 0, 65535, 0, 0, 0, 0, 0 }
};

// =====================================================
//  SETUP
// =====================================================
void setup() {
  Serial.begin(9600);
  while (!Serial); // aguarda Serial estar pronto (útil no Leonardo/Mega)

  Serial.println(F("============================================"));
  Serial.println(F("  CALIBRADOR DE COR - TCS34725"));
  Serial.println(F("============================================"));

  if (!tcs.begin()) {
    Serial.println(F("ERRO: Sensor TCS34725 nao encontrado!"));
    Serial.println(F("Verifique a fiacao e reinicie."));
    while (1);
  }
  Serial.println(F("Sensor TCS34725 OK!"));
  Serial.println();

  // Calibra as 3 cores em sequência
  for (int i = 0; i < 3; i++) {
    calibrarCor(i);
  }

  // Imprime resumo final com os thresholds prontos para copiar
  imprimirResumo();

  Serial.println(F("============================================"));
  Serial.println(F("Calibracao concluida! Copie os valores acima"));
  Serial.println(F("para a funcao lerCor() do projeto principal."));
  Serial.println(F("============================================"));
}

// =====================================================
//  LOOP — nada a fazer após a calibração
// =====================================================
void loop() {
  // Mantém o Serial Monitor aberto para consulta
  delay(10000);
}

// =====================================================
//  CALIBRA UMA COR: espera confirmação, coleta amostras
// =====================================================
void calibrarCor(int indice) {
  EstatisticasCor& cor = cores[indice];

  Serial.println(F("--------------------------------------------"));
  Serial.print(F("Proximo: tampa "));
  Serial.println(cor.nome);
  Serial.println(F("  -> Posicione o sensor sobre a tampa"));
  Serial.println(F("  -> Quando pronto, envie qualquer tecla no Serial Monitor"));
  Serial.println(F("--------------------------------------------"));

  // Aguarda o usuário enviar qualquer byte pelo Serial Monitor
  aguardarEntrada();

  Serial.print(F("Coletando "));
  Serial.print(NUM_AMOSTRAS);
  Serial.print(F(" amostras de "));
  Serial.print(cor.nome);
  Serial.println(F("..."));
  Serial.println(F("  N   |   R   |   G   |   B   |   C"));
  Serial.println(F("------|-------|-------|-------|-------"));

  uint32_t somaR = 0, somaG = 0, somaB = 0, somaC = 0;

  for (int n = 1; n <= NUM_AMOSTRAS; n++) {
    uint16_t r, g, b, c;
    tcs.getRawData(&r, &g, &b, &c);

    // Atualiza mínimos e máximos
    if (r < cor.rMin) cor.rMin = r;
    if (r > cor.rMax) cor.rMax = r;
    if (g < cor.gMin) cor.gMin = g;
    if (g > cor.gMax) cor.gMax = g;
    if (b < cor.bMin) cor.bMin = b;
    if (b > cor.bMax) cor.bMax = b;
    if (c < cor.cMin) cor.cMin = c;
    if (c > cor.cMax) cor.cMax = c;

    somaR += r;
    somaG += g;
    somaB += b;
    somaC += c;

    // Imprime cada leitura formatada
    Serial.print(F("  "));
    if (n < 10) Serial.print(F(" "));
    Serial.print(n);
    Serial.print(F("  | "));
    imprimirValor(r); Serial.print(F(" | "));
    imprimirValor(g); Serial.print(F(" | "));
    imprimirValor(b); Serial.print(F(" | "));
    imprimirValor(c);
    Serial.println();

    delay(INTERVALO_MS);
  }

  // Calcula médias
  cor.rMedia = (float)somaR / NUM_AMOSTRAS;
  cor.gMedia = (float)somaG / NUM_AMOSTRAS;
  cor.bMedia = (float)somaB / NUM_AMOSTRAS;
  cor.cMedia = (float)somaC / NUM_AMOSTRAS;

  // Resumo imediato da cor calibrada
  Serial.println(F("------|-------|-------|-------|-------"));
  Serial.print(F("MIN   | "));
  imprimirValor(cor.rMin); Serial.print(F(" | "));
  imprimirValor(cor.gMin); Serial.print(F(" | "));
  imprimirValor(cor.bMin); Serial.print(F(" | "));
  imprimirValor(cor.cMin); Serial.println();

  Serial.print(F("MAX   | "));
  imprimirValor(cor.rMax); Serial.print(F(" | "));
  imprimirValor(cor.gMax); Serial.print(F(" | "));
  imprimirValor(cor.bMax); Serial.print(F(" | "));
  imprimirValor(cor.cMax); Serial.println();

  Serial.print(F("MEDIA | "));
  Serial.print(cor.rMedia, 0); Serial.print(F("   | "));
  Serial.print(cor.gMedia, 0); Serial.print(F("   | "));
  Serial.print(cor.bMedia, 0); Serial.print(F("   | "));
  Serial.print(cor.cMedia, 0); Serial.println();

  Serial.print(F("  "));
  Serial.print(cor.nome);
  Serial.println(F(" calibrada!"));
  Serial.println();
}

// =====================================================
//  IMPRIME RESUMO FINAL COM THRESHOLDS PRONTOS
//  (formato copiável diretamente para o lerCor())
// =====================================================
void imprimirResumo() {
  Serial.println();
  Serial.println(F("============================================"));
  Serial.println(F("  RESUMO - THRESHOLDS CALIBRADOS"));
  Serial.println(F("============================================"));

  for (int i = 0; i < 3; i++) {
    EstatisticasCor& cor = cores[i];
    Serial.print(F("  "));
    Serial.print(cor.nome);
    Serial.println(F(":"));
    Serial.print(F("    R: ")); Serial.print(cor.rMin); Serial.print(F(" - ")); Serial.println(cor.rMax);
    Serial.print(F("    G: ")); Serial.print(cor.gMin); Serial.print(F(" - ")); Serial.println(cor.gMax);
    Serial.print(F("    B: ")); Serial.print(cor.bMin); Serial.print(F(" - ")); Serial.println(cor.bMax);
    Serial.print(F("    C: ")); Serial.print(cor.cMin); Serial.print(F(" - ")); Serial.println(cor.cMax);
    Serial.println();
  }

  // Calcula e sugere thresholds com margem de segurança (+10%)
  EstatisticasCor& v = cores[0]; // VERMELHO
  EstatisticasCor& a = cores[1]; // AZUL
  EstatisticasCor& p = cores[2]; // PRETO

  // Threshold recomendado para PRETO: cMax do preto + 10% de margem
  uint16_t limitePreto = (uint16_t)(p.cMax * 1.10);
  // Razão R/G mínima observada no vermelho (usa a mais conservadora)
  float razaoRG_min = (float)v.rMin / (float)v.gMax;
  float razaoRB_min = (float)v.rMin / (float)v.bMax;
  // Usa a menor das duas razões e subtrai 10% de margem
  float razaoMin = min(razaoRG_min, razaoRB_min) * 0.90;
  // R mínimo para vermelho com margem
  uint16_t rMinVerm = (uint16_t)(v.rMin * 0.85);
  // Threshold de R máximo para azul (rMax do azul + 20% margem)
  uint16_t rMaxAzul = (uint16_t)(a.rMax * 1.20);
  // C mínimo para azul (cMin do azul - 10% margem)
  uint16_t cMinAzul = (uint16_t)(a.cMin * 0.90);

  Serial.println(F("============================================"));
  Serial.println(F("  CODIGO SUGERIDO PARA lerCor()"));
  Serial.println(F("  (cole no projeto principal)"));
  Serial.println(F("============================================"));
  Serial.println();
  Serial.println(F("  // PRETO: pouca luz refletida"));
  Serial.print(F("  if (c < "));
  Serial.print(limitePreto);
  Serial.println(F(") return \"PRETO\";"));
  Serial.println();
  Serial.println(F("  // VERMELHO: canal R dominante"));
  Serial.print(F("  float razaoRG = (float)r / (g > 0 ? g : 1);"));
  Serial.println();
  Serial.print(F("  float razaoRB = (float)r / (b > 0 ? b : 1);"));
  Serial.println();
  Serial.print(F("  if (razaoRG > "));
  Serial.print(razaoMin, 2);
  Serial.print(F(" && razaoRB > "));
  Serial.print(razaoMin, 2);
  Serial.print(F(" && r > "));
  Serial.print(rMinVerm);
  Serial.println(F(") return \"VERMELHO\";"));
  Serial.println();
  Serial.println(F("  // AZUL: G e B maiores que R"));
  Serial.print(F("  if (g > r && b > r && r < "));
  Serial.print(rMaxAzul);
  Serial.print(F(" && c >= "));
  Serial.print(cMinAzul);
  Serial.println(F(") return \"AZUL\";"));
  Serial.println();
  Serial.println(F("  return \"INDEFINIDA\";"));
  Serial.println();
}

// =====================================================
//  AGUARDA QUALQUER BYTE PELO SERIAL MONITOR
// =====================================================
void aguardarEntrada() {
  // Descarta bytes pendentes
  while (Serial.available()) Serial.read();
  // Espera novo byte
  while (!Serial.available()) delay(50);
  while (Serial.available()) Serial.read(); // limpa buffer
}

// =====================================================
//  IMPRIME VALOR COM PADDING PARA ALINHAR COLUNAS
// =====================================================
void imprimirValor(uint16_t val) {
  if (val < 10)   Serial.print(F("   "));
  else if (val < 100)  Serial.print(F("  "));
  else if (val < 1000) Serial.print(F(" "));
  Serial.print(val);
}
