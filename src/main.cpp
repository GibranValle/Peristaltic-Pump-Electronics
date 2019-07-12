#include "variables.h"




LiquidCrystal_I2C lcd(I2C_ADD, COL, ROW);





void setup()
{
  // ---------------- STEPPER -----------------------------//
  DDRB &= ~((1<<ENABLE)|(1<<MODE0)|(1<<MODE1)|(1<<RST)); // DIRECTION, OUTPUT
  DDRB |= (1<<FLT); // DIRECTION, OUTPUT
  DDRD &= ~((1<<MODE2)|(1<<DIR)|(1<<STEP)|(1<<SLP)); // DIRECTION, OUTPUT
  setuSteps();
  ENABLE_HIGH
  // ------------------------lCD --------------------------/
  lcd.init();
  lcd.backlight();  
  lcd.createChar(0, flechita);
  lcd.createChar(1, play);
  lcd.createChar(2, pause);
  lcd.createChar(3, revolucion);
  LCD();
  // ------------------------SERIAL --------------------------/
  UCSR0B |= (1<<TXEN0)|(1<<RXEN0); //INTERRUPCION
  UBRR0L = 103;  // 9600 BAUDS
  UCSR0B |= (1<<RXCIE0);//RX
  // --------------------- TIMER1 16B ------------------------/
  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1  = 0; //contador
  OCR1A  = 625; //0.1s
  TCCR1B |= (1<<WGM12); //MODO 4 CTC
  TIMSK1 |= (1 << OCIE1A); //MASCARA DE INTERRUPCION
  timer1On();  //seleccion de prescaler 1024 y fuente de reloj
  TIMER1_OFF;
  // ---------------- EEEPROM -----------------------------//
  recuperarVariables(); //PARAMETRO DE CALIBRACION EEEPROM
  ml_rev = array_ml_rev[diametro];
  paso_proporcion = ml_rev;
  paso_flujo = ml_rev;
  paso_volumen = ml_rev;
  flujo_max = RPMAX * ml_rev;
  flujo_min = RPMMIN * ml_rev;
  flujops_max = flujo_max/60.00;
  if(sentido == 1)
  {
    PORTD |= (1<<DIR);
  }
  else
  {
    PORTD &= ~(1<<DIR);
  }
  // ------------------------GPIOs --------------------------/
  DDRD &= ~((1<<DDD2)|(1<<DDD3)|(1<<DDD4)); 
    // DIRECTION, INPUT
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
  //------------ SECUENCIA DE INICIO --------------------------/
  Serial.println("CONFIGURADO");
  cadena.reserve(16);
  calcularTocs();
}





void loop()
{
  //TERMINAR PULSO DE STEP
  if(flag_timer==1)
  {
    flag_timer=0;
    delayMicroseconds(20);
    //Serial.println("INTERRUPT");
    STEP_LOW
  }
  //encoder
  if(flag_push == 1)
  {
    push();
    flag_push = 0;
    cursor();
  }
  if(flag_encoder == 3 || flag_encoder == 6)
  {
    encoder();
    LCD();
    cursor();
  }
  // ENCENDER MOTOR
  if(estado == 1) 
  {
    if(modo == MODO_BOMBA)
    {
      motorOn();
      estado=2; //NO CALCULAR OTRA VEZ
    }
    if(modo == MODO_DOSIFICADOR) 
    {
      if(contador_pasos > pasos_totales)  //BOMBA COMPLETO PASOS
      {
        motorOff();
        Serial.print(" DOSIS N TERMINADA");
        if(disparos_conteo < disparos_total)
        {
          disparos_conteo += 1;
          contador_pasos = 0; 
          if(disparos_conteo == disparos_total)
          {
            disparos_conteo = 0;
            motorOff();
            Serial.print(" DOSIFICACION TERMINADA");
          }
          else
          {
            actualizarConteo();
            delay(pausa*1000);
            motorOn();
          }
        }
      }
      contador_pasos = contador_pasos+1;
    }
  } 
  else if(estado =0) //APAGAR MOTOR
  {
    motorOff();
  }
  // VER COMUNICACION
  RX();
}




/* ---------------- METODOS PARA RX ---------------- */
inline void RX()
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
          if(estado == 0)
          {
            if(modo == MODO_BOMBA) // BOMBA
            {
              Serial.println("PARANDO BOMBA");
              modo = MODO_BOMBA;
            }
            else if(modo == MODO_DOSIFICADOR) // DOSIFICADOR
            {
              Serial.println("PARANDO DOSIFICADOR");
              modo = MODO_DOSIFICADOR;
            }
            else if(modo == MODO_CALIBRACION) // CALIBRACION
            {
              Serial.println("PARANDO CALIBRACION");
              modo = MODO_CALIBRACION;
            }
            if(modo == MODO_BOMBA || modo == MODO_DOSIFICADOR || modo == MODO_CALIBRACION) // FUNCIONES COMUNES
            {
              if(etapa != ETAPA_OPERACION)
              {
              etapa = ETAPA_OPERACION;
              LCD();
              }
              cursor();
            }
          }
          else
          {
            if(modo == MODO_BOMBA) // BOMBA
            {
              Serial.println("ARRANCANDO BOMBA");
              etapa = ETAPA_OPERACION;
              modo = MODO_BOMBA;
              LCD();
              cursor();
            }
            else if(modo == MODO_DOSIFICADOR) // DOSIFICADOR
            {
              Serial.println("ARRANCANDO DOSIFICADOR");
              etapa = ETAPA_OPERACION;
              modo = MODO_DOSIFICADOR;
              LCD();
              cursor();
            }
            else if(modo == MODO_CALIBRACION) // CALIBRACION
            {
              Serial.println("ARRANCANDO CALIBRACION");
              etapa = ETAPA_OPERACION;
              modo = MODO_CALIBRACION;
              LCD();
              cursor();
            }
          }
        }
        // PARAMETROS DE MODO BOMBA
        else if(cadena.startsWith("F")) 
        {
          cadena.remove(0,1);
          float flujoTemp = cadena.toFloat();
          // VALIDACION
          if(validar(flujoTemp,flujo_min,flujo_max)==-1)
          {
            Serial.println("FLUJO INVALIDO");
          }
          else
          {
            Serial.println("FLUJO EDITADO");
            flujo = flujoTemp;
            etapa = ETAPA_PARAMETROS;
            modo = MODO_BOMBA;
            LCD();
          }
        }
        // PARAMETROS DE MODO BOMBA

        // PARAMETROS DE MODO DOSIFICADOR
        else if(cadena.startsWith("P")) //PAUSA
        {
          Serial.println("INICIANDO DOSIFICADOR");
          cadena.remove(0,1);
          int pausaTemp = cadena.toInt();
          // VALIDAR
          if(validar(pausaTemp,pausa_min,pausa_max)==-1)
          {
            Serial.println("PAUSA INVALIDO");
          }
          else
          {
            Serial.println("PAUSA EDITADO");
            pausa = pausaTemp;
            etapa = ETAPA_PARAMETROS;            
            modo = MODO_DOSIFICADOR;
            parametro = PARAMETRO_PAUSA;
            LCD();
          }   
        }
        else if(cadena.startsWith("V")) //VOLUMEN
        {
          Serial.println("INICIANDO DOSIFICADOR");
          cadena.remove(0,1);
          float volumenTemp = cadena.toFloat();
          // VALIDAR
          if(validar(volumenTemp,volumen_min,volumen_max)==-1)
          {
            Serial.println("VOLUMEN INVALIDO");
          }
          else
          {
            Serial.println("VOLUMEN EDITADO");
            volumen = volumenTemp;
            etapa = ETAPA_PARAMETROS;                
            modo = MODO_DOSIFICADOR;
            parametro = PARAMETRO_VOLUMEN;
            LCD();
          }
        }
        else if(cadena.startsWith("T")) //TIEMPO
        {
          Serial.println("INICIANDO DOSIFICADOR");
          cadena.remove(0,1);
          int tiempoTemp = cadena.toInt();
          // VALIDAR
          if(validar(tiempoTemp,tiempo_min,tiempo_max)==-1)
          {
            Serial.println("TIEMPO INVALIDO");
          }
          else
          {
            Serial.println("TIEMPO EDITADO");
            tiempo = tiempoTemp;
            etapa = ETAPA_PARAMETROS;                
            modo = MODO_DOSIFICADOR;
            parametro = PARAMETRO_TIEMPO;        
            LCD();    
          }
        }
        else if(cadena.startsWith("Q")) //DISPAROS TOTALES
        {
          Serial.println("INICIANDO DOSIFICADOR");
          cadena.remove(0,1);
          int disparosTemp = cadena.toInt();
          // VALIDAR
          if(validar(disparosTemp,disparos_min,disparos_max)==-1)
          {
            Serial.println("DISPAROS INVALIDO");
          }
          else
          {
            Serial.println("DISPAROS EDITADO");
            disparos_total = disparosTemp;
            etapa = ETAPA_PARAMETROS;                
            modo = MODO_DOSIFICADOR;
            parametro = PARAMETRO_DISPAROS;
            LCD();
          }
        }
        // PARAMETROS DE MODO DOSIFICADOR

        // PARAMETROS DE MODO CALIBRACION + GUARDAR CONFIGURACION EEPROM
        else if(cadena.startsWith("D")) // DIAMETRO
        {
          cadena.remove(0,1);
          int diametroTemp = cadena.toInt();
          // VALIDAR
          if(validar(diametroTemp,diametro_min,diametro_max)==-1)
          {
            Serial.println("diametro INVALIDO");
          }
          else
          {

            diametro = diametroTemp;
            ml_rev = array_ml_rev[diametro];
            etapa = ETAPA_PARAMETROS;                
            modo = MODO_CALIBRACION;
            parametro = PARAMETRO_DIAMETRO;
            guardarVariable(GUARDAR_DIAMETRO);
            LCD();
            Serial.print("diametro: ");
            Serial.println(diametro);
            Serial.println("proporcion: ");
            Serial.println(ml_rev);
          }
        }
        else if(cadena.startsWith("S")) // SENTIDO
        {
          cadena.remove(0,1);
          sentido = cadena.toInt();

          if(sentido == 0 || sentido == 1)
          {
            etapa = ETAPA_PARAMETROS;                
            modo = MODO_CALIBRACION;
            parametro = PARAMETRO_SENTIDO;
            guardarVariable(GUARDAR_SENTIDO);
            LCD();
            if(sentido == 1)
            {
              DIR_CW
            }
            else if(sentido == 0)
            {
              DIR_ACW
            }
          }
        }
        else if(cadena.startsWith("R")) // PROPORCION
        {
          cadena.remove(0,1);
          float proporcionTemp = cadena.toFloat();  
                    // VALIDAR
          if(validar(proporcionTemp,proporcion_min,proporcion_max)==-1)
          {
            Serial.println("PROPORCION INVALIDA");
          }
          else
          {
            Serial.println("PROPORCION GUARDADA");
            ml_rev = proporcionTemp;
            array_ml_rev[diametro] = ml_rev;
            paso_flujo = ml_rev;
            etapa = ETAPA_PARAMETROS;                
            modo = MODO_CALIBRACION;
            parametro = PARAMETRO_PROPORCION;
            guardarVariable(GUARDAR_PROPORCION);
            LCD();
          }          
        }
        else if(cadena.startsWith("U")) //DEBUG SOLAMENTE.
        {
          cadena.remove(0,1);
          if(array_microsteps.indexOf(cadena)>0)
          {
            microstep = cadena.toInt();
            setuSteps();
            Serial.println("MICROSTEP EDITADO");
          }
          else
          {
            Serial.println("MICROSTEP INVALIDO");
          }          
        }
        // PARAMETROS DE MODO CALIBRACION + GUARDAR CONFIGURACION EEPROM
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
inline float validar(float temp, float min, float max)
{
  if(temp <= max && temp >= min)
  {
    return temp;
  }
  else
  {
    return -1.0;
  }
}
inline int validar(int temp, int min, int max)
{
  if(temp <= max && temp >= min)
  {
    return temp;
  }
  else
  {
    return -1;
  }
}
/* ---------------- METODOS PARA RX ---------------- */





/* ---------------- METODOS PARA MENU ---------------- */
void LCD()
{
  if(etapa == ETAPA_MODOS)  // MENU INICIO
  {
    menuModos();
  }
  else if(etapa == ETAPA_PARAMETROS)  // EDICION DE PARAMETROS
  {
    if(modo == MODO_BOMBA) // MODO BOMBA, CONTROL FLUJO
    {
      menuBomba();
    }
    if(modo == MODO_DOSIFICADOR) // MODO DOSIFICADOR, CONTROL DISPAROS, VOLUMEN, TIEMPO, PAUSA
    {
      menuDosificador();     
    }
    if(modo == MODO_CALIBRACION) // MODO DOSIFICADOR, CONTROL DISPAROS, VOLUMEN, TIEMPO, PAUSA
    {
      menuCalibracion();
    }
  }
  else if(etapa == ETAPA_OPERACION)  // MENU OPERACION
  {
    if(modo == MODO_BOMBA) // MODO BOMBA CONTROL DE INICIO
    {
      bombear();
    }
    else if(modo == MODO_DOSIFICADOR) // MODO DOSIFICADOR CONTROL DE INICIO
    {
      dosificar();
    }
    else if(modo == MODO_CALIBRACION) // MODO DOSIFICADOR CONTROL DE INICIO
    {
      calibrar();
    }
  }
}

void menuModos()
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

void menuBomba()
{
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("FLUJO:");
  lcd.setCursor(1,1);
  lcd.print(flujo,1);
  lcd.print("mL/min:");
}

void menuDosificador()
{
  lcd.clear();
  lcd.setCursor(0,0);
  if(parametro == PARAMETRO_DISPAROS)
  {
    lcd.print("DISPAROS:");
    lcd.setCursor(1,1);
    lcd.print(disparos_total);
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
    lcd.print(pausa);
    lcd.print("s");
  }   
}

void menuCalibracion()
{
  lcd.clear();
  lcd.setCursor(0,0);
  if(parametro == PARAMETRO_DIAMETRO)
  {
    lcd.print("diametro:");
    lcd.setCursor(1,1);
    lcd.print(diametro);
    lcd.print("mm");
  }
  else if(parametro == PARAMETRO_PROPORCION)
  {
    lcd.print("PROPORCION:");
    lcd.setCursor(1,1);
    lcd.print(ml_rev,4);
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

void bombear()
{
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("F:");
  lcd.print(flujo,1);
  lcd.print("mL/min:");
  lcd.setCursor(14,0);
  lcd.write(byte(0)); // FLECHITA
}
void dosificar()
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
  lcd.print(disparos_conteo);
  lcd.print("/");
  lcd.print(disparos_total);
  lcd.setCursor(14,0);
  lcd.write(byte(0)); // FLECHITA  
}
void calibrar()
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
  lcd.setCursor(14,0);
  lcd.write(byte(0)); // FLECHITA  
}

inline void actualizarConteo()
{
  lcd.setCursor(11,1);
  lcd.print(disparos_conteo);
  lcd.print("/");
  lcd.print(disparos_total);
}

inline void cursor()
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
/* ----------------  METODOS PARA MENU ----------------  */





/* ---------------- METODOS PARA encoder ---------------- */
void push()
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
      motorOn();
    }
    else
    {
      motorOff();
    }
  }
}
void encoder()
{
  if(flag_encoder == 3)
  {
    Serial.println("sumar");
    sumar();
    flag_encoder = 0;
    LCD();
  }
  if(flag_encoder == 6)
  {
    Serial.println("restar");
    restar();
    flag_encoder = 0;
    LCD();
  }
}
void sumar()
{
  if(config == 0) // MOVERSE A TRAVES DE MENUS
  {
    switch (modo)
    {
      case 0:
        parametro = parametro_bomba;
        parametro_max = PARAMETROS_BOMBA;     
        break;
      case 1:
        parametro = parametro_dosificador;
        parametro_max = PARAMETROS_DOSIFICADOR;     
        break;            
      case 2:
        parametro = parametro_calibracion;
        parametro_max = PARAMETROS_CALIBRACION;     
        break;
    }
    if(etapa == 1) // MENU DE PARAMETROS
    {
      if(parametro < parametro_max)
      {
        parametro += 1;
      }
      else if(parametro == parametro_max)
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
        parametro_bomba = parametro;
        break;
      case 1:
        parametro_dosificador = parametro;
        break;            
      case 2:
        parametro_calibracion = parametro;
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
        if(parametro_bomba == PARAMETRO_FLUJO)
        {
          if(flujo < flujo_max)
          {
            flujo = flujo + paso_flujo;
            Serial.print("paso_flujo: ");
            Serial.println(paso_flujo);
            Serial.print("flujo: ");
            Serial.println(flujo);
          }
        }
      }
      else if(modo == MODO_DOSIFICADOR)  // MODO DOSIFICADOR CONTROL DISPAROS, VOLUMEN, TIEMPO, PAUSA
      {
        if(parametro_dosificador == PARAMETRO_DISPAROS)
        {
          if(disparos_total < disparos_max)
          {
            disparos_total += 1;
          }
        }    
        else if(parametro_dosificador == PARAMETRO_VOLUMEN)
        {
          if(volumen < volumen_max)
          {
            volumen += paso_volumen;
            tiempoMin();
          }
        }    
        else if(parametro_dosificador == PARAMETRO_TIEMPO)
        {
          if(tiempo < tiempo_max)
          {
            tiempo += 1;
          }
        }    
        else if(parametro_dosificador == PARAMETRO_PAUSA)
        {
          if(pausa < pausa_max)
          {
            pausa += 1;
          }
        }      
      }
      else if(modo == MODO_CALIBRACION) // MODO CALIBRACION CONTROL diametro, PROPORCION, sentido
      {
        if(parametro_calibracion == PARAMETRO_DIAMETRO)
        {
          if(diametro < diametro_max)
          {
            diametro += 1;
          }
        }    
        else if(parametro_calibracion == PARAMETRO_PROPORCION)
        {
          if(ml_rev < proporcion_max)
          {
            ml_rev += paso_proporcion;
          }
        }    
        else if(parametro_calibracion == PARAMETRO_SENTIDO)
        {
          if(sentido < sentido_max)
          {
            sentido += 1;
          }
        }
      }    
    }
  }
}
void restar()
{
  if(config == 0) // MOVERSE A TRAVES DE MENUS
  {
    switch (modo)
    {
      case 0:
        parametro = parametro_bomba;
        break;
      case 1:
        parametro = parametro_dosificador;
        break;            
      case 2:
        parametro = parametro_calibracion;
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
        parametro_bomba = parametro;
        break;
      case 1:
        parametro_dosificador = parametro;
        break;            
      case 2:
        parametro_calibracion = parametro;
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
        if(parametro_bomba == PARAMETRO_FLUJO)
        {
          if(flujo > flujo_min)
          {
            Serial.println("restar flujo");
            flujo -= paso_flujo;
          }
        }
      }
      else if(modo == MODO_DOSIFICADOR)  // MODO DOSIFICADOR CONTROL DISPAROS, VOLUMEN, TIEMPO, PAUSA
      {
        if(parametro_dosificador == PARAMETRO_DISPAROS)
        {
          if(disparos_total > disparos_min)
          {
            disparos_total -= 1;
          }
        }    
        else if(parametro_dosificador == PARAMETRO_VOLUMEN)
        {
          if(volumen > volumen_min)
          {
            volumen -= paso_volumen;
            tiempoMin();
          }
        }    
        else if(parametro_dosificador == PARAMETRO_TIEMPO)
        {
          if(tiempo > tiempo_min)
          {
            tiempo -= 1;
          }
        }    
        else if(parametro_dosificador == PARAMETRO_PAUSA)
        {
          if(pausa > pausa_min)
          {
            pausa -= 1;
          }
        }      
      }
      else if(modo == MODO_CALIBRACION) // MODO CALIBRACION CONTROL diametro, PROPORCION, sentido
      {
        if(parametro_calibracion == PARAMETRO_DIAMETRO)
        {
          if(diametro > sentido_min)
          {
            diametro -= 1;
          }
        }    
        else if(parametro_calibracion == PARAMETRO_PROPORCION)
        {
          if(ml_rev > proporcion_min)
          {
            ml_rev -= paso_proporcion;
          }
        }    
        else if(parametro_calibracion == PARAMETRO_SENTIDO)
        {
          if(sentido > sentido_min)
          {
            sentido -= 1;
          }
        }
      }    
    }
  }
}
/* ---------------- METODOS PARA encoder ---------------- */





/* ----------------  METODOS PARA CALCULAR  ---------------- */
void calcularTocs()
{
  float volumenLocal;
  int tiempoLocal;
  float flujoLocal;
  float g_ml;
  float g;
  long pasos; 
  float velocidad;
  long clics = RELOJ/prescaler;
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
  /*
  if(flujoLocal > 3)
  {
    microstep = 4; 
    setuSteps();
  }
  else if(flujoLocal > 2)
  {
    microstep = 8; 
    setuSteps();
  }
  else if(flujoLocal > 1)
  {
    microstep = 16; 
    setuSteps();
  }
  else if(flujoLocal > 0)
  {
    microstep = 32; 
    setuSteps();
  }
  */
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
  pasos_totales = pasos*microstep;
  velocidad = pasos_totales/ (float) tiempoLocal; 
  tocs = clics / velocidad / 2;

  if(estado == 0)
  {
    Serial.print("\n\n");
    Serial.print("uStep: ");
    Serial.println(microstep);
    Serial.print("prescaler: ");
    Serial.println(prescaler);
    Serial.print("volumenLocal: ");
    Serial.println(volumenLocal);
    Serial.print("flujo_max: ");
    Serial.println(flujo_max);
    Serial.print("Flujo_min: ");
    Serial.println(flujo_min);
    Serial.print("volumen_min: ");
    Serial.println(volumen_min);    
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
    Serial.print("pasos_totales: ");
    Serial.println(pasos_totales);
    Serial.print("velocidad: ");
    Serial.println(velocidad);
    Serial.print("tocs: ");
    Serial.println(tocs);
  }
}
/*
void calcularTocs()
{
  flag_accelerar = 0;
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
      disparos_conteo = 0;
      actualizarConteo();
    break;
  }
  contador_pasos = 0;
  resolucion = (float) prescaler/16;
  pasos_totales = (long) (volumenLocal*microstep)/((g_paso/(360/ml_rev)));
  if(pasos_totales > 40000)
  {
    microstep = 4;
    setuSteps();
  }
  else if(pasos_totales > 32000)
  {
    microstep = 8;
    setuSteps();
  }
    else if(pasos_totales > 16000)
  {
    microstep = 16;
    setuSteps();
  }
  pasos_totales = (long) (volumenLocal*microstep)/((g_paso/(360/ml_rev)));
  velocidad = (float) pasos_totales/tiempoLocal;
  periodo = (long) round(1000000/velocidad);
  tocs = (long) round(periodo/(resolucion));
  if(modo == MODO_DOSIFICADOR) // VELOCIDAD POR DEFECTO PARA CALIBRACION
  {
    pasos_totales = 2*MOTOR_STEPS*microstep;
    tocs = 200; // CALCULAR
  }
  else if(tocs < tocs_limite) //EXCESO DE ACELERACION
  {
    tocs_min = tocs;
    tocs = maxTocs; // LIMITE INFERIOR DE TOCS.
    flag_accelerar = 1;
  }
  OCR1A = tocs;
  Serial.print("tocs: ");
  Serial.println(tocs);
}
*/
void validarTocs()
{
  
}
void calibrar(int opcion)
{
  guardarVariable(opcion); // diametro
  switch(opcion)
  {
    case 0: // PROPORCION
      ml_rev = array_ml_rev[diametro];
      flujo_max = RPMAX * ml_rev;
      flujo_min = RPMMIN * ml_rev;
      flujops_max = flujo_max/60.00;
      paso_flujo = ml_rev;
      paso_volumen = ml_rev;
      tiempoMin();
    break;
    case 1: // diametro
    break;
    case 2: // sentido
    break;
  }
}
inline void tiempoMin()
{
  tiempo_min = (int) round(volumen/flujops_max);
  tiempo = tiempo_min + 1;
}
/* ----------------  METODOS PARA CALCULAR ----------------  */





/* ----------------  METODOS EEPROM ---------------- */
void recuperarVariables()
{
  bool primeraVez;
  EEPROM.get(direccion_inicializado, primeraVez);
  if(primeraVez == 0) //variables recuperadas
  {
    Serial.println("VARIABLES CARGADAS");
    for(int o = diametro_min-1; o <= diametro_max-1; o++)
    {
      EEPROM.get(32*o, array_ml_rev[o]);
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
    for(int u = diametro_min; u <= diametro_max; u++)
    {
      EEPROM.put(32*u, 1.0);
    }
  } 
}
void guardarVariable(int posicion)
{
  int direccion_calibracion = 32*diametro;
  switch (posicion)
  {
    case 0:
      EEPROM.update(direccion_calibracion, array_ml_rev[diametro]);
    break;
    case 1:
      EEPROM.update(direccion_diametro, diametro);
    break;
    case 2:
      EEPROM.update(direccion_sentido, sentido);
    break;
  }
}
/* ----------------  METODOS EEPROM ----------------  */





/* ----------------  METODOS PARA STEPPER ----------------  */
inline void setuSteps()
{
  PORTB &= ~((1<<MODE2)|(1<<MODE0)|(1<<MODE1));
  switch (microstep)
  {
    case 1:
      PORTB &= ~((1 << MODE0) | (1 << MODE1)| (1 << MODE2));

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
      PORTB |= (1<<MODE2);
    break;

    case 32:
      PORTB |= (1<<MODE2)|(1<<MODE0);
    break;
  }
}
inline void motorOn()
{
  ENABLE_LOW
  Serial.println("MOTORON");
  calcularTocs();
  TCNT1  = 0; //contador
  timer1On();
  cursor();
}
inline void motorOff()
{
  TIMER1_OFF
  ENABLE_HIGH
  cursor();
}
inline void timer1On()
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
/* ----------------  METODOS PARA STEPPER ----------------  */





/* ----------------  INTERRUPCIONES ----------------  */
ISR(INT0_vect)  //INTERRUPCION POR push
{
  delay(2);
  statusP = (PIND & (1<<PUSH_PIN)) >> 2;
  //Serial.println(statusP,BIN);
  if(statusP == 0)
  {
    flag_push = 1;
  }
}
ISR(INT1_vect) // DT
{
  delay(2);
  statusD = (PIND & (1<<DT_PIN)) >> DT_PIN;
  if(statusD == 0)
  {
    if(DT == 1 && CLK == 1 && flag_encoder == 0)
    {
      DT = 0;
      CLK = 1;
      flag_encoder = 1;
    }
    else if(DT == 1 && CLK == 0 && flag_encoder == 4)
    {
      DT = 0;
      CLK = 0;
      flag_encoder = 5;
    }
  }
}
ISR(PCINT2_vect) //CLK
{
  delay(2);
  statusC = (PIND & (1<<CLK_PIN)) >> CLK_PIN;
  if(statusC == 0)
  {
    if(DT == 0 && CLK == 1 && flag_encoder == 1)
    {
      DT = 0;
      CLK = 0;
      flag_encoder = 2;
    }
    else if(DT == 1 && CLK == 1 && flag_encoder == 0)
    {
      DT = 1;
      CLK = 0;
      flag_encoder = 4;
    }
  }
  if(statusC == 1)
  {
    if(DT == 0 && CLK == 0 && flag_encoder == 2)
    {
      DT = 1;
      CLK = 1;
      flag_encoder = 3;
    }
    else if(DT == 0 && CLK == 0 && flag_encoder == 5)
    {
      DT = 1;
      CLK = 1;
      flag_encoder = 6;
    }
  }
}
ISR(TIMER1_COMPA_vect)  //INTERRUPCION TIMER
{
  flag_timer = 1;
  STEP_HIGH
}
/* ----------------  INTERRUPCIONES ----------------  */






