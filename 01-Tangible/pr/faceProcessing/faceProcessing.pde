// Importa la llibreria necessària per comunicar Processing amb el port sèrie
import processing.serial.*;

/* ==================================================
 1. CONFIGURACIÓ GENERAL
 ===================================================== */

// Declara l'objecte Serial que gestionarà la comunicació amb Arduino
Serial myPort;

// Variable on es guardarà cada línia rebuda pel port sèrie
String serialLine = "";

// Constant que controla la velocitat de transició suau entre estats visuals
// Valors petits = transicions més lentes i suaus
final float LERP_SPEED = 0.05;

/* ==================================================
 2. ESTATS DEL SISTEMA
 ===================================================== */

// Defineix els possibles estats lògics rebuts des d'Arduino
enum State {
  SILENCE,   // Aula en silenci
  GREEN,     // Soroll baix
  YELLOW,    // Soroll mitjà
  RED        // Soroll alt
}

// Estat visual actual que s'està mostrant a pantalla
State currentState = State.SILENCE;

// Estat objectiu cap al qual es vol fer la transició
State targetState  = State.SILENCE;

/* ==================================================
 3. VARIABLES VISUALS
 ===================================================== */

// Curvatura actual de la boca (expressió facial)
float mouthCurvature = 0;

// Valor objectiu de la curvatura de la boca segons l’estat
float targetMouth    = 0;

// Mida actual dels ulls
float eyeSize        = 50;

// Mida objectiu dels ulls segons l’estat
float targetEyeSize  = 50;

// Color del fons de l’escena
color bgColor;

// Color utilitzat per dibuixar la cara (ulls i boca)
color faceColor;

/* ==================================================
 4. SETUP
 ===================================================== */

// Funció que s’executa una sola vegada a l’inici del programa
void setup() {

  // Defineix la mida de la finestra gràfica
  size(1280, 1024);

  // Activa suavitzat gràfic per millorar la qualitat visual
  smooth();

  // Inicialitza els colors per defecte (estat de silenci)
  initColors();

  // Inicialitza la comunicació sèrie amb Arduino
  initSerial();
}

/* ==================================================
 5. DRAW PRINCIPAL (MOLT LLEGIBLE)
 ===================================================== */

// Funció que s’executa contínuament (loop gràfic)
void draw() {

  // Actualitza progressivament les expressions facials
  updateExpression();

  // Dibuixa l’escena completa a pantalla
  renderScene();
}

/* ==================================================
 6. SERIAL
 ===================================================== */

// Inicialitza la connexió amb el port sèrie
void initSerial() {

  // Mostra per consola la llista de ports disponibles
  println(Serial.list());

  // Obre el primer port de la llista a 9600 bauds
  myPort = new Serial(this, Serial.list()[0], 9600);

  // Indica que les dades s’han de llegir línia a línia fins al salt de línia
  myPort.bufferUntil('\n');
}

// Funció que s’executa automàticament quan arriba una línia pel port sèrie
void serialEvent(Serial port) {

  // Llegeix una línia completa fins al salt de línia
  serialLine = port.readStringUntil('\n');

  // Comprova que la línia no sigui nul·la
  if (serialLine != null) {

    // Elimina espais i salts de línia sobrants
    serialLine = serialLine.trim();

    // Analitza el contingut rebut per detectar l’estat
    parseState(serialLine);
  }
}

/* ==================================================
 7. PROCESSAMENT D’ESTAT
 ===================================================== */

// Analitza el text rebut des d’Arduino
void parseState(String line) {

  // Si el text conté la paraula "SILENCI", es passa a estat SILENCE
  if (line.contains("SILENCI")) {
    setTargetState(State.SILENCE);

  // Si conté "VERD", estat GREEN
  } else if (line.contains("VERD")) {
    setTargetState(State.GREEN);

  // Si conté "GROC", estat YELLOW
  } else if (line.contains("GROC")) {
    setTargetState(State.YELLOW);

  // Si conté "VERMELL", estat RED
  } else if (line.contains("VERMELL")) {
    setTargetState(State.RED);
  }
}

// Actualitza l’estat objectiu si és diferent de l’actual
void setTargetState(State newState) {

  // Evita recalcular si l’estat ja és el mateix
  if (newState == targetState) return;

  // Assigna el nou estat objectiu
  targetState = newState;

  // Aplica els paràmetres visuals corresponents a l’estat
  applyVisualPreset(targetState);
}

/* ==================================================
 8. PRESETS VISUALS PER ESTAT
===================================================== */

// Defineix els valors visuals associats a cada estat
void applyVisualPreset(State state) {

  // Selecciona el preset segons l’estat
  switch (state) {

  case SILENCE:
    // Fons blanc per indicar calma
    bgColor   = color(255, 255, 255);

    // Cara en negre
    faceColor = color(0, 0, 0);

    // Boca lleugerament positiva
    targetMouth   = 0.4;

    // Ulls lleugerament oberts
    targetEyeSize = 52;
    break;

  case GREEN:
    // Fons verd intens
    bgColor = color(0, 255, 0);

    // Cara clara
    faceColor = color(230);

    // Somriure més marcat
    targetMouth   = 0.8;

    // Ulls una mica més grans
    targetEyeSize = 54;
    break;

  case YELLOW:
    // Fons groc d’alerta
    bgColor = color(255, 230, 0);

    // Cara clara
    faceColor = color(230);

    // Boca neutra
    targetMouth   = 0.0;

    // Ulls una mica més petits
    targetEyeSize = 46;
    break;

  case RED:
    // Fons vermell d’alerta clara
    bgColor = color(255, 0, 0);

    // Cara clara
    faceColor = color(230);

    // Boca negativa (tristesa / advertència)
    targetMouth   = -0.8;

    // Ulls més petits (tensió)
    targetEyeSize = 38;
    break;
  }
}

/* ==================================================
 9. ACTUALITZACIÓ SUAU
===================================================== */

// Actualitza progressivament les expressions utilitzant interpolació lineal
void updateExpression() {

  // Suavitza la transició de la boca cap al valor objectiu
  mouthCurvature = lerp(mouthCurvature, targetMouth, LERP_SPEED);

  // Suavitza la transició de la mida dels ulls
  eyeSize        = lerp(eyeSize, targetEyeSize, LERP_SPEED);
}

/* ==================================================
 10. RENDERITZAT
===================================================== */

// Dibuixa l’escena completa
void renderScene() {

  // Pinta el fons amb el color actual
  background(bgColor);

  // Dibuixa la cara
  drawFace();
}

// Dibuixa la cara centrada a la pantalla
void drawFace() {

  // Calcula el centre horitzontal
  float cx = width / 2;

  // Calcula el centre vertical
  float cy = height / 2;

  // Dibuixa els ulls
  drawEyes(cx, cy);

  // Dibuixa la boca
  drawMouth(cx, cy);
}

// Dibuixa els ulls segons la mida actual
void drawEyes(float cx, float cy) {

  // Desactiva el contorn
  noStroke();

  // Aplica el color de la cara
  fill(faceColor);

  // Ull esquerre
  ellipse(cx - 80, cy - 50, eyeSize, eyeSize);

  // Ull dret
  ellipse(cx + 80, cy - 50, eyeSize, eyeSize);
}

// Dibuixa la boca amb forma d’arc segons la curvatura
void drawMouth(float cx, float cy) {

  // No omple l’arc
  noFill();

  // Defineix el color del contorn
  stroke(faceColor);

  // Gruix del traç
  strokeWeight(6);

  // Amplada de la boca
  float mouthWidth  = 300;

  // Alçada proporcional a la curvatura
  float mouthHeight = 60 * mouthCurvature;

  // Dibuixa l’arc de la boca (somriure o tristesa)
  arc(
    cx,
    cy + 100,
    mouthWidth,
    abs(mouthHeight) + 50,
    mouthCurvature >= 0 ? 0 : PI,
    mouthCurvature >= 0 ? PI : TWO_PI
  );
}

/* ==================================================
 11. INICIALITZACIÓ
===================================================== */

// Inicialitza els colors per defecte
void initColors() {

  // Color de la cara (negre)
  faceColor = color(0, 0, 0);

  // Fons blanc corresponent a l’estat de silenci
  bgColor   = color(255, 255, 255);
}
