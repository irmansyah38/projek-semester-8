#ifndef VARIABLES_H
#define VARIABLES_H

// time
#define interval 1000

// oled
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET -1    // Reset pin # (or -1 if sharing Arduino reset pin)

// lcd
#define lcdColumns 16
#define lcdRows 2

// telegram
#define myDomain "api.telegram.org"

// microwave
#define radarPin 20

// button
#define buttonPin 14

// led
#define ledPin 2

extern const char index_html[];

#endif
