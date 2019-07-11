#include "variables.h"

// CONSTRUCTOR PARA LCD
LiquidCrystal_I2C lcd(I2C_ADD, COL, ROW);

void setup()
{
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

  // --------------------- TIMER1 16B ------------------------/
  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1  = 0; //contador
  OCR1A  = 6250; //0.1s
  TCCR1B |= (1<<WGM12); //MODO 4 CTC
  TIMER1_ON();  //seleccion de prescaler y fuente de reloj
  TIMSK1 |= (1 << OCIE1A); //MASCARA DE INTERRUPCION
  TIMER1_OFF;
  // ------------------------lCD --------------------------/
  lcd.init();
  lcd.backlight();  
  lcd.createChar(0, flechita);
  lcd.createChar(1, play);
  lcd.createChar(2, pause);
  lcd.createChar(3, revolucion);
  // ------------------------SERIAL --------------------------/
  UCSR0B |= (1<<TXEN0)|(1<<RXEN0);
  UBRR0L = 103;  // 9600 BAUDS
  UCSR0B |= (1<<RXCIE0);
  // ------------------------GPIOs --------------------------/
  DDRD &= ~((1<<DDD2)|(1<<DDD3)|(1<<DDD4)); // DIRECTION, INPUT
  // CONFIGURAR PUERTO D SIN PULL UP RS //clearing bits 2-4
  PORTD &= ~((1<<DDD2)|(1<<DDD3)|(1<<DDD4));
  EICRA |= (1<<ISC01);
  // MASCARA PIN 2
  EIMSK |= (1<<INT0);
  //MASCARA PIN 4
  PCMSK2 |= (1<<PCINT20);
  // HABILITAD LA INTERRUPCION DEL PUERTO PCINT18,19,20 | D2-D4
  PCICR  |=  (1<<PCIE2);
  // CONFIGURACION DE INTERRUPCIONES
  SREG   |=  (1 << 7); // GLOBAL INTERRUPT ENABLE

  menuInicio();
  cadena.reserve(16);
  calcularTocs();
}

void loop()
{ 
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
    if(modo == 1) // DOSIFICADOR
    {
      if(contadorPasos > pasosTotales)
      {
        motorPause();
        Serial.print(" DOSIS N TERMINADA");
        if(disparosConteo < disparosTotal)
        {
          disparosConteo += 1;
          contadorPasos = 0;
          pantallaInfo();
          if(disparosConteo == disparosTotal)
          {
            disparosConteo = 0;
            motorOff();
            Serial.print(" DOSIFICACION TERMINADA");
          }
          else
          {
            delay(pausa*1000);
            motorReplay();
          }
        }
      }
      contadorPasos = contadorPasos+1;
    }
    else if(modo == 2) // CALIBRACION
    {
      if(contadorPasos > pasosTotales)
      {
        motorOff();
      }
      contadorPasos = contadorPasos+1;
    }
  }

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
          pantallaInfo();
          if(estado == 0)
          {
            if(modo == 0) // BOMBA
            {
              Serial.println("PARANDO BOMBA");
            }
            else if(modo == 1) // DOSIFICADOR
            {
              Serial.println("PARANDO DOSIFICADOR");
            }
            else if(modo == 2) // CALIBRACION
            {
              Serial.println("PARANDO CALIBRACION");
            }
            motorOff();
          }
          else
          {
            if(modo == 0) // BOMBA
            {
              Serial.println("ARRANCANDO BOMBA");
            }
            else if(modo == 1) // DOSIFICADOR
            {
              Serial.println("ARRANCANDO DOSIFICADOR");
            }
            else if(modo == 2) // CALIBRACION
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
            modo = 0; //MODO BOMBA
            pantallaInfo();
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
          modo = 1; //MODO DOSIFICADOR
          pantallaInfo();
        }
        else if(cadena.startsWith("V")) //VOLUMEN
        {
          Serial.println("INICIANDO DOSIFICADOR");
          cadena.remove(0,1);
          volumen = cadena.toFloat();
          tiempoMin();
          // calcular tiempo maximo
          modo = 1; //MODO DOSIFICADOR
          //calcularTocs();
          pantallaInfo();
        }
        else if(cadena.startsWith("T")) //TIEMPO
        {
          Serial.println("INICIANDO DOSIFICADOR");
          cadena.remove(0,1);
          tiempo = cadena.toInt();
          modo = 1; //MODO DOSIFICADOR
          //calcularTocs();
          pantallaInfo();
        }
        else if(cadena.startsWith("Q")) //DISPAROS TOTALES
        {
          Serial.println("INICIANDO DOSIFICADOR");
          cadena.remove(0,1);
          disparosTotal = cadena.toInt();
          modo = 1; //MODO DOSIFICADOR
          pantallaInfo();
        }
        else if(cadena.startsWith("D")) //diametro y dselector
        {
          cadena.remove(0,1);
          int diametroTemp = cadena.toInt();
          if(diametroTemp >= dmin && diametroTemp <= dmax)
          {
            diametro = diametroTemp;
            diametroModificado();
            modo = 2;
            Serial.print(" dselector: ");
            Serial.println(dselector);
            Serial.print(" proporcion: ");
            Serial.println(ml_rev,3);
            pantallaInfo();
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
          modo = 2;
          salvarVariables(2);
          pantallaInfo();
        }
        else if(cadena.startsWith("R")) //proporcion
        {
          cadena.remove(0,1);
          ml_rev = cadena.toFloat();
          ml_revs[dselector] = ml_rev;
          fpaso = ml_rev;
          modo = 2;
          salvarVariables(0);
          pantallaInfo();
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
  // NO EDITAR INTERRUPCION POR PUSH
  else if(flag == 2)
  {
    flag = 0;
    sw = B100 & statusD;
    if(sw == 0)
    {
      if (etapa == 0)
      {
        if(config == 0)
        {
          config = 1;
          lcdFlecha();
        }
        else
        {
          config = 0;
          borrarFlecha();
        }
      }
      else if (etapa == 1)
      { // SUBMENU
        if  (config == 0)
        {
          config = 1;
          lcdFlecha();
        }
        else if (config == 1)
        {
          config = 0;
          borrarFlecha();
        }
      }
      else if (etapa == 2)
      { //PANTALLA DE INFO
        if (estado == 0)
        {
          motorOn();
        }
        else if (estado == 1)
        {
          motorOff();
        }
      }
    }
  }

  // NO EDITAR INTERRUPCION POR ENCODER
  else if(flag == 1)
  {
    //Serial.println("ENCODER");
    flag = 0;
    dt = (statusDa & (1<<3))>>3;
    clk = (statusDa & (1<<4))>>4;
    if(conteo>0)
    {
      conteo = 0;
      B = dt & clk;
      //Serial.print(" B: ");
      //Serial.println(B);
      C = A|B;
      if(C == 1) //GIRO A LA DERECHA
      {
        if (etapa == 0) // Menu de inicio
        {
          if(config == 1)
          {
            if (modo < 2)// limite superior de modos ya que solo tenemos 3
            {
                modo = modo + 1; //Suma el modo
                editarModo();
            }
          }
          else
          {
            subMenu();
          }
        }
        else if (etapa == 1)// SUBMENU
        {
          if (modo == 0)  //MODO BOMBA
          {
            if (config == 1)
            {
              if (flujo < flmax) //CHECAR LIMITESUPERIOR
              {
                flujo = flujo + fpaso;
                flujoPaso();
                actualizarV();
              }
            }
            else
            {
              pantallaInfo();
            }
          }
          else if (modo == 1) //MODO DOSIFICADOR
          {
            if (config == 1)
              {
                if (modoDosi == 0) //DISPAROS
                {
                  if (disparosTotal < disparosmax) //CHECAR LIMITESUPERIOR
                  {
                    disparosTotal += 1;
                    actualizarV();
                  }
                }
                else if (modoDosi == 1) // PAUSA
                {
                  if (pausa < pmax) //CHECAR LIMITESUPERIOR
                  {
                    pausa = pausa + 1;
                    actualizarV();
                  }
                }
                else if (modoDosi == 2) // VOLUMEN
                {
                  if (volumen < vmax) //CHECAR LIMITESUPERIOR
                  {
                    volumen += vpaso;
                    tiempoMin();
                    actualizarV();
                  }
                }
                else if (modoDosi == 3) // TIEMPO
                {
                  if (tiempo < tmax) //CHECAR LIMITESUPERIOR
                  {
                    tiempo += 1;
                    actualizarV();
                  }
                }
              }
              else if (config == 0)
              {
                if (modoDosi < 3)// limite superior de modos ya que solo tenemos 3
                {
                    modoDosi += 1; //Suma el modo
                    subMenu();
                }
                else
                {
                  pantallaInfo();
                }
              }
          }
          else if (modo == 2) //MODO CALIBRACION
          {
            if (config == 1)
            {
              if (modoCali == 0) //DIAMETRO
              {
                if (diametro < dmax) //CHECAR LIMITESUPERIOR
                {
                  diametro += 1;
                  diametroModificado();
                  actualizarV();
                }
              }
              else if (modoCali == 1) //DIRECCION
              {
                direccion = 1;
                DIR_CW
                salvarVariables(2);
                actualizarV();
              }
              else if (modoCali == 2) //PROPORCION
              {
                if (ml_rev < rmax) //CHECAR LIMITESUPERIOR
                {
                  ml_rev = ml_rev + rres;
                  fpaso = ml_rev;
                  salvarVariables(0);
                  actualizarV();
                }
              }
            }
            else if (config == 0)
            {
              if (modoCali < 2)// limite superior de modos ya que solo tenemos 3
              {
                  modoCali += 1; //Suma el modo
                  subMenu();
              }
              else
              {
                pantallaInfo();
              }
            }
          }
        }
      }//FIN C=1
      /////////////////////////////
      else //GIRO A LA IZQUIERDA
      {
        if (etapa == 0) // Menu de inicio
        {
          if (modo > 0)// limite inferior de modos ya que solo tenemos 3
          {
              if(config == 1)
              {
                modo = modo - 1; //Resta el modo
                //Cambiar modo
                editarModo();
              }
          }
        }
        else  if (etapa == 1) //SUBMENU
        {
          if (modo == 0)
          {
            if (config == 1)
            {
              //EDITAR FLUJO
              if (flujo > flmin) //CHECAR LIMITE INFERIOR
              {
                flujo = flujo - fpaso;
                flujoPaso();
                actualizarV();
              }
            }
            else //REGRESA A MENU INICIO
            {
              menuInicio();
            }
          }
          else if (modo == 1) //MODO DOSIFICADOR
          {
            if (config == 1)
            {
              if (modoDosi == 0) //DISPAROS
              {
                if (disparosTotal > disparosmin) //CHECAR LIMITESUPERIOR
                {
                  disparosTotal -= 1;
                  actualizarV();
                }
              }
              else if (modoDosi == 1) // PAUSA
              {
                if (pausa > pmin) //CHECAR LIMITEINFERIOR
                {
                  pausa -= 1;
                  actualizarV();
                }
              }
              else if (modoDosi == 2) // VOLUMEN
              {
                if (volumen > vmin) //CHECAR LIMITEINFERIOR
                {
                  volumen -= vpaso;
                  actualizarV();
                }
              }
              else if (modoDosi == 3) // TIEMPO
              {
                if (tiempo > tmin) //CHECAR LIMITEINFERIOR
                {
                  tiempo -= 1;
                  actualizarV();
                }
              }
            }
           else if (config == 0)
           {

             if (modoDosi > 0)// limite inferior de modos ya que solo tenemos 3
             {
                 modoDosi -= 1; //Resta el modo
                 subMenu();
             }
             else if (modoDosi == 0)
             {
               menuInicio();
             }
           }
          }
          else if (modo == 2) // MODO CALIBRACION
          {
            if ( config == 1)
            {
              if (modoCali == 0)
              {
                if (diametro > dmin) //CHECAR LIMITEINFERIOR
                {
                  diametro -= 1;
                  diametroModificado();
                  actualizarV();
                }
              }
              else if (modoCali == 1) //DIRECCION
              {
                direccion = 0;
                DIR_ACW
                salvarVariables(2);
                actualizarV();
              }
              else if (modoCali == 2) // PROPORCION
              {
                if (ml_rev > rmin) //CHECAR LIMITEINFERIOR
                {
                  ml_rev = ml_rev - rres;
                  fpaso = ml_rev;
                  salvarVariables(0);
                  actualizarV();
                }
              }
            }
            else if (config == 0)
            {
              if (modoCali > 0)
              {
                modoCali -= 1;
                subMenu();

              }
              else if (modoCali == 0)
              {
                menuInicio();
              }
            }
          }
        }
        else if (etapa == 2) // PANTALLA INFO
        {
          subMenu();
        }
      }// FIN ELSE
    }
    else
    {
      A = dt & clk;
      conteo = conteo + 1;
    }
  }
}

void borrarFlecha()
{
  if (cursor == 1)
  {
    lcd.setCursor(0,1);
    lcd.print(" "); // BORRAR FLECHITA
  }
  else if (cursor == 2)
  {
    cursor = 1;
    lcd.setCursor(14,0);
    lcd.print(" "); // BORRAR FLECHITA
  }
}
void lcdFlecha()
{
  if (cursor == 1)
  {
    lcd.setCursor(0,1);
    lcd.write(byte(0)); // ESCRIBIR FLECHITA
  }
  else if (cursor == 2)
  {
    cursor = 1;
    lcd.setCursor(14,0);
    lcd.write(byte(0)); // ESCRIBIR FLECHITA
  }

}
void editarModo()
{
  switch (modo)
  {
    case 0:      //MODO BOMBA
      lcd.setCursor(1,1);
      lcd.print("                ");
      lcd.setCursor(1,1);
      lcd.print("BOMBA BOMBA");
    break;
    case 1:     // MODO DOSIFICADOR
      lcd.setCursor(1,1);
      lcd.print("DOSIFICADOR");
    break;
    case 2:           //CALIBRACION
      lcd.setCursor(1,1);
      lcd.print("               ");
      lcd.setCursor(1,1);
      lcd.print("calibracion");
    }
}
void menuInicio()
{
  etapa= 0;
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("MODO:");
  editarModo();
}

void subMenu()
{
  etapa = 1;
  lcd.clear();
  lcd.setCursor(0,0);
  if (modo == 0)  // MODO BOMBA
  {
    lcd.print("FLUJO:");
    lcd.setCursor(1,1);
    lcd.print("20");
    //lcd.print(flujo,decimales);
    lcd.print("mL/m");
  }
  else if (modo == 1)//MODO DISPENSADOR
  {
    switch (modoDosi)
    {
      case 0: // DISPAROS
        lcd.print("DISPAROS:");
        actualizarV();
      break;
      case 1: //PAUSA
        lcd.print("PAUSA:");
        actualizarV();
      break;
      case 2: //VOLUMEN
        lcd.print("VOLUMEN:");
        actualizarV();
      break;
      case 3: //TIEMPO
        lcd.print("TIEMPO P LLENAR");
        actualizarV();
      break;
    }
  }
  else if (modo == 2)//MODO CALIBRACION
  {
    switch (modoCali)
    {
      case 0:      //DIAMETRO
      lcd.print("DIAMETRO INT:");
      actualizarV();
      break;
      case 1:     //DIRECCION
      lcd.print("DIRECCION:");
      actualizarV();
      break;
      case 2:     // PROPORCION
      lcd.print("PROPORCION:");
      actualizarV();
      break;
    }
  }
}
void pantallaInfo()
{
  etapa = 2;
  if (modo == 0)  // BOMBA
  {
    lcd.clear();
    //FLUJO
    lcd.setCursor(0,1);
    lcd.print("F:");
    lcd.print(flujo);
    lcd.print("mL/m");
    //PLAY
    cursor = 2;
    lcdFlecha();
    lcdPlay();
  }
  else if (modo == 1) // MODO DOSIFICADOR
  {
    lcd.clear();
    //VOLUMEN
    lcd.setCursor(0,1);
    lcd.print("V:");
    lcd.print(volumen,decimales);
    lcd.print("mL");
    //TIEMPO
    lcd.setCursor(7,0);
    lcd.print("T:");
    lcd.print(tiempo);
    lcd.print("s");
    //PAUSA
    lcd.setCursor(0,0);
    lcd.print("P:");
    lcd.print(pausa);
    lcd.print("s");
    //DISPAROS
    lcd.setCursor(11,1);
    lcd.print(disparosConteo);
    lcd.print("/");
    lcd.print(disparosTotal);
    cursor = 2;
    lcdFlecha();
    lcdPlay();
  }
  else if (modo == 2) // CALIBRACION
  {
    lcd.clear();
    //DIAMETRO
    lcd.setCursor(0,0);
    lcd.print("Di:");
    lcd.print(diametro);
    lcd.print("mm");
    //DISPAROS
    lcd.setCursor(0,1);
    lcd.print("R:");
    lcd.print(ml_rev,3);
    lcd.print("mL/rev");
    //PLAY
    lcd.setCursor(10,0);
    cursor = 2;
    lcdFlecha();
    lcdPlay();
  }
}
void actualizarV()
{
  if (modo == 1) //MODO DOSIFICADOR
  {
    if (modoDosi == 0)
    {
      lcd.setCursor(1,1);
      lcd.print("               ");
      lcd.setCursor(1,1);
      lcd.print(disparosTotal);
    }
    else if (modoDosi == 1)
    {
      lcd.setCursor(1,1);
      lcd.print("               ");
      lcd.setCursor(1,1);
      lcd.print(pausa);
      lcd.print("s");
    }
    else if (modoDosi == 2)
    {
      lcd.setCursor(1,1);
      lcd.print("               ");
      lcd.setCursor(1,1);
      lcd.print(volumen,decimales);
      lcd.print("mL");
    }
    else if (modoDosi == 3)
    {
      lcd.setCursor(1,1);
      lcd.print("               ");
      lcd.setCursor(1,1);
      lcd.print(tiempo);
      lcd.print("s");
    }
  }
  else if (modo == 2) //MODO CALIBRACION
  {
    if (modoCali == 0)  // DIAMETRO
    {
      lcd.setCursor(1,1);
      lcd.print("               ");
      lcd.setCursor(1,1);
      lcd.print(diametro);
      lcd.print("mm");
    }
    else if (modoCali == 1) // DIRECCION
    {
      lcd.setCursor(1,1);
      lcd.print("               ");
      lcd.setCursor(1,1);
      if(direccion == 0)
      {
        lcd.print("ANTICLOCKWISE");
      }
      else
      {
        lcd.print("CLOCKWISE");
      }
    }
    else if (modoCali == 2) //PROPORCION
    {
      lcd.setCursor(1,1);
      lcd.print("               ");
      lcd.setCursor(1,1);
      lcd.print(ml_rev,3);
      lcd.print("mL/rev");
    }
  }
}
void lcdPlay()
{
  lcd.setCursor(15,0);
  if(modo == 2)
  {
    lcd.write(byte(3)); // ESCRIBIR REV
  }
  else
  {
    lcd.write(byte(1)); // ESCRIBIR PLAY
  }
}
void lcdPause()
{
  lcd.setCursor(15,0);
  lcd.write(byte(2)); // ESCRIBIR PAUSE
}
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
      lcd.setCursor(11,1);
      lcd.print(disparosConteo);
      lcd.print("/");
      lcd.print(disparosTotal);
    break;
  }
  contadorPasos = 0;
  resolucion = (float) prescaler/16;
  pasosTotales = (long) (volumenLocal*MICROSTEPS)/((g_paso/(360/ml_rev)));
  velocidad = (float) pasosTotales/tiempoLocal;
  periodo = (long) round(1000000/velocidad);
  tocs = (long) round(periodo/(resolucion));
  if(modo == 2) // VELOCIDAD POR DEFECTO PARA CALIBRACION
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
}
void setSteps()
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
void motorReplay()
{
  if(error == 0)
  {
    TCNT1  = 0; //contador
    TIMER1_ON();
    ENABLE_LOW
    lcdPause();
  }
}
void motorPause()
{
  TIMER1_OFF
  ENABLE_HIGH
  lcdPlay();
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
    lcdPause();
  }
}
void motorOff()
{
  estado = 0;
  TIMER1_OFF
  ENABLE_HIGH
  lcdPlay();
}
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
// ------------------ RUTINAS DE INTERRUPCION --------------------------//
ISR(PCINT2_vect)  // INTERRUXPTCION POR PIN CHANGE
{
  flag = 1; // INTERRUPCION POR ENCODER
  statusDa = PIND;
}
ISR(INT0_vect)
{
  statusD = PIND; //INTERRUPCION POR PUSH
  flag = 2;
}
ISR(TIMER1_COMPA_vect)
{
  flagTimer = 1;
  TOGGLE_STEP
}
// ------------------ RUTINAS DE INTERRUPCION --------------------------//