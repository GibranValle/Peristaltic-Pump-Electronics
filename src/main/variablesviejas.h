#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include "DRV8825.h"
#include <EEPROM.h>
#include <math.h>

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// CONSTANTES PARA STEPPER
#define MOTOR_STEPS 200
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
// CONSTANTES PARA LCD
#define I2C_ADD     0x27
#define COL         16
#define ROW         2
byte flechita[8] = {B00000, B00100, B11110, B11111, B11110, B00100, B00000};
byte play[8] = {B00000, B01000, B01100, B01110, B01100, B01000, B00000};
byte pause[8] = {B00000, B01010, B01010, B01010, B01010, B01010, B00000};
byte revolucion[8] = {B00000, B11111, B10001, B00001, B11101, B11001, B10111};

//CONSTANTES PARA OLED
/*
const unsigned char flechita [] PROGMEM = {
  // 'flechita, 12x16px
  0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 0x06, 0x00, 0x07, 0x00, 0x07, 0x80, 0x7f, 0xc0, 0x7f, 0xe0, 
  0x7f, 0xe0, 0x7f, 0xc0, 0x07, 0x80, 0x07, 0x00, 0x06, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00
};

const unsigned char replay [] PROGMEM = {
  // 'replay, 12x16px
  0x00, 0x00, 0x1f, 0xc0, 0x3f, 0xe0, 0x3f, 0xe0, 0x38, 0xe0, 0x38, 0xe0, 0x00, 0xe0, 0x10, 0xe0, 
  0x38, 0xe0, 0x7c, 0xe0, 0xfe, 0xe0, 0x38, 0xe0, 0x3f, 0xe0, 0x3f, 0xe0, 0x1f, 0xc0, 0x00, 0x00
};

const unsigned char pause [] PROGMEM = {
  // 'pause, 12x16px
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x39, 0xc0, 0x39, 0xc0, 0x39, 0xc0, 0x39, 0xc0, 0x39, 0xc0, 
  0x39, 0xc0, 0x39, 0xc0, 0x39, 0xc0, 0x39, 0xc0, 0x39, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

const unsigned char play [] PROGMEM = {
  // 'play, 12x16px
  0x00, 0x00, 0x10, 0x00, 0x18, 0x00, 0x1c, 0x00, 0x1e, 0x00, 0x1f, 0x00, 0x1f, 0x80, 0x1f, 0xc0, 
  0x1f, 0xc0, 0x1f, 0x80, 0x1f, 0x00, 0x1e, 0x00, 0x1c, 0x00, 0x18, 0x00, 0x10, 0x00, 0x00, 0x00,
};

#define OLED_RESET 4
#define FIL0    0
#define FIL1    16
#define FIL2    32
#define FIL3    48
#define COL0    0
#define COL1    12
#define COL2    24
#define COL3    36
#define COL4    48
#define COL5    60
#define COL6    72
#define COL7    84
#define COL8    96
#define COL9    108
#define XSYMBOL 16
#define YSYMBOL 16
#define COLOR   1
*/

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

//VARIABLES PARA CALCULOS
float ml_rev = 1.25;
float g_ml = 288;
float g_paso = 1.8;
long pasosTotales = 0;
float velocidad = 0;
long periodo = 0;
long tocs = 0;
int prescaler = 1024;
int MICROSTEPS = 32;
float resolucion = 0;

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
int tiempo = 5;  
int pausa = 5;
int disparosConteo = 0;
int disparosTotal = 1;
short diametro = 4;
bool direccion = 1;


//LIMITES
float flmax; //definir en funcion del diametro
float fpsmax; //definir en funcion del diametro
float flmin;  // definir en funcino del diametro
float fpaso;
float vpaso = 0.5;
int vmax = 1000;
int vmin = 1 ;
int pmax = 99;
int pmin = 1;
int tmax = 99;
int tmin = 1;
int disparosmax = 99;
int disparosmin = 1;
int dmax = 4;
int dmin = 1;
float rmax = 3.0;
float rmin = 0.005;
short dirmax = 1;
short dirmin = 0;
float rres = 0.005;
float RPMmax = 250;
float RPMmin = 1;

// VARIABLES PARA Serial
String cadena ="";
bool finRX = 0;

//VARIABLES PARA MOTOR
long contadorPasos = 0;
long contadorTocs = 0;
int flagAccel = 0;
int limtocs = 54;
int maxTocs = 640;
int minTocs = 0;
long maxCTocs = 100;
int flagTimer = 0;
bool error = 0;

// VARIABLES PARA CALIBRAR
float ml_revs[5] = {};
int tocsCali = 10000;
int dselector = 4;
//VARIABLES PARA EEEPROM
int dirDselector = 200;
int dirDireccion = 216;

// PROTOTIPOS
void Push();
void Encoder();
void RX();
void Operando();
void LCD();
void Cursor();
/*
void OLED();
void OSimbolos();
*/
void Sumar();
void Restar();
void calcularTocs();
void recuperarVariables(int posicion);
void salvarVariables(int posicion);
void setuSteps();
void TIMER1_ON();
void motorOn();
void motorOff();
void diametroModificado();
void tiempoMin();
void motorReplay();
void motorPause();
void flujoPaso();

/*
void menuInicio();
void pantallaInfo();
void editarModo();
void subMenu();
void actualizarV();
void lcdFlecha();
void borrarFlecha();
void lcdPlay();

int cursor = 0;
int cursorMax = 2;
int modoDosi;
int modoCali;
int decimales = 2;
*/

