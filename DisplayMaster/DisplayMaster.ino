/*----------------------------------------------------------------------------
* DisplayMaster is using five Colorduino slave modules as
* LED Matrix. Each slave has to be programmed with a 
* slave address (Wire/I2C) and connected to SDA/SCL
* Max power consumption at 5VDC is 1500mA with five slaves
*
* Anders Wilhelmsson March '17
* - Multiple slaves.  1 slave  = 1482 bytes Uno/Mega
*                     2 slaves = 1684 bytes Uno/Mega
*                     3 slaves = 1876 bytes Uno/Mega
*                     4 slaves = 2068 bytes Mega
*                     5 slaves = 2260 bytes Mega 5932 bytes free
* - Separate white balance per slave
* - Morph demo
* - Scroll demo 
*
* The project is based on open source.   
* https://github.com/funnypolynomial/Chromaduino
* 
* 
*----------------------------------------------------------------------------*/

#include <Wire.h>

//#define DEBUG         // Startup and RAM info in Monitor 
//#define DEBUG_WIRE    // Wire/I2C info in Monitor
//#define DEBUG_PIXEL   // Pixel info in Monitor. Slows down everything...
//#define DEBUG_SCROLL  // Scroll info in Monitor

const uint16_t NO_OF_MODULES = 5;   // Max 10 pcs  

const uint16_t MATRIX_WIDTH_X = 8;
const uint16_t MATRIX_HEIGHT_Y = 8;
const uint16_t NUM_LEDS_PER_MODULE = MATRIX_WIDTH_X * MATRIX_HEIGHT_Y;
const uint16_t DISPLAY_WIDTH_X = MATRIX_WIDTH_X * NO_OF_MODULES;
const uint16_t DISPLAY_HEIGHT_Y = MATRIX_HEIGHT_Y;
const uint16_t NUM_LEDS = DISPLAY_WIDTH_X * DISPLAY_HEIGHT_Y;

// Slave node address. Module 1 - 10
uint8_t I2C_Addr[] = {0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E};

// One pixel RGB
typedef struct {
  uint8_t r;
  uint8_t g;
  uint8_t b;
} pixelRGB;
pixelRGB colorRGB;

// One pixel HSV
typedef struct {
  uint8_t h;
  uint8_t s;
  uint8_t v;
} pixelHSV;
pixelHSV colorHSV;
  
// Display buffer with structs of RGB pixels
pixelRGB displayBuffer[DISPLAY_WIDTH_X*DISPLAY_HEIGHT_Y];

// Morph
long paletteShift;

// White balance Colorduino V1.A
// Set display to all white to check white balance
// Then adjust those parameters
// Six bits 0 - 63
pixelRGB whiteBalanceCalib[10] = {{20, 63, 43},  // 1
                                  {18, 61, 42},  // 2
                                  {20, 63, 40},  // 3
                                  {16, 57, 63},  // 4
                                  {17, 57, 43},  // 5
                                  {18, 63, 45},
                                  {18, 60, 45},
                                  {18, 60, 45},
                                  {18, 60, 45},
                                  {18, 60, 45}};

//-----------------------------------------------------------------------------  
// This is the demo text that is scrolling....
const char charMsg[] = "Hello World ~ LED Matrix Demo    ";

// Number of milliseconds to pause after message (0 = no pause):
const uint16_t msgPauseTime = 0;
// Controls the message scroll speed in ms (lower = faster):
const uint8_t scrollSpeed = 0;  // Wait time in ms

uint32_t thisMs = 0;            // Elapsed time in ms that Arduino has been running
uint32_t lastMs;                // Interval since the message was last scrolled one column
uint32_t pauseStart;            // Elapsed time in ms since end-of-message pause was started
uint8_t msgIdx = 0;             // Keeps track of the message character current being scrolled
uint8_t charBuffer[8];          // Actual characer scrolling in from rigth
uint8_t bufferIdx = 0;          // Keeps track of scrolling (after 8 columns, fetch next character)
bool    pauseDisplay = false;   // Determines when scrolling should be paused

// List of supported characters and index into corresponding character definitions array
const byte charSet[] = " ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789!#$%&'()*+-,.~:;<>?@[]/\\=_";

// Character definitions (8 bytes per character)
const byte charDef[] = {
  0,0,0,0,0,0,0,0,                // Space symbol (must be the first character in this array)
  // Upper case letters:
  24,60,102,126,102,102,102,0,    // A
  124,102,102,124,102,102,124,0,  // B
  60,102,96,96,96,102,60,0,       // C
  120,108,102,102,102,108,120,0,  // D
  126,96,96,120,96,96,126,0,      // E
  126,96,96,120,96,96,96,0,       // F
  60,102,96,110,102,102,60,0,     // G
  102,102,102,126,102,102,102,0,  // H
  60,24,24,24,24,24,60,0,         // I
  30,12,12,12,12,108,56,0,        // J
  102,108,120,112,120,108,102,0,  // K
  96,96,96,96,96,96,126,0,        // L
  99,119,127,107,99,99,99,0,      // M
  102,118,126,126,110,102,102,0,  // N
  60,102,102,102,102,102,60,0,    // O
  124,102,102,124,96,96,96,0,     // P
  60,102,102,102,102,60,14,0,     // Q
  124,102,102,124,120,108,102,0,  // R
  60,102,96,60,6,102,60,0,        // S
  126,24,24,24,24,24,24,0,        // T
  102,102,102,102,102,102,60,0,   // U
  102,102,102,102,102,60,24,0,    // V
  99,99,99,107,127,119,99,0,      // W
  102,102,60,24,60,102,102,0,     // X
  102,102,102,60,24,24,24,0,      // Y
  126,6,12,24,48,96,126,0,        // Z
  // Lower case letters:
  0,0,60,6,62,102,62,0,           // a 
  0,96,96,124,102,102,124,0,      // b
  0,0,60,96,96,96,60,0,           // c
  0,6,6,62,102,102,62,0,          // d
  0,0,60,102,126,96,60,0,         // e
  0,14,24,62,24,24,24,0,          // f
  0,0,62,102,102,62,6,124,        // g
  0,96,96,124,102,102,102,0,      // h
  0,24,0,56,24,24,60,0,           // i
  0,6,0,6,6,6,6,60,               // j
  0,96,96,108,120,108,102,0,      // k
  0,56,24,24,24,24,60,0,          // l
  0,0,102,127,127,107,99,0,       // m
  0,0,124,102,102,102,102,0,      // n
  0,0,60,102,102,102,60,0,        // o
  0,0,124,102,102,124,96,96,      // p
  0,0,62,102,102,62,6,6,          // q
  0,0,124,102,96,96,96,0,         // r
  0,0,62,96,60,6,124,0,           // s
  0,24,126,24,24,24,14,0,         // t
  0,0,102,102,102,102,62,0,       // u
  0,0,102,102,102,60,24,0,        // v
  0,0,99,107,127,62,54,0,         // w
  0,0,102,60,24,60,102,0,         // x
  0,0,102,102,102,62,12,120,      // y
  0,0,126,12,24,48,126,0,         // z
  // Numbers
  60,102,110,118,102,102,60,0,    // 0
  24,24,56,24,24,24,126,0,        // 1
  60,102,6,12,48,96,126,0,        // 2
  60,102,6,28,6,102,60,0,         // 3
  6,14,30,102,127,6,6,0,          // 4
  126,96,124,6,6,102,60,0,        // 5
  60,102,96,124,102,102,60,0,     // 6
  126,102,12,24,24,24,24,0,       // 7
  60,102,102,60,102,102,60,0,     // 8
  60,102,102,62,6,102,60,0,       // 9
  // Characters and symbols
  24,24,24,24,0,0,24,0,           // !
  102,102,255,102,255,102,102,0,  // #
  24,62,96,60,6,124,24,0,         // $
  98,102,12,24,48,102,70,0,       // %
  60,102,60,56,103,102,63,0,      // &
  6,12,24,0,0,0,0,0,              // '
  12,24,48,48,48,24,12,0,         // (
  48,24,12,12,12,24,48,0,         // )
  0,102,60,255,60,102,0,0,        // *
  0,24,24,126,24,24,0,0,          // +
  0,0,0,126,0,0,0,0,              // -
  0,0,0,0,0,24,24,48,             // ,
  0,0,0,0,0,24,24,0,              // .
  60,66,165,129,165,153,66,60,    // "Smiley face" replaces the tilde (~) character
  0,0,24,0,0,24,0,0,              // :
  0,0,24,0,0,24,24,48,            // ;
  14,24,48,96,48,24,14,0,         // <
  112,24,12,6,12,24,112,0,        // >
  60,102,6,12,24,0,24,0,          // ?
  60,102,110,110,96,98,60,0,      // @
  60,48,48,48,48,48,60,0,         // [
  60,12,12,12,12,12,60,0,         // ]
  0,3,6,12,24,48,96,0,            // /
  0,96,48,24,12,6,3,0,            // \
  0,0,126,0,126,0,0,0,            // =
  0,0,0,0,0,0,0,255               // _
};  
//-----------------------------------------------------------------------------  
uint16_t freeRam () {
  extern uint16_t __heap_start, *__brkval; 
  uint16_t v; 
  return (uint16_t) &v - (__brkval == 0 ? (uint16_t) &__heap_start : (uint16_t) __brkval); 
}
//-----------------------------------------------------------------------------  
void refreshDisplay() 
{
  #ifdef DEBUG_PIXEL
  printBuffer();
  #endif 
  
  for (uint8_t m=0; m<NO_OF_MODULES; m++) {
    // Send start command
    startSendBufferToSlave(I2C_Addr[m]);
    // Send buffer, pixel by pixel, one module 8x8 at the time
    for (uint8_t y=0; y<MATRIX_HEIGHT_Y; y++) {
      for (uint8_t x=0; x<MATRIX_WIDTH_X; x++) {
        pixelRGB *p = getPixelAddress(x+m*MATRIX_WIDTH_X,y);
        sendOnePixelToSlave(I2C_Addr[m],(uint16_t)p);
      }
    } 
  }
  // Activate buffer in slaves
  for (uint8_t m=0; m<NO_OF_MODULES; m++) {
    swapBufferAtSlave(I2C_Addr[m]);   
  }
}
//-----------------------------------------------------------------------------  
void printBuffer() 
{
  char s[10];
  Serial.println(F("Start printing ---"));  
  for (uint8_t y=0; y<8; y++) {
    for (uint8_t x=0; x<32; x++) {
      pixelRGB *p = getPixelAddress(x,y);
      uint8_t moduleNo = x*MATRIX_WIDTH_X/NUM_LEDS_PER_MODULE; 
      sprintf( s, "%02d", x);
      Serial.print(F("x="));Serial.print(s);
      sprintf( s, "%02d", y);
      Serial.print(F(" y="));Serial.print(s);
      sprintf( s, "%02X", moduleNo);
      Serial.print(F(" m="));Serial.print(s);
      sprintf( s, "%4d", (uint16_t)p);
      Serial.print(F(" a="));Serial.print(s);
      sprintf( s, "%3d", p->r);
      Serial.print(F(" r="));Serial.print(s);
      sprintf( s, "%3d", p->g);
      Serial.print(F(" g="));Serial.print(s);
      sprintf( s, "%3d", p->b);
      Serial.print(F(" b="));Serial.println(s);
    }
  } 
  Serial.println(F("Buffer printed ---"));  
  Serial.println();
}
//-----------------------------------------------------------------------------  
// Set one pixel in display buffer
void setPixelRGB(uint8_t x, uint8_t y, uint8_t r, uint8_t g, uint8_t b) 
{
  pixelRGB *p = getPixelAddress(x,y);
  if (p) {
    p->r = r;
    p->g = g;
    p->b = b;
  }
}
//-----------------------------------------------------------------------------  
// Get pixel address in display buffer, return null if out of display
pixelRGB *getPixelAddress(uint8_t x, uint8_t y) 
{
  // Return address of first byte in pixel struct
  if ( x < 0 || x >= DISPLAY_WIDTH_X || y < 0 || y >= DISPLAY_HEIGHT_Y ) {
    return NULL;
  }
  // Calculate module number 0 - 3
  uint8_t moduleNo = x*MATRIX_WIDTH_X/NUM_LEDS_PER_MODULE; 
   
  // Calculate position in display buffer
  uint16_t ptr = x + (moduleNo*NUM_LEDS_PER_MODULE) - (moduleNo*MATRIX_WIDTH_X) + (y*MATRIX_WIDTH_X);  
  return displayBuffer + ptr;
}
//-----------------------------------------------------------------------------  
// Fill entire display with onte color
void fillDisplayBufferWithColorRGB(uint8_t R,uint8_t G,uint8_t B) 
{
  pixelRGB *p = getPixelAddress(0,0);
  #ifdef DEBUG
  Serial.print(F("Start fill address in RAM = "));Serial.println((uint16_t)p);
  #endif
  for (uint8_t y=0; y<DISPLAY_HEIGHT_Y;y++) {
    for (uint8_t x=0;x<DISPLAY_WIDTH_X;x++) { 
      p->r = R;
      p->g = G;
      p->b = B;
      p++;        
    }
  }
  #ifdef DEBUG
  Serial.print(F("End address in RAM = "));Serial.println((uint16_t)p-1);
  #endif
}
//-----------------------------------------------------------------------------  
void startSendBufferToSlave(uint8_t I2C_ADDR) 
{
  #ifdef DEBUG_WIRE
  Serial.print(F("Start sending buffer to slave with address "));Serial.println(I2C_ADDR, HEX);
  #endif
  Wire.beginTransmission(I2C_ADDR);
  Wire.write((uint8_t)0x00);  // Set slave write buffer pointer to start
  Wire.endTransmission();
  delayMicroseconds(200);  // If this is too low, pixel error will appear...  
}
//-----------------------------------------------------------------------------  
void sendOnePixelToSlave(uint8_t I2C_ADDR, uint8_t *pRGB) // First pixel byte in mem
{
  Wire.beginTransmission(I2C_ADDR);
  for (uint8_t idx = 0; idx < 3; idx++, pRGB++) {
    Wire.write(*pRGB);  // Send three bytes R, G, B, one by one
  }  
  Wire.endTransmission();
  delayMicroseconds(200);  // If this is too low, pixel error will appear...  
}
//-----------------------------------------------------------------------------  
void swapBufferAtSlave(uint8_t I2C_ADDR)   // refreshDisplay entire buffer now
{
  #ifdef DEBUG_WIRE
  Serial.print(F("Swap buffer at slave "));Serial.println(I2C_ADDR, HEX);
  #endif
 
  Wire.beginTransmission(I2C_ADDR);
  Wire.write((uint8_t)0x01); // Swap read and write buffer at slave
  Wire.endTransmission();
  delayMicroseconds(200);  // If this is too low, pixel error will appear...  
}
//-----------------------------------------------------------------------------  
bool slaveWhiteBalanceIsSet(uint8_t I2C_ADDR) 
{
  Wire.requestFrom(I2C_ADDR,(uint8_t)1);  // Request one byte
  uint8_t count = 0;
  if ( Wire.available())
    count = Wire.read();
  Wire.beginTransmission( I2C_ADDR);
  Wire.write((uint8_t)0x02); // activate white balance in slave
  Wire.endTransmission();
  return count == 3;  // true if slave got 3 bytes
} 
//-----------------------------------------------------------------------------  
// write white balance, true if got expected response from slave
bool setWhiteBalance(uint8_t I2C_ADDR,pixelRGB *pRGB ) 
{
  startSendBufferToSlave(I2C_ADDR);
  sendOnePixelToSlave(I2C_ADDR,(uint16_t)pRGB);
  return slaveWhiteBalanceIsSet(I2C_ADDR);
} 
//-----------------------------------------------------------------------------  
// Clear Screen
void CLS() 
{
  fillDisplayBufferWithColorRGB(0,0,0);
  refreshDisplay();
}
//-----------------------------------------------------------------------------  
//Converts an HSV color to RGB color
void HSVtoRGB(void *vRGB, void *vHSV) 
{
  float r, g, b, h, s, v; //this function works with floats between 0 and 1
  float f, p, q, t;
  int i;
  pixelRGB *colorRGB=(pixelRGB *)vRGB;
  pixelHSV *colorHSV=(pixelHSV *)vHSV;

  h = (float)(colorHSV->h / 256.0);
  s = (float)(colorHSV->s / 256.0);
  v = (float)(colorHSV->v / 256.0);

  //if saturation is 0, the color is a shade of grey
  if(s == 0.0) {
    b = v;
    g = b;
    r = g;
  }
  //if saturation > 0, more complex calculations are needed
  else
  {
    h *= 6.0; //to bring hue to a number between 0 and 6, better for the calculations
    i = (int)(floor(h)); //e.g. 2.7 becomes 2 and 3.01 becomes 3 or 4.9999 becomes 4
    f = h - i;//the fractional part of h

    p = (float)(v * (1.0 - s));
    q = (float)(v * (1.0 - (s * f)));
    t = (float)(v * (1.0 - (s * (1.0 - f))));

    switch(i)
    {
      case 0: r=v; g=t; b=p; break;
      case 1: r=q; g=v; b=p; break;
      case 2: r=p; g=v; b=t; break;
      case 3: r=p; g=q; b=v; break;
      case 4: r=t; g=p; b=v; break;
      case 5: r=v; g=p; b=q; break;
      default: r = g = b = 0; break;
    }
  }
  colorRGB->r = (uint8_t)(r * 255.0);
  colorRGB->g = (uint8_t)(g * 255.0);
  colorRGB->b = (uint8_t)(b * 255.0);
}
//-----------------------------------------------------------------------------  
float dist(float a, float b, float c, float d) 
{
  return sqrt((c-a)*(c-a)+(d-b)*(d-b));
}
//-----------------------------------------------------------------------------
// Scroll functions
//-----------------------------------------------------------------------------
void initScroll() 
{
  // Prepare to start displaying banner message
  for (int i = 0; i < 8; i++)
    charBuffer[i] = 0;         // Clear scrolling buffer prior to displaying the message
  bufferIdx = 0;               // Buffer index is set to first column x in char buffer
  msgIdx = 0;                  // Reset pointer to first character in the message
  fetchChar();                 // Read first character in message and prepare to scroll it
  pauseDisplay = false;        // No need to pause, since we're starting over again
}
//-----------------------------------------------------------------------------  
void fetchChar() 
{
  int foundVal = 0;
  // Check to see whether we've reached the end of the null-terminated message character string
  //if (charMsg[msgIdx] == 0) {
  if (msgIdx > sizeof(charMsg)) {
    msgIdx = 0;
    pauseDisplay = true;
    pauseStart = thisMs;
  }
  // For each character in the message, make sure there is a matching character definition
  for (int i = 0; i < 89; i++) {
    if (charMsg[msgIdx] == charSet[i]) {
      foundVal = i;
      #ifdef DEBUG_SCROLL
      Serial.print(F("Search for "));Serial.print(charMsg[msgIdx]);Serial.print(F(", found "));Serial.print((char)charSet[foundVal]);Serial.print(F(" ("));Serial.print(charSet[i]);Serial.println(F(")"));
      #endif
    }
  }
  msgIdx++;
  // Load the scroll buffer with the newly-found character (or blank, if none was found)
  for (int i = 0; i < 8; i++) {
    charBuffer[i] = charDef[foundVal * 8 + i];
  }
}
//-----------------------------------------------------------------------------  
void scrollLeft(uint8_t hue) 
{
  // Scroll entire display one left
  for(uint8_t y = 0; y < DISPLAY_HEIGHT_Y; y++) {
    for(uint8_t x = 0; x < DISPLAY_WIDTH_X-1; x++) {
      // move one pixel from right to left      
      pixelRGB *p = getPixelAddress(x+1,y);
      colorHSV.h=hue;  // Only change hue
      colorHSV.s=255; 
      colorHSV.v=255;
      HSVtoRGB(&colorRGB, &colorHSV);
      setPixelRGB(x, y, p->r, p->g, p->b);      
    }
  }  
  // Activate LED matrix pixels most right Y column with matching bits in char buffer 
  for (int y = 0; y < 8; y++) {    
    bool b = charBuffer[y] >> (7-bufferIdx) & 1;  // Pixel set?
    if (b) setPixelRGB(DISPLAY_WIDTH_X-1, y, colorRGB.r, colorRGB.g, colorRGB.b);
    else setPixelRGB(DISPLAY_WIDTH_X-1, y, 0, 0, 0);
  }
  
  // After 8 columns have been scrolled, fetch next character
  bufferIdx++;
  if (bufferIdx > 7) {
    bufferIdx = 0;
    fetchChar();
  }
}
//-----------------------------------------------------------------------------  
// Morph demo
//-----------------------------------------------------------------------------
void displayMorph() 
{  
  uint8_t x,y;
  float value;
  pixelRGB colorRGB;
  pixelHSV colorHSV;

  for(y = 0; y < DISPLAY_HEIGHT_Y; y++) 
    for(x = 0; x < DISPLAY_WIDTH_X; x++) { {
      value = sin(dist(x + paletteShift, y, 128.0, 128.0) / 8.0)
        + sin(dist(x, y, 64.0, 64.0) / 8.0)
        + sin(dist(x, y + paletteShift / 7, 192.0, 64) / 7.0)
        + sin(dist(x, y, 192.0, 100.0) / 8.0);
      colorHSV.h=(uint8_t)((value) * 128)&0xff;
      colorHSV.s=255; 
      colorHSV.v=255;
      HSVtoRGB(&colorRGB, &colorHSV);
      
      setPixelRGB(x, y, colorRGB.r, colorRGB.g, colorRGB.b);
    }
  }
  paletteShift += 2;
}
void setWhiteBalance()  
{
  // Set white balance
  for (uint8_t m=0; m<NO_OF_MODULES; m++ ) {
    uint8_t count=0;
    while (count<15) {
      if ( setWhiteBalance(I2C_Addr[m], &whiteBalanceCalib[m])) {
        #ifdef DEBUG_WIRE
        Serial.print(F("White balance set, module "));Serial.print(m+1);
        Serial.print(F(", address 0x"));
        if (I2C_Addr[m]<0xF) 
          Serial.print(F("0"));
        Serial.println(I2C_Addr[m], HEX);
        #endif
        count = 15; // force next
      } else { 
        delay(200);
        count++;
        #ifdef DEBUG_WIRE      
        Serial.print(F("."));
        if ( count = 15) {
          Serial.print(F("Failed to communicate with slave module #"));Serial.println(m+1);
          Serial.print(F(" Address"));Serial.println(I2C_Addr[m], HEX);
        }
        #endif
      }  
    } 
  }
}  
void scanI2C() 
{
  Serial.println(F("Looking for modules..."));
  uint8_t nDevices = 0;
  for(uint8_t address=1; address<127; address++ ) {
    // The i2c_scanner uses the return value of
    // the Write.endTransmisstion to see if
    // a device did acknowledge to the address.
    Wire.beginTransmission(address);
    uint8_t error = Wire.endTransmission();

    if (error == 0){
      Serial.print(F("I2C device found at address 0x"));
      if (address<16) 
        Serial.print(F("0"));
      Serial.println(address,HEX);
      nDevices++;
    }
    else if (error==4) {
      Serial.print(F("Unknow error at address 0x"));
      if (address<16) 
        Serial.print(F("0"));
      Serial.println(address,HEX);
    }    
  }
  if (nDevices == 0)
    Serial.println(F("No I2C devices found\n"));
}
//-----------------------------------------------------------------------------
// Setup
//-----------------------------------------------------------------------------
void setup() 
{
  Serial.begin(115200);
  #ifdef DEBUG
  Serial.println(F("Arduino started..."));
  #endif
  
  // DISPLAY_SIZE_X * DISPLAY_HEIGHT_Y * RGB Example 32*8*3 = 768 bytes
  #ifdef DEBUG
  Serial.print(F("Size of RGB array: "));Serial.print(sizeof(displayBuffer));Serial.println(F(" bytes"));
  Serial.print(F("Free RAM: "));Serial.print(freeRam());Serial.println(F(" bytes"));
  #endif
  
  // Start Wire/I2C/Twi communication
  Wire.begin();
  #ifdef DEBUG_WIRE
  Serial.println(F("Wire started"));
  
  // Scan I2C bus looking to slave modules
  scanI2C();
  #endif
  
  // Set white balance  
  setWhiteBalance();  
  
  // Initiate scroll character buffer
  initScroll();
  
  // Init morph
  paletteShift=128000;

  // Clear Screen
  CLS();

  // Use this row to set white balance and stop demo from running.
  //fillDisplayBufferWithColorRGB(255,255,255);
  //refreshDisplay();
 
  // Done
  #ifdef DEBUG
  Serial.println(F("init done"));
  #endif
}
//-----------------------------------------------------------------------------  
// Loop
//-----------------------------------------------------------------------------  
uint16_t seq = 0;
uint32_t ctr, prev = 0;
bool cls = false;

void loop() 
{
  // Step sequence every second
  ctr = millis();
  if (ctr - prev > 1000) { 
    prev = ctr; 
    seq++;
  }  
  
   // Scroll demo
  if (seq >= 0 && seq < 63) {
    cls = false;
    // Scroll left and change hue 0 - 255 
    scrollLeft(seq*4);
    refreshDisplay();
  }
 
  // Clear
  if (seq == 63 && !cls) {
    CLS();
    cls = true;
  }
  
  // Draw all colors and brightness
  if (seq == 64 && cls) {
    cls = false;
    for(uint8_t y = 0; y < DISPLAY_HEIGHT_Y; y++) {
      for(uint8_t x = 0; x < DISPLAY_WIDTH_X; x++) {   
        colorHSV.h=x*255/DISPLAY_WIDTH_X;
        colorHSV.s=255; 
        colorHSV.v=32 + y*(255-32)/DISPLAY_HEIGHT_Y; // Offset 32 to get brightness at y=0
        HSVtoRGB(&colorRGB, &colorHSV);
        setPixelRGB(x, y, colorRGB.r, colorRGB.g, colorRGB.b);        
      }      
    }     
    refreshDisplay();  
  }

  // Clear
  if (seq == 75 && !cls) {
    CLS();
    cls = true;
  }
  
  // Classic Morph demo
  if (seq > 75 && seq < 103) {
    cls = false;
    displayMorph();
    refreshDisplay();
  }
 
  // Start over again 
  if (seq == 103) {
    initScroll();
    seq = 0;
  }  
}  
//-----------------------------------------------------------------------------  
