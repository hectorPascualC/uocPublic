// Love-O-Meter
// ----------------------------------------------------------------
// Variació del projecte inicial d'exemple d'Arduino Starter Kit:
// - Detecció automàtica de temperatura inicial
// - Simulació de refredament per codi per tornar a posició inicial
// ----------------------------------------------------------------
// By: Hèctor Pascual
// Activity: PAC02 - Interacció Tangible
// Date: 30/10/2025

// --- DECLARACIÓ DE CONSTANTS DE PINATGE ---
const int sensorPin = A0;   // Pin analògic on està connectat el sensor TMP36
const int led1 = 3;         // Pin digital del primer LED (nivell 1)
const int led2 = 4;         // Pin digital del segon LED (nivell 2)
const int led3 = 5;         // Pin digital del tercer LED (nivell 3)

// --- VARIABLES DE CONTROL DE TEMPERATURA ---
float baselineTemp = 0.0;   // Temperatura inicial de referència (ambient)
float simulatedTemp = 0.0;  // Temperatura simulada, usada per controlar el refredament automàtic
float lastRealTemp = 0.0;   // Última lectura real del sensor de temperatura
bool coolingMode = false;   // Booleà que indica si estem en mode de refredament

// --- CONSTANTS DE COMPORTAMENT ---
const float heatThreshold = 0.10;        // Increment mínim de temperatura per considerar que puja
const float coolStep = 0.25;             // Quantitat de graus que baixa cada cicle en mode refredament
const unsigned long checkInterval = 200; // Interval de temps entre comprovacions (en mil·lisegons)

// --- FUNCIÓ D'INICIALITZACIÓ ---
void setup() {
  Serial.begin(9600);                // Inicialitza la comunicació sèrie per enviar dades al monitor

  pinMode(led1, OUTPUT);             // Defineix el pin del LED1 com a sortida
  pinMode(led2, OUTPUT);             // Defineix el pin del LED2 com a sortida
  pinMode(led3, OUTPUT);             // Defineix el pin del LED3 com a sortida

  Serial.println("Calculant temperatura base inicial..."); // Mostra missatge d'inici
  baselineTemp = calculaBaseline();  // Calcula la temperatura base mitjançant diverses lectures
  simulatedTemp = baselineTemp;      // Inicialitza la temperatura simulada amb el valor base
  lastRealTemp = baselineTemp;       // Desa la temperatura base com a últim valor llegit

  Serial.print("Temperatura base: "); // Missatge inicial amb la temperatura base
  Serial.print(baselineTemp, 2);      // Mostra el valor numèric amb dos decimals
  Serial.println(" °C");              
  Serial.println("---------------------------");
}

// --- BUCLE PRINCIPAL ---
void loop() {
  // Si no estem en mode de refredament, llegim el sensor i actualitzem la temperatura
  if (!coolingMode) {
    float realTemp = readTemperature(); // Llegeix la temperatura actual del sensor TMP36

    // Comprovem si la temperatura ha pujat prou respecte a l'últim valor
    if (realTemp > lastRealTemp + heatThreshold) {
      simulatedTemp = realTemp;   // Actualitzem la temperatura simulada amb la nova lectura
      lastRealTemp = realTemp;    // Guardem aquesta lectura com a última lectura real
    } 
    else {
      // Si la temperatura ja és alta i comença a baixar, activem el mode refredament
      if (simulatedTemp - baselineTemp > 3.0) {
        coolingMode = true; // Activem el mode de refredament
        
        Serial.println("*** Inici refredament complet ***"); // Missatge informatiu
      }
    }
  } 
  else {
    // --- MODE DE REFREDAMENT AUTOMÀTIC ---
    simulatedTemp -= coolStep;            // Reduïm la temperatura simulada gradualment
    
    if (simulatedTemp <= baselineTemp) {  // Si hem arribat a la temperatura inicial
      simulatedTemp = baselineTemp;       // Assegurem que no baixi més del baseline
      coolingMode = false;                // Desactivem el mode refredament
      
      Serial.println("*** Refredament complet finalitzat ***"); // Missatge final
    }
  }

  // Calculem la diferència entre la temperatura actual i la temperatura base
  float diff = simulatedTemp - baselineTemp;

  // Determinem quin nivell d’intensitat (0 a 3) correspon a la diferència actual
  int nivell = calcularNivell(diff);

  // Mostrem totes les dades actuals al monitor sèrie
  Serial.print("Sim: ");                   
  Serial.print(simulatedTemp, 2);          // Valor de la temperatura simulada amb dos decimals
  Serial.print(" °C | Diff: ");            
  Serial.print(diff, 2);                   // Valor de la temperatura diferent amb dos decimals
  Serial.print(" °C | Nivell: ");          
  Serial.println(nivell);                  // Valor del nivell actual, entre 0 i 3

  // --- CONTROL DELS LEDS SEGONS EL NIVELL ---
  digitalWrite(led1, nivell >= 1 ? HIGH : LOW); // Encén LED1 si el nivell és 1 o més
  digitalWrite(led2, nivell >= 2 ? HIGH : LOW); // Encén LED2 si el nivell és 2 o més
  digitalWrite(led3, nivell >= 3 ? HIGH : LOW); // Encén LED3 si el nivell és 3 o més

  delay(checkInterval);                         // Espera un breu interval abans del següent cicle
}

// --- FUNCIONS AUXILIARS ---

// Funció per llegir la temperatura real des del sensor TMP36
float readTemperature() {
  int sensorVal = analogRead(sensorPin);       // Llegeix el valor analògic del sensor entre 0 i 1023
  float voltage = (sensorVal / 1023.0) * 5.0;  // Converteix la lectura a volts
  float temperature = (voltage - 0.5) * 100.0; // Converteix volts a graus Celsius
  
  return temperature;                          // Retorna la temperatura calculada
}

// Funció per calcular la temperatura base inicial en un promig de 100 lectures
float calculaBaseline() {
  float suma = 0;                              // Variable acumuladora per sumar les lectures
                                
  for (int i = 0; i < 100; i++) {              // Fa 100 lectures seguides
    suma += readTemperature();                 // Afegeix cada lectura a la suma total
    delay(50);                                 // Petita pausa entre lectures per estabilització
  }
  
  float mitjana = suma / 100.0;                // Calcula la mitjana de les lectures
  
  return mitjana;                              // Retorna el valor mitjà (temperatura base)
}

// Funció per determinar quin nivell de calor mostrar segons la diferència
int calcularNivell(float diff) {
  if (diff < 0.8) return 0;                    // Si la diferència és petita, nivell 0 - cap LED encès
  else if (diff < 1.5) return 1;               // Si és moderada, nivell 1 - LED1 encès
  else if (diff < 3.0) return 2;               // Si és més alta, nivell 2 - LED1 i LED2 encesos
  else return 3;                               // Si és molt alta, nivell 3 - tots els LEDs encesos
}
