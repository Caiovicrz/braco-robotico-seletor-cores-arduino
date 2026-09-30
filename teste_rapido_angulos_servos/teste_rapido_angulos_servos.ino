#include <Servo.h>

// Cria um objeto para controlar o servo
Servo meuServo; 

void setup() {
  // Conecta o objeto do servo ao pino digital 9
  meuServo.attach(9); 
}

void loop() {
  // Vai para 0 graus
  meuServo.write(0);
  delay(1000); // Aguarda 1 segundo

  // Vai para 90 graus
  meuServo.write(90);
  delay(1000);

  // Vai para 180 graus
  meuServo.write(180);
  delay(1000);

  // Zera novamente (volta para 0)
  meuServo.write(0);
  delay(1500); // Aguarda um pouco mais antes de recomeçar o ciclo
}