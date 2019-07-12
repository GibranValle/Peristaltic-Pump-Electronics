/*
VERSION 1.0.8
LCD ESTABLE
RX ESTABLE
ENCODER ESTABLE
VARIABLES ESTABLES
VERIFICAR CALCULOS 

VERSION 1.0.7
LCD MEJORADO/MEJORABLE
RX MEJORABLE
VERIFICAR CALCULOS 
BUG
ENCODER
VARIABLES

VERSION 1.0.6
VARIABLES Y FUNCIONES NOMBRE CAMBIADO
LCD MEJORABLE
VERIFICAR CALCULOS 

VERSION 1.0.5
RX FUNCIONANDO
STEPPER FUNCIONANDO
TIMER FUNCIONANDO
ENCODER FUNCIONANDO
LCD MEJORABLE
VERIFICAR CALCULOS 

VERSION 1.0.0
PROYECTO RECUPERADO DEL 2017
 */
#include <Arduino.h>
#include <math.h>
#include <EEPROM.h>
#include <LiquidCrystal_I2C.h>





// CONSTANTES PARA LCD
#define I2C_ADD     0x27
#define COL         16
#define ROW         2
// SIMBOLOS
byte flechita[8] = {B00000, B00100, B11110, B11111, B11110, B00100, B00000};
byte play[8] = {B00000, B01000, B01100, B01110, B01100, B01000, B00000};
byte pause[8] = {B00000, B01010, B01010, B01010, B01010, B01010, B00000};
byte revolucion[8] = {B00000, B11111, B10001, B00001, B11101, B11001, B10111};
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
#define PARAMETRO_SENTIDO           3
// CONSTANTES DE MENU
int disparos_max = 99;
int tiempo_max = 99;
int pausa_max = 99;
int pausa_min = 0;
float proporcion_max = 3.0;
float proporcion_min = 0.005;
int diametro_max = 5;
int diametro_min = 1;
bool sentido_max = 1;
bool sentido_min = 0;
int disparos_min = 1;
// VARIABLES GLOBALES PARA MENU
short etapa = 0;
short modo = 0;
short estado = 0;
bool config = 0;
short parametro;
short parametro_max;
short parametro_bomba = 1;
short parametro_calibracion = 1;
short parametro_dosificador = 1;
float flujo = 20.0;
float volumen = 5.0;
int tiempo = 5;  
int pausa = 5;
int disparos_conteo = 0;
int disparos_total = 1;
bool sentido = 1;
float volumen_min = 1;
float volumen_max = 999;





// CONSTANTES EEPROM
#define GUARDAR_PROPORCION          0 
#define GUARDAR_DIAMETRO            1
#define GUARDAR_SENTIDO             2
//VARIABLES PARA EEEPROM
int direccion_diametro = 200;
int direccion_sentido = 216;
int direccion_inicializado = 232;





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
#define RELOJ       16000000
#define G_PASO      1.8
#define STEP_HIGH         PORTD |=  (1<<STEP);
#define STEP_LOW          PORTD &= ~(1<<STEP);
#define DIR_CW            PORTD |=  (1<<DIR);
#define DIR_ACW           PORTD &= ~(1<<DIR);
#define TIMER1_OFF        TCCR1B &= ~((1 << CS12) | (1 << CS11)| (1 << CS10));
#define TOGGLE_STEP       PIND = PIND | (1<<STEP);
#define STEP_HIGH         PORTD |= (1<<STEP);
#define STEP_LOW          PORTD &= ~(1<<STEP);
#define ENABLE_HIGH       PORTB |= (1<<ENABLE);
#define ENABLE_LOW        PORTB &= ~(1<<ENABLE);
// LIMITES FISICOS DEL MOTOR
#define RPMAX           250
#define RPMMIN          1
//VARIABLES PARA MOTOR
unsigned long contador_pasos = 0;
unsigned long contador_tocs = 0;
int flag_accelerar = 0;
int tocs_limite = 54;
int maxTocs = 640;
int tocs_min = 0;
unsigned long maxCTocs = 100;
int flag_timer = 0;





//VARIABLES PARA CALCULOS
float ml_rev;
float array_ml_rev[6]={0.7,0.8,0.9,1.0,1.2};
unsigned long tocs;
unsigned long pasos_totales;
String array_prescaler = "1,8,64,256,1024";
int prescaler = 1024;
String array_microsteps = "1,2,4,8,16,32";
int microstep = 32;
int diametro = 4;
float paso_flujo;
float paso_volumen;
float flujo_max;
float paso_proporcion;
float flujo_min;
int tiempo_min;
float flujops_max;







// VARIABLES GLOBALES PARA ENCODER
#define PUSH_PIN       DDD2  //PD2
#define DT_PIN         DDD3  //PD3
#define CLK_PIN        DDD4  //PD4
bool flag_push = 0;
short flag_encoder = 0;
bool CLK = 1;
bool DT = 1;
short statusD = 0;
short statusC = 0;
short statusP = 0;
short flag = 0;





// VARIABLES PARA Serial
String cadena ="";
bool finRX = 0;





// PROTOTIPOS ENCODER
void push();
void encoder();
void sumar();
void restar();





// PROTOTIPOS MENU
void operando();
void LCD();
inline void actualizarConteo();
inline void cursor();
void menuModos();
void editarModo();
void menuBomba();
void menuDosificador();
void menuCalibracion();
void bombear();
void dosificar();
void calibrar();




//PROTOTIPOS EEPROM
void recuperarVariables();
void guardarVariable(int posicion);





//PROTOTIPOS CALCULOS
void calcularTocs();
void setuSteps();
void calibrar(int opcion);





// PROTOTIPOS SERIAL
inline void RX();
inline float validar(float temp, float min, float max);
inline int validar(int temp, int min, int max);




// PROTOTIPOS STEPPER
inline void motorOn();
inline void motorOff();
inline void timer1On();
inline void tiempoMin();