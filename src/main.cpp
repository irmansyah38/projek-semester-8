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
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>

using eloq::camera;
using eloq::tinyml::zoo::personDetection;

// object
WiFiManager wm;
WebServer server(90);
WiFiClientSecure client;
UniversalTelegramBot bot("", client);
Preferences preferences;
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// wifi
bool setupWifi();

// nvs
int setupNVS();
bool saveBotTelegram(bool);
bool deleteTelegram();

// website
void setupWebsite();
void executeWebsite();

// oled
void setupOLED();
void displayOled(String); // bisa menampung 44 karakter

// telegram
void setupTelegram();
String sendPhoToTelegram(camera_fb_t *);
bool checkTelegram(String);
bool checkAPITelegram();

// sensor Radar
void setupRadar();

// tombol
void setupButton();
void resetAll();
void IRAM_ATTR handleButtonInterrupt();

// sleep
void modeSleep();

void mainProgram();

void setup()
{
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  setupRadar();
  setupOLED();
  displayOled("OLED OK !");
  delay(5000);

  // mengecek data yang tersimpan
  int kondisi_penyimpanan = setupNVS();
  if (kondisi_penyimpanan == 0)
  {
    Serial.println("Token & NO HP tidak ada");
    displayOled("Token &   NO HP     tidak ada");
  }
  else if (kondisi_penyimpanan == 3)
  {
    Serial.println("NO HP sudah ada");
    displayOled("NO HP     sudah ada");
    condition_main = 3;
  }
  else if (kondisi_penyimpanan == 4)
  {
    Serial.println("Token & NO HP sudah ada");
    displayOled("Token &   NO HP     sudah ada");
    condition_main = 4;
  }

  delay(5000);
  if (condition_main >= 3)
  {
    setupButton();
    displayOled("Tekan     reset");
    Serial.println("Tekan reset");
    delay(2000);
    if (condition_main == 3)
    {
      displayOled("Untuk     menghapus nomor     handphone");
      Serial.println("Untuk menghapus nomor handphone");
    }
    else if (condition_main == 4)
    {
      displayOled("Untuk     menghapus nomor HP &telegram");
      Serial.println("Untuk menghapus nomor HP & telegram");
    }
    last_time = millis();
    while (true)
    {
      int buttonState = digitalRead(Button_PIN);
      if (buttonState == HIGH)
      {
        resetAll();
      }

      if (millis() - last_time > INTERVAL * 20)
      {
        break;
      }
    }
  }

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
  displayOled("Camera    OK !");
  delay(5000);

  // init tf model
  while (!personDetection.begin().isOk())
    Serial.println(personDetection.exception.toString());
  Serial.println("Model OK");

  if (condition_main != 3)
  {
    displayOled("Akses ke: SmartCam  Wifi");
    if (setupWifi())
    {
      Serial.println("terhubung wifi");
      displayOled("terhubung wifi");
      ip = WiFi.localIP().toString() + ":90";
    }
    else
    {
      Serial.println("tidak terhubung wifi");
      displayOled("Gagal    coba lagi");
    }
    delay(5000);
    setupTelegram();
    if (condition_main != 4)
    {
      setupWebsite();
      while (web_condition)
      {
        displayOled("Akses ke: " + ip);
        executeWebsite();
      }
    }
  }
  displayOled("Selesai & masuk     sleep");
  delay(5000);
  display.dim(true);
  last_time = millis();
  modeSleep();
}

void loop()
{
  mainProgram();
}
// --- Wifi ---
bool setupWifi()
{
  WiFi.mode(WIFI_STA);
  bool res;
  res = wm.autoConnect("SmartCam Wifi");
  return res;
}

// --- NVS ---
int setupNVS()
{
  int condition = 0;
  if (preferences.begin("telegram", false))
  {
    nomor_handphone = preferences.getString("nomor_handphone", "0");
    bot_token = preferences.getString("bot_token", "0");
    chat_id = preferences.getString("chat_id", "0");

    if (nomor_handphone != "0")
    {
      condition = 3;

      if (bot_token != "0" && chat_id != "0")
      {
        condition = 4;
      }
    }
  }
  else
  {
    condition = -1;
  }
  preferences.end();
  return condition;
}

bool saveBotTelegram(String nomor_handphone, String token = "", String chat_id = "")
{
  bool condition = false;
  if (preferences.begin("telegram", false))
  {
    preferences.putString("nomor_handphone", nomor_handphone);

    if (token != "" && chat_id != "")
    {
      preferences.putString("bot_token", token);
      preferences.putString("chat_id", chat_id);
    }
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
    preferences.clear();
    condition = true;
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
      nomor_handphone = server.arg("nomorHandphone");
      if (server.arg("remember") == "true")
      {
        if (saveBotTelegram(nomor_handphone))
        {
          server.send(200, "application/json", "{\"status\": \"1\"}");
          displayOled("");
          displayOled("Nomor HP  ada dan   menyimpan");
        }
        else
        {
          server.send(200, "application/json", "{\"status\": \"2\"}");
          displayOled("Nomor HP  ada dan   gagal     menyimpan");
        }
      }
      else
      {
        server.send(200, "application/json", "{\"status\": \"3\"}");
        displayOled("Nomor HP  ada dan   tidak     menyimpan");
      }
      web_condition = false;
      condition_main = 3;
      server.close();
    }

    else
    {
      String token = server.arg("botToken");
      if (checkTelegram(token))
      {
        bot_token = token;
        chat_id = server.arg("chatID");
        nomor_handphone = server.arg("nomorHandphone");
        if (server.arg("remember") == "true")
        {
          if (saveBotTelegram(nomor_handphone, bot_token, chat_id))
          {
            server.send(200, "application/json", "{\"status\": \"4\"}");
            displayOled("Token &   Nomor HP  menyimpan");
          }
          else
          {
            server.send(200, "application/json", "{\"status\": \"5\"}");
            displayOled("Token &   Nomor HP  gagal     menyimpan");
          }
        }
        else
        {
          server.send(200, "application/json", "{\"status\": \"6\"}");
          displayOled("Token &   Nomor HP  tidak     menyimpan");
        }
        web_condition = false;
        condition_main = 4;
        server.close();
      }
      else
      {
        server.send(400, "application/json", "{\"status\": \"7\"}");
        displayOled("Token &   Nomor HP  gagal");
      }
    }
  }
  else
  {
    Serial.println("ada argumen yang kosong");
    server.send(500, "application/json", "{\"status\": \"failed server\"}");
    displayOled("Coba lagi atau      restart");
  }
  delay(5000);
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

  String head = "--RandomNerdTutorials\r\nContent-Disposition: form-data; name=\"chat_id\"; \r\n\r\n" + chat_id + "\r\n--RandomNerdTutorials\r\nContent-Disposition: form-data; name=\"photo\"; filename=\"esp32-cam.jpg\"\r\nContent-Type: image/jpeg\r\n\r\n";
  String tail = "\r\n--RandomNerdTutorials--\r\n";

  size_t imageLen = fbLen;
  size_t extraLen = head.length() + tail.length();
  size_t totalLen = imageLen + extraLen;

  client.println("POST /bot" + bot_token + "/sendPhoto HTTP/1.1");
  client.println("Host: " + String(TELEGRAM_DOMAIN));
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

  while ((startTimer + INTERVAL) > millis())
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
  Serial.println("Connect to " + String(TELEGRAM_DOMAIN));
  return client.connect(TELEGRAM_DOMAIN, 443);
}

// --- Radar ---
void setupRadar()
{
  // Set GPIO as Input
  pinMode(RADAR_PIN, INPUT_PULLDOWN);
}

// oled
void setupOLED()
{
  Wire.begin(41, 42);
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C))
  { // Address 0x3D for 128x64
    Serial.println(F("SSD1306 allocation failed"));
    for (;;)
      ;
  }
  delay(2000);
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(WHITE);
}

void displayOled(String sentance)
{
  if (sentance != last_sentance)
  {
    last_sentance = sentance;
    display.clearDisplay();
    display.setCursor(0, 0);
    display.print(sentance);
    display.display();
  }
}

// reset all
void setupButton()
{
  pinMode(Button_PIN, INPUT);
}

void resetAll()
{
  if (condition_main >= 3)
  {
    deleteTelegram();
    if (condition_main == 3)
    {
      displayOled("Nomor      handphone berhasil  dihapus");
      Serial.println("Nomor handphone berhasil  dihapus");
    }
    else if (condition_main == 4)
    {
      displayOled("Nomor  &  telegram  berhasil  dihapus");
      Serial.println("Nomor & telegram berhasil dihapus");
    }
  }
  delay(5000);
  displayOled("Reset wifi");
  Serial.println("Reset wifi");
  wm.resetSettings();
  displayOled("Restarting...");
  Serial.println("Restarting...");
  delay(3000);
  ESP.restart();
}

void modeSleep()
{
  Serial.println("Masuk ke mode light sleep");
  gpio_wakeup_enable(RADAR_PIN, GPIO_INTR_HIGH_LEVEL);
  esp_sleep_enable_gpio_wakeup();
  delay(INTERVAL * 10);
  esp_light_sleep_start();
}

void mainProgram()
{
  Serial.println("sedang medeteksi orang");
  digitalWrite(LED_PIN, HIGH);
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

    if (condition_main == 3)
    {
      Serial.println("kirim sms condition main 3");
    }
    else if (condition_main == 4)
    {
      Serial.println("kirim pesan condition main 4");

      if (checkAPITelegram())
      {
        Serial.println("mengirim gambar ke telegram");
        sendPhoToTelegram(camera.frame);
      }
      else
      {
        Serial.println("kirim sms condition main 4");
      }
    }
    digitalWrite(LED_PIN, LOW);
    last_time = millis();
    modeSleep();
  }

  long time = millis();
  if (time >= last_time + INTERVAL * 120)
  {
    digitalWrite(LED_PIN, LOW);
    last_time = time;
    modeSleep();
  }
}
