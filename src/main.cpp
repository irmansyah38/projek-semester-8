#include <UniversalTelegramBot.h>
#include <WiFiClientSecure.h>
#include <WebServer.h>
#include <Preferences.h>
#include <WiFiManager.h>
#include <strings_en.h>
#include <wm_consts_en.h>
#include <wm_strings_en.h>
#include <wm_strings_es.h>
#include <Wire.h>
#include "variables.h"
#include "driver/rtc_io.h"
#include "esp_sleep.h"
#include <SPI.h>
#include <tflm_esp32.h>
#include <eloquent_tinyml.h>
#include <eloquent_tinyml/zoo/person_detection.h>
#include <eloquent_esp32cam.h>
#include <LiquidCrystal_I2C.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>
#include <Arduino.h>

using eloq::camera;
using eloq::tinyml::zoo::personDetection;

// kondisi esp
int conditionMain = 0;
bool finishedConfiguration = false;

// wifi
String ip;

// oled
String lastSentance;

// telegram
String botToken;
String chatID;

// disable sleep
bool disableSleep = true;
unsigned long lastTime;

// tombol
int buttonPin = 13;
volatile bool isReset = false; // Flag untuk memeriksa apakah ISR dipicu

// nomor handphone
String nomorHandphone;

// object
WiFiManager wm;
WebServer server(90);
WiFiClientSecure client;
UniversalTelegramBot bot("", client);
Preferences preferences;
LiquidCrystal_I2C lcd(0x27, lcdColumns, lcdRows);
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// wifi
void setupWifi();
void executeWifi();
bool isConnectedWifi();

// nvs
void setupNVS();
bool saveBotTelegram(bool);
bool deleteTelegram();

// website
void setupWebsite();
void executeWebsite();

// lcd
void setupLCD();
void displayLCD(String);

// oled
void setupOLED();
void displayOled(String); // bisa menampung 44 karakter

// telegram
void setupTelegram();
String sendPhoToTelegram(camera_fb_t *);
void executeTelegram();
bool checkTelegram(String);
bool checkAPITelegram();

// sensor pir
// void setupPirSensor();
// void executePirSensor();

// sensor Radar
void setupRadar();

// tombol
void setupButton();
void resetAll();
void IRAM_ATTR handleButtonInterrupt();

// sleep
void modeSleep();

// configuration
void beforeConfiguration();
void afterConfiguration();

void setup()
{
  Serial.begin(115200);
  pinMode(ledPin, OUTPUT);
  // setupLCD();
  // setupOLED();
  setupRadar();
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

  // init tf model
  while (!personDetection.begin().isOk())
    Serial.println(personDetection.exception.toString());

  Serial.println("Camera OK");
  Serial.println("Point the camera to yourself");

  setupWifi();
  setupNVS();
  setupTelegram();

  if (isConnectedWifi())
  {
    Serial.println("Wifi sudah terkoneksi");
    // displayLCD("Sudah terkoneksi dengan Wifi");
    delay(2000);
    if (nomorHandphone != "0")
    {
      finishedConfiguration = true;
      conditionMain = 3;
      Serial.println("Nomor Handphone sudah ada");
      // displayLCD("No HP menggunkan yg sudah ada");
      delay(2000);
      if (botToken != "0" && chatID != "0" && checkTelegram(botToken))
      {
        conditionMain = 4;
        Serial.println("Token & chatID ada");
        // displayLCD("Token & chatID ada");
        delay(2000);
      }
      else
      {
        Serial.println("Token & chatID tidak ada");
        // displayLCD("ada Token & chatID tidak ada");
        delay(2000);
      }
      // lcd.noBacklight();
      // display.dim(true);
    }
    else
    {
      Serial.println("Nomor Handphone belum ada");
      // displayLCD("tidak ada yang disimpan");
      delay(2000);
      setupWebsite();
      conditionMain = 2;
    }
  }
  else
  {
    Serial.println("Wifi belum terkoneksi");
    // displayLCD("belum terkoneksi dengan Wifi");
    delay(2000);
  }
}

void loop()
{
  if (isReset)
  {
    resetAll();
  }

  if (!finishedConfiguration)
  {
    beforeConfiguration();
  }
  else
  {
    afterConfiguration();
  }
}
// --- Wifi ---
void setupWifi()
{

  WiFi.mode(WIFI_STA);
  wm.setConfigPortalBlocking(false);
  wm.setConfigPortalTimeout(60);
  if (wm.autoConnect("SmartCam Wifi"))
  {
    Serial.println("connected...yeey :)");
  }
  else
  {
    Serial.println("Configportal running");
  }
}

void executeWifi()
{
  wm.process();
}

bool isConnectedWifi()
{
  if (WiFi.status() == WL_CONNECTED)
  {
    ip = WiFi.localIP().toString() + ":90";
    return true;
  }
  return false;
}

// --- NVS ---
void setupNVS()
{
  if (preferences.begin("telegram", false))
  {
    botToken = preferences.getString("botToken", "0");
    chatID = preferences.getString("chatID", "0");
    nomorHandphone = preferences.getString("nomorHandphone", "0");

    if (botToken == "0" && chatID == "0")
    {
      Serial.println("Token tidak tersimpan");
    }
    else
    {
      Serial.println("Token ada");
    }
  }
  else
  {
    Serial.println("setup nvs gagal");
  }
  preferences.end();
}

bool saveBotTelegram(bool saveAll)
{
  bool condition = false;
  if (preferences.begin("telegram", false))
  {
    if (saveAll)
    {
      preferences.putString("botToken", botToken);
      preferences.putString("chatID", chatID);
    }
    else
    {
      Serial.println("berhasil menyimpan nomor handphone");
    }
    preferences.putString("nomorHandphone", nomorHandphone);
    condition = true;
  }
  else
  {
    Serial.println("gagal menyimpan telegram");
  }

  preferences.end();
  return condition;
}

bool deleteTelegram()
{
  bool condition = false;
  if (preferences.begin("telegram", false))
  {
    if (conditionMain == 3)
    {
      if (preferences.isKey("nomorHandphone"))
      {
        preferences.remove("nomorHandphone");
        condition = true;
      }
    }
    else if (conditionMain == 4)
    {
      if (preferences.isKey("botToken") && preferences.isKey("chatID") && preferences.isKey("nomorHandphone"))
      {
        preferences.remove("botToken");
        preferences.remove("chatID");
        preferences.remove("nomorHandphone");
        condition = true;
      }
    }
  }
  else
  {
    Serial.println("gagal menghapus telegram");
  }

  preferences.end();
  return condition;
}

// --- Website ---
void handlePageForm()
{
  server.send(200, "text/html", index_html);
}

void handleSubmit()
{
  if (server.hasArg("botToken") && server.hasArg("chatID") && server.hasArg("nomorHandphone") && server.hasArg("onlyHandphone") && server.hasArg("remember"))
  {
    if (server.arg("onlyHandphone") == "true")
    {
      nomorHandphone = server.arg("nomorHandphone");
      if (server.arg("remember") == "true")
      {
        if (saveBotTelegram(false))
        {
          server.send(200, "application/json", "{\"status\": \"1\"}");
        }
        else
        {
          server.send(200, "application/json", "{\"status\": \"2\"}");
        }
      }
      else
      {
        server.send(200, "application/json", "{\"status\": \"3\"}");
      }
      conditionMain = 3;
      server.close();
    }

    else
    {
      String token = server.arg("botToken");
      if (checkTelegram(token))
      {
        botToken = token;
        chatID = server.arg("chatID");
        nomorHandphone = server.arg("nomorHandphone");
        if (server.arg("remember") == "true")
        {
          if (saveBotTelegram(true))
          {
            server.send(200, "application/json", "{\"status\": \"4\"}");
          }
          else
          {
            server.send(200, "application/json", "{\"status\": \"5\"}");
          }
        }
        else
        {
          server.send(200, "application/json", "{\"status\": \"6\"}");
        }
        conditionMain = 4;
        server.close();
      }
      else
      {
        server.send(400, "application/json", "{\"status\": \"7\"}");
      }
    }
  }
  else
  {
    Serial.println("ada argumen yang kosong");
    server.send(500, "application/json", "{\"status\": \"failed server\"}");
  }
}

void setupWebsite()
{
  server.on("/", handlePageForm);
  server.on("/submit", handleSubmit);
  server.begin();
  Serial.println("website dibuka");
}

void executeWebsite()
{
  server.handleClient();
}

// --- Telegram ---
void setupTelegram()
{
  client.setCACert(TELEGRAM_CERTIFICATE_ROOT);
  Serial.println("setup tele sudah");
}

bool checkTelegram(String token)
{
  bot.updateToken(token);
  return bot.getMe();
}

String sendPhoToTelegram(camera_fb_t *fb)
{
  String getAll = "";
  String getBody = "";

  uint8_t *fbBuf;
  size_t fbLen;

  frame2jpg(fb, 80, &fbBuf, &fbLen);

  String head = "--RandomNerdTutorials\r\nContent-Disposition: form-data; name=\"chat_id\"; \r\n\r\n" + chatID + "\r\n--RandomNerdTutorials\r\nContent-Disposition: form-data; name=\"photo\"; filename=\"esp32-cam.jpg\"\r\nContent-Type: image/jpeg\r\n\r\n";
  String tail = "\r\n--RandomNerdTutorials--\r\n";

  size_t imageLen = fbLen;
  size_t extraLen = head.length() + tail.length();
  size_t totalLen = imageLen + extraLen;

  client.println("POST /bot" + botToken + "/sendPhoto HTTP/1.1");
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

// --- Radar ---
void setupRadar()
{
  // Set GPIO as Input
  pinMode(radarPin, INPUT_PULLDOWN);
}

// lcd
void setupLCD()
{
  Wire.begin(41, 42);
  lcd.init();
  lcd.backlight();
}

void displayLCD(String sentance)
{
  if (sentance != lastSentance)
  {
    lastSentance = sentance;
    lcd.clear();
    lcd.print(sentance);
  }
}

// oled
void setupOLED()
{
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(WHITE);
}

void displayOled(String sentance)
{
  if (sentance != lastSentance)
  {
    lastSentance = sentance;
    display.clearDisplay();
    display.setCursor(0, 0);
    display.print(sentance);
    display.display();
  }
}

// reset all
void setupButton()
{
  pinMode(buttonPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(buttonPin), handleButtonInterrupt, FALLING); // Interrupt pada saat tombol ditekan (FALLING edge)
}

void IRAM_ATTR handleButtonInterrupt()
{
  isReset = true;
}

void resetAll()
{

  if (conditionMain >= 3)
  {
    if (deleteTelegram())
    {
      if (conditionMain == 3)
      {
        displayLCD("Berhasil reset nomor HP !!");
      }
      else if (conditionMain == 4)
      {
        displayLCD("Berhasil reset token & no HP !");
      }
    }
  }
  delay(2000);
  wm.resetSettings();
  ESP.restart();
}

void modeSleep()
{
  Serial.println("masuk ke mode light sleep");
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_20, 1);
  delay(interval * 10);
  esp_light_sleep_start();
}

void beforeConfiguration()
{
  if (conditionMain == 0)
  {
    executeWifi();
    Serial.println("hubungkan ke wifi : SmartCam Wifi");
    // displayLCD("Hubungkan ke Wifi:SmartCam Wifi");
    if (isConnectedWifi())
    {
      conditionMain = 1;
      wm.stopWebPortal();
      // displayLCD("Sudah terkoneksi dengan Wifi");
      delay(2000);
    }
  }
  else if (conditionMain == 1)
  {
    if (nomorHandphone != "0")
    {
      finishedConfiguration = true;
      conditionMain = 3;
      // displayLCD("No HP ada di alat");
      Serial.println("No HP ada di alat");
      delay(2000);
      if (botToken != "0" && chatID != "0" && checkTelegram(botToken))
      {
        conditionMain = 4;
        Serial.println("Lalu Token & chatID ada");
        // displayLCD("Lalu Token & chatID ada");
        delay(2000);
      }
      else
      {
        Serial.println("tetapi token & chatID tidak ada");
        // displayLCD("tetapi token & chatID tidak ada");
        delay(2000);
      }

      // lcd.noBacklight();
    }
    else
    {
      // displayLCD("tidak ada yang disimpan di alat");
      Serial.println("tidak ada yang disimpan di alat");
      delay(2000);
      setupWebsite();
      conditionMain = 2;
    }
  }
  else if (conditionMain == 2)
  {
    executeWebsite();
    Serial.println("akses website di : " + ip);
    // displayLCD(ip);
  }
  else if (conditionMain == 3 || conditionMain == 4)
  {
    finishedConfiguration = true;
    lcd.noBacklight();
    modeSleep();
  }
}

void afterConfiguration()
{
  if (disableSleep)
  {
    esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_EXT0);
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

    if (conditionMain == 3)
    {
      Serial.println("kirim pesan condition main 3");
      // if (checkAPITelegram())
      // {
      //   sendPhoToTelegram(camera.frame);
      // }
    }
    else if (conditionMain == 4)
    {
      Serial.println("kirim pesan condition main 4");

      if (checkAPITelegram())
      {
        Serial.println("mengirim gambar ke telegram");
        sendPhoToTelegram(camera.frame);
      }
      else
      {
        Serial.println("kirim pesan condition main 4");
        // displayLCD("Gagal kirim gambar coba SMS");
      }
    }

    disableSleep = true;
    digitalWrite(ledPin, LOW);
    lastTime = millis();
    modeSleep();
  }

  long time = millis();
  if (time >= lastTime + interval * 120)
  {
    digitalWrite(ledPin, LOW);
    lastTime = time;
    modeSleep();
  }
}