#include <Arduino.h>
#include "DRV8825.h"

// 10mL por 280 / 1mL por 280°
// Motor steps per revolution. Most steppers are 200 steps or 1.8 degrees/step
#define MOTOR_STEPS 200
#define MICROSTEPS 32
#define DIR DDD5
#define STEP DDD6
#define ENABLE DDD7
#define MODE0 10
#define MODE1 11
#define MODE2 12
DRV8825 stepper(MOTOR_STEPS, DIR, STEP, ENABLE, MODE0, MODE1, MODE2);

float rev = 0;
long grados = 0;
float rev_mL = 0.8;
int RPM = 0;
float flujo = 280;

void calculosparaFlujo();

void setup()
{
    //Serial.begin(9600);
    stepper.begin(1, MICROSTEPS);
    calculosparaFlujo();
    stepper.setRPM(RPM);
    stepper.rotate(grados);
}

void loop()
{
}

void calculosparaFlujo()
{
  // sacar flujo en mL/s
  float flujoS = flujo/60;
  // encontrar velocidad
  float velocidad = rev_mL*flujoS;
  //convertir a RPM
  RPM = velocidad*60;
  //obtener grados
  grados = (long)RPM*360;

  /* VISUALIZAR
  Serial.print("flujo[mL/min]: ");
  Serial.println(flujo);
  Serial.print("flujo[mL/s]: ");
  Serial.println(flujoS);
  Serial.print("velocidad: ");
  Serial.println(velocidad);
  Serial.print("RPM: ");
  Serial.println(RPM);
  Serial.print("grados: ");
  Serial.println(grados);
  */
}
