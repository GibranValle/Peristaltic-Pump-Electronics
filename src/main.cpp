#include "variables.h"

LiquidCrystal_I2C lcd(I2C_ADD, COL, ROW);

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
  TIMER1_ON();  //seleccion de prescaler 1024 y fuente de reloj
  TIMSK1 |= (1 << OCIE1A); //MASCARA DE INTERRUPCION
  TIMER1_OFF;

  // ---------------- EEEPROM -----------------------------//
  recuperarVariables(); //PARAMETRO DE CALIBRACION EEEPROM
  ml_rev = ml_revs[diametro];
  pasoProporcion = ml_rev;
  pasoFlujo = ml_rev;
  pasoVolumen = ml_rev;
  flujoMax = RPMAX * ml_rev;
  flujoMin = RPMMIN * ml_rev;
  flujoPSMax = flujoMax/60.00;

  // ---------------- STEPPER -----------------------------//
  DDRB &= ~((1<<ENABLE)|(1<<MODE0)|(1<<MODE1)); // DIRECTION, OUTPUT
  DDRD &= ~((1<<MODE2)|(1<<DIR)|(1<<STEP)); // DIRECTION, OUTPUT
  setuSteps();
  ENABLE_HIGH
  if(sentido == 1)
  {
    PORTD |= (1<<DIR);
  }
  else
  {
    PORTD &= ~(1<<DIR);
  }

  // ------------------------GPIOs --------------------------/
  DDRD &= ~((1<<DDD2)|(1<<DDD3)|(1<<DDD4)); // DIRECTION, INPUT
  // CONFIGURAR PUERTO D SIN PULL UP RS //clearing bits 2-4
  PORTD |= (1<<DDD2)|(1<<DDD3)|(1<<DDD4);
  //The falling edge of INT0 generates an interrupt request.
  //The falling edge of INT1 generates an interrupt request.
  EICRA |= (1<<ISC11)|(1<<ISC01);
  // MASCARA PIN PAD D2 D3  
  EIMSK |= (1<<INT0) | (1<<INT1);
  //MASCARA PIN 4
  PCMSK2 |= (1<<PCINT20);
  // HABILITAD LA INTERRUPCION DEL PUERTO PCINT18,19,20 | D2-D4
  PCICR  |=  (1<<PCIE2);
  // CONFIGURACION DE INTERRUPCIONES
  SREG   |=  (1 << 7); // GLOBAL INTERRUPT ENABLE
  
      // ------------------------lCD --------------------------/
  lcd.init();
  lcd.backlight();  
  lcd.createChar(0, flechita);
  lcd.createChar(1, play);
  lcd.createChar(2, pause);
  lcd.createChar(3, revolucion);
  LCD();

  //------------ SECUENCIA DE INICIO --------------------------/
  Serial.println("CONFIGURADO");
  cadena.reserve(16);
  calcularTocs();
}

void loop()
{
  /* 
  if(flagPush == 1)
  {
    Push();
    flagPush = 0;
    Cursor();
  }
  if(flagEncoder == 3 || flagEncoder == 6)
  {
    Encoder();
    LCD();
    Cursor();
  }
  if(estado == 1)
  {
    if(flagAccel == 1)  // INCREMENTAR ACELERACION LINEALMENTE
    {
      flagAccel = 0;  
      if(contadorTocs > maxCTocs)
      {
        tocs = minTocs;
        flagAccel = 0;
        OCR1A = tocs;
        contadorTocs = 0;
      }
      contadorTocs = contadorTocs + 1;
    }
    if(modo == MODO_DOSIFICADOR) 
    {
      if(contadorPasos > pasosTotales)  //BOMBA COMPLETO PASOS
      {
        MotorOff();
        Serial.print(" DOSIS N TERMINADA");
        if(disparosConteo < disparosTotal)
        {
          disparosConteo += 1;
          contadorPasos = 0; 
          if(disparosConteo == disparosTotal)
          {
            disparosConteo = 0;
            MotorOff();
            Serial.print(" DOSIFICACION TERMINADA");
          }
          else
          {
            ActualizarConteo();
            delay(pausa*1000);
            MotorOn();
          }
        }
      }
      contadorPasos = contadorPasos+1;
    }
  }
  */
}

void LCD()
{
  if(etapa == ETAPA_MODOS)  // MENU INICIO
  {
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("MODO:");
    lcd.setCursor(1,1);
    if(modo == MODO_BOMBA) // MOBO BOMBA
    {
      lcd.print("BOMBA");
    }
    else if(modo == MODO_DOSIFICADOR) // MOBO DOSIFICADOR
    {
      lcd.print("DOSIFICADOR");
    }
    else if(modo == MODO_CALIBRACION) // MOBO CALIBRACION
    {
      lcd.print("CALIBRACION");
    }
  }
  else if(etapa == ETAPA_PARAMETROS)  // EDICION DE PARAMETROS
  {
    if(modo == MODO_BOMBA) // MODO BOMBA, CONTROL FLUJO
    {
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("FLUJO:");
      lcd.setCursor(1,1);
      lcd.print(flujo,1);
      lcd.print("mL/min:");
    }
    if(modo == MODO_DOSIFICADOR) // MODO DOSIFICADOR, CONTROL DISPAROS, VOLUMEN, TIEMPO, PAUSA
    {
      lcd.clear();
      lcd.setCursor(0,0);
      if(parametro == PARAMETRO_DISPAROS)
      {

        lcd.print("DISPAROS:");
        lcd.setCursor(1,1);
        lcd.print(disparosConteo);
      }
      else if(parametro == PARAMETRO_VOLUMEN)
      {
        lcd.print("VOLUMEN:");
        lcd.setCursor(1,1);
        lcd.print(volumen,1);
        lcd.print("mL");
      } 
      else if(parametro == PARAMETRO_TIEMPO)
      {
        lcd.print("LLENAR EN:");
        lcd.setCursor(1,1);
        lcd.print(tiempo);
        lcd.print("s");
      }      
      else if(parametro == PARAMETRO_PAUSA)
      {
        lcd.print("PAUSA:");
        lcd.setCursor(1,1);
        lcd.print(tiempo);
        lcd.print("s");
      }            
    }
    if(modo == MODO_CALIBRACION) // MODO DOSIFICADOR, CONTROL DISPAROS, VOLUMEN, TIEMPO, PAUSA
    {
      lcd.clear();
      lcd.setCursor(0,0);
      if(parametro == PARAMETRO_DIAMETRO)
      {

        lcd.print("DIAMETRO:");
        lcd.setCursor(1,1);
        lcd.print(diametro);
        lcd.print("mm");
      }
      else if(parametro == PARAMETRO_PROPORCION)
      {
        lcd.print("PROPORCION:");
        lcd.setCursor(1,1);
        lcd.print(ml_rev,2);
        lcd.print("mL/rev");
      } 
      else if(parametro == PARAMETRO_SENTIDO)
      {
        lcd.print("SENTIDO:");
        lcd.setCursor(1,1); 
        if(sentido == 1)
        {
          lcd.print("CLOCKWISE");
        }
        else if(sentido == 0)
        {
          lcd.print("ANTICLOCKWISE");
        }
      }        
    }
  }
  else if(etapa == ETAPA_OPERACION)  // MENU OPERACION
  {
    lcd.clear();
    lcd.setCursor(0,0);
    if(modo == MODO_BOMBA) // MODO BOMBA CONTROL DE INICIO
    {
      lcd.print("F:");
      lcd.print(flujo,1);
      lcd.print("mL/min:");
    }
    else if(modo == MODO_DOSIFICADOR) // MODO DOSIFICADOR CONTROL DE INICIO
    {
      lcd.print("T:");
      lcd.print(tiempo);
      lcd.print("s");
      lcd.setCursor(7,0);
      lcd.print("P:");
      lcd.print(pausa);
      lcd.print("s");
      lcd.setCursor(0,1);
      lcd.print("V:");
      lcd.print(volumen,1);
      lcd.print("mL");
      lcd.setCursor(11,1);
      lcd.print(disparosConteo);
      lcd.print("/");
      lcd.print(disparosTotal);
    }
    else if(modo == MODO_CALIBRACION) // MODO DOSIFICADOR CONTROL DE INICIO
    {
      lcd.print("Di:");
      lcd.print(diametro);
      lcd.print("mm");
      lcd.setCursor(8,0);
      lcd.print("D:");
      if(sentido == 1)
      {
        lcd.print("CW:");
      }
      else if(sentido == 0)
      {
        lcd.print("ACW:");
      }
      lcd.setCursor(0,1);
      lcd.print("P");
      lcd.print(ml_rev,3);
      lcd.print("mL/rev:");
    }
    lcd.setCursor(14,0);
    lcd.write(byte(0)); // FLECHITA
  }
}

inline void ActualizarConteo()
{
  lcd.setCursor(11,1);
  lcd.print(disparosConteo);
  lcd.print("/");
  lcd.print(disparosTotal);
}

inline void Cursor()
{
  if(etapa == ETAPA_MODOS || etapa == ETAPA_PARAMETROS)
  {
    lcd.setCursor(0,1);
    if(config)
    {
      lcd.write(byte(0)); // FLECHITA
    }
    else
    {
      lcd.print(" ");
    }
  }  
  else if(etapa == ETAPA_OPERACION)
  {
    lcd.setCursor(15,0);
    if(estado)
    {
      lcd.write(byte(2)); // PAUSA
    }
    else
    {
      lcd.write(byte(1)); // PAUSA
    }
  }
}

void Push()
{
  if(etapa == ETAPA_MODOS || etapa == ETAPA_PARAMETROS)
  {
    config ^= 1; //TOGGLE BIT
    Serial.println("push");
  }
  else if(etapa == ETAPA_OPERACION)
  {
    estado ^= 1; //TOGGLE BIT
    if(estado)
    {
      Serial.print("flujo: ");
      Serial.println(flujo,2 );
      MotorOn();
    }
    else
    {
      MotorOff();
    }
  }
}

void Encoder()
{
  if(flagEncoder == 3)
  {
    Sumar();
    flagEncoder = 0;
    LCD();
  }
  if(flagEncoder == 6)
  {
    Restar();
    flagEncoder = 0;
    LCD();
  }
}

void Sumar()
{
  if(config == 0) // MOVERSE A TRAVES DE MENUS
  {
    switch (modo)
    {
      case 0:
        parametro = parametroBomba;
        parametroMAX = PARAMETROS_BOMBA;     
        break;
      case 1:
        parametro = parametroDosificador;
        parametroMAX = PARAMETROS_DOSIFICADOR;     
        break;            
      case 2:
        parametro = parametroCalibracion;
        parametroMAX = PARAMETROS_CALIBRACION;     
        break;
    }
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
    else if(etapa < ETAPA_MAXIMA)
    {
      etapa += 1;
    }
    switch (modo)
    {
      case 0:
        parametroBomba = parametro;
        break;
      case 1:
        parametroDosificador = parametro;
        break;            
      case 2:
        parametroCalibracion = parametro;
        break;
    }
  }
  else if(config == 1)//CAMBIAR MODO, VALOR DE PARAMETRO, 
  {
    if(etapa == ETAPA_MODOS) // CAMBIAR MODO
    {
      if(modo < MODOS_MAXIMOS)
      {
        modo +=1;
      }
    }
    else if(etapa == ETAPA_PARAMETROS) // CAMBIAR VALOR DE PARAMETRO
    {
      if(modo == MODO_BOMBA) // MODO BOMBA CONTROL FLUJO
      {
        if(parametroBomba == PARAMETRO_FLUJO)
        {
          if(flujo < flujoMax)
          {
            flujo = flujo + pasoFlujo;
            Serial.print("pasoFlujo: ");
            Serial.println(pasoFlujo);
            Serial.print("flujo: ");
            Serial.println(flujo);
          }
        }
      }
      else if(modo == MODO_DOSIFICADOR)  // MODO DOSIFICADOR CONTROL DISPAROS, VOLUMEN, TIEMPO, PAUSA
      {
        if(parametroDosificador == PARAMETRO_DISPAROS)
        {
          if(disparosTotal < disparosmax)
          {
            disparosTotal += 1;
          }
        }    
        else if(parametroDosificador == PARAMETRO_VOLUMEN)
        {
          if(volumen < volumenMax)
          {
            volumen += pasoVolumen;
            TiempoMin();
          }
        }    
        else if(parametroDosificador == PARAMETRO_TIEMPO)
        {
          if(tiempo < tiempoMax)
          {
            tiempo += 1;
          }
        }    
        else if(parametroDosificador == PARAMETRO_PAUSA)
        {
          if(pausa < pausaMax)
          {
            pausa += 1;
          }
        }      
      }
      else if(modo == MODO_CALIBRACION) // MODO CALIBRACION CONTROL DIAMETRO, PROPORCION, sentido
      {
        if(parametroCalibracion == PARAMETRO_DIAMETRO)
        {
          if(diametro < diametroMax)
          {
            diametro += 1;
          }
        }    
        else if(parametroCalibracion == PARAMETRO_PROPORCION)
        {
          if(ml_rev < proporcionMax)
          {
            ml_rev += pasoProporcion;
          }
        }    
        else if(parametroCalibracion == PARAMETRO_SENTIDO)
        {
          if(sentido < sentidoMax)
          {
            sentido += 1;
          }
        }
      }    
    }
  }
}

void Restar()
{
  if(config == 0) // MOVERSE A TRAVES DE MENUS
  {
    switch (modo)
    {
      case 0:
        parametro = parametroBomba;
        break;
      case 1:
        parametro = parametroDosificador;
        break;            
      case 2:
        parametro = parametroCalibracion;
        break;
    }
    if(etapa == ETAPA_PARAMETROS) // MENU DE PARAMETROS
    {
      if(parametro > PARAMETRO_MINIMO)
      {
        parametro -= 1;
      }
      else if(parametro == 1)
      {
        etapa -= 1;
      }
    }
    else if(etapa > ETAPA_MINIMA)
    {
      etapa -= 1;
    }
    switch (modo)
    {
      case 0:
        parametroBomba = parametro;
        break;
      case 1:
        parametroDosificador = parametro;
        break;            
      case 2:
        parametroCalibracion = parametro;
        break;
    }
  }
  else if(config == 1)//CAMBIAR MODO, VALOR DE PARAMETRO, 
  {
    if(etapa == ETAPA_MODOS) // CAMBIAR MODO
    {
      if(modo > MODOS_MINIMO)
      {
        modo -=1;
      }
    }
    else if(etapa == ETAPA_PARAMETROS) // CAMBIAR VALOR DE PARAMETRO
    {
      if(modo == MODO_BOMBA) // MODO BOMBA CONTROL FLUJO
      {
        if(parametroBomba == PARAMETRO_FLUJO)
        {
          if(flujo > flujoMin)
          {
            Serial.println("restar flujo");
            flujo -= pasoFlujo;
          }
        }
      }
      else if(modo == MODO_DOSIFICADOR)  // MODO DOSIFICADOR CONTROL DISPAROS, VOLUMEN, TIEMPO, PAUSA
      {
        if(parametroDosificador == PARAMETRO_DISPAROS)
        {
          if(disparosTotal > disparosMin)
          {
            disparosTotal -= 1;
          }
        }    
        else if(parametroDosificador == PARAMETRO_VOLUMEN)
        {
          if(volumen > volumenMin)
          {
            volumen -= pasoVolumen;
            TiempoMin();
          }
        }    
        else if(parametroDosificador == PARAMETRO_TIEMPO)
        {
          if(tiempo > tiempoMin)
          {
            tiempo -= 1;
          }
        }    
        else if(parametroDosificador == PARAMETRO_PAUSA)
        {
          if(pausa > pausaMin)
          {
            pausa -= 1;
          }
        }      
      }
      else if(modo == MODO_CALIBRACION) // MODO CALIBRACION CONTROL DIAMETRO, PROPORCION, sentido
      {
        if(parametroCalibracion == PARAMETRO_DIAMETRO)
        {
          if(diametro > sentidoMin)
          {
            diametro -= 1;
          }
        }    
        else if(parametroCalibracion == PARAMETRO_PROPORCION)
        {
          if(ml_rev > proporcionMin)
          {
            ml_rev -= pasoProporcion;
          }
        }    
        else if(parametroCalibracion == PARAMETRO_SENTIDO)
        {
          if(sentido > sentidoMin)
          {
            sentido -= 1;
          }
        }
      }    
    }
  }
}


void salvarVariable(int posicion)
{
  int direccion_calibracion = 32*diametro;
  switch (posicion)
  {
    case 0:
      EEPROM.update(direccion_calibracion, ml_revs[diametro]);
    break;
    case 1:
      EEPROM.update(direccion_diametro, diametro);
    break;
    case 2:
      EEPROM.update(direccion_sentido, sentido);
    break;
  }
}


void calcularTocs()
{
  float volumenLocal;
  int tiempoLocal;
  float flujoLocal;
  float g_ml;
  float g;
  long pasos; 
  float velocidad;
  long clics = RELOJ/array_prescaler[prescaler];
  // OBTENER EL VOLUMEN TOTAL A DESPLAZAR
  if(modo == MODO_BOMBA)
  {
    // volumen por minuto
    volumenLocal = flujo;
    tiempoLocal = 60; 
    flujoLocal = volumenLocal/tiempoLocal;
  }
  else if(modo == MODO_DOSIFICADOR)
  {
    volumenLocal = volumen;
    tiempoLocal = tiempo; 
    flujoLocal = volumenLocal/tiempoLocal;
  }
  else if(modo == MODO_CALIBRACION)
  {
    volumenLocal = flujo;
    tiempoLocal = 60; 
    flujoLocal = volumenLocal/tiempoLocal;
  }
  if(flujoLocal > 3)
  {
    MICROSTEPS = 4; 
    setuSteps();
  }
  else if(flujoLocal > 2)
  {
    MICROSTEPS = 8; 
    setuSteps();
  }
  else if(flujoLocal > 1)
  {
    MICROSTEPS = 16; 
    setuSteps();
  }
  else if(flujoLocal > 0)
  {
    MICROSTEPS = 32; 
    setuSteps();
  }
  // calcular grados totales
  // mL/rev = mL/360°   ............................... volumen/grados
  // 1/(mL/360°) ...................................... 1) grados/volumen
  // grados/volumen * volumen ......................... 2) grados
  // ° / (°/paso) ..................................... 3) pasos
  // uSteps * pasos ................................... 4) pasos totales
  // pasos totales / tiempo ........................... 5) velocidad
  // 1/velocidad ...................................... 6) periodo
  // clics * periodo/2 ................................ 7) tocs
  // clics/velocidad/2 ................................ 6) tocs....

  // 1/(1.25mL/360°) =288°/mL
  // 288°/mL * 20.00 mL = 5760°
  // 5760° / (1.8°/paso)  = 3200 pasos
  // 3200 pasos * 32us = 102400 upasos
  // 102400upasos / 60s = 1706.6upasos/s 
  // 1/1706.6upasos/s = 585.96us  
  //  16 000 000 / 64 * 585.96/ 1 000 000 /2= 73 tocs

  g_ml = (1.0/(ml_rev/360));
  g = g_ml * volumenLocal;
  pasos = (long) g / G_PASO;
  pasosTotales = pasos*MICROSTEPS;
  velocidad = pasosTotales/ (float) tiempoLocal; 
  tocs = clics / velocidad / 2;

  //VALIDAR TOCS
  Serial.print("\n\n");
  Serial.print("uStep: ");
  Serial.println(MICROSTEPS);
  Serial.print("prescaler: ");
  Serial.println(array_prescaler[prescaler]);
  Serial.print("volumenLocal: ");
  Serial.println(volumenLocal);
  Serial.print("Flujomax: ");
  Serial.println(flujoMax);
  Serial.print("Flujomix: ");
  Serial.println(flujoMin);  
  Serial.print("tiempoLocal: ");
  Serial.println(tiempoLocal);
  Serial.print("mL/rev: ");
  Serial.println(ml_rev);
  Serial.print("g_ml: ");
  Serial.println(g_ml);
  Serial.print("g: ");
  Serial.println(g);
  Serial.print("pasos: ");
  Serial.println(pasos);
  Serial.print("pasosTotales: ");
  Serial.println(pasosTotales);
  Serial.print("velocidad: ");
  Serial.println(velocidad);
  Serial.print("tocs: ");
  Serial.println(tocs);
}
/*
void calcularTocs()
{
  flagAccel = 0;
  float volumenLocal;
  int tiempoLocal;
  switch (modo)
  {
    case 0: // MODO BOMBA
      volumenLocal = flujo;
      tiempoLocal = 60/2;
    break;

    case 1: //MODO DOSIFICADOR
      tiempoLocal = tiempo;
      volumenLocal = volumen*2;
      //DISPAROS
      disparosConteo = 0;
      ActualizarConteo();
    break;
  }
  contadorPasos = 0;
  resolucion = (float) prescaler/16;
  pasosTotales = (long) (volumenLocal*MICROSTEPS)/((g_paso/(360/ml_rev)));
  if(pasosTotales > 40000)
  {
    MICROSTEPS = 4;
    setuSteps();
  }
  else if(pasosTotales > 32000)
  {
    MICROSTEPS = 8;
    setuSteps();
  }
    else if(pasosTotales > 16000)
  {
    MICROSTEPS = 16;
    setuSteps();
  }
  pasosTotales = (long) (volumenLocal*MICROSTEPS)/((g_paso/(360/ml_rev)));
  velocidad = (float) pasosTotales/tiempoLocal;
  periodo = (long) round(1000000/velocidad);
  tocs = (long) round(periodo/(resolucion));
  if(modo == MODO_DOSIFICADOR) // VELOCIDAD POR DEFECTO PARA CALIBRACION
  {
    pasosTotales = 2*MOTOR_STEPS*MICROSTEPS;
    tocs = 200; // CALCULAR
  }
  else if(tocs < limtocs) //EXCESO DE ACELERACION
  {
    minTocs = tocs;
    tocs = maxTocs; // LIMITE INFERIOR DE TOCS.
    flagAccel = 1;
  }
  OCR1A = tocs;
  Serial.print("tocs: ");
  Serial.println(tocs);
}
*/

void recuperarVariables()
{
  bool primeraVez;
  EEPROM.get(direccion_inicializado, primeraVez);
  if(primeraVez == 0)
  {
    Serial.println("VARIABLES GUARDADAS");
    for(int o = diametroMin; o <= diametroMax; o++)
    {
      EEPROM.get(32*o, ml_revs[o]);
    }
    EEPROM.get(direccion_diametro, diametro);
    EEPROM.get(direccion_sentido, sentido);  
  }
  else  // primera vez;
  {
    Serial.println("SIN VARIABLES GUARDADAS");
    for (unsigned int i = 0 ; i < EEPROM.length() ; i++) 
    {
    EEPROM.write(i, 0);
    }
    EEPROM.put(direccion_inicializado, 0);
    EEPROM.put(direccion_diametro, diametro);
    EEPROM.put(direccion_sentido, sentido);
    for(int u = diametroMin; u <= diametroMax; u++)
    {
      EEPROM.put(32*u, 1.0);
    }
  } 
}

inline void setuSteps()
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

void Calibrar(int opcion)
{
  salvarVariable(opcion); // diametro
  switch(opcion)
  {
    case 0: // PROPORCION
      ml_rev = ml_revs[diametro];
      flujoMax = RPMAX * ml_rev;
      flujoMin = RPMMIN * ml_rev;
      flujoPSMax = flujoMax/60.00;
      pasoFlujo = ml_rev;
      pasoVolumen = ml_rev;
      TiempoMin();
    break;
    case 1: // DIAMETRO
    break;
    case 2: // sentido
    break;
  }
}

inline void MotorOn()
{
  calcularTocs();
  TCNT1  = 0; //contador
  TIMER1_ON();
  ENABLE_LOW
  Cursor();
}

inline void MotorOff()
{
  TIMER1_OFF
  ENABLE_HIGH
  Cursor();
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
inline void TiempoMin()
{
  tiempoMin = (int) round(volumen/flujoPSMax);
  tiempo = tiempoMin + 1;
}

ISR(INT0_vect)  //statusD = PIND; //INTERRUPCION POR PUSH
{
  delay(2);
  statusC = (PIND & B00000100) >> 2;
  if(statusC == 0)
  {
    flagPush = 1;
  }
}

ISR(INT1_vect)  // INTERRUPCION ENCODER DT PADPD3 DT
{
  delay(2);
  statusD = (PIND & B00001000) >> 3;
  if(statusD == 0)
  {
    //Serial.println(statusD,BIN);
    if(DT == 1 && CLK == 1 && flagEncoder == 0)
    {
      DT = 0;
      CLK = 1;
      flagEncoder = 1;
    }
    else if(DT == 1 && CLK == 0 && flagEncoder == 4)
    {
      DT = 0;
      CLK = 0;
      flagEncoder = 5;
    }
  }
}

ISR(PCINT2_vect)  // INTERRUPCION CLK.  PADPD4 CLK
{
  delay(2);
  statusC = (PIND & B00010000) >> 4;
  //Serial.println(statusC,BIN);
  if(statusC == 0)
  {
    if(DT == 0 && CLK == 1 && flagEncoder == 1)
    {
      DT = 0;
      CLK = 0;
      flagEncoder = 2;
    }
    else if(DT == 1 && CLK == 1 && flagEncoder == 0)
    {
      DT = 1;
      CLK = 0;
      flagEncoder = 4;
    }
  }

  if(statusC == 1)
  {
    if(DT == 0 && CLK == 0 && flagEncoder == 2)
    {
      DT = 1;
      CLK = 1;
      flagEncoder = 3;
    }

    else if(DT == 0 && CLK == 0 && flagEncoder == 5)
    {
      DT = 1;
      CLK = 1;
      flagEncoder = 6;
    }
  }
}

ISR(TIMER1_COMPA_vect)
{
  flagTimer = 1;
  TOGGLE_STEP
}