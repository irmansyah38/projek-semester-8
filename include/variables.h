#ifndef VARIABLES_H
#define VARIABLES_H

// time
#define INTERVAL 1000

// oled
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET -1    // Reset pin # (or -1 if sharing Arduino reset pin)

// lcd
#define lCD_COLUMNS 16
#define lCD_ROWS 2

// telegram
#define TELEGRAM_DOMAIN "api.telegram.org"

// microwave
#define RADAR_PIN GPIO_NUM_20

// button
#define Button_PIN GPIO_NUM_14

// led
#define LED_PIN GPIO_NUM_2

// sim800l
#define SIM800L_RX_PIN GPIO_NUM_46
#define SIM800L_TX_PIN GPIO_NUM_3

// kondisi esp
extern int condition_main;

// wifi
extern String ip;

// oled
extern String last_sentance;

// telegram
extern String bot_token;
extern String chat_id;

// disable sleep
extern unsigned long last_time;

// website
extern bool web_condition;

// nomor handphone
extern String nomor_handphone;

extern const char index_html[];

#endif
