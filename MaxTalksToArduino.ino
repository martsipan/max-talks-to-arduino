#include <FastLED.h>

// ------------------------------------
// Configuration
// ------------------------------------

#define SERIAL_BAUD_RATE 115200           // Serial baud rate for data transport
#define SERIAL_TIMEOUT 500                // Timeout for unanswered Serial messages (ms)

#define LED_NUM_STRIPS 2                  // How many LED strips (change data pins in setupLedStripes)
#define LED_NUM_LEDS_PER_STRIP 300        // How many LEDs per stripe

// ------------------------------------
// Variable initalization
// ------------------------------------

// Markers for our serial communication packages
const char END_MARKER = '>';
const char DELIMITER_MARKER = '|';

// Maximum byte size of a serial command
const byte COMMAND_BUFFER_SIZE = 64;

// Prepare a buffer of chars to fill it with an incoming command
char commandBuffer[COMMAND_BUFFER_SIZE];

// Signals if we finished reading a new command
boolean hasNewData = false;
boolean isReadingCommand = false;
byte commandBufferIndex = 0;

// Current argument index for scanning the command
int currentArgumentIndex = 0;

// Define LEDs
CRGBArray<LED_NUM_LEDS_PER_STRIP> leds[LED_NUM_STRIPS];

// Initial global brightness
int globalBrightness = 255;

// ------------------------------------
// LED setup method
// ------------------------------------

// Initialize LED interface
void setupLedStripes() {
  // Change this when data pins of Arduino board and / or number of stripes changed:
  FastLED.addLeds<WS2812B, 10, GRB>(leds[0], LED_NUM_LEDS_PER_STRIP);
  FastLED.addLeds<WS2812B, 11, GRB>(leds[1], LED_NUM_LEDS_PER_STRIP);
  // FastLED.addLeds<WS2812B, 5, GRB>(leds[2], LED_NUM_LEDS_PER_STRIP);
  // FastLED.addLeds<WS2812B, 6, GRB>(leds[3], LED_NUM_LEDS_PER_STRIP);
  // FastLED.addLeds<WS2812B, 8, GRB>(leds[4], LED_NUM_LEDS_PER_STRIP);
}

// ------------------------------------
// LED control methods
// ------------------------------------

// Reset all LEDS
void resetAllLed() {
  for (int x = 0; x < LED_NUM_STRIPS; x += 1) {
    leds[x] = CRGB::Black;
  
    FastLED[x].showLeds();
  }
}

// ------------------------------------
// Serial communication methods
// ------------------------------------

// Our function to read incoming data from the serial port
void readSerialData() {
  char rc;

  // Read incoming data until we are sure we have a complete command
  while (Serial.available() > 0 && !hasNewData) {
    rc = Serial.read();

    if (rc != END_MARKER) {
      // Add incoming data to our buffer
      commandBuffer[commandBufferIndex] = rc;
      commandBufferIndex += 1;

      // Silently solve a buffer overflow
      if (commandBufferIndex >= COMMAND_BUFFER_SIZE) {
        commandBufferIndex = COMMAND_BUFFER_SIZE - 1;
      }
    } else {
      // We detected the end of the command, stop reading
      commandBuffer[commandBufferIndex] = '\0'; // terminate the string

      // Indicate that we have received a command, ready to be processed
      hasNewData = true;
      commandBufferIndex = 0;
    }
  }
}

// Splits the command buffer string via delimiters and returns it at a certain index
String readStringArgument() {
  String data = commandBuffer;

  int argumentIndex = currentArgumentIndex;
  int foundArgumentIndex = 0;
  
  int strCharIndex[] = { 0, -1 };
  int maxCharIndex = data.length() - 1;

  // Find the right argument by scanning the string for delimiters
  for (int i = 0; i <= maxCharIndex && foundArgumentIndex <= argumentIndex; i++) {
    if (data.charAt(i) == DELIMITER_MARKER || i == maxCharIndex) {
      foundArgumentIndex++;
      
      strCharIndex[0] = strCharIndex[1] + 1;
      strCharIndex[1] = (i == maxCharIndex) ? (i + 1) : i;
    }
  }

  // Store scanning variables for next argument
  currentArgumentIndex += 1;

  // Split the string at the found delimiter points, otherwise return 0
  return foundArgumentIndex > argumentIndex ? data.substring(strCharIndex[0], strCharIndex[1]) : "0";
}

// Parse command argument as an integer value
int readIntArgument() {
  return readStringArgument().toInt();
}

// Check if we have a complete command, call matching method
void dispatchCommand() {
  // Check first if we have something new for us here, otherwise skip it
  if (!hasNewData) {
    return;
  }

  // Reset scanning variables
  currentArgumentIndex = 0;

  // Read the command name
  String cmdName = readStringArgument();

  // Dispatch command via command name
  if (cmdName == "R") {
    onReset();
  } else if (cmdName == "S") {
    onSet();
  } else if (cmdName == "A") {
    onSetAll();
  } else if (cmdName == "D") {
    onDarken();
  } else if (cmdName == "B") {
    onSetBrightness();
  }

  // We are done processing that command!
  hasNewData = false;
}

// ------------------------------------
// Validation methods
// ------------------------------------

bool isStripIndexValid(int stripIndex) {
  return stripIndex < LED_NUM_STRIPS && stripIndex >= 0;
}

bool isLedRangeValid(int ledStartIndex, int ledEndIndex) {
  return (
    ledStartIndex >= 0,
    ledEndIndex >= 0,
    ledStartIndex < LED_NUM_LEDS_PER_STRIP &&
    ledEndIndex < LED_NUM_LEDS_PER_STRIP &&
    ledStartIndex <= ledEndIndex
   );
}

bool isColorValid(int red, int green, int blue) {
  return isValid(red) && isValid(green) && isValid(blue);
}

bool isValid(int value) {
  return value >= 0 && value <= 255;
}

// ------------------------------------
// Serial command callbacks
// ------------------------------------

void onReset() {
  resetAllLed();
}

void onSet() {
  int stripIndex = readIntArgument();

  if (!isStripIndexValid(stripIndex)) {
    return;
  }
  
  int ledStartIndex = readIntArgument();
  int ledEndIndex = readIntArgument();

  if (!isLedRangeValid(ledStartIndex, ledEndIndex)) {
    return;
  }

  int colorRed = readIntArgument();
  int colorGreen = readIntArgument();
  int colorBlue = readIntArgument();

  if (!isColorValid(colorRed, colorGreen, colorBlue)) {
    return;
  }

  leds[stripIndex](ledStartIndex, ledEndIndex) = CRGB(colorRed, colorGreen, colorBlue);
  
  FastLED[stripIndex].showLeds(globalBrightness);
}

void onSetAll() {
  int stripIndex = readIntArgument();

  if (!isStripIndexValid(stripIndex)) {
    return;
  }
  
  int colorRed = readIntArgument();
  int colorGreen = readIntArgument();
  int colorBlue = readIntArgument();

  if (!isColorValid(colorRed, colorGreen, colorBlue)) {
    return;
  }

  leds[stripIndex] = CRGB(colorRed, colorGreen, colorBlue);
  
  FastLED[stripIndex].showLeds(globalBrightness);
}

void onDarken() {
  int stripIndex = readIntArgument();

  if (!isStripIndexValid(stripIndex)) {
    return;
  }
  
  int ledStartIndex = readIntArgument();
  int ledEndIndex = readIntArgument();

  if (!isLedRangeValid(ledStartIndex, ledEndIndex)) {
    return;
  }

  int percentage = readIntArgument();

  if (!isValid(percentage)) {
    return;
  }

  Serial.print(percentage);

  leds[stripIndex](ledStartIndex, ledEndIndex).fadeToBlackBy(percentage);
  
  FastLED[stripIndex].showLeds(globalBrightness);
}

void onSetBrightness() {
  int brightness = readIntArgument();

  if (!isValid(brightness)) {
    return;
  }

  globalBrightness = brightness;
}

// ------------------------------------
// Core methods
// ------------------------------------

void setup() {
  // Listen on serial port for messages from the Laptop
  Serial.begin(SERIAL_BAUD_RATE);
  Serial.setTimeout(SERIAL_TIMEOUT);

  // Setup LED stripes and reset them in the beginning
  setupLedStripes();
  resetAllLed();
}

void loop() {
  // Processes incoming serial data
  readSerialData();

  // Perform callback when command given
  dispatchCommand();
}
