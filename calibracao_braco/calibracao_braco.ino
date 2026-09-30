/*
 * =====================================================
 *  CALIBRAÇÃO DO BRAÇO ROBÓTICO
 *  Ferramenta de calibração interativa via Serial
 * =====================================================
 *
 *  COMO USAR:
 *    1. Faça upload DESTE arquivo no Arduino
 *    2. Abra o Monitor Serial (9600 baud)
 *    3. Siga o menu para ajustar cada ângulo
 *    4. Anote os valores exibidos no final
 *    5. Cole os valores no arquivo principal
 *
 *  COMANDOS NO MONITOR SERIAL:
 *    Digite um número de 1 a 4 → seleciona o servo
 *    +   → aumenta 1°
 *    -   → diminui 1°
 *    ++  → aumenta 5°
 *    --  → diminui 5°
 *    s   → SALVA este ângulo para a posição atual
 *    n   → próxima posição
 *    r   → repete o movimento (executa ciclo com valores salvos)
 *    p   → imprime todos os valores salvos até agora
 *    m   → mostra o menu novamente
 *
 * =====================================================
 */

#include <VarSpeedServo.h>

// ── Pinos (iguais ao código principal) ────────────────
#define PIN_BASE   9
#define PIN_OMBRO  10
#define PIN_COTOV  11
#define PIN_GARRA  6

// ── Velocidade de calibração (lenta para controle fino) 
#define VEL_CAL  30

VarSpeedServo servoBase;
VarSpeedServo servoOmbro;
VarSpeedServo servoCotov;
VarSpeedServo servoGarra;

// ── Ângulos atuais em tempo real ──────────────────────
int angBase  = 90;
int angOmbro = 150;
int angCotov = 60;
int angGarra = 30;

// ── Estrutura para guardar cada posição do ciclo ──────
struct Posicao {
  const char* nome;
  int base;
  int ombro;
  int cotov;
  int garra;
  const char* descricao;
};

// ── As 6 posições que precisam ser calibradas ─────────
Posicao posicoes[] = {
  { "INICIAL",   90, 150, 60, 30,  "Repouso: braço erguido, garra aberta"           },
  { "PEGAR",     90, 150, 60, 30,  "Frente da tampa: ajuste cotov p/ esticar"        },
  { "ALTO",      90, 170, 40, 90,  "Transporte: sobe bastante antes de girar"        },
  { "ESQUERDA", 170, 150, 60, 90,  "Destino: base à esquerda com tampa"              },
  { "SOLTAR",   170, 150, 60, 90,  "Soltura: altura certa sobre a caixa"             },
};
const int NUM_POSICOES = 5;

int posAtual = 0;   // qual posição está sendo calibrada
int servoSel = 2;   // servo selecionado: 1=base 2=ombro 3=cotov 4=garra

// ──────────────────────────────────────────────────────
void setup() {
  Serial.begin(9600);
  while (!Serial) {}

  servoBase.attach(PIN_BASE);
  servoOmbro.attach(PIN_OMBRO);
  servoCotov.attach(PIN_COTOV);
  servoGarra.attach(PIN_GARRA);

  // Vai para posição inicial suavemente
  moverTodos(angBase, angOmbro, angCotov, angGarra, true);

  imprimirBoasVindas();
  imprimirPosicaoAtual();
  imprimirMenu();
}

// ──────────────────────────────────────────────────────
void loop() {
  if (!Serial.available()) return;

  String cmd = Serial.readStringUntil('\n');
  cmd.trim();
  cmd.toLowerCase();

  if (cmd == "") return;

  // ── Seleção de servo ──────────────────────────────
  if (cmd == "1") { servoSel = 1; Serial.println(F(">>> Servo: BASE")); }
  else if (cmd == "2") { servoSel = 2; Serial.println(F(">>> Servo: OMBRO")); }
  else if (cmd == "3") { servoSel = 3; Serial.println(F(">>> Servo: COTOVELO")); }
  else if (cmd == "4") { servoSel = 4; Serial.println(F(">>> Servo: GARRA")); }

  // ── Ajuste fino ──────────────────────────────────
  else if (cmd == "+")  ajustar(+1);
  else if (cmd == "-")  ajustar(-1);
  else if (cmd == "++") ajustar(+5);
  else if (cmd == "--") ajustar(-5);
  else if (cmd == "+++") ajustar(+10);
  else if (cmd == "---") ajustar(-10);

  // ── Ir direto para um ângulo (ex: "90" ou "b=45") ─
  else if (cmd.startsWith("b=")) definirAngulo(1, cmd.substring(2).toInt());
  else if (cmd.startsWith("o=")) definirAngulo(2, cmd.substring(2).toInt());
  else if (cmd.startsWith("c=")) definirAngulo(3, cmd.substring(2).toInt());
  else if (cmd.startsWith("g=")) definirAngulo(4, cmd.substring(2).toInt());
  else if (isNumeric(cmd))       definirAngulo(servoSel, cmd.toInt());

  // ── Controle do fluxo ──────────────────────────
  else if (cmd == "s") salvarPosicao();
  else if (cmd == "n") proximaPosicao();
  else if (cmd == "v") voltarPosicao();
  else if (cmd == "r") executarCiclo();
  else if (cmd == "p") imprimirResultados();
  else if (cmd == "m") imprimirMenu();
  else if (cmd == "status") imprimirStatus();
  else {
    Serial.print(F("Comando desconhecido: "));
    Serial.println(cmd);
  }
}

// ──────────────────────────────────────────────────────
//  AJUSTA O SERVO SELECIONADO
// ──────────────────────────────────────────────────────
void ajustar(int delta) {
  int* ang = getAngPtr(servoSel);
  int novo = constrain(*ang + delta, 0, 180);
  *ang = novo;
  moverServo(servoSel, novo);
  imprimirStatus();
}

void definirAngulo(int servo, int angulo) {
  int* ang = getAngPtr(servo);
  *ang = constrain(angulo, 0, 180);
  servoSel = servo;
  moverServo(servo, *ang);
  imprimirStatus();
}

int* getAngPtr(int servo) {
  if (servo == 1) return &angBase;
  if (servo == 2) return &angOmbro;
  if (servo == 3) return &angCotov;
  return &angGarra;
}

void moverServo(int servo, int angulo) {
  if (servo == 1) servoBase.write(angulo,  VEL_CAL, true);
  if (servo == 2) servoOmbro.write(angulo, VEL_CAL, true);
  if (servo == 3) servoCotov.write(angulo, VEL_CAL, true);
  if (servo == 4) servoGarra.write(angulo, VEL_CAL, true);
}

void moverTodos(int b, int o, int c, int g, bool aguardar) {
  servoBase.write(b,  VEL_CAL, false);
  servoOmbro.write(o, VEL_CAL, false);
  servoCotov.write(c, VEL_CAL, false);
  servoGarra.write(g, VEL_CAL, aguardar);
  if (aguardar) delay(300);
}

// ──────────────────────────────────────────────────────
//  SALVAR / NAVEGAR POSIÇÕES
// ──────────────────────────────────────────────────────
void salvarPosicao() {
  posicoes[posAtual].base  = angBase;
  posicoes[posAtual].ombro = angOmbro;
  posicoes[posAtual].cotov = angCotov;
  posicoes[posAtual].garra = angGarra;

  Serial.print(F("\n✓ SALVO ["));
  Serial.print(posicoes[posAtual].nome);
  Serial.print(F("] B="));
  Serial.print(angBase);
  Serial.print(F(" O="));
  Serial.print(angOmbro);
  Serial.print(F(" C="));
  Serial.print(angCotov);
  Serial.print(F(" G="));
  Serial.println(angGarra);
  Serial.println(F("  Digite 'n' para próxima posição ou continue ajustando."));
}

void proximaPosicao() {
  // Auto-salva antes de avançar
  salvarPosicao();

  if (posAtual < NUM_POSICOES - 1) {
    posAtual++;
    // Carrega os ângulos já salvos desta posição
    angBase  = posicoes[posAtual].base;
    angOmbro = posicoes[posAtual].ombro;
    angCotov = posicoes[posAtual].cotov;
    angGarra = posicoes[posAtual].garra;
    moverTodos(angBase, angOmbro, angCotov, angGarra, true);
    Serial.println();
    imprimirPosicaoAtual();
  } else {
    Serial.println(F("\n=== Todas as posições calibradas! ==="));
    Serial.println(F("Digite 'r' para testar o ciclo completo"));
    Serial.println(F("ou 'p' para ver os valores finais."));
  }
}

void voltarPosicao() {
  if (posAtual > 0) {
    posAtual--;
    angBase  = posicoes[posAtual].base;
    angOmbro = posicoes[posAtual].ombro;
    angCotov = posicoes[posAtual].cotov;
    angGarra = posicoes[posAtual].garra;
    moverTodos(angBase, angOmbro, angCotov, angGarra, true);
    imprimirPosicaoAtual();
  } else {
    Serial.println(F("Já está na primeira posição."));
  }
}

// ──────────────────────────────────────────────────────
//  EXECUTAR CICLO COMPLETO COM OS VALORES SALVOS
// ──────────────────────────────────────────────────────
void executarCiclo() {
  Serial.println(F("\n>>> EXECUTANDO CICLO COMPLETO..."));
  delay(500);

  Serial.println(F("[1] Posição INICIAL"));
  moverTodos(posicoes[0].base, posicoes[0].ombro, posicoes[0].cotov, posicoes[0].garra, true);
  delay(800);

  Serial.println(F("[2] Posição PEGAR (frente + estender)"));
  moverTodos(posicoes[1].base, posicoes[1].ombro, posicoes[1].cotov, GARRA_ABERTA_VAL(), true);
  delay(800);

  Serial.println(F("[3] Fechando garra"));
  servoGarra.write(posicoes[1].garra, VEL_CAL, true);
  delay(600);

  Serial.println(F("[4] Subindo ALTO para transporte"));
  moverTodos(posicoes[1].base, posicoes[2].ombro, posicoes[2].cotov, posicoes[2].garra, true);
  delay(800);

  Serial.println(F("[5] Girando para ESQUERDA"));
  servoBase.write(posicoes[3].base, VEL_CAL, true);
  delay(600);

  Serial.println(F("[6] Descendo para SOLTAR"));
  servoOmbro.write(posicoes[4].ombro, VEL_CAL, false);
  servoCotov.write(posicoes[4].cotov, VEL_CAL, true);
  delay(600);

  Serial.println(F("[7] Abrindo garra (soltando)"));
  servoGarra.write(GARRA_ABERTA_VAL(), VEL_CAL, true);
  delay(600);

  Serial.println(F("[8] Voltando à posição INICIAL"));
  moverTodos(posicoes[0].base, posicoes[0].ombro, posicoes[0].cotov, posicoes[0].garra, true);

  angBase  = posicoes[0].base;
  angOmbro = posicoes[0].ombro;
  angCotov = posicoes[0].cotov;
  angGarra = posicoes[0].garra;

  Serial.println(F(">>> Ciclo concluído!\n"));
}

int GARRA_ABERTA_VAL() { return posicoes[0].garra; }

// ──────────────────────────────────────────────────────
//  IMPRESSÃO DE RESULTADOS FINAIS
//  Cole estes valores no arquivo principal!
// ──────────────────────────────────────────────────────
void imprimirResultados() {
  Serial.println(F("\n"));
  Serial.println(F("╔══════════════════════════════════════════════════╗"));
  Serial.println(F("║         VALORES CALIBRADOS — COPIE ESTES        ║"));
  Serial.println(F("║    Cole em braco_robotico_varspeed.ino           ║"));
  Serial.println(F("╚══════════════════════════════════════════════════╝"));
  Serial.println();

  Serial.println(F("// ── Posição de repouso ────────────────────────────"));
  Serial.print(F("const int BASE_INICIAL  = ")); Serial.print(posicoes[0].base);  Serial.println(F(";"));
  Serial.print(F("const int OMBRO_INICIAL = ")); Serial.print(posicoes[0].ombro); Serial.println(F(";"));
  Serial.print(F("const int COTOV_INICIAL = ")); Serial.print(posicoes[0].cotov); Serial.println(F(";"));
  Serial.print(F("const int GARRA_ABERTA  = ")); Serial.print(posicoes[0].garra); Serial.println(F(";"));
  Serial.println();

  Serial.println(F("// ── Posição de pega (frente, esticado) ────────────"));
  Serial.print(F("const int BASE_FRENTE   = ")); Serial.print(posicoes[1].base);  Serial.println(F(";"));
  Serial.print(F("const int OMBRO_PEGAR   = ")); Serial.print(posicoes[1].ombro); Serial.println(F(";"));
  Serial.print(F("const int COTOV_PEGAR   = ")); Serial.print(posicoes[1].cotov); Serial.println(F(";  // aumentar para esticar mais"));
  Serial.print(F("const int GARRA_FECHADA = ")); Serial.print(posicoes[1].garra); Serial.println(F(";"));
  Serial.println();

  Serial.println(F("// ── Posição ALTA de transporte ─────────────────────"));
  Serial.print(F("const int OMBRO_ALTO    = ")); Serial.print(posicoes[2].ombro); Serial.println(F(";"));
  Serial.print(F("const int COTOV_ALTO    = ")); Serial.print(posicoes[2].cotov); Serial.println(F(";"));
  Serial.println();

  Serial.println(F("// ── Destino: esquerda ──────────────────────────────"));
  Serial.print(F("const int BASE_ESQUERDA = ")); Serial.print(posicoes[3].base);  Serial.println(F(";"));
  Serial.println();

  Serial.println(F("// ── Posição de soltura ─────────────────────────────"));
  Serial.print(F("const int OMBRO_SOLTAR  = ")); Serial.print(posicoes[4].ombro); Serial.println(F(";"));
  Serial.print(F("const int COTOV_SOLTAR  = ")); Serial.print(posicoes[4].cotov); Serial.println(F(";"));
  Serial.println();
  Serial.println(F("════════════════════════════════════════════════════"));
}

// ──────────────────────────────────────────────────────
//  MENSAGENS DE AJUDA
// ──────────────────────────────────────────────────────
void imprimirBoasVindas() {
  Serial.println(F("\n"));
  Serial.println(F("╔══════════════════════════════════════════════════╗"));
  Serial.println(F("║       CALIBRAÇÃO DO BRAÇO ROBÓTICO              ║"));
  Serial.println(F("╚══════════════════════════════════════════════════╝"));
  Serial.println(F("Vamos calibrar 5 posições uma por uma."));
  Serial.println(F("No final, copie os valores para o código principal."));
  Serial.println();
}

void imprimirPosicaoAtual() {
  Serial.println(F("──────────────────────────────────────────────────"));
  Serial.print(F("POSIÇÃO "));
  Serial.print(posAtual + 1);
  Serial.print(F("/"));
  Serial.print(NUM_POSICOES);
  Serial.print(F(": "));
  Serial.println(posicoes[posAtual].nome);
  Serial.print(F("  → "));
  Serial.println(posicoes[posAtual].descricao);
  Serial.println(F("──────────────────────────────────────────────────"));
  Serial.println(F("Selecione o servo: 1=Base  2=Ombro  3=Cotovelo  4=Garra"));
  Serial.println(F("Ajuste: +/-  (1°)    ++/--  (5°)    +++/---  (10°)"));
  Serial.println(F("Ou defina direto: b=90  o=150  c=60  g=30"));
  Serial.println(F("Quando ok: 's' salva | 'n' próxima | 'v' volta"));
  imprimirStatus();
}

void imprimirMenu() {
  Serial.println(F("\n─── COMANDOS ────────────────────────────────────"));
  Serial.println(F("  1/2/3/4   → seleciona servo (Base/Ombro/Cotov/Garra)"));
  Serial.println(F("  + / -     → ajusta ±1°"));
  Serial.println(F("  ++ / --   → ajusta ±5°"));
  Serial.println(F("  +++ / --- → ajusta ±10°"));
  Serial.println(F("  b= o= c= g=  → define ângulo direto (ex: c=120)"));
  Serial.println(F("  s  → salva posição atual"));
  Serial.println(F("  n  → próxima posição (auto-salva)"));
  Serial.println(F("  v  → volta posição anterior"));
  Serial.println(F("  r  → executa ciclo completo de teste"));
  Serial.println(F("  p  → imprime valores finais para copiar"));
  Serial.println(F("  m  → mostra este menu"));
  Serial.println(F("─────────────────────────────────────────────────\n"));
}

void imprimirStatus() {
  Serial.print(F("  ["));
  Serial.print(posicoes[posAtual].nome);
  Serial.print(F("] "));

  // Marca o servo selecionado com asterisco
  Serial.print(servoSel == 1 ? F("*BASE=") : F(" BASE="));  Serial.print(angBase);
  Serial.print(servoSel == 2 ? F("  *OMBRO=") : F("  OMBRO="));  Serial.print(angOmbro);
  Serial.print(servoSel == 3 ? F("  *COTOV=") : F("  COTOV="));  Serial.print(angCotov);
  Serial.print(servoSel == 4 ? F("  *GARRA=") : F("  GARRA="));  Serial.println(angGarra);
}

bool isNumeric(String s) {
  for (int i = 0; i < s.length(); i++) {
    if (!isDigit(s[i])) return false;
  }
  return s.length() > 0;
}
