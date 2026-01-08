/****************************************************
 * SOUND LEVEL CLASSROOM METER — VERSION MODULAR
 * Interacció Tangible · Arduino
 *
 * Objectiu:
 * Regular el nivell de soroll d’una aula mitjançant
 * feedback visual i no verbal (LEDs + Processing)
 *
 * Sensor:
 * DFRobot Analog Sound Sensor V2
 ****************************************************/

/* ==================================================
   1. CONFIGURACIÓ GENERAL
===================================================== */

// Pin analògic on està connectat el sensor de so
const int SOUND_PIN = A0;

// Pins digitals dels LEDs del semàfor
const int LED_GREEN  = 2;
const int LED_YELLOW = 3;
const int LED_RED    = 4;

// --------------------------------------------------
// Sensibilitat i calibratge
// --------------------------------------------------

// Guany per software: amplifica digitalment el senyal
// perquè les diferències de so siguin més llegibles
const float SOFTWARE_GAIN = 2.4;

// Marges utilitzats per definir els canvis d’estat
// No són valors físics (decibels), sinó llindars funcionals
const int SILENCE_MARGIN  = 10;
const int MARGIN_MEDIUM   = 30;
const int MARGIN_HIGH     = 65;

// --------------------------------------------------
// Inèrcia temporal
// --------------------------------------------------

// Temps mínim que el so ha de mantenir-se
// abans de canviar a groc (evita canvis bruscos)
const unsigned long MEDIUM_CONFIRM_TIME = 400; // ms

// Temps mínim per confirmar el nivell vermell
const unsigned long HIGH_CONFIRM_TIME   = 800; // ms

// --------------------------------------------------
// Calibratge inicial
// --------------------------------------------------

// Temps total de calibratge en silenci
const unsigned long CALIBRATION_TIME_MS = 5000;

// Temps entre lectures durant el calibratge
const int SAMPLE_DELAY_MS = 10;

/* ==================================================
   2. ESTATS DEL SISTEMA
===================================================== */

// Estats lògics del sistema de soroll
enum SoundState {
  STATE_SILENCE, // Silenci o calma
  STATE_LOW,     // Soroll baix (verd)
  STATE_MEDIUM,  // Soroll mitjà (groc)
  STATE_HIGH     // Soroll alt (vermell)
};

/* ==================================================
   3. VARIABLES GLOBALS
===================================================== */

// Valor mitjà del soroll base de l’aula
int baseNoiseLevel = 0;

// Llindars calculats a partir del soroll base
int thresholdMedium = 0;
int thresholdHigh   = 0;

// Valor del senyal suavitzat (filtrat)
int smoothedValue = 0;

// Estat actual del sistema
SoundState currentState = STATE_SILENCE;

// Marques de temps per confirmar canvis d’estat
unsigned long mediumStartTime = 0;
unsigned long highStartTime   = 0;

/* ==================================================
   4. SETUP
===================================================== */

void setup() {
  // Inicialitza comunicació sèrie (Arduino → Processing)
  Serial.begin(9600);

  // Configura els LEDs com a sortida
  setupLEDs();

  // Registra el soroll base real de l’aula
  calibrateNoise();

  // Inicialitza el filtre amb el soroll base
  smoothedValue = baseNoiseLevel;

  Serial.println("Sistema llest\n");
}

/* ==================================================
   5. LOOP PRINCIPAL
   (Estructurat com una seqüència clara)
===================================================== */

void loop() {

  // 1. Llegeix el so actual del sensor
  int soundValue = readSound();

  // 2. Suavitza el senyal per evitar pics puntuals
  soundValue = smoothSignal(soundValue);

  // 3. Decideix en quin estat es troba el sistema
  SoundState newState = determineState(soundValue);

  // 4. Actualitza el semàfor físic
  updateLEDs(newState);

  // 5. Envia l’estat a Processing (visualització)
  sendStateToSerial(soundValue, newState);

  // Pausa curta per estabilitat general
  delay(100);
}

/* ==================================================
   6. LECTURA DEL SENSOR
===================================================== */

int readSound() {

  // Llegeix el valor analògic del sensor (0–1023)
  int value = analogRead(SOUND_PIN);

  // Aplica amplificació per software
  value = value * SOFTWARE_GAIN;

  // Assegura que el valor no surti del rang permès
  return constrain(value, 0, 1023);
}

/* ==================================================
   7. PROCESSAMENT DEL SENYAL
===================================================== */

int smoothSignal(int value) {

  // Mitjana ponderada:
  // dóna més pes al valor anterior que al nou
  // → crea una transició suau i estable
  smoothedValue = (smoothedValue * 7 + value) / 8;

  return smoothedValue;
}

/* ==================================================
   8. DETERMINACIÓ DE L’ESTAT
===================================================== */

SoundState determineState(int value) {

  // ------------------------------------------------
  // Zona de silenci
  // ------------------------------------------------
  if (value <= 1) {
    resetTimers();           // Reinicia temporitzadors
    return STATE_SILENCE;
  }

  // ------------------------------------------------
  // Nivell baix (verd)
  // ------------------------------------------------
  if (value < thresholdMedium) {
    resetTimers();
    return STATE_LOW;
  }

  // ------------------------------------------------
  // Nivell mitjà (groc amb confirmació temporal)
  // ------------------------------------------------
  if (value < thresholdHigh) {
    highStartTime = 0;

    // Marca l’inici del possible estat groc
    if (mediumStartTime == 0)
      mediumStartTime = millis();

    // Només canvia si el so es manté prou temps
    if (millis() - mediumStartTime >= MEDIUM_CONFIRM_TIME)
      return STATE_MEDIUM;

    return STATE_LOW;
  }

  // ------------------------------------------------
  // Nivell alt (vermell amb confirmació)
  // ------------------------------------------------
  mediumStartTime = 0;

  if (highStartTime == 0)
    highStartTime = millis();

  if (millis() - highStartTime >= HIGH_CONFIRM_TIME)
    return STATE_HIGH;

  return STATE_MEDIUM;
}

/* ==================================================
   9. SORTIDA (LEDS)
===================================================== */

void updateLEDs(SoundState state) {

  // Apaga tots els LEDs abans d’actualitzar
  digitalWrite(LED_GREEN,  LOW);
  digitalWrite(LED_YELLOW, LOW);
  digitalWrite(LED_RED,    LOW);

  // Encén només el LED corresponent a l’estat
  switch (state) {
    case STATE_LOW:
      digitalWrite(LED_GREEN, HIGH);
      break;

    case STATE_MEDIUM:
      digitalWrite(LED_YELLOW, HIGH);
      break;

    case STATE_HIGH:
      digitalWrite(LED_RED, HIGH);
      break;

    default:
      break;
  }
}

/* ==================================================
   10. SORTIDA (SERIAL → Processing)
===================================================== */

void sendStateToSerial(int value, SoundState state) {

  // Envia informació llegible (no valors crus)
  Serial.print("Valor so: ");
  Serial.print(value);
  Serial.print(" -> Estat: ");

  switch (state) {
    case STATE_SILENCE: Serial.println("SILENCI"); break;
    case STATE_LOW:     Serial.println("BAIX (VERD)"); break;
    case STATE_MEDIUM:  Serial.println("MITJÀ (GROC)"); break;
    case STATE_HIGH:    Serial.println("ALT (VERMELL)"); break;
  }
}

/* ==================================================
   11. UTILITATS
===================================================== */

// Configura els pins dels LEDs com a sortida
void setupLEDs() {
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_YELLOW, OUTPUT);
  pinMode(LED_RED, OUTPUT);
}

// Calibratge inicial del soroll base de l’aula
void calibrateNoise() {

  Serial.println("Calibrant soroll base...");

  long sum = 0;              // Suma de lectures
  int samples = 0;           // Nombre de mostres
  unsigned long start = millis();

  // Llegeix el soroll durant uns segons en silenci
  while (millis() - start < CALIBRATION_TIME_MS) {
    sum += readSound();      // Valor ja amplificat
    samples++;
    delay(SAMPLE_DELAY_MS);
  }

  // Calcula la mitjana del soroll base
  baseNoiseLevel = sum / samples;

  // Defineix els llindars a partir del context real
  thresholdMedium = baseNoiseLevel + MARGIN_MEDIUM;
  thresholdHigh   = baseNoiseLevel + MARGIN_HIGH;

  Serial.print("Soroll base: ");
  Serial.println(baseNoiseLevel);
}

// Reinicia temporitzadors de confirmació
void resetTimers() {
  mediumStartTime = 0;
  highStartTime   = 0;
}
