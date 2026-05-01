#define LED_PIN 4           
#define POT_PIN 34          
#define BOTON_PIN 15        

// Variables para la máquina de estados
volatile int state = 0;           
volatile int contador = 0;  
volatile int botonState = 0;       

// Variables para el Estado 2 (Secuencia de Alerta)
volatile int parpadeoCount = 0;    
volatile int ledState = 0;         
volatile int secuenciaActiva = 0;  

// Variables para debounce 
volatile int lastButtonState = HIGH;
volatile int debounceCounter = 0;
volatile int buttonProcessed = 0;

// Timer hardware
hw_timer_t * timer = NULL;
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;

// Variable para PWM manual
volatile int pwmValue = 0;

// Prototipos
void IRAM_ATTR onTimer();
void cambiarEstado();
void ejecutarStandby();
void ejecutarAnalogico();
void ejecutarAlerta();
void actualizarPWM(int valor);

void setup() 
{
  Serial.begin(9600);
  Serial.println("Sistema de Control Adaptativo con FSM");
  
  // Configurar pines
  pinMode(LED_PIN, OUTPUT);
  pinMode(POT_PIN, INPUT);
  pinMode(BOTON_PIN, INPUT_PULLUP);
  
  // Configurar Timer 
  timer = timerBegin(1000000);           
  timerAttachInterrupt(timer, &onTimer);
  timerAlarm(timer, 1000000, true, 0);  
  
  // Apagar LED al inicio
  digitalWrite(LED_PIN, LOW);
  Serial.println("Estado 0: STANDBY (LED apagado)");
  Serial.println("Presione el botón para cambiar de estado");
}

void loop() 
{
  // Leer botón y cambiar estado si es necesario
  cambiarEstado();
  
  // Ejecutar la acción correspondiente al estado actual
  if (state == 0)
  {
    ejecutarStandby();
  } 
  else if (state == 1)
  {
    ejecutarAnalogico();
  } 
  else if (state == 2)
  {
    ejecutarAlerta();
  }
}

// INTERRUPCIÓN DEL TIMER 
void IRAM_ATTR onTimer() 
{
  portENTER_CRITICAL_ISR(&timerMux);
  contador++;  
  
  // Control de parpadeo en Estado 2
  if(state == 2 && secuenciaActiva == 1) 
  {
    // Alternar estado del LED
    if(ledState == 0) 
    {
      ledState = 1;
      pwmValue = 255;
    } 
    else 
    {
      ledState = 0;
      pwmValue = 0;
      parpadeoCount++;
    }
    
    // Aplicar el valor al LED
    analogWrite(LED_PIN, pwmValue);
    
    // 5 parpadeos COMPLETOS (cuando parpadeoCount llega a 5)
    if(parpadeoCount >= 5) 
    {
      secuenciaActiva = 0;
      state = 0;  
      analogWrite(LED_PIN, 0);  // Apagar LED
    }
  }
  
  portEXIT_CRITICAL_ISR(&timerMux);
}

// CAMBIO DE ESTADO CON DEBOUNCE 
void cambiarEstado()
{
  int reading = digitalRead(BOTON_PIN);
  
  // Detectar flanco de bajada (botón presionado)
  if(reading == LOW && lastButtonState == HIGH && buttonProcessed == 0) 
  {
    buttonProcessed = 1;
    
    // Cambiar al siguiente estado
    if(state == 0) 
    {
      state = 1;
      Serial.println("Estado 1: Control Analógico (Potenciómetro)");
    } 
    else if(state == 1) 
    {
      state = 2;
      Serial.println("Estado 2: Secuencia de Alerta (Parpadeo)");
      
      // Inicializar secuencia de parpadeo
      secuenciaActiva = 1;
      parpadeoCount = 0;
      ledState = 0;
      pwmValue = 0;
      analogWrite(LED_PIN, 0);
    } 
    else if(state == 2) 
    {
      if(secuenciaActiva == 1) 
      {
        secuenciaActiva = 0;
        analogWrite(LED_PIN, 0);
      }
      state = 0;
      Serial.println("Estado 0: Standby (Inactivo)");
    }
  }
  
  // Reset cuando se suelta el botón
  if(reading == HIGH && lastButtonState == LOW) 
  {
    buttonProcessed = 0;
  }
  lastButtonState = reading;
  }

// ESTADO 0: STANDBY (Inactivo)
void ejecutarStandby()
{
  // Asegurar que el LED esté apagado
  static int lastState = -1;
  if(lastState != 0) {
    analogWrite(LED_PIN, 0);
    lastState = 0;
  }
}

// Función para actualizar PWM (usando analogWrite estándar)
void actualizarPWM(int valor) 
{
  analogWrite(LED_PIN, valor);
}

// ESTADO 1: CONTROL ANALÓGICO (Potenciómetro)
void ejecutarAnalogico()
{
  static int lastPrint = 0;
  static int lastValue = -1;
  
  int rawValue = analogRead(POT_PIN);
  int pwmValue = map(rawValue, 0, 4095, 0, 255);

  analogWrite(LED_PIN, pwmValue);
  
  // Imprimir cuando cambia el valor (usando contador del timer)
  if(contador != lastPrint && pwmValue != lastValue) 
  {
    lastPrint = contador;
    lastValue = pwmValue;
    Serial.print("Potenciómetro: ");
    Serial.print(rawValue);
    Serial.print(" → PWM: ");
    Serial.println(pwmValue);
  }
}

// ESTADO 2: SECUENCIA DE ALERTA (Parpadeo)
void ejecutarAlerta()
{
  static int lastLedState = -1;
  static int lastCount = 0;

  if(secuenciaActiva == 1 && ledState != lastLedState)
  {
    if(contador != lastCount)
    {
      lastCount = contador;
      lastLedState = ledState;

      Serial.print("Parpadeo ");
      Serial.print(parpadeoCount + (ledState == 1 ? 1 : 0));

      if(ledState == 1)
      {
        Serial.println(": LED ENCENDIDO");
      }
      else
      {
        Serial.println(": LED APAGADO");
      }
    }
  }
}