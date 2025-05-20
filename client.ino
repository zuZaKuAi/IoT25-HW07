#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEClient.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <cmath>  // 거리 추정용

#define LED_PIN 2
// OLED 설정
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// BLE UUID 설정
static BLEUUID serviceUUID("12345678-1234-1234-1234-1234567890ab");
static BLEUUID charUUID("abcd1234-ab12-cd34-ef56-abcdef123456");

static bool doConnect = false;
static bool connected = false;
static BLEAdvertisedDevice* myDevice = nullptr;
static BLEClient* pClient = nullptr;

// 거리 추정 함수 (n=2.0, txPower=-59 기준)
float estimateDistance(int rssi, int txPower = -72, float n = 3.70) {
  return pow(10.0, ((float)txPower - rssi) / (10.0 * n));
}



// BLE 알림 콜백 (필요 시)
static void notifyCallback(
  BLERemoteCharacteristic* pBLERemoteCharacteristic,
  uint8_t* pData, size_t length, bool isNotify) {
    // 수신 데이터 무시하거나 시각화 용도 사용 가능
}

// 서버 연결 함수
bool connectToServer() {
  Serial.print("Connecting to ");
  Serial.println(myDevice->getAddress().toString().c_str());

  pClient = BLEDevice::createClient();
  if (!pClient->connect(myDevice)) {
    Serial.println("Failed to connect");
    return false;
  }

  Serial.println(" - Connected to server");
  connected = true;
  return true;
}

// 광고 수신 콜백
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    Serial.print("Found device: ");
    Serial.println(advertisedDevice.toString().c_str());

    if (advertisedDevice.haveServiceUUID() &&
        advertisedDevice.isAdvertisingService(serviceUUID)) {
      BLEDevice::getScan()->stop();
      myDevice = new BLEAdvertisedDevice(advertisedDevice);
      doConnect = true;

      int rssi = advertisedDevice.getRSSI();
      float distance = estimateDistance(rssi);

      Serial.print("RSSI: ");
      Serial.print(rssi);
      Serial.print(" dBm -> Distance: ");
      Serial.print(distance);
      Serial.println(" m");

      // OLED 출력
      display.clearDisplay();
      display.setCursor(0, 0);
      display.setTextSize(1);
      display.setTextColor(SSD1306_WHITE);
      display.println("BLE RSSI Scan:");
      display.print("RSSI: ");
      display.print(rssi);
      display.println(" dBm");

      display.print("Est. Dist: ");
      display.print(distance, 2);
      display.println(" m");

      display.display();
    }
  }
};

void setup() {
  Serial.begin(115200);

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  // OLED 초기화
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("SSD1306 allocation failed");
    while (true);
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("Scanning for BLE...");
  display.display();

  BLEDevice::init("");
  BLEScan* pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setInterval(1349);
  pBLEScan->setWindow(449);
  pBLEScan->setActiveScan(true);
  pBLEScan->start(5, false);
}

void loop() {
  // 연결 요청 처리
  if (doConnect && !connected) {
    if (connectToServer()) {
      display.clearDisplay();
      display.setCursor(0, 0);
      display.setTextSize(1);
      display.setTextColor(SSD1306_WHITE);
      display.println("Connected!");
      display.display();
      delay(1000);
    } else {
      display.clearDisplay();
      display.setCursor(0, 0);
      display.println("Connect Failed");
      display.display();
      delay(2000);
    }
    doConnect = false;
  }

  // 연결 유지 시 실시간 거리 출력
  if (connected && pClient->isConnected()) {
    int rssi = pClient->getRssi();
    float distance = estimateDistance(rssi);

    Serial.print("RSSI: ");
    Serial.print(rssi);
    Serial.print(" dBm -> Distance: ");
    Serial.print(distance);
    Serial.println(" m");

    // OLED 출력
    display.clearDisplay();
    display.setCursor(0, 0);
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.println("RSSI & Distance:");
    display.print("RSSI: ");
    display.print(rssi);
    display.println(" dBm");

    display.print("Est. Dist: ");
    display.print(distance, 2);
    display.println(" m");

    display.display();

    if (distance < 1.0) {
    digitalWrite(LED_PIN, HIGH);
    delay(100);
    digitalWrite(LED_PIN, LOW);
    delay(100);
  } else {
    digitalWrite(LED_PIN, LOW);  // 범위 밖이면 꺼짐
    delay(1800);  // 남은 딜레이 (총 2초 간격 유지)
  }
  }

  

  // 연결 끊김 감지 및 재스캔
  if (connected && !pClient->isConnected()) {
    Serial.println("Lost connection. Reconnecting...");
    connected = false;
    doConnect = true;
    BLEDevice::getScan()->start(5, false);
  }

  //delay(1000);  // 2초마다 거리 측정
}
