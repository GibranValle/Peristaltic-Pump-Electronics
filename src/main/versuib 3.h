#include "variables.h"

int inicio[2] = {0,0};

// CONSTRUCTOR PARA LCD
LiquidCrystal_I2C lcd(I2C_ADD, COL, ROW);
// FUNCION PARA OLED
//Adafruit_SSD1306 display(OLED_RESET);

void setup()
{
    // ------------------------lCD --------------------------/
  lcd.init();
  lcd.backlight();  
  lcd.createChar(0, flechita);
  lcd.createChar(1, play);
  lcd.createChar(2, pause);
  lcd.createChar(3, revolucion);
  // ---------------- EEEPROM -----------------------------//
  recuperarVariables(0); //PARAMETRO DE CALIBRACION
  recuperarVariables(1); //DSELECTOR
  recuperarVariables(2); //DIRECCION
  diametro = dselector;
  ml_rev = ml_revs[dselector];
  flmax = RPMmax * ml_rev;
  flmin = RPMmin * ml_rev;
  fpsmax = flmax/60.00;
  fpaso = 1;
  // ---------------- STEPPER -----------------------------//
  /*
  DDRB &= ~((1<<ENABLE)|(1<<MODE0)|(1<<MODE1)); // DIRECTION, OUTPUT
  DDRD &= ~((1<<MODE2)|(1<<DIR)|(1<<STEP)); // DIRECTION, OUTPUT
  setuSteps();
  ENABLE_HIGH
  if(direccion == 1)
  {
    PORTD |= (1<<DIR);
  }
  else
  {
    PORTD &= ~(1<<DIR);
  }
  */
    // --------------------- TIMER1 16B ------------------------/
  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1  = 0; //contador
  OCR1A  = 6250; //0.1s
  TCCR1B |= (1<<WGM12); //MODO 4 CTC
  TIMER1_ON();  //seleccion de prescaler y fuente de reloj
  TIMSK1 |= (1 << OCIE1A); //MASCARA DE INTERRUPCION
  TIMER1_OFF;


  // ----------------------- OLED --------------------------/
  // by default, we'll generate the high voltage from the 3.3v line internally! (neat!)
  /*
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // initialize with the I2C addr 0x3C (for the 128x32)
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  */
  // ------------------------SERIAL --------------------------/
  UCSR0B |= (1<<TXEN0)|(1<<RXEN0);
  UBRR0L = 103;  // 9600 BAUDS
  UCSR0B |= (1<<RXCIE0);

  // ------------------------GPIOs --------------------------/
  DDRD &= ~((1<<DDD2)|(1<<DDD3)|(1<<DDD4)); // DIRECTION, INPUT
  // CONFIGURAR PUERTO D SIN PULL UP RS //clearing bits 2-4
  //PORTD |= (1<<DDD2)|(1<<DDD3)|(1<<DDD4);
  PORTD = 0;
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

  Serial.println("CONFIGURADO");
  cadena.reserve(16);
  calcularTocs();
  //OLED();
  LCD();
}

void loop()
{
  pinMode(13, OUTPUT); 

  RX();
    // OPERACION MOTOR
  if(estado == 1 && flagTimer == 1)
  {
    flagTimer = 0;
    if(flagAccel == 1) // aceleracion mmaxima rebasada
    {
      if(contadorTocs > maxCTocs)
      {
        tocs = minTocs;
        flagAccel = 0;
        OCR1A = tocs;
        contadorTocs = 0;
      }
      contadorTocs = contadorTocs + 1;
    }
    if(modo == MODOD) // DOSIFICADOR
    {
      if(contadorPasos > pasosTotales)
      {
        motorPause();
        lcd.setCursor(15,0);
        lcd.write(byte(3)); // ESCRIBIR REV
        Serial.print(" DOSIS N TERMINADA");
        if(disparosConteo < disparosTotal)
        {
          disparosConteo += 1;
          contadorPasos = 0;
          //pantallaInfo();
          if(disparosConteo == disparosTotal)
          {
            disparosConteo = 0;
            motorOff();
            lcd.setCursor(15,0);
            lcd.write(byte(1)); // ESCRIBIR PLAY
            Serial.print(" DOSIFICACION TERMINADA");
          }
          else
          {
            lcd.setCursor(15,0);
            lcd.write(byte(2)); // ESCRIBIR PAUSA
            delay(pausa*1000);
            motorReplay();
          }
          LCD();
        }
      }
      contadorPasos = contadorPasos+1;
    }
    else if(modo == MODOC) // CALIBRACION
    {
      if(contadorPasos > pasosTotales)
      {
        motorOff();
      }
      contadorPasos = contadorPasos+1;
    }
  }

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

}

void LCD()
{
  if(etapa == EMODOS)  // MENU INICIO
  {
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("MODO:");
    lcd.setCursor(1,1);
    if(modo == MODOB) // MOBO BOMBA
    {
      lcd.print("BOMBA");
    }
    else if(modo == MODOD) // MOBO DOSIFICADOR
    {
      lcd.print("DOSIFICADOR");
    }
    else if(modo == MODOC) // MOBO CALIBRACION
    {
      lcd.print("CALIBRACION");
    }
  }
  else if(etapa == EPARAM)  // EDICION DE PARAMETROS
  {
    if(modo == MODOB) // MODO BOMBA, CONTROL FLUJO
    {
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("FLUJO:");
      lcd.setCursor(1,1);
      lcd.print(flujo,1);
      lcd.print("mL/min:");
    }
    if(modo == MODOD) // MODO DOSIFICADOR, CONTROL DISPAROS, VOLUMEN, TIEMPO, PAUSA
    {
      lcd.clear();
      lcd.setCursor(0,0);
      if(parametro == PDISPAROS)
      {

        lcd.print("DISPAROS:");
        lcd.setCursor(1,1);
        lcd.print(disparosConteo);
      }
      else if(parametro == PVOLUMEN)
      {
        lcd.print("VOLUMEN:");
        lcd.setCursor(1,1);
        lcd.print(volumen,1);
        lcd.print("mL");
      } 
      else if(parametro == PTIEMPO)
      {
        lcd.print("LLENAR EN:");
        lcd.setCursor(1,1);
        lcd.print(tiempo);
        lcd.print("s");
      }      
      else if(parametro == PPAUSA)
      {
        lcd.print("PAUSA:");
        lcd.setCursor(1,1);
        lcd.print(tiempo);
        lcd.print("s");
      }            
    }
    if(modo == MODOC) // MODO DOSIFICADOR, CONTROL DISPAROS, VOLUMEN, TIEMPO, PAUSA
    {
      lcd.clear();
      lcd.setCursor(0,0);
      if(parametro == PDIAMETRO)
      {

        lcd.print("DIAMETRO:");
        lcd.setCursor(1,1);
        lcd.print(diametro);
        lcd.print("mm");
      }
      else if(parametro == PPROPORCION)
      {
        lcd.print("PROPORCION:");
        lcd.setCursor(1,1);
        lcd.print(ml_rev,2);
        lcd.print("mL/rev");
      } 
      else if(parametro == PDIRECCION)
      {
        lcd.print("DIRECCION:");
        lcd.setCursor(1,1); 
        if(direccion == 1)
        {
          lcd.print("CLOCKWISE");
        }
        else if(direccion == 0)
        {
          lcd.print("ANTICLOCKWISE");
        }
      }        
    }
  }
  else if(etapa == EOPERA)  // MENU OPERACION
  {
    lcd.clear();
    lcd.setCursor(0,0);
    if(modo == MODOB) // MODO BOMBA CONTROL DE INICIO
    {
      lcd.print("F:");
      lcd.print(flujo,1);
      lcd.print("mL/min:");
    }
    else if(modo == MODOD) // MODO DOSIFICADOR CONTROL DE INICIO
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
    else if(modo == MODOC) // MODO DOSIFICADOR CONTROL DE INICIO
    {
      lcd.print("Di:");
      lcd.print(diametro);
      lcd.print("mm");
      lcd.setCursor(8,0);
      lcd.print("D:");
      if(direccion == 1)
      {
        lcd.print("CW:");
      }
      else if(direccion == 0)
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

void Cursor()
{
  if(etapa == EMODOS || etapa == EPARAM)
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
  else if(etapa == EOPERA)
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
  if(etapa == EMODOS || etapa == EPARAM)
  {
    config ^= 1; //TOGGLE BIT
  }
  else if(etapa == EOPERA)
  {
    estado ^= 1; //TOGGLE BIT
    if(estado)
    {
      motorOn();
    }
    else
    {
      motorOff();
    }
  }
    //config = config^1;
    //Serial.println(PIND,BIN);
    //Serial.println(config);
}

void Encoder()
{
  if(flagEncoder == 3)
  {
  //Serial.print("SUMAR");  
    Sumar();
    Serial.print("etapa: ");  
    Serial.print(etapa);  
    Serial.print(" param: ");  
    Serial.println(parametro);  
    flagEncoder = 0;
  }
  if(flagEncoder == 6)
  {
    //Serial.print("RESTAR");  
    Restar();
    Serial.print("etapa: ");  
    Serial.print(etapa);  
    Serial.print(" param: ");  
    Serial.println(parametro);  
    flagEncoder = 0;
  }
}

void RX()
{
    // RECEPCION DE SERIAL.
  if (Serial.available())
  {
    if(finRX)
    {
      finRX = false;
      if(cadena.length()>1)
      {
        Serial.println(cadena);
        if(cadena.startsWith("E"))  //CAMBIAR ON/OFF
        {
          cadena.remove(0,1);
          estado = cadena.toInt();
          //pantallaInfo();
          if(estado == 0)
          {
            if(modo == MODOB) // BOMBA
            {
              Serial.println("PARANDO BOMBA");
            }
            else if(modo == MODOD) // DOSIFICADOR
            {
              Serial.println("PARANDO DOSIFICADOR");
            }
            else if(modo == MODOC) // CALIBRACION
            {
              Serial.println("PARANDO CALIBRACION");
            }
            motorOff();
          }
          else
          {
            if(modo == MODOB) // BOMBA
            {
              Serial.println("ARRANCANDO BOMBA");
            }
            else if(modo == MODOD) // DOSIFICADOR
            {
              Serial.println("ARRANCANDO DOSIFICADOR");
            }
            else if(modo == MODOC) // CALIBRACION
            {
              Serial.println("ARRANCANDO CALIBRACION");
            }
              motorOn();
          }
        }
        else if(cadena.startsWith("F")) //EDITAR FLUJO - BOMBA
        {
          cadena.remove(0,1);
          int flujoTemp = cadena.toFloat();
          Serial.print("flmax");
          Serial.println(flmax);
          Serial.print("flmin");
          Serial.println(flmin);


          if(flujoTemp <= flmax && flujoTemp >= flmin)
          {
            error = 0;
            Serial.println("INICIANDO BOMBA");
            flujo = flujoTemp;
            modo = MODOB; //MODO BOMBA
            //pantallaInfo();
          }
          else
          {
            error = 1;
            Serial.println("FLUJO INVALIDO");
          }
        }
        else if(cadena.startsWith("P")) //PAUSA
        {
          Serial.println("INICIANDO DOSIFICADOR");
          cadena.remove(0,1);
          pausa = cadena.toInt();
          modo = MODOD; //MODO DOSIFICADOR
          //pantallaInfo();
        }
        else if(cadena.startsWith("V")) //VOLUMEN
        {
          Serial.println("INICIANDO DOSIFICADOR");
          cadena.remove(0,1);
          volumen = cadena.toFloat();
          tiempoMin();
          // calcular tiempo maximo
          modo = MODOD; //MODO DOSIFICADOR
          //calcularTocs();
          //pantallaInfo();
        }
        else if(cadena.startsWith("T")) //TIEMPO
        {
          Serial.println("INICIANDO DOSIFICADOR");
          cadena.remove(0,1);
          tiempo = cadena.toInt();
          modo = MODOD; //MODO DOSIFICADOR
          //calcularTocs();
          //pantallaInfo();
        }
        else if(cadena.startsWith("Q")) //DISPAROS TOTALES
        {
          Serial.println("INICIANDO DOSIFICADOR");
          cadena.remove(0,1);
          disparosTotal = cadena.toInt();
          modo = MODOD; //MODO DOSIFICADOR
          //pantallaInfo();
        }
        else if(cadena.startsWith("D")) //diametro y dselector
        {
          cadena.remove(0,1);
          int diametroTemp = cadena.toInt();
          if(diametroTemp >= dmin && diametroTemp <= dmax)
          {
            diametro = diametroTemp;
            diametroModificado();
            modo = MODOC; // MODO CALIBRACION
            Serial.print(" dselector: ");
            Serial.println(dselector);
            Serial.print(" proporcion: ");
            Serial.println(ml_rev,3);
            //pantallaInfo();
          }
          else
          {
            Serial.print(" DIAMETRO INVALIDO: ");
          }

        }
        else if(cadena.startsWith("S")) //direccion
        {
          cadena.remove(0,1);
          direccion = cadena.toInt();
          if(direccion == 1)
          {
            DIR_CW
          }
          else
          {
            DIR_ACW
          }
          modo = MODOC; // MODO CALIBRACION
          salvarVariables(2);
          //pantallaInfo();
        }
        else if(cadena.startsWith("R")) //proporcion
        {
          cadena.remove(0,1);
          ml_rev = cadena.toFloat();
          ml_revs[dselector] = ml_rev;
          fpaso = ml_rev;
          modo = MODOC; // MODO CALIBRACION
          salvarVariables(0);
          //pantallaInfo();
          Serial.print(" dselector: ");
          Serial.println(dselector);
          Serial.print(" proporcion: ");
          Serial.println(ml_rev,3);
        }
        else if(cadena.startsWith("U")) //proporcion
        {
          cadena.remove(0,1);
          MICROSTEPS = cadena.toInt();
          //calcularTocs();
          setuSteps();
        }
      }
      cadena = ""; // borrar cadena
    }
    else
    {
      char caracter = Serial.read();
      if((caracter == '\n') || (caracter == '\r'))
      {
        finRX = true;
      }
      else
      {
        cadena += caracter;
      }
    }
  }
}

void Sumar()
{
  if(config == 0) // MOVERSE A TRAVES DE MENUS
  {
    switch (modo)
    {
      case 0:
        parametro = paraBomba;
        parametroMAX = PARAMETROSBOMBA;     
        break;
      case 1:
        parametro = paraDosi;
        parametroMAX = PARAMETROSDOSI;     
        break;            
      case 2:
        parametro = paraCali;
        parametroMAX = PARAMETROSCALI;     
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
  else if(config == 1)//CAMBIAR MODO, VALOR DE PARAMETRO, 
  {
    if(etapa == EMODOS) // CAMBIAR MODO
    {
      if(modo < MODOSMAX)
      {
        modo +=1;
      }
    }
    else if(etapa == EPARAM) // CAMBIAR VALOR DE PARAMETRO
    {
      if(modo == MODOB) // MODO BOMBA CONTROL FLUJO
      {
        if(paraBomba == PFLUJO)
        {
          if(flujo < flmax)
          {
            flujo += fpaso;
          }
        }
      }
      else if(modo == MODOD)  // MODO DOSIFICADOR CONTROL DISPAROS, VOLUMEN, TIEMPO, PAUSA
      {
        if(paraDosi == PDISPAROS)
        {
          if(disparosTotal < disparosmax)
          {
            disparosTotal += 1;
          }
        }    
        else if(paraDosi == PVOLUMEN)
        {
          if(volumen < vmax)
          {
            volumen += vpaso;
          }
        }    
        else if(paraDosi == PTIEMPO)
        {
          if(tiempo < tmax)
          {
            tiempo += 1;
          }
        }    
        else if(paraDosi == PPAUSA)
        {
          if(pausa < pmax)
          {
            pausa += 1;
          }
        }      
      }
      else if(modo == MODOC) // MODO CALIBRACION CONTROL DIAMETRO, PROPORCION, DIRECCION
      {
        if(paraCali == PDIAMETRO)
        {
          if(diametro < dmax)
          {
            diametro += 1;
          }
        }    
        else if(paraCali == PPROPORCION)
        {
          if(ml_rev < rmax)
          {
            ml_rev += rres;
          }
        }    
        else if(paraCali == PDIRECCION)
        {
          if(direccion < dirmax)
          {
            direccion += 1;
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
        parametro = paraBomba;
        break;
      case 1:
        parametro = paraDosi;
        break;            
      case 2:
        parametro = paraCali;
        break;
    }
    if(etapa == EPARAM) // MENU DE PARAMETROS
    {
      if(parametro > PARAMETROMIN)
      {
        parametro -= 1;
      }
      else if(parametro == 1)
      {
        etapa -= 1;
      }
    }
    else if(etapa > etapaMIN)
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
  else if(config == 1)//CAMBIAR MODO, VALOR DE PARAMETRO, 
  {
    if(etapa == EMODOS) // CAMBIAR MODO
    {
      if(modo > MODOSMIN)
      {
        modo -=1;
      }
    }
    else if(etapa == EPARAM) // CAMBIAR VALOR DE PARAMETRO
    {
      if(modo == MODOB) // MODO BOMBA CONTROL FLUJO
      {
        if(paraBomba == PFLUJO)
        {
          if(flujo > flmin)
          {
            flujo -= fpaso;
          }
        }
      }
      else if(modo == MODOD)  // MODO DOSIFICADOR CONTROL DISPAROS, VOLUMEN, TIEMPO, PAUSA
      {
        if(paraDosi == PDISPAROS)
        {
          if(disparosTotal > disparosmin)
          {
            disparosTotal -= 1;
          }
        }    
        else if(paraDosi == PVOLUMEN)
        {
          if(volumen > vmin)
          {
            volumen -= vpaso;
          }
        }    
        else if(paraDosi == PTIEMPO)
        {
          if(tiempo > tmin)
          {
            tiempo -= 1;
          }
        }    
        else if(paraDosi == PPAUSA)
        {
          if(pausa > pmin)
          {
            pausa -= 1;
          }
        }      
      }
      else if(modo == MODOC) // MODO CALIBRACION CONTROL DIAMETRO, PROPORCION, DIRECCION
      {
        if(paraCali == PDIAMETRO)
        {
          if(diametro > dmin)
          {
            diametro -= 1;
          }
        }    
        else if(paraCali == PPROPORCION)
        {
          if(ml_rev > rmin)
          {
            ml_rev -= rres;
          }
        }    
        else if(paraCali == PDIRECCION)
        {
          if(direccion > dirmin)
          {
            direccion -= 1;
          }
        }
      }    
    }
  }
}

/*
void OLED()
{
  if(etapa == 0)
  {
    display.clearDisplay();
    display.setCursor(COL0,FIL0);
    display.println("MODO: ");
    display.setCursor(COL1,FIL1);
    display.println("BOMBA");
    display.display();
  }
  else if(etapa == 1)
  {
    display.clearDisplay();
    display.setCursor(COL0,FIL0);
    display.println("FLUJO: ");
    display.setCursor(COL1,FIL1);
    display.println(flujo,2);
    display.println("mL/min");
    display.display();
  }
  else if(etapa == 2)
  {
    display.clearDisplay();
    display.setCursor(COL0,FIL0);
    display.println("F:");
    display.println(flujo,2);
    display.println("mL/min");
    display.display();
  }
}


void OSimbolos()
{
  if(etapa == 1 || etapa == 0)  // MENU INICIO Y PARAMETROS
  {
    // display flecha
    display.drawBitmap(COL0, FIL1,  flechita, XSYMBOL, YSYMBOL, config);      
  } 
  else if(etapa == 2) // MENU OPERACION
  { 
    display.drawBitmap(COL8, FIL1,  flechita, XSYMBOL, YSYMBOL, config);      
    if(estado == 1)
    {
      display.drawBitmap(COL9, FIL1,  pause, XSYMBOL, YSYMBOL, COLOR);      
    }
    else if(estado == 0)
    {
      display.drawBitmap(COL9, FIL1,  play, XSYMBOL, YSYMBOL, COLOR);      
    }
  }
  display.display();
}
*/

/*
void calcularTocs()
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
  pasosTotales = (long) (volumenLocal*MICROSTEPS)/((g_paso/(360/ml_rev)));
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
void recuperarVariables(int posicion)
{
  int dirCalibracion = 16;
  switch (posicion)
  {
    case 0: // calibracion
      for(int o = 1; o<5; o++)
      {
        EEPROM.get(dirCalibracion*o, ml_revs[o]);
      }
    break;
    case 1: // dselector
    EEPROM.get(dirDselector, dselector);
    break;
    case 2: // direccion
      EEPROM.get(dirDireccion, direccion);
    break;
  }
}
void salvarVariables(int posicion)
{
  int dirCalibracion = (int) 16*dselector;
  switch (posicion)
  {
    case 0:
      EEPROM.put(dirCalibracion, ml_revs[dselector]);
    break;
    case 1:
      EEPROM.put(dirDselector, dselector);
    break;
    case 2:
      EEPROM.put(dirDireccion, direccion);
    break;
  }
}*/

/*
void setuSteps()
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
*/
/*
void TIMER1_ON()
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
*/

/*
void motorReplay()
{
  if(error == 0)
  {
    TCNT1  = 0; //contador
    TIMER1_ON();
    ENABLE_LOW
  }
}
void motorPause()
{
  TIMER1_OFF
  ENABLE_HIGH
}
void motorOn()
{
  if(error == 0)
  {
    estado = 1;
    calcularTocs();
    TCNT1  = 0; //contador
    TIMER1_ON();
    ENABLE_LOW
  }
}
void motorOff()
{
  estado = 0;
  TIMER1_OFF
  ENABLE_HIGH
}
*/
void diametroModificado()
{
  dselector = diametro;
  salvarVariables(1); // diametro
  recuperarVariables(0); // calibraciones
  ml_rev = ml_revs[dselector];
  flmax = RPMmax * ml_rev;
  flmin = RPMmin * ml_rev;
  fpsmax = flmax/60.00;
  fpaso = ml_rev;
}
void tiempoMin()
{
  tmin = (int) round(volumen/fpsmax);
  tiempo = tmin + 1;
}
void flujoPaso()
{
  if(flujo >= 200)
  {
    fpaso = 10;
  }
  else if(flujo >= 150)
  {
    fpaso = 5;
  }
  else if(flujo >= 90)
  {
    fpaso = 2;
  }
  else
  {
    fpaso = 1;
  }
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
