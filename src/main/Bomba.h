// ensure this library description is only included once
#include <Arduino.h>
#include <EEPROM.h>

#define TIMER1_OFF        TCCR1B &= ~((1 << CS12) | (1 << CS11)| (1 << CS10));
#define MODOB               0
#define MODOD               1
#define MODOC               2

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
#define DIR_CW            PORTD |=  0b00100000;
#define DIR_ACW           PORTD &= ~0b00100000;
#define TIMER1_OFF        TCCR1B &= ~((1 << CS12) | (1 << CS11)| (1 << CS10));
#define TOGGLE_STEP       PIND = PIND | (1<<STEP);
#define ENABLE_HIGH       PORTB |= (1<<ENABLE);
#define ENABLE_LOW        PORTB &= ~(1<<ENABLE);

// LIMITES FISICOS DEL MOTOR
#define RPMAX           250
#define RPMMIN          1

//VARIABLES PARA EEEPROM
int dirDiametro = 200;
int dirDireccion = 216;
int dirInicializado = 232;

//VARIABLES PARA CALCULOS
float ml_rev;
float ml_revs[8];
unsigned long tocs;
unsigned long pasosTotales;
int prescaler = 2;
int array_prescaler[8] = {1, 8, 64, 256, 1024};
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


class Bomba
{
    // constructor
    public:
        Bomba();
        // METODOS PUBLICOS
        void Init();
        bool Bombear(float flujo);
        bool Dosificar(float volumen, int tiempo, int disparos);
        bool Parar();
        bool Reiniciar();
        bool Calibrar(float ML_REV);
        bool Calibrar(int DIAMETRO);
        bool Calibrar(bool DIRECCION);
        bool Calibrar(float ML_REV=0, int DIAMETRO=0, bool DIRECCION=0);
        // METODOS PARA CALCULAR
        void eepromSetUp();
        void recuperarVariables();
        inline void stepperSetUo();
        inline void timer1SetUp();
        inline void setuSteps();
        float getPasoVolumen();
        float getPasoFlujo();
        float getFlujoMax();
        float getFlujoMin();
        float getVolumenMax();                
        float getVolumenMin();
        int getTiempoMin();
        inline void ToggleStep();
        int flagTimer = 0;
        float g_paso = 1.8;
        int flagAccel;
        float velocidad = 0;


    private:
        // VARIABLES PRIVADAS
        float _ML_REV = 0;
        int _DIAMETRO = 0;
        int _DIRECCION;
        int MICROSTEPS;
        int modo;
        float flujo;
        float volumen;
        int tiempo;
        int disparosConteo;
        int contadorPasos;
        float resolucion;
        long pasosTotales = 0;
        long periodo = 0;
        long tocs = 0;   
        
        // VARIABLES PARA CALCULOS
        int prescaler = 1024;
        int limtocs = 54;
        int maxTocs = 640;
        int minTocs = 0;
        long maxCTocs = 100;
        // METODOS PRIVADOS
        void SetSteps();
        void CalcularTocs();
        void PinFisicoABitMask();
        inline void StepHigh();
        inline void StepLow();
        inline void DirCW();
        inline void DirACW();
        inline void EnableHigh();
        inline void EnableLow();
        inline void TIMER1_ON();
};
