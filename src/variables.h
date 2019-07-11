#include <Arduino.h>
#include <math.h>
#include <EEPROM.h>
#include <LiquidCrystal_I2C.h>

// CONSTANTES PARA LCD
#define I2C_ADD     0x27
#define COL         16
#define ROW         2
// CONSTANTES PARA MENU
byte flechita[8] = {B00000, B00100, B11110, B11111, B11110, B00100, B00000};
byte play[8] = {B00000, B01000, B01100, B01110, B01100, B01000, B00000};
byte pause[8] = {B00000, B01010, B01010, B01010, B01010, B01010, B00000};
byte revolucion[8] = {B00000, B11111, B10001, B00001, B11101, B11001, B10111};
//VARIABLES PARA EEEPROM
int direccion_diametro = 200;
int direccion_sentido = 216;
int direccion_inicializado = 232;
//CONSTANTES DE MENU
#define ETAPA_MAXIMA                2
#define ETAPA_MINIMA                0
#define MODOS_MAXIMOS               2
#define MODOS_MINIMO                0
#define PARAMETRO_MINIMO            1 
#define PARAMETROS_BOMBA            1
#define PARAMETROS_DOSIFICADOR      4
#define PARAMETROS_CALIBRACION      3
#define MODO_BOMBA                  0
#define MODO_DOSIFICADOR            1
#define MODO_CALIBRACION            2
#define ETAPA_MODOS                 0
#define ETAPA_PARAMETROS            1
#define ETAPA_OPERACION             2 
#define PARAMETRO_FLUJO             1
#define PARAMETRO_DISPAROS          1
#define PARAMETRO_VOLUMEN           2 
#define PARAMETRO_TIEMPO            3
#define PARAMETRO_PAUSA             4
#define PARAMETRO_DIAMETRO          1
#define PARAMETRO_PROPORCION        2
#define PARAMETRO_SENTIDO         3
// VARIABLES GLOBALES PARA MENU
short etapa = 0;
short modo = 0;
short estado = 0;
bool config = 0;
short parametro;
short parametroMAX;
short parametroBomba = 1;
short parametroCalibracion = 1;
short parametroDosificador = 1;
float flujo = 20.0;
float volumen = 5.0;
int tiempo = 5;  
int pausa = 5;
int disparosConteo = 0;
int disparosTotal = 1;
bool sentido = 1;
// LIMITES DE MENU
int disparosmax = 99;
int tiempoMax = 99;
int pausaMax = 99;
float proporcionMax = 3.0;
float proporcionMin = 0.005;
int diametroMax = 4;
int diametroMin = 1;
bool sentidoMax = 1;
bool sentidoMin = 0;
int disparosMin = 1;
int pausaMin = 0;


// LIMITES FISICOS DEL MOTOR
#define RPMAX           250
#define RPMMIN          1
// CONSTANTES PARA STEPPER
#define MOTOR_STEPS 200
#define FLT       DDB5  //PD13
#define ENABLE    DDB4  //PD12
#define MODE0     DDB3  //PD11
#define MODE1     DDB2  //PD10
#define MODE2     DDB1  //PD9
#define RST       DDB0  //PD8
#define SLP       DDD7  //PD7
#define STEP      DDD6  //PD6 
#define DIR       DDD5  //PD5
#define RELOJ 16000000
#define G_PASO      1.8
#define STEP_HIGH         PORTD |=  (1<<STEP);
#define STEP_LOW          PORTD &= ~(1<<STEP);
#define DIR_CW            PORTD |=  (1<<DIR);
#define DIR_ACW           PORTD &= ~(1<<DIR);
#define TIMER1_OFF        TCCR1B &= ~((1 << CS12) | (1 << CS11)| (1 << CS10));
#define TOGGLE_STEP       PIND = PIND | (1<<STEP);
#define ENABLE_HIGH       PORTB |= (1<<ENABLE);
#define ENABLE_LOW        PORTB &= ~(1<<ENABLE);
//VARIABLES PARA CALCULOS
float ml_rev;
float ml_revs[8];
unsigned long tocs;
unsigned long pasosTotales;
int prescaler = 3;
int array_prescaler[5] = {1, 8, 64, 256, 1024};
int MICROSTEPS = 32;
short diametro = 4;
float pasoFlujo;
float pasoVolumen;
float volumenMax;
float flujoMax;
float pasoProporcion;
float flujoMin;
float volumenMin;
int tiempoMin;
float flujoPSMax;
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


// VARIABLES GLOBALES PARA ENCODER
bool flagPush = 0;
short flagEncoder = 0;
bool CLK = 1;
bool DT = 1;
short statusD = 0;
short statusC = 0;
short flag = 0;

// VARIABLES PARA Serial
String cadena ="";
bool finRX = 0;

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
