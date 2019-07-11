#include "variables.h"
void setup()
{
  // ---------------- EEEPROM -----------------------------//
  /*
  recuperarVariables(0); //PARAMETRO DE CALIBRACION
  recuperarVariables(1); //DSELECTOR
  recuperarVariables(2); //DIRECCION
  diametro = dselector;
  ml_rev = ml_revs[dselector];
  flmax = RPMmax * ml_rev;
  flmin = RPMmin * ml_rev;
  fpsmax = flmax/60.00;
  fpaso = 1;
  */
  // ---------------- STEPPER -----------------------------//
  DDRB &= ~((1<<ENABLE)|(1<<MODE0)|(1<<MODE1)); // DIRECTION, OUTPUT
  DDRD &= ~((1<<MODE2)|(1<<DIR)|(1<<STEP)); // DIRECTION, OUTPUT
  //setuSteps();
  ENABLE_HIGH
  if(direccion == 1)
  {
    PORTD |= (1<<DIR);
  }
  else
  {
    PORTD &= ~(1<<DIR);
  }

    // --------------------- TIMER1 16B ------------------------//
    /*
  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1  = 0; //contador
  OCR1A  = 6250; //0.1s
  TCCR1B |= (1<<WGM12); //MODO 4 CTC
  TIMER1_ON();  //seleccion de prescaler y fuente de reloj
  TIMSK1 |= (1 << OCIE1A); //MASCARA DE INTERRUPCION
  TIMER1_OFF;
  */

  // ------------------------lCD --------------------------/

  // ----------------------- OLED --------------------------/
  // by default, we'll generate the high voltage from the 3.3v line internally! (neat!)
  /*
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // initialize with the I2C addr 0x3C (for the 128x32)
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(WHITE);
  */
  // ------------------------SERIAL --------------------------/
  UCSR0B |= (1<<TXEN0)|(1<<RXEN0);
  UBRR0L = 103;  // 9600 BAUDS
  UCSR0B |= (1<<RXCIE0);

  // ------------------------GPIOs --------------------------/
  DDRD &= ~((1<<DDD2)|(1<<DDD3)|(1<<DDD4)); // DIRECTION, INPUT
  // CONFIGURAR PUERTO D SIN PULL UP RS //clearing bits 2-4
  PORTD = 0;
  //The falling edge of INT0 generates an interrupt request.
  //The falling edge of INT1 generates an interrupt request.
  EICRA |= (1<<ISC11)|(1<<ISC01);
  // MASCARA PIN PAD D2 D3  
  EIMSK |= (1<<INT0) | (1<<INT1);
  //MASCARA PIN 4
  //PCMSK2 |= (1<<PCINT20);
  // HABILITAD LA INTERRUPCION DEL PUERTO PCINT18,19,20 | D2-D4
  //PCICR  |=  (1<<PCIE2);
  // CONFIGURACION DE INTERRUPCIONES
  SREG   |=  (1 << 7); // GLOBAL INTERRUPT ENABLE

  Serial.println("CONFIGURADO");
  cadena.reserve(16);
  //calcularTocs();
  //Pantalla();
}

void loop()
{
  if(flag == 1)
  { 
    //Serial.print("CLK: ");  
    //Serial.println(CLK,BIN);  
    if(CLK)
    {
      Serial.print("SUMAR");  
    }
    else
    {
      Serial.print("RESTAR");  
    }
    flag = 0;
  }
}

void Sumar()
{
  if(config == 0) // MOVERSE A TRAVES DE MENUS
  {
    if(etapa == 1) // MENU DE PARAMETROS
    {
      if(parametro < parametroMAX)
      {
        parametro += 1;
      }
      else if(parametro == parametroMAX)
      {
        etapa += 1;
      }
    }
    else if(etapa < ETAPAMAX)
    {
      etapa += 1;
    }
    switch (modo)
    {
      case 0:
        paraBomba = parametro;
        break;
      case 1:
        paraDosi = parametro;
        break;            
      case 2:
        paraCali = parametro;
        break;
    }
  }
  else if(config == 1)//INCREMENTAR MODO, VALOR DE PARAMETRO, PLAY/PAUSA
  {
  }
}

void Restar()
{
  if(config == 0) // MOVERSE A TRAVES DE MENUS
  {
    switch (modo)
    {
      case 0:
        parametro = paraBomba;
        break;
      case 1:
        parametro = paraDosi;
        break;            
      case 2:
        parametro = paraCali;
        break;
    }
    if(etapa == 1) // MENU DE PARAMETROS
    {
      if(parametro > 1)
      {
        parametro -= 1;
      }
      else if(parametro == 1)
      {
        etapa -= 1;
      }
    }
    else if(etapa > 0)
    {
      etapa -= 1;
    }
    switch (modo)
    {
      case 0:
        paraBomba = parametro;
        break;
      case 1:
        paraDosi = parametro;
        break;            
      case 2:
        paraCali = parametro;
        break;
    }
  }
}

ISR(INT1_vect)  // INTERRUPCION SOLO CON 1 PULSO.
{
  CLK = PIND>>4;
  flag  = 1;
}