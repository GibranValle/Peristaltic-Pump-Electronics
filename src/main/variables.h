#include <Arduino.h>
#include <EEPROM.h>
#include <math.h>
#include "Bomba.h"

// CONSTANTES PARA STEPPER
#define RELOJ 16000000
#define MOTOR_STEPS 200
#define G_PASO      1.8
#define ENABLE    DDB2
#define MODE0     DDB1
#define MODE1     DDB0
#define MODE2     DDD7
#define STEP      DDD6
#define DIR       DDD5
#define STEP_HIGH         PORTD |=  (1<<STEP);
#define STEP_LOW          PORTD &= ~(1<<STEP);
#define DIR_CW            PORTD |=  0b00100000;
#define DIR_ACW           PORTD &= ~0b00100000;
#define TIMER1_OFF        TCCR1B &= ~((1 << CS12) | (1 << CS11)| (1 << CS10));
#define TOGGLE_STEP       PIND = PIND | (1<<STEP);
#define ENABLE_HIGH       PORTB |= (1<<ENABLE);
#define ENABLE_LOW        PORTB &= ~(1<<ENABLE);
// CONSTANTES PARA MENU
byte flechita[8] = {B00000, B00100, B11110, B11111, B11110, B00100, B00000};
byte play[8] = {B00000, B01000, B01100, B01110, B01100, B01000, B00000};
byte pause[8] = {B00000, B01010, B01010, B01010, B01010, B01010, B00000};
byte revolucion[8] = {B00000, B11111, B10001, B00001, B11101, B11001, B10111};
// LIMITES FISICOS DEL MOTOR
#define RPMAX           250
#define RPMMIN          1
//CONSTANTES DE MENU
#define ETAPAMAX            2
#define etapaMIN            0
#define MODOSMAX            2
#define MODOSMIN             0
#define PARAMETROMIN        1 
#define PARAMETROSBOMBA     1
#define PARAMETROSDOSI      4
#define PARAMETROSCALI      3
#define MODOB               0
#define MODOD               1
#define MODOC               2
#define EMODOS              0
#define EPARAM              1
#define EOPERA              2 
#define PFLUJO              1
#define PDISPAROS           1
#define PVOLUMEN            2 
#define PTIEMPO             3
#define PPAUSA              4
#define PDIAMETRO           1
#define PPROPORCION         2
#define PDIRECCION          3
// VARIABLES GLOBALES PARA ENCODER
bool flagPush = 0;
short flagEncoder = 0;
bool CLK = 1;
bool DT = 1;
short statusD = 0;
short statusC = 0;
short flag = 0;
// VARIABLES GLOBALES PARA MENU
short etapa = 0;
short modo = 0;
int estado = 0;
bool config = 0;
short parametro;
short parametroMAX;
short paraBomba = 1;
short paraCali = 1;
short paraDosi = 1;
float flujo = 20.0;
float volumen = 5.0;
float pasoFlujo;
float pasoVolumen;
int tiempo = 5;  
int pausa = 5;
int disparosConteo = 0;
int disparosTotal = 1;
short diametro = 4;
bool direccion = 1;
// LIMITES DE MENU
int disparosmax = 99;
int tiempoMax = 99;
int pausaMax = 99;
float proporcionMax = 3.0;
float proporcionMin = 0.005;
int diametroMax = 4;
int diametroMin = 1;
bool direccionMax = 1;
bool direccionMin = 0;
int disparosMin = 1;
int pausaMin = 0;
// LIMITES CALCULADOS DE MENU
float volumenMax;
float flujoMax;
float pasoProporcion;
float flujoMin;
float volumenMin;
int tiempoMin;
float flujoPSMax;

//VARIABLES PARA CALCULOS
float ml_rev;
float ml_revs[8];
unsigned long tocs;
unsigned long pasosTotales;
int prescaler = 2;
int array_prescaler[8] = {1, 8, 64, 256, 1024};
int MICROSTEPS = 32;
//VARIABLES PARA MOTOR
unsigned long contadorPasos = 0;
unsigned long contadorTocs = 0;
int flagAccel = 0;
int limtocs = 54;
int maxTocs = 640;
int minTocs = 0;
unsigned long maxCTocs = 100;
int flagTimer = 0;
bool error = 0;

// VARIABLES PARA Serial
String cadena ="";
bool finRX = 0;
//VARIABLES PARA EEEPROM
int dirDiametro = 200;
int dirDireccion = 216;
int dirInicializado = 232;
// PROTOTIPOS
void Push();
void Encoder();
//void RX();
void Operando();
void LCD();
void Sumar();
void Restar();
void recuperarVariables();
void salvarVariable(int posicion);
void calcularTocs();
void setuSteps();
void Calibrar(int opcion);
inline void ActualizarConteo();
inline void Cursor();
inline void MotorOn();
inline void MotorOff();
inline void TIMER1_ON();
inline void TiempoMin();
