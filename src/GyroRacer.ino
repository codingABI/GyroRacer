/*
 * Project: GyroRacer
 * Description: Simple arcade like motorcycling racing game controlled by gyroscope sensor. Game is short and more a technical demo
 * Hardward: Arduino Uno/Nano with gyroscope sensor MPU6050, SSD1306 OLED 128x64 pixel display and an optional passive buzzer
 * License: MIT License
 * Copyright (c) 2023 codingABI
 * 
 * created by codingABI https://github.com/codingABI/GyroRacer
 * 
 * History:
 * 21.05.2022, Initial version
 * 22.05.2022, Improve drift in curves, decrease automatic acceleration and increase minimum speed in grass
 * 24.05.2022, More sprites for player, change tracklist format from start to segment length
 * 25.05.2022, Change tracklist to define type of a segment (curve, finish gate...). Add curve warnings/triangles. Move g_sin128 to PROGMEM to reduce memory consumption
 * 26.05.2022, Add a demo mode to play the game automatically without a gyroscope sensor
 * 27.05.2022, Release version v0.1.0
 * 28.05.2022, Add delay for setup in demo mode to show intro text
 * 21.03.2023, Make sky movements more realistic
 * 21.03.2023, Release version v0.2.0
 * 22.03.2023, Make sky movements dependent on speed
 */
 
//#define DEMOMODE // uncomment this line, if you want only the demo mode without a gyroscope sensor

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h> // dont forget to uncomment #define SSD1306_NO_SPLASH in Adafruit_SSD1306.h to free program storage

#ifndef DEMOMODE

// we need I2Cdev from https://github.com/jrowberg/i2cdevlib
#include <I2Cdev.h>
#include <MPU6050_6Axis_MotionApps20.h> // older, but smaller (~1k) binary than <MPU6050_6Axis_MotionApps612.h>

#endif 

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

#define BUZZERPIN 5 // Pin of buzzer
#define USE_BUZZER // comment out this line, if you want no sound

// SSD1306 I2C 
#define OLED_RESET -1 // no reset pin
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#ifndef DEMOMODE

// MPU I2C
MPU6050 mpu;
#define MPU_INTERRUPT_PIN 2
// MPU control/status vars
bool dmpReady = false;  // set true if DMP init was successful
volatile bool mpuInterrupt = false; // indicates whether MPU interrupt pin has gone high

// Interrupthandler for mpu
void dmpDataReady() {
  mpuInterrupt = true;
}

#endif

// Game definitions and variables
#define STREET_WIDTH 90 // street width at scene bottom, in pixels
#define STREETBORDER_WIDTH 10 // width of street border at scene bottom, in pixels
#define STREET_MINWIDTH 10 // minimum street width on horizont, in pixels
#define MAXLAPS 3 // game finishes after MAXLAPS
#define MAXSPEED 160 // global max speed
#define GRASSMINSPEED 4 // Min speed in grass
#define WARNINGWIDTH 16 // width of direction arrow at scene bottom, in pixels
#define WARNINGHEIGHT 16 // height of direction arrow at scene bottom, in pixels

unsigned long g_startMS; // start of game
unsigned int g_distance; // position on track
int g_playerPos; // horizontal position of player
byte g_speed; // current speed
byte g_laps; // number of finished laps
signed char g_curve; // current curve
byte g_sprite; // current player sprite
signed char g_streetMiddle; // center of street

#define SPRITEWIDTH 16 // width of sprite, in pixels
#define SPRITEHEIGHT 16 // height of sprite, in pixels

// sprites made with gimp and converted bye https://javl.github.io/image2cpp/
#define INDIVIDUALSPRITES 7
// white pixels for motorcycle sprites
const PROGMEM byte g_whiteSprites [INDIVIDUALSPRITES][SPRITEWIDTH/8*SPRITEHEIGHT] = {
{ 0x03, 0xc0, 0x02, 0x40, 0x07, 0xe0, 0x0d, 0x70, 0x1a, 0xb8, 0x15, 0x58, 0x3f, 0xec, 0x34, 0x3c, 
  0x1f, 0xf8, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x08, 0x10, 0x04, 0x20, 0x04, 0x20, 0x03, 0xc0, },
{ 0x00, 0xe0, 0x03, 0x20, 0x0f, 0xe0, 0x1a, 0xb0, 0x15, 0x58, 0x3f, 0xa8, 0x35, 0xf8, 0x3e, 0x38, 
  0x31, 0xf8, 0x30, 0x30, 0x30, 0x30, 0x10, 0x30, 0x10, 0x20, 0x10, 0x40, 0x0c, 0x80, 0x03, 0x00, },
{ 0x00, 0x00, 0x00, 0x78, 0x03, 0xc8, 0x06, 0xf8, 0x0d, 0x58, 0x1a, 0xac, 0x1f, 0xd4, 0x1a, 0x6c, 
  0x1f, 0x3c, 0x18, 0xfc, 0x10, 0x18, 0x10, 0x38, 0x10, 0x30, 0x10, 0x30, 0x10, 0xc0, 0x0f, 0x80, },
{ 0x00, 0x00, 0x00, 0x30, 0x00, 0x6c, 0x03, 0xf4, 0x0f, 0x5c, 0x0e, 0xac, 0x0f, 0xd4, 0x1e, 0x6c, 
  0x1b, 0x34, 0x10, 0xec, 0x10, 0x3c, 0x10, 0x38, 0x10, 0x30, 0x10, 0x60, 0x19, 0xc0, 0x07, 0x00, },
{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x3c, 0x07, 0xf4, 0x0d, 0x5c, 0x0e, 0xac, 0x0f, 0xd4, 0x1a, 0x6c, 
  0x39, 0xb4, 0x30, 0xec, 0x20, 0x7c, 0x20, 0x3c, 0x20, 0x70, 0x30, 0xe0, 0x1b, 0xc0, 0x0e, 0x00, },
{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0xfe, 0x07, 0x5a, 0x0e, 0xae, 0x1f, 0xf6, 
  0x3b, 0x6c, 0x31, 0x34, 0x20, 0xac, 0x60, 0x74, 0x40, 0x3c, 0x60, 0xf8, 0x31, 0xc0, 0x1f, 0x80, },
{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0xfe, 0x07, 0xaa, 0x0d, 0x5d, 
  0x1f, 0xef, 0x31, 0x76, 0x21, 0x2c, 0x60, 0xb4, 0x40, 0xec, 0x40, 0x78, 0x20, 0xf8, 0x3f, 0xc0 },
};

// black pixels for motorcycle sprites
const PROGMEM byte g_blackSprites [INDIVIDUALSPRITES][SPRITEWIDTH/8*SPRITEHEIGHT] = {
{ 0x07, 0xe0, 0x07, 0xe0, 0x0f, 0xf0, 0x1f, 0xf8, 0x3f, 0xfc, 0x3f, 0xfc, 0x7f, 0xfe, 0x7f, 0xfe, 
  0x3f, 0xfc, 0x3f, 0xfc, 0x3f, 0xfc, 0x3f, 0xfc, 0x1f, 0xf8, 0x0f, 0xf0, 0x0f, 0xf0, 0x07, 0xe0, }, 
{ 0x03, 0xf0, 0x0f, 0xf0, 0x1f, 0xf0, 0x3f, 0xf8, 0x3f, 0xfc, 0x7f, 0xfc, 0x7f, 0xfc, 0x7f, 0xfc, 
  0x7f, 0xfc, 0x7f, 0xf8, 0x7f, 0xf8, 0x3f, 0xf8, 0x3f, 0xf0, 0x3f, 0xe0, 0x1f, 0xc0, 0x0f, 0x80, },
{ 0x00, 0x78, 0x03, 0xfc, 0x07, 0xfc, 0x0f, 0xfc, 0x1f, 0xfc, 0x3f, 0xfe, 0x3f, 0xfe, 0x3f, 0xfe, 
  0x3f, 0xfe, 0x3f, 0xfe, 0x3f, 0xfc, 0x3f, 0xfc, 0x3f, 0xf8, 0x3f, 0xf8, 0x3f, 0xf0, 0x1f, 0xc0, },
{ 0x00, 0x30, 0x00, 0x7c, 0x03, 0xfe, 0x0f, 0xfe, 0x1f, 0xfe, 0x1f, 0xfe, 0x1f, 0xfe, 0x3f, 0xfe, 
  0x3f, 0xfe, 0x3f, 0xfe, 0x3f, 0xfe, 0x3f, 0xfc, 0x3f, 0xf8, 0x3f, 0xf0, 0x3f, 0xe0, 0x1f, 0xc0, },
{ 0x00, 0x00, 0x00, 0x3c, 0x07, 0xfe, 0x0f, 0xfe, 0x1f, 0xfe, 0x1f, 0xfe, 0x1f, 0xfe, 0x3f, 0xfe, 
  0x7f, 0xfe, 0x7f, 0xfe, 0x7f, 0xfe, 0x7f, 0xfe, 0x7f, 0xfc, 0x7f, 0xf0, 0x3f, 0xe0, 0x1f, 0xc0, },
{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0xfe, 0x0f, 0xff, 0x0f, 0xff, 0x1f, 0xff, 0x3f, 0xff, 
  0x7f, 0xfe, 0x7f, 0xfe, 0x7f, 0xfe, 0xff, 0xfe, 0xff, 0xfe, 0xff, 0xfc, 0x7f, 0xf8, 0x3f, 0xc0, },
{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0xfe, 0x07, 0xff, 0x0f, 0xff, 0x1f, 0xff, 
  0x3f, 0xff, 0x7f, 0xff, 0x7f, 0xfe, 0xff, 0xfe, 0xff, 0xfe, 0xff, 0xfc, 0x7f, 0xfc, 0x7f, 0xf8}
};

const PROGMEM signed char g_spriteOffsetX[INDIVIDUALSPRITES]={0,2,2,3,4,4,5}; // x-offset for sprites

// track definition
typedef struct {
  byte segmentLength100; // Length of segment. Do reduce memory consumtion segmentLength100 is byte and equal real segment lenght / 100
  byte segmentType; // Bit pattern for segment type
} trackSegment;

#define TRACKLENGTH 8900 // Sum of all track segments in g_trackSegments (real segmentLenght=segmentLength100*100)
#define SEGMENTTYPE_DEFAULT 0
#define SEGMENTTYPE_LEFTCURVE 1
#define SEGMENTTYPE_RIGHTCURVE 2
#define SEGMENTTYPE_LEFTWARNING 32
#define SEGMENTTYPE_RIGHTWARNING 64
#define SEGMENTTYPE_FINISHGATE 128

#define MAXSEGMENTS 12
trackSegment g_trackSegments[MAXSEGMENTS] = {
  {5,SEGMENTTYPE_DEFAULT}, // straight on
  {7,SEGMENTTYPE_RIGHTWARNING}, // right warning
  {10,SEGMENTTYPE_RIGHTCURVE}, // right
  {12,SEGMENTTYPE_DEFAULT}, // straight on
  {2,SEGMENTTYPE_LEFTCURVE}, // short curve change
  {2,SEGMENTTYPE_RIGHTCURVE}, // short curve change
  {5,SEGMENTTYPE_DEFAULT}, // straight on
  {7,SEGMENTTYPE_LEFTWARNING}, // left warning
  {10,SEGMENTTYPE_LEFTCURVE}, // left
  {7,SEGMENTTYPE_RIGHTWARNING}, // right warning  
  {20,SEGMENTTYPE_RIGHTCURVE}, // long right curve
  {2,SEGMENTTYPE_FINISHGATE}, // last tracksegment for finish gate
};

// precalculated sin to improve performance (degree 0-90 with values [0;128])
const PROGMEM byte g_sin128[91] {
  0,2,4,7,9,11,13,16,18,20,22,24,27,29,31,33,35,37,40,42,44,46,48,50,52,54,56,58,60,62,64,66,68,70,72,73,75,77,79,81,82,84,86,87,89,91,92,94,95,97,98,99,101,102,104,105,106,107,109,110,111,112,113,114,115,116,117,118,119,119,120,121,122,122,123,124,124,125,125,126,126,126,127,127,127,128,128,128,128,128,128
};

// function to get precalculated sins (return values [-128;128])
int sin128(int degree) {
  degree = degree % 360;
  if (degree < 0) degree+=360;
  if (degree < 90) return pgm_read_byte_near(&g_sin128[degree]);
  if (degree < 180) return pgm_read_byte_near(&g_sin128[180-degree]);
  if (degree < 270) return - (int) pgm_read_byte_near(&g_sin128[degree-180]);
  return -(int) pgm_read_byte_near(&g_sin128[360-degree]);
}

// draw player sprite
void drawPlayer() {
  int  displayBeginX, displayBeginY;
  byte valueWhite, valueBlack;
  byte internalSpriteNbr;

  displayBeginX = g_playerPos - (SPRITEWIDTH>>1);
  displayBeginY = SCREEN_HEIGHT - SPRITEHEIGHT - 4;
  
  internalSpriteNbr = g_sprite;
  if (g_sprite > INDIVIDUALSPRITES-1) {
    internalSpriteNbr-=(INDIVIDUALSPRITES-1); // sprite number 7-12 and higher are only mirrored 1-6
    displayBeginX-=pgm_read_byte_near(&(g_spriteOffsetX[internalSpriteNbr]));
  } else {
    displayBeginX+=pgm_read_byte_near(&(g_spriteOffsetX[internalSpriteNbr]));
  }

  // Draw sprite pixels
  for (int i=0;i<SPRITEHEIGHT;i++) { // every sprite line
    for (int j=0;j<2;j++) { // every 8 pixel per line (<SPRITEWIDTH/8)
      valueWhite = pgm_read_byte_near(&(g_whiteSprites[internalSpriteNbr][(i<<1) + j])); // ..][i * (SPRITEWIDTH/8) + j]
      valueBlack = pgm_read_byte_near(&(g_blackSprites[internalSpriteNbr][(i<<1) + j]));
      for (int k=0;k<8;k++) { // check bits from msb to lsb
        if (valueWhite & (0b10000000>>k)) {
          if (g_sprite > INDIVIDUALSPRITES-1) // mirror sprite
            display.drawPixel(displayBeginX+SPRITEWIDTH-((j<<3)+k)-1,displayBeginY+i,SSD1306_WHITE);
          else 
            display.drawPixel(displayBeginX+(j<<3)+k,displayBeginY+i,SSD1306_WHITE);
        } else if (valueBlack & (0b10000000>>k)) {
          if (g_sprite > INDIVIDUALSPRITES-1) // mirror sprite
            display.drawPixel(displayBeginX+SPRITEWIDTH-((j<<3)+k)-1,displayBeginY+i,SSD1306_BLACK);
          else 
            display.drawPixel(displayBeginX+(j<<3)+k,displayBeginY+i,SSD1306_BLACK);
        }
      }
    }
  }
}

// Some stars in the sky
void drawSky() {
  static byte offset = 0;
  byte screenOffset;

  // Move skycenter dependent on curve
  if (g_curve > 0) offset -=(g_speed>>5);
  if (g_curve < 0) offset +=(g_speed>>5);

  screenOffset = (offset>>1);
  display.drawPixel((screenOffset -37)&127, 16,SSD1306_WHITE);
  display.drawPixel((screenOffset +59)&127, 20,SSD1306_WHITE);
  display.drawPixel((screenOffset +13)&127, 15,SSD1306_WHITE);
  display.drawPixel((screenOffset +55)&127, 14,SSD1306_WHITE);
  display.drawPixel((screenOffset +120)&127, 19,SSD1306_WHITE);
  display.drawPixel((screenOffset -74)&127, 23,SSD1306_WHITE);
  display.drawPixel((screenOffset +4)&127, 10,SSD1306_WHITE);
  display.drawPixel((screenOffset +35)&127, 13,SSD1306_WHITE);
}

// draw street on grass (and finish gate and curve warnings if needed)
void drawScene() {
  byte currentStreetWidth;
  byte currentStreetBorderWidth;
  byte currentMiddle;
  unsigned int currentSegmentPosition;
  unsigned int currentSegmentLenght;
  unsigned int segmentSum;
  byte sceneHeight;
  byte currentTrackSegment;
  byte currentSegmentType;
  static unsigned long lastSlowdownMS = 0;
  signed char targetCurve;
  byte grassLeftBegin;
  byte grassLeftWidth;
  byte grassRightBegin;
  byte grassRightWidth;
  byte borderLeftBegin;
  byte borderLeftWidth;
  byte borderRightBegin;
  byte borderRightWidth;
  byte currentWidth;
  byte currentHeight;
  
  // half of screen
  sceneHeight = (SCREEN_HEIGHT>>1);

  // increase player distance dependent on speed
  g_distance+=g_speed>>2; // increase by 1/4th of speed
  if (g_distance >= TRACKLENGTH) g_laps++; // count laps when finished
  g_distance = g_distance % TRACKLENGTH; // modulo distance to complete track length

  // get current track segment
  currentTrackSegment = 0;
  currentSegmentLenght = 0;
  currentSegmentType = SEGMENTTYPE_DEFAULT;
  targetCurve = 0;
  segmentSum = 0;
  for (int i=0;i<MAXSEGMENTS;i++){
    if (g_distance < segmentSum+g_trackSegments[i].segmentLength100*100) { 
      currentTrackSegment = i; 
      currentSegmentLenght = g_trackSegments[i].segmentLength100*100;
      currentSegmentType = g_trackSegments[i].segmentType;

      if ((g_trackSegments[i].segmentType & SEGMENTTYPE_RIGHTCURVE) == SEGMENTTYPE_RIGHTCURVE) targetCurve = 100;
      if ((g_trackSegments[i].segmentType & SEGMENTTYPE_LEFTCURVE) == SEGMENTTYPE_LEFTCURVE) targetCurve = -100;
      // Position in current track segment
      currentSegmentPosition = g_distance-segmentSum;
      break;
    }
    segmentSum+=g_trackSegments[i].segmentLength100*100;    
  }

  // align curve to target curve of segment
  if (g_curve < targetCurve) {
    g_curve+=g_speed>>2;
    if (g_curve > targetCurve) g_curve = targetCurve;
  }
  if (g_curve > targetCurve) {
    g_curve-=g_speed>>2;
    if (g_curve < targetCurve) g_curve = targetCurve;
  }
  for (int y=0;y<sceneHeight;y++) { // for each line in lower screen half
    // center of the road dependent on curve and fake "depth"
    currentMiddle = (SCREEN_WIDTH>>1)+g_curve/(y+1);

    // width of street and border dependent on fake "depth"
    currentStreetWidth = ((STREET_WIDTH*(y+1)) >> 5)+STREET_MINWIDTH; 
    currentStreetBorderWidth = (STREETBORDER_WIDTH*(y+1)) >> 5;

    // Draw grass
    grassLeftBegin = 0;
    grassLeftWidth = currentMiddle-currentStreetBorderWidth-(currentStreetWidth>>1);
    grassRightBegin = currentMiddle+currentStreetBorderWidth+(currentStreetWidth>>1);
    grassRightWidth = SCREEN_WIDTH-grassRightBegin;

    // fake "depth" oscillation with phase shifting (based on sin(frequenceScaler * (1.0f - (y/sceneHeight))^3 + distance*phaseshiftScaler) )
    if (sin128(((((31-y)*(31-y)*(31-y))>>5) + g_distance)) > 0) { 
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

    // fake "depth" oscillation with phase shifting (use 4x faster frequency than grass)
    if (sin128(((((31-y)*(31-y)*(31-y))>>5) + g_distance)<<2) > 0) { 
      display.drawFastHLine(borderLeftBegin,y+sceneHeight,borderLeftWidth,SSD1306_WHITE);
      display.drawFastHLine(borderRightBegin,y+sceneHeight, borderRightWidth,SSD1306_WHITE);      
    };

    // finish gate
    if ((currentSegmentType & SEGMENTTYPE_FINISHGATE) == SEGMENTTYPE_FINISHGATE) {
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

    // right curve warning
    if ((currentSegmentType & SEGMENTTYPE_RIGHTWARNING) == SEGMENTTYPE_RIGHTWARNING) {
      // y-position dependent on perspective y = sceneHeight * currentSegmentPosition / (currentSegmentLenght-currentSegmentPosition)
      if ((currentSegmentLenght-currentSegmentPosition>0) && (y == (sceneHeight*currentSegmentPosition/(currentSegmentLenght-currentSegmentPosition)))) {
        currentWidth = (y+1)*WARNINGWIDTH/sceneHeight;
        if (currentWidth < 4) currentWidth = 4;
        currentHeight = (y+1)*WARNINGHEIGHT/sceneHeight;
        if (currentHeight < 4) currentHeight = 4;
         // triangle to right
         display.fillTriangle(borderLeftBegin,sceneHeight+y-currentHeight/2,
          borderLeftBegin-currentWidth,sceneHeight+y-currentHeight,
          borderLeftBegin-currentWidth,sceneHeight+y, SSD1306_BLACK);
        display.drawTriangle(borderLeftBegin,sceneHeight+y-currentHeight/2,
          borderLeftBegin-currentWidth,sceneHeight+y-currentHeight,
          borderLeftBegin-currentWidth,sceneHeight+y, SSD1306_WHITE);
      }
    }

    // left curve warning
    if ((currentSegmentType & SEGMENTTYPE_LEFTWARNING) == SEGMENTTYPE_LEFTWARNING) {
      // y-position dependent on perspective y = sceneHeight * currentSegmentPosition / (currentSegmentLenght-currentSegmentPosition)
      if ((currentSegmentLenght-currentSegmentPosition>0) && (y == (sceneHeight*currentSegmentPosition/(currentSegmentLenght-currentSegmentPosition)))) {        
        currentWidth = (y+1)*WARNINGWIDTH/sceneHeight;
        if (currentWidth < 4) currentWidth = 4;
        currentHeight = (y+1)*WARNINGHEIGHT/sceneHeight;
        if (currentHeight < 4) currentHeight = 4;
        // triangle to left
        display.fillTriangle(borderRightBegin+borderRightWidth-1,sceneHeight+y-currentHeight/2,
          borderRightBegin+borderRightWidth-1+currentWidth,sceneHeight+y-currentHeight,
          borderRightBegin+borderRightWidth-1+currentWidth,sceneHeight+y, SSD1306_BLACK);
        display.drawTriangle(borderRightBegin+borderRightWidth-1,sceneHeight+y-currentHeight/2,
          borderRightBegin+borderRightWidth-1+currentWidth,sceneHeight+y-currentHeight,
          borderRightBegin+borderRightWidth-1+currentWidth,sceneHeight+y, SSD1306_WHITE);
      }
    }

    if (y == 20) { // player line
      g_streetMiddle = currentMiddle;
      // Reduce speed in the grass
      if (millis()-lastSlowdownMS > 50) {
        if (((g_playerPos < borderLeftBegin) || (g_playerPos > grassRightBegin - 1))) {
          if (g_speed > 11) g_speed-=8; else if (g_speed > GRASSMINSPEED) g_speed = GRASSMINSPEED;
        }
        lastSlowdownMS = millis();
      }
    }
  } 
}

// initial game settings
void resetGame() {
  g_streetMiddle = SCREEN_WIDTH/2; 
  g_playerPos = g_streetMiddle;
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

  #ifndef DEMOMODE
  
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

  #endif

   // OLED init
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Default Address is 0x3D for 128x64, but my OLED uses 0x3C 
    // SSD1306 allocation failed
    blink(1000);
    blink(1000);
    blink(1000);
    while (true);
  }

  // Intro text
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(2);
  display.setCursor(0,0);
  display.println(F("3 fast"));
  display.print(F("laps..."));
  display.display();
  display.setTextSize(1);

  #ifndef DEMOMODE
  
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

  #else

  delay(3000); // start delay in demo mode

  #endif
  
  resetGame();
}

void loop(void) {
  char strData[24];
  unsigned long startMS, endMS;
  static unsigned long lastPlayerMS = 0; 
  static unsigned long lastBuzzerMS = 0;
  static unsigned int fps = 0;
  static int roll = 0;
  static int pitch = 0;

  #ifndef DEMOMODE

  Quaternion q;           // [w, x, y, z]         quaternion container
  VectorFloat gravity;    // [x, y, z]            gravity vector
  float ypr[3];           // [yaw, pitch, roll]   yaw/pitch/roll container and gravity vector
  uint8_t fifoBuffer[64]; // FIFO storage buffer
  uint8_t rc;

  #endif

  // record start of frame
  startMS = millis();

  #ifndef DEMOMODE
  
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

  #else

  // Demomode tilt
  roll=(g_streetMiddle - g_playerPos);
  #endif

  // change player sprite
  g_sprite  = 0;
  
  if (roll < -1) { // tilt to left
    g_sprite=INDIVIDUALSPRITES-(roll>>3);
    if (g_sprite > (INDIVIDUALSPRITES<<1)-2) g_sprite = (INDIVIDUALSPRITES<<1)-2;  
  }
  if (roll > 1) { // tilt to right
    g_sprite=1+(roll>>3);
    if (g_sprite > INDIVIDUALSPRITES-1) g_sprite=INDIVIDUALSPRITES-1;    
  }

  // control player every 100ms
  if (millis()-lastPlayerMS>100) {
    // increase/decrease speed
    if (pitch > 10) { // decrease by pitch
      if (g_speed > 1) g_speed--;
    } else { // increase automatically
      if (g_speed > 140) g_speed++; else g_speed+=2;
      if (g_speed > MAXSPEED) g_speed = MAXSPEED;
    }

    // Control left/right by roll    
    g_playerPos += (1+g_speed/30)*20*roll/90;

    // Drift in curves
    if (g_streetMiddle != SCREEN_WIDTH/2) g_playerPos+=(SCREEN_WIDTH/2-g_streetMiddle)*(1+g_speed/60);
      
    if (g_playerPos < SPRITEWIDTH/2) g_playerPos = SPRITEWIDTH/2;
    if (g_playerPos > SCREEN_WIDTH-SPRITEWIDTH/2-1) g_playerPos = SCREEN_WIDTH-SPRITEWIDTH/2-1;

    if ((roll <-5) && (g_speed > GRASSMINSPEED)) g_speed--; // reduce speed when tilt left
    if ((roll > 5) && (g_speed > GRASSMINSPEED)) g_speed--; // reduce speed when tilt right

    lastPlayerMS = millis();
  }

  // clear display buffer
  display.clearDisplay();

  // End of game
  if (g_laps >= MAXLAPS) {
    #ifdef USE_BUZZER
    noTone(BUZZERPIN);
    #endif
    display.setTextSize(2);
    display.setCursor(50,20);
    display.print(F("Fin"));
    display.setTextSize(1);
    display.setCursor(0,56);
    display.print(F("in "));
    display.print((millis()-g_startMS)/1000UL);
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

  // Buzzersound
  #ifdef USE_BUZZER
  if (millis() - lastBuzzerMS > 50) {
    if (g_speed < 1) noTone(BUZZERPIN); else tone(BUZZERPIN,50+g_speed*5,10);
    lastBuzzerMS = millis();
  }
  #endif
  
  // calculate frames per second
  endMS = millis();
  if (endMS - startMS > 0) fps = 1000/(endMS - startMS);
}
