#include "Bomba.h"

Bomba bombaInterna();

Bomba::Bomba()
{

}
void Bomba::Init()
{
// --------------------- TIMER1 16B ------------------------/
  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1  = 0; //contador
  OCR1A  = 6250; //0.1s
  TCCR1B |= (1<<WGM12); //MODO 4 CTC
  TIMER1_ON();  //seleccion de prescaler y fuente de reloj
  TIMSK1 |= (1 << OCIE1A); //MASCARA DE INTERRUPCION
  TIMER1_OFF;
}

bool Bomba::Bombear(float flujo)
{
    return 1;
}

bool Bomba::Dosificar(float volumen, int tiempo, int disparos)
{
 return 1;   
}

bool Bomba::Parar()
{
    return 1;
}

bool Bomba::Reiniciar()
{
    return 1;
}

bool Bomba::Calibrar(float ML_REV)
{
    _ML_REV = ML_REV;
    if(_ML_REV == 0 || _DIAMETRO == 0)
    {
        return 0;
    }
    else
    {
        return 1;
    }
    return 1;
}

bool Bomba::Calibrar(int DIAMETRO)
{
    _DIAMETRO = DIAMETRO;
    if(_ML_REV == 0 || _DIAMETRO == 0)
    {
        return 0;
    }
    else
    {
        return 1;
    }
}

bool Bomba::Calibrar(bool DIRECCION)
{
    _DIRECCION = DIRECCION;
    if(_ML_REV == 0 || _DIAMETRO == 0)
    {
        return 0;
    }
    else
    {
        return 1;
    }
}

bool Bomba::Calibrar(float ML_REV, int DIAMETRO, bool DIRECCION)
{
    _ML_REV = ML_REV;
    _DIAMETRO = DIAMETRO;
    _DIRECCION = DIRECCION;
    CalcularTocs();
    return 1;
}

float Bomba::getPasoVolumen()
{
    return 1;
}

float Bomba::getPasoFlujo()
{
    return 1;
}

float Bomba::getFlujoMax()
{
    return 1;
}

float Bomba::getFlujoMin()
{
    return 1;
}

float Bomba::getVolumenMax()
{
    return 1;
}

float Bomba::getVolumenMin()
{
    return 1;
}

void Bomba::SetSteps()
{
    PORTD &= ~((1<<MODE2));
    PORTB &= ~((1<<MODE0)|(1<<MODE1));
    switch (MICROSTEPS)
    {
    case 1:
    break;

    case 2:
    PORTB |= (1<<MODE0);
    break;

    case 4:
    PORTB |= (1<<MODE1);
    break;

    case 8:
    PORTB |= (1<<MODE0)|(1<<MODE1);
    break;

    case 16:
    PORTD |= (1<<MODE2);
    break;

    case 32:
    PORTD |= (1<<MODE2);
    PORTB |= (1<<MODE0);
    break;
    }
}
void Bomba::CalcularTocs()
{
    flagAccel = 0;
    float volumenLocal;
    int tiempoLocal;
    switch (modo)
    {
    case MODOB: // MODO BOMBA
        volumenLocal = flujo;
        tiempoLocal = 60/2;
    break;

    case MODOD: //MODO DOSIFICADOR
        tiempoLocal = tiempo;
        volumenLocal = volumen*2;
        //DISPAROS
        disparosConteo = 0;
    break;
    }
    contadorPasos = 0;
    resolucion = (float) prescaler/16;
    pasosTotales = (long) (volumenLocal*MICROSTEPS)/((g_paso/(360/_ML_REV)));
    velocidad = (float) pasosTotales/tiempoLocal;
    periodo = (long) round(1000000/velocidad);
    tocs = (long) round(periodo/(resolucion));
    if(modo == MODOC) // VELOCIDAD POR DEFECTO PARA CALIBRACION
    {
    pasosTotales = 2*MOTOR_STEPS*MICROSTEPS;
    tocs = 200;
    }
    else if(tocs < limtocs)
    {
    minTocs = tocs;
    tocs = maxTocs;
    flagAccel = 1;
    }
    OCR1A = tocs;
    Serial.print("tocs: ");
    Serial.println(tocs);
}
void Bomba::PinFisicoABitMask()
{

}
inline void Bomba::StepHigh()
{

}
inline void Bomba::StepLow()
{

}
inline void Bomba::DirCW()
{

}
inline void Bomba::DirACW()
{

}
inline void Bomba::EnableHigh()
{

}
inline void Bomba::EnableLow()
{

}
inline void Bomba::ToggleStep()
{
    
}

inline void Bomba::TIMER1_ON()
{
  switch (prescaler) {
    case 1:
      TCCR1B &= ~((1 << CS12) | (1 << CS11));
      TCCR1B |= (1 << CS10);
    break;

    case 8:
      TCCR1B &= ~((1 << CS12) | (1 << CS10));
      TCCR1B |= (1 << CS11);
    break;

    case 64:
      TCCR1B &= ~((1 << CS12));
      TCCR1B |= (1 << CS11)|(1 << CS10);
    break;

    case 256:
      TCCR1B &= ~((1 << CS11) | (1 << CS10));
      TCCR1B |= (1 << CS12);
    break;

    case 1024:
      TCCR1B &= ~((1 << CS11));
      TCCR1B |= (1 << CS12)|(1 << CS10);
    break;
  }
}
