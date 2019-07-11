// ensure this library description is only included once
#include <Arduino.h>
#define TIMER1_OFF        TCCR1B &= ~((1 << CS12) | (1 << CS11)| (1 << CS10));
#define MODOB               0
#define MODOD               1
#define MODOC               2

// CONSTANTES PARA STEPPER
#define MOTOR_STEPS 200
#define ENABLE    DDB2
#define MODE0     DDB1
#define MODE1     DDB0
#define MODE2     DDD7
#define STEP      DDD6
#define DIR       DDD5

class Bomba
{
    // constructor
    public:
        Bomba(int uno);
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
