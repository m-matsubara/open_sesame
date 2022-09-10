#define FASTLED_INTERNAL

#include <M5Atom.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Setting.h>

const String URL = "https://api.candyhouse.co/public/sesame/" + DEVICE_ID;

// ボタン短押しの境界時間(ms)
const uint32_t BUTTON_SHORT_PRESS_THRESHOLD = 30;
// ボタン長押しの境界時間(ms)
const uint32_t BUTTON_LONG_PRESS_THRESHOLD = 3000;

// Wi-Fi 接続で帰宅判定の時間しきい値(ms)
const uint32_t GOING_HOME_THRESHOLD = 600000;

// 帰宅判定時の開錠タイマー(ms)
const uint32_t UNLOCK_TIMER = 5000;

// loop処理の時刻（loop()関数の中で更新）
uint32_t timeValue = millis();

// ボタンが押されているか
boolean btnPressFlag = false;
// ボタンが押下された時刻
uint32_t timeBtnPressed = 0;

// ロック状態(ロック=true)
boolean lockStatus = false;

// 帰宅判定フラグ
boolean goingHomeFlag = false;
// 帰宅時刻
uint32_t timeGoingHome = 0;

// JSON の解析用
DynamicJsonDocument doc(10240);


/**
 * ロック状態取得
 */
boolean getLockState() {
  HTTPClient http;
  http.begin(URL);
  http.addHeader("Authorization", API_KEY);
  http.addHeader("Content-Type", "application/json");

  http.GET();
  String jsonString = http.getString();
  http.end();
  deserializeJson(doc,  jsonString);
  Serial.println(jsonString);
  String locked = doc["locked"];
  Serial.println(locked);
  return (locked != "false");
}


/**
 * 施錠/開錠
 **/ 
void lock(boolean lockFlag) {
  if (lockFlag)
    M5.dis.drawpix(0, CRGB(100, 0, 0));
  else
    M5.dis.drawpix(0, CRGB(0, 100, 0));
  HTTPClient http;
  http.begin(URL);
  http.addHeader("Authorization", API_KEY);
  http.addHeader("Content-Type", "application/json");

  int httpState;
  if (lockFlag) {
    httpState = http.POST("{\"command\": \"lock\"}");
    Serial.println("Close SESAME.");
  } else { 
    httpState = http.POST("{\"command\": \"unlock\"}");
    Serial.println("Open SESAME.");
  }
  http.end();

  delay(2000);
  M5.dis.drawpix(0, CRGB(0, 0, 0));
  lockStatus = lockFlag;
}


/**
 * Wi-Fi に接続する
 */
void connectWifi() {
  int counter = 0;
  Serial.print("Wi-Fi Connecting.");
  while (WiFi.status() != WL_CONNECTED) {
    if (counter % 5 == 0) {
      WiFi.disconnect();
      WiFi.begin(SSID.c_str(), SSID_PASSWORD.c_str());
    }
    Serial.print(".");
    M5.dis.drawpix(0, CRGB(100, 100, 0));
    delay(100);
    M5.dis.drawpix(0, CRGB(0, 0, 0));
    delay(900);
    counter++;
  }
  Serial.println("Wi-Fi Connected.");
}


void setup() {
  setCpuFrequencyMhz(80);
  M5.begin(true, false, true);
  Serial.begin(115200);
  M5.dis.drawpix(0, CRGB(0, 100, 0));
  delay(1000);
  M5.dis.drawpix(0, CRGB(0, 0, 0));

  Serial.println("Open SESAME v0.1 Copyright (C) m.matsubara");
  Serial.println();
}


void loop() {
  // Wifiが切れていたら接続
  if (WiFi.status() != WL_CONNECTED) {
    uint32_t timeConnectStart = millis();  // 接続開始時刻
    connectWifi();  // 接続できない間、ずっと処理を戻さない
    uint32_t timeConnectEnd = millis();  // 接続完了時刻
    M5.dis.drawpix(0, CRGB(100, 100, 0));
    lockStatus = getLockState();
    M5.dis.drawpix(0, CRGB(0, 0, 0));

    // 接続に時間がかかった（＝家から離れていたが戻ってきた）とき、鍵がかかっていたら、帰宅フラグをONにする。
    if (timeConnectEnd - timeConnectStart >= GOING_HOME_THRESHOLD && lockStatus != false) {
      goingHomeFlag = true;
      timeGoingHome = timeConnectEnd;
    }
  }

  // 処理時刻の更新
  timeValue = millis();

  M5.update();

  if (goingHomeFlag) {
    // 帰宅判定時はみじかく点灯
    if ((timeValue % 1000) < 500) {
      M5.dis.drawpix(0, CRGB(0, 100, 0));
    } else {
      M5.dis.drawpix(0, CRGB(0, 0, 0));
    }
  } else {
    // 施錠時は赤、開錠時は緑でゆっくり点滅
    if ((timeValue % 3000) < 100) {
      if (lockStatus)
        M5.dis.drawpix(0, CRGB(20, 0, 0));
      else
        M5.dis.drawpix(0, CRGB(0, 20, 0));
    } else {
      M5.dis.drawpix(0, CRGB(0, 0, 0));
    }
  }

  // 帰宅フラグ=ONで一定時間たつと、開錠する。
  if (goingHomeFlag && (timeValue - timeGoingHome >= UNLOCK_TIMER)) {
    goingHomeFlag = false;
    lock(false);
  }

  // ボタン押下の時刻を記録
  if (M5.Btn.wasPressed()) {
    btnPressFlag = true;
    timeBtnPressed = timeValue;
  }
  
  // ボタン短く押下
  if (M5.Btn.wasReleased()) {
    btnPressFlag = false;
    if (goingHomeFlag) {
      // 帰宅フラグON の時、OFFに変更
      goingHomeFlag = false;
    } else {
      // 開錠
      if ((timeValue - timeBtnPressed >= BUTTON_SHORT_PRESS_THRESHOLD) && (timeValue - timeBtnPressed < BUTTON_LONG_PRESS_THRESHOLD)) {
        lock(false);
      }
    }
  }
  
  // 長押しで施錠(M5.Btn.wasReleased() を待たない)
  if ((btnPressFlag) && (timeValue - timeBtnPressed >= BUTTON_LONG_PRESS_THRESHOLD)) {
    lock(true);
    goingHomeFlag = false;  // いちおう

    // ボタンを離すまで待つ
    while (M5.Btn.read() != 0) {
      M5.update();
      delay(10);
    }
  }
  delay(10);
}