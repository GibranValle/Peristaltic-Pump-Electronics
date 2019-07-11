#include "variables.h"
void setup()
{  
// ------------------------SERIAL --------------------------/
  UCSR0B |= (1<<TXEN0)|(1<<RXEN0); //INTERRUPCION
  UBRR0L = 103;  // 9600 BAUDS
  UCSR0B |= (1<<RXCIE0);//RX
// --------------------- TIMER1 16B ------------------------/
  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1  = 0; //contador
  OCR1A  = 6250; //0.1s
  TCCR1B |= (1<<WGM12); //MODO 4 CTC
  TIMSK1 |= (1 << OCIE1A); //MASCARA DE INTERRUPCION
  TIMER1_ON();  //seleccion de prescaler 1024 y fuente de reloj
  TIMER1_OFF;
  Serial.println("CONFIGURADO");
}

void loop()
{
    if(flagTimer == 1)
    {
        flagTimer = 0;
        Serial.println("INTERRUPT");
    }    
}

inline void TIMER1_ON()
{
  switch (array_prescaler[prescaler]) {
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

ISR(TIMER1_COMPA_vect)
{
  flagTimer = 1;
}