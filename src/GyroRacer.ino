/*
 * Project: GyroRacer
 * Description: Simple arcade like motorcycling racing game controlled by gyroscope sensor. Game is short and more a technical demo
 * Hardward: Arduino Uno/Nano with gyroscope sensor MPU6050 and SSD1306 OLED 128x64 pixel display
 * License: MIT License
 * Copyright (c) 2022 codingABI
 * 
 * created by codingABI https://github.com/codingABI/GyroRacer
 * 
 * History:
 * 21.05.2022, Initial version
 */
  
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h> // dont forget to uncomment #define SSD1306_NO_SPLASH in Adafruit_SSD1306.h to free program storage
#include <I2Cdev.h>

#include <MPU6050_6Axis_MotionApps20.h> // older, but smaller (~1k) binary than <MPU6050_6Axis_MotionApps612.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// SSD1306 I2C 
#define OLED_RESET -1 // no reset pin
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
// MPU I2C
MPU6050 mpu;

#define MPU_INTERRUPT_PIN 2
// MPU control/status vars
bool dmpReady = false;  // set true if DMP init was successful
volatile bool mpuInterrupt = false; // indicates whether MPU interrupt pin has gone high

// Game definitions and variables
#define STREET_WIDTH 90 // street width
#define STREETBORDER_WIDTH 10 // width of street border
#define STREET_MINWIDTH 10 // minimum street width on horizont
#define MAXLAPS 3 // game finishes after MAXLAPS

unsigned long g_startMS; // start of game
unsigned int g_distance; // position on track
int g_playerPos; // horizontal position of player
byte g_speed; // speed
byte g_laps; // number of finished laps
signed char g_curve; // current curve
byte g_sprite; // current player sprite
int g_drift; // drift in curves

#define SPRITEWIDTH 16
#define SPRITEHEIGHT 16
#define SPRITEPIXELWHITE 1 
#define SPRITEPIXELBLACK 2

const PROGMEM byte g_playerSprites[3][SPRITEHEIGHT][SPRITEWIDTH] = {
  { // Normal
    { 0,0,0,0,0,2,1,1,1,1,2,0,0,0,0,0 },
    { 0,0,0,0,0,2,1,2,2,1,2,0,0,0,0,0 },
    { 0,0,0,0,2,1,1,1,1,1,1,2,0,0,0,0 },
    { 0,0,0,2,1,2,1,1,1,1,1,1,2,0,0,0 },
    { 0,0,2,1,2,1,2,1,2,1,2,1,1,2,0,0 },
    { 0,0,2,1,1,2,1,2,1,2,1,2,1,2,0,0 },
    { 0,2,1,1,2,1,1,1,1,1,2,1,1,1,2,0 },
    { 0,2,1,1,1,1,2,2,2,2,1,2,1,1,2,0 },
    { 0,0,2,1,1,1,1,1,1,1,1,1,1,2,0,0 },
    { 0,0,2,1,1,2,2,2,2,2,2,1,1,2,0,0 },
    { 0,0,2,1,1,2,2,2,2,2,2,1,1,2,0,0 },
    { 0,0,2,1,1,2,2,2,2,2,2,1,1,2,0,0 },
    { 0,0,0,2,1,2,2,2,2,2,2,1,2,0,0,0 },
    { 0,0,0,0,2,1,2,2,2,2,1,2,0,0,0,0 },
    { 0,0,0,0,2,1,2,2,2,2,1,2,0,0,0,0 },
    { 0,0,0,0,0,2,1,1,1,1,2,0,0,0,0,0 },
  },
  { // slightly tilt to right
    { 0,0,0,0,0,0,0,0,0,2,2,2,0,0,0,0 },
    { 0,0,0,0,0,0,0,0,2,1,1,1,2,0,0,0 },
    { 0,0,0,0,0,0,0,2,1,1,2,2,1,2,0,0 },
    { 0,0,0,0,2,2,2,1,1,1,1,1,1,2,0,0 },
    { 0,0,0,2,1,1,1,1,2,1,2,1,1,2,0,0 },
    { 0,0,0,2,1,2,1,2,1,2,1,2,1,1,2,0 },
    { 0,0,0,2,1,1,1,1,1,1,2,1,1,1,2,0 },
    { 0,0,0,2,1,1,1,2,2,1,1,1,1,1,2,0 },
    { 0,0,0,2,1,1,1,1,2,2,2,2,1,1,1,2 },
    { 0,0,2,1,1,1,2,2,1,1,1,1,1,1,1,2 },
    { 0,0,2,1,1,2,2,2,2,2,2,1,1,2,2,0 },
    { 0,0,2,1,1,2,2,2,2,2,2,1,1,2,0,0 },
    { 0,0,2,1,2,2,2,2,2,2,1,1,2,0,0,0 },
    { 0,0,2,1,2,2,2,2,2,1,1,1,2,0,0,0 },
    { 0,0,0,2,1,2,2,2,1,1,2,2,0,0,0,0 },
    { 0,0,0,0,2,1,1,1,1,2,0,0,0,0,0,0 },
  },
  { // strong tilt to right
    { 0,0,0,0,0,0,0,0,0,0,0,0,0,2,0,0 },
    { 0,0,0,0,0,0,0,0,2,2,2,0,2,1,2,0 },
    { 0,0,0,0,2,2,2,2,1,1,1,2,1,2,1,2 },
    { 0,0,0,2,1,1,1,1,2,1,2,1,1,2,2,1 },
    { 0,0,0,0,2,1,1,2,1,2,1,1,1,1,1,2 },
    { 0,0,0,2,1,1,1,1,2,1,2,1,1,1,2,0 },
    { 0,0,2,1,1,1,1,1,1,2,1,2,1,2,1,2 },
    { 0,2,1,1,1,2,1,2,2,1,1,1,2,1,1,2 },
    { 0,0,2,1,2,2,2,1,2,2,1,2,1,1,2,0 },
    { 0,2,1,2,2,2,2,2,1,2,1,1,2,1,1,2 },
    { 0,2,1,2,2,2,2,2,2,1,1,2,1,1,2,0 },
    { 0,2,1,2,2,2,2,2,2,2,1,1,1,1,2,0 },
    { 0,2,1,2,2,2,2,2,2,1,1,1,2,1,2,0 },
    { 0,2,1,1,2,2,2,2,1,1,1,1,1,1,2,0 },
    { 0,0,2,1,1,1,1,1,2,1,1,1,1,2,0,0 },
    { 0,0,0,2,2,2,2,2,0,2,2,2,2,0,0,0 },
  }
};

// track definition
typedef struct {
  unsigned int startDistance;
  signed char targetCurve;
} trackSegment;

#define MAXSEGMENTS 8
#define TRACKLENGTH 7000

trackSegment g_trackSegments[MAXSEGMENTS] = {
  {0,0},
  {1200,100},
  {2200,0},
  {3400,30},
  {3600,-30},
  {3800,-100},
  {4800,60},
  {6800,0}, // last tracksegment for finish gate (length 200 is a good value)
};

// Interrupthandler for mpu
void dmpDataReady() {
  mpuInterrupt = true;
}

// precalculated sin to improve performance (degree 0-90 with values [0;128])
byte g_sin128[91] {
  0,2,4,7,9,11,13,16,18,20,22,24,27,29,31,33,35,37,40,42,44,46,48,50,52,54,56,58,60,62,64,66,68,70,72,73,75,77,79,81,82,84,86,87,89,91,92,94,95,97,98,99,101,102,104,105,106,107,109,110,111,112,113,114,115,116,117,118,119,119,120,121,122,122,123,124,124,125,125,126,126,126,127,127,127,128,128,128,128,128,128
};

// function to get precalculated sins (return values [-128;128])
int sin128(int degree) {
  degree = degree % 360;
  if (degree < 0) degree+=360;
  if (degree < 90) return g_sin128[degree];
  if (degree < 180) return g_sin128[180-degree];
  if (degree < 270) return - (int) g_sin128[degree-180];
  return -(int) g_sin128[360-degree];
}

// draw player sprite
void drawPlayer() {
  byte displayBeginX, displayBeginY;
  byte color;

  displayBeginX = g_playerPos - (SPRITEWIDTH>>1);
  displayBeginY = SCREEN_HEIGHT - SPRITEHEIGHT - 4;

  // Draw sprite
  for (int i=0;i<SPRITEHEIGHT;i++) {
    for (int j=0;j<SPRITEWIDTH;j++) {
      if (g_sprite > 2) { // sprite number 3 and 4 are only mirrored sprites 1 and 2
        color = pgm_read_byte_near(&(g_playerSprites[g_sprite-2][i][SPRITEWIDTH-j]));
      } else { 
        color = pgm_read_byte_near(&(g_playerSprites[g_sprite][i][j]));
      }
      if (color == SPRITEPIXELBLACK) { // Black pixel
        display.drawPixel(displayBeginX+j,displayBeginY+i,SSD1306_BLACK);
      }
      if (color == SPRITEPIXELWHITE) { // White pixel
        display.drawPixel(displayBeginX+j,displayBeginY+i,SSD1306_WHITE);
      }
    }
  }
}

// Some stars in the sky
void drawSky() {
  int currentMiddle;

  // center of street for the most far away horizontal line dependent on current curve
  currentMiddle = (SCREEN_WIDTH>>1)-g_curve;

  display.drawPixel((currentMiddle -37)%128, 16,SSD1306_WHITE);
  display.drawPixel((currentMiddle +59)%128, 20,SSD1306_WHITE);
  display.drawPixel((currentMiddle +13)%128, 15,SSD1306_WHITE);
  display.drawPixel((currentMiddle +55)%128, 14,SSD1306_WHITE);
  display.drawPixel((currentMiddle +120)%128, 19,SSD1306_WHITE);
  display.drawPixel((currentMiddle -74)%128, 23,SSD1306_WHITE);
  display.drawPixel((currentMiddle +4)%128, 10,SSD1306_WHITE);
  display.drawPixel((currentMiddle +35)%128, 13,SSD1306_WHITE);

}

// draw street on grass (and finish gate if needed)
void drawScene() {
  int currentStreetWidth;
  int currentStreetBorderWidth;
  int currentMiddle;
  unsigned int currentSegmentPosition;
  unsigned int currentSegmentLenght;
  byte sceneHeight;
  byte currentTrackSegment;
  static unsigned long lastDriftMS = 0;
  int grassLeftBegin;
  int grassLeftWidth;
  int grassRightBegin;
  int grassRightWidth;
  int borderLeftBegin;
  int borderLeftWidth;
  int borderRightBegin;
  int borderRightWidth;

  // half of screen
  sceneHeight = (SCREEN_HEIGHT>>1);

  // increase player distance dependent on speed
  g_distance+=g_speed>>2; // increase by 1/4th of speed
  if (g_distance >= TRACKLENGTH) g_laps++; // count laps when finished
  g_distance = g_distance % TRACKLENGTH; // modulo distance to complete track length

  // get current track segment
  currentTrackSegment = MAXSEGMENTS-1;
  currentSegmentLenght = TRACKLENGTH - g_trackSegments[MAXSEGMENTS-1].startDistance;
  for (int i=0;i<MAXSEGMENTS-1;i++){
    if (g_distance < g_trackSegments[i+1].startDistance) { 
      currentTrackSegment = i; 
      currentSegmentLenght = g_trackSegments[i+1].startDistance - g_trackSegments[i].startDistance;
      break; 
    }
  }

  // Position in current track segment
  currentSegmentPosition = g_distance - g_trackSegments[currentTrackSegment].startDistance;

  // align curve to target curve of segment
  if (g_curve < g_trackSegments[currentTrackSegment].targetCurve) {
    g_curve+=g_speed>>2;
    if (g_curve > g_trackSegments[currentTrackSegment].targetCurve) g_curve = g_trackSegments[currentTrackSegment].targetCurve;
  }
  if (g_curve > g_trackSegments[currentTrackSegment].targetCurve) {
    g_curve-=g_speed>>2;
    if (g_curve < g_trackSegments[currentTrackSegment].targetCurve) g_curve = g_trackSegments[currentTrackSegment].targetCurve;
  }
  for (int y=0;y<sceneHeight;y++) { // for each line in lower screen half
    // center of the road dependent on curve and fake "depth"
    currentMiddle = (SCREEN_WIDTH>>1)-g_curve/(y+1);

    // width of street and border dependent on fake "depth"
    currentStreetWidth = ((STREET_WIDTH*(y+1)) >> 5)+STREET_MINWIDTH; 
    currentStreetBorderWidth = (STREETBORDER_WIDTH*(y+1)) >> 5;

    // Draw grass
    grassLeftBegin = 0;
    grassLeftWidth = currentMiddle-currentStreetBorderWidth-(currentStreetWidth>>1);
    grassRightBegin = currentMiddle+currentStreetBorderWidth+(currentStreetWidth>>1);
    grassRightWidth = SCREEN_WIDTH-grassRightBegin;

    if (sin128(((((31-y)*(31-y)*(31-y))>>5) + g_distance)) > 0) { // fake "depth" oscillation with phase shifting 
      // Solid grass
      if (grassLeftWidth > 0) display.drawFastHLine(grassLeftBegin,y+sceneHeight, grassLeftWidth,SSD1306_WHITE);
      if (grassRightWidth > 0) display.drawFastHLine(grassRightBegin,y+sceneHeight, grassRightWidth,SSD1306_WHITE);
    } else {
      // Grass with simple dithering
      if (grassLeftWidth > 0) for (int k=grassLeftBegin;k<grassLeftBegin+grassLeftWidth;k++) {
        if ((k%2+y)%2) display.drawPixel(k,y+sceneHeight,SSD1306_WHITE);
      }
      if (grassRightWidth > 0) for (int k=grassRightBegin;k<grassRightBegin+grassRightWidth;k++) {
        if ((k%2+y)%2) display.drawPixel(k,y+sceneHeight,SSD1306_WHITE);
      }
    }

    // Draw street border
    borderLeftBegin = currentMiddle-currentStreetBorderWidth-(currentStreetWidth>>1);
    borderLeftWidth = currentStreetBorderWidth;
    borderRightBegin = currentMiddle+(currentStreetWidth>>1);
    borderRightWidth = currentStreetBorderWidth;

    if (sin128(((((31-y)*(31-y)*(31-y))>>5) + g_distance)<<2) > 0) { // fake "depth" oscillation with phase shifting (use 4x faster frequency than grass)
      display.drawFastHLine(borderLeftBegin,y+sceneHeight,borderLeftWidth,SSD1306_WHITE);
      display.drawFastHLine(borderRightBegin,y+sceneHeight, borderRightWidth,SSD1306_WHITE);      
    };

    // Draw finish gate when in last track segment
    if (currentTrackSegment == MAXSEGMENTS-1) {
      // y-position of gate dependent on perspective y = sceneHeight * currentSegmentPosition / (currentSegmentLenght-currentSegmentPosition)
      if ((currentSegmentLenght-currentSegmentPosition>0) && (y == (sceneHeight*currentSegmentPosition/(currentSegmentLenght-currentSegmentPosition)))) {
        // left pole
        display.drawFastVLine(borderLeftBegin,sceneHeight-1,y+1,SSD1306_WHITE);
        display.drawFastVLine(borderLeftBegin+1,sceneHeight-1,y+1,SSD1306_BLACK);
        display.drawFastVLine(borderLeftBegin-1,sceneHeight-1,y+1,SSD1306_BLACK);

        // right pole
        display.drawFastVLine(borderRightBegin+borderRightWidth-1,sceneHeight-1,y+1,SSD1306_WHITE);
        display.drawFastVLine(borderRightBegin+borderRightWidth-2,sceneHeight-1,y+1,SSD1306_BLACK);
        display.drawFastVLine(borderRightBegin+borderRightWidth,sceneHeight-1,y+1,SSD1306_BLACK);

        // top banner
        display.fillRect(borderLeftBegin,sceneHeight-((y+1)>>1)-1,borderRightBegin-borderLeftBegin+borderRightWidth,(y+1)>>1,SSD1306_WHITE);
      }
    }

    // Drift when not in center of the road
    if ((millis()-lastDriftMS > 50) && (y == 20)) { 
      g_drift = (SCREEN_WIDTH>>1) - currentMiddle;
      // Reduce speed in the grass
      if (((g_playerPos < borderLeftBegin) || (g_playerPos > grassRightBegin - 1))) {
        if (g_speed > 8) g_speed-=8; else g_speed = 1;
      }
      lastDriftMS = millis();
    }
  } 
}

// initial game settings
void resetGame() {
  g_playerPos = SCREEN_WIDTH/2;
  g_speed = 0;
  g_laps = 0;
  g_startMS = millis();
  g_distance = 0;
}

// blink internal led
void blink(int time) {
  digitalWrite(LED_BUILTIN,HIGH);
  delay(time);
  digitalWrite(LED_BUILTIN,LOW);
  delay(time);
}

void setup(void) {
  uint8_t mpuIntStatus;   // holds actual interrupt status byte from MPU
  uint16_t packetSize;    // expected DMP packet size (default is 42 bytes)
  uint8_t devStatus;      // return status after each device operation (0 = success, !0 = error)
  
  pinMode(LED_BUILTIN,OUTPUT);
  
  //Start I2C
  Wire.begin();

  // MPU6050 init
  mpu.initialize();

  pinMode(MPU_INTERRUPT_PIN, INPUT);

  // verify connection
  if (!mpu.testConnection()) {
    // MPU6050 connection failed
    blink(1000);
    while (true);
  }

  // load and configure the DMP
  devStatus = mpu.dmpInitialize();
  
   // OLED init
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Default Address is 0x3D for 128x64, but my OLED uses 0x3C 
    // SSD1306 allocation failed
    blink(1000);
    blink(1000);
    blink(1000);
    while (true);
  }

  // Font settings
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(2);
  display.setCursor(0,0);
  display.println(F("3 fast"));
  display.print(F("laps..."));
  display.display();
  display.setTextSize(1);

  // Calibration based on IMU_ZERO
  mpu.setXGyroOffset(1907);
  mpu.setYGyroOffset(130);
  mpu.setZGyroOffset(-1);
  mpu.setXAccelOffset(-647);
  mpu.setYAccelOffset(-3985);  
  mpu.setZAccelOffset(-4111);

  // make sure it worked (returns 0 if so)
  if (devStatus == 0) {
    // Calibration Time: generate offsets and calibrate our MPU6050
    mpu.CalibrateAccel(6);
    mpu.CalibrateGyro(6);
    mpu.PrintActiveOffsets();
    // turn on the DMP, now that it's ready
    mpu.setDMPEnabled(true);

    // enable Arduino interrupt detection
    attachInterrupt(digitalPinToInterrupt(MPU_INTERRUPT_PIN), dmpDataReady, RISING);
    mpuIntStatus = mpu.getIntStatus();

    // set our DMP Ready flag so the main loop() function knows it's okay to use it
    dmpReady = true;

    // get expected DMP packet size for later comparison
    packetSize = mpu.dmpGetFIFOPacketSize();
  } else {
    // ERROR!
    // 1 = initial memory load failed
    // 2 = DMP configuration updates failed
    // (if it's going to break, usually the code will be 1)

    // DMP Initialization failed
    blink(1000);
    blink(1000);
    if (devStatus==1) blink(500);
    if (devStatus==2) { blink(500);blink(500); }
    while (true);
  }
  
  resetGame();
}

void loop(void) {
  char strData[24];
  unsigned long startMS, endMS;
  static unsigned long lastPlayerMS = 0; 
  static unsigned int fps = 0;
  static int roll = 0;
  static int pitch = 0;
  Quaternion q;           // [w, x, y, z]         quaternion container
  VectorFloat gravity;    // [x, y, z]            gravity vector
  float ypr[3];           // [yaw, pitch, roll]   yaw/pitch/roll container and gravity vector
  uint8_t fifoBuffer[64]; // FIFO storage buffer
  uint8_t rc;
  
  // record start of frame
  startMS = millis();

  // get pitch, roll and yaw from MPU6050
  if (dmpReady) {
    // read a packet from FIFO
    rc = mpu.dmpGetCurrentFIFOPacket(fifoBuffer); 

    if (rc) { // Get the Latest packet 
      mpu.dmpGetQuaternion(&q, fifoBuffer);
      mpu.dmpGetGravity(&gravity, &q);
      mpu.dmpGetYawPitchRoll(ypr, &q, &gravity);
      pitch = -ypr[1] * 180/M_PI;
      roll = -ypr[2] * 180/M_PI;
    }
  }
  
  // control player every 100ms
  if (millis()-lastPlayerMS>100) {
    // increase/decrease speed
    if (pitch > 10) { // decrease by pitch
      if (g_speed > 1) g_speed--;
    } else { // increase automatically
      if (g_speed > 140) g_speed+=1; else g_speed+=4;
      if (g_speed > 160) g_speed = 160;
    }

    // Control left/right by roll
    g_sprite  = 0;
    g_playerPos += 20*roll/90 + g_drift;
    if (g_playerPos < SPRITEWIDTH/2) g_playerPos = SPRITEWIDTH/2;
    if (g_playerPos > SCREEN_WIDTH-SPRITEWIDTH/2-1) g_playerPos = SCREEN_WIDTH-SPRITEWIDTH/2-1;
    
    if (roll < -20) { // tilt to left
      g_sprite=4;  
      if (g_speed> 100) g_speed-=10; else if (g_speed > 2) g_speed-=2; else g_speed = 1; // reduce speed when tilt
    } else if (roll <- 5) {
      g_sprite=3;
      if (g_speed> 100) g_speed-=5; else if (g_speed > 1) g_speed--; // reduce speed when tilt
    } 

    if (roll > 20) { // tilt to right
      g_sprite=2;
      if (g_speed> 100) g_speed-=10; else if (g_speed > 2) g_speed-=2; else g_speed = 1; // reduce speed when tilt
    } else if (roll > 5) {
      g_sprite=1;
      if (g_speed> 100) g_speed-=5; else if (g_speed > 1) g_speed--; // reduce speed when tilt
    } 
    lastPlayerMS = millis();
  }

  // clear display buffer
  display.clearDisplay();

  // End of game
  if (g_laps >= MAXLAPS) {
    display.setTextSize(2);
    display.setCursor(50,20);
    display.print(F("Fin"));
    display.setTextSize(1);
    display.setCursor(0,56);
    display.print(F("in "));
    display.print((millis()-g_startMS)/1000);
    display.print(F(" seconds"));
    display.display();
    delay(10000);
    // start again
    display.clearDisplay();
    resetGame();
  } 

  // draw sky
  drawSky();

  // draw rendered scene
  drawScene();

  // draw player sprite
  drawPlayer();

  // draw statistics data
  snprintf(strData,24,"Lap%2d       %3d km/h",g_laps+1,g_speed);
  display.setCursor(0,0);
  display.print(strData);

  // show display buffer on screen
  display.display();

  // calculate frames per second
  endMS = millis();
  if (endMS - startMS > 0) fps = 1000/(endMS - startMS);
}
