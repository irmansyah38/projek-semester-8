#include <UniversalTelegramBot.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <Wire.h>
#include "variables.h"
#include "driver/rtc_io.h"
#include "esp_sleep.h"
#include <SPI.h>
#include <tflm_esp32.h>
#include <eloquent_tinyml.h>
#include <eloquent_tinyml/zoo/person_detection.h>
#include <eloquent_esp32cam.h>
#include <Arduino.h>

using eloq::camera;
using eloq::tinyml::zoo::personDetection;

// telegram
String BOT_TOKEN = "7648288075:AAF9feZLkXTealeJfEtxVsLdtSiAaX_bEjA";
String CHAT_ID = "5898924169";

bool is_reset = false;

// disable sleep
bool disableSleep = true;
unsigned long lastTime;

// object
WiFiClientSecure client;
UniversalTelegramBot bot(BOT_TOKEN, client);

// telegram
void setupTelegram();
String sendPhoToTelegram(camera_fb_t *);
bool checkTelegram(String);
bool checkAPITelegram();

// sensor Radar
void setupRadar();

// tombol
void setupButton();
void IRAM_ATTR handleButtonInterrupt();
void reset();

// sleep
void modeSleep();

// configuration
void afterConfiguration();

void setup()
{
  Serial.begin(115200);
  pinMode(ledPin, OUTPUT);
  setupRadar();
  // setupButton();
  // camera settings
  // replace with your own model!
  camera.pinout.freenove_s3();
  camera.brownout.disable();
  // only works on 96x96 (yolo) grayscale images
  camera.resolution.yolo();
  camera.pixformat.gray();

  // init camera
  while (!camera.begin().isOk())
    Serial.println(camera.exception.toString());
  Serial.println("Camera OK");

  // init tf model
  while (!personDetection.begin().isOk())
    Serial.println(personDetection.exception.toString());
  Serial.println("Model OK");
  delay(5000);

  WiFi.mode(WIFI_STA);
  WiFi.begin(SSID, PASSWORD);
  Serial.print("Connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print('.');
    delay(1000);
  }
  Serial.println(WiFi.localIP());
  setupTelegram();

  delay(5000);
  modeSleep();
}

void loop()
{
  // if (is_reset)
  // {
  //   reset();
  // }
  afterConfiguration();
}

// --- Telegram ---
void setupTelegram()
{
  client.setCACert(TELEGRAM_CERTIFICATE_ROOT);
  Serial.println("setup tele sudah");
}

String sendPhoToTelegram(camera_fb_t *fb)
{
  String getAll = "";
  String getBody = "";

  uint8_t *fbBuf;
  size_t fbLen;

  frame2jpg(fb, 80, &fbBuf, &fbLen);

  String head = "--RandomNerdTutorials\r\nContent-Disposition: form-data; name=\"chat_id\"; \r\n\r\n" + CHAT_ID + "\r\n--RandomNerdTutorials\r\nContent-Disposition: form-data; name=\"photo\"; filename=\"esp32-cam.jpg\"\r\nContent-Type: image/jpeg\r\n\r\n";
  String tail = "\r\n--RandomNerdTutorials--\r\n";

  size_t imageLen = fbLen;
  size_t extraLen = head.length() + tail.length();
  size_t totalLen = imageLen + extraLen;

  client.println("POST /bot" + BOT_TOKEN + "/sendPhoto HTTP/1.1");
  client.println("Host: " + String(myDomain));
  client.println("Content-Length: " + String(totalLen));
  client.println("Content-Type: multipart/form-data; boundary=RandomNerdTutorials");
  client.println();
  client.print(head);

  for (size_t n = 0; n < fbLen; n = n + 1024)
  {
    if (n + 1024 < fbLen)
    {
      client.write(fbBuf, 1024);
      fbBuf += 1024;
    }
    else if (fbLen % 1024 > 0)
    {
      size_t remainder = fbLen % 1024;
      client.write(fbBuf, remainder);
    }
  }

  client.print(tail);

  long startTimer = millis();
  boolean state = false;

  while ((startTimer + interval) > millis())
  {
    Serial.print(".");
    delay(100);
    while (client.available())
    {
      char c = client.read();
      if (state == true)
        getBody += String(c);
      if (c == '\n')
      {
        if (getAll.length() == 0)
          state = true;
        getAll = "";
      }
      else if (c != '\r')
        getAll += String(c);
      startTimer = millis();
    }
    if (getBody.length() > 0)
      break;
  }
  client.stop();
  Serial.println(getBody);

  return getBody;
}

bool checkAPITelegram()
{
  Serial.println("Connect to " + String(myDomain));
  return client.connect(myDomain, 443);
}

// --- Button ---
void setupButton()
{
  pinMode(buttonPin, INPUT_PULLDOWN);                                                // Set GPIO as Input
  attachInterrupt(digitalPinToInterrupt(buttonPin), handleButtonInterrupt, FALLING); // Interrupt pada saat tombol ditekan (FALLING edge)
}

void IRAM_ATTR handleButtonInterrupt()
{
  is_reset = true;
}

void reset()
{
  esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_EXT1);
  Serial.println("Reset");
  ESP.restart();
}

// --- Radar ---
void setupRadar()
{
  // Set GPIO as Input
  pinMode(radarPin, INPUT_PULLDOWN);
}

void modeSleep()
{
  Serial.println("Masuk ke mode light sleep");
  gpio_wakeup_enable(GPIO_NUM_20, GPIO_INTR_HIGH_LEVEL);
  esp_sleep_enable_gpio_wakeup();
  delay(interval * 10);
  esp_light_sleep_start();
}

void afterConfiguration()
{
  if (disableSleep)
  {
    Serial.println("disable sleep");
    digitalWrite(ledPin, HIGH);
    disableSleep = false;
  }
  Serial.println("sedang medeteksi orang");

  // capture picture
  if (!camera.capture().isOk())
  {
    Serial.println(camera.exception.toString());
    return;
  }

  // run person detection
  if (!personDetection.run(camera).isOk())
  {
    Serial.println(personDetection.exception.toString());
    return;
  }

  // a person has been detected!
  if (personDetection)
  {
    Serial.print("Person detected in ");
    Serial.print(personDetection.tf.benchmark.millis());
    Serial.println("ms");

    Serial.println("kirim pesan condition main 4");

    if (checkAPITelegram())
    {
      Serial.println("mengirim gambar ke telegram");
      sendPhoToTelegram(camera.frame);
    }
    else
    {
      Serial.println("kirim pesan condition main 4");
    }

    disableSleep = true;
    digitalWrite(ledPin, LOW);
    lastTime = millis();
    modeSleep();
  }

  long time = millis();
  if (time >= lastTime + interval * 120)
  {
    disableSleep = true;
    digitalWrite(ledPin, LOW);
    lastTime = time;
    modeSleep();
  }
}
