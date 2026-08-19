#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>
#include <sys/time.h>

#define SERVICE_UUID "96bde720-973d-4f43-820b-0cd2ff8b666c"
#define CHARACTERISTIC_UUID "d5c94e7e-47e3-484d-897a-ea417b91b77a"
#define TIME_CHARACTERISTIC_UUID "7677590e-7808-4e26-84e1-da269b480206"

const int ledPin = 2;
const int hallSensorPin = 27;
const int buttonPin = 26;
int highCount = 0;

BLECharacteristic *pCharacteristic;
int lastSensorState = -1;

class ServerCallbacks : public BLEServerCallbacks {
  void onDisconnect(BLEServer *pServer) {
    Serial.println("Client disconnected, resuming advertising");
    digitalWrite(ledPin, HIGH);
    pServer->startAdvertising();
  }

  void onConnect(BLEServer *pServer) {
    Serial.println("Client connected");
    digitalWrite(ledPin, LOW);
  }
};

class TimeCharacteristicCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pTimeCharacteristic) {
    if (pTimeCharacteristic->getLength() != sizeof(int64_t)) {
      return;
    }

    int64_t epochSeconds;
    memcpy(&epochSeconds, pTimeCharacteristic->getData(), sizeof(epochSeconds));
    struct timeval tv = { .tv_sec = epochSeconds, .tv_usec = 0 };
    settimeofday(&tv, NULL);
    Serial.printf("System time set to %lld\n", (long long)epochSeconds);
  }
};

void setupBLE() {
  BLEDevice::init("Zyncounter");
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new ServerCallbacks());
  BLEService *pService = pServer->createService(SERVICE_UUID);
  pCharacteristic = pService->createCharacteristic(
    CHARACTERISTIC_UUID,
    BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_NOTIFY);
  pCharacteristic->addDescriptor(new BLE2902());

  BLECharacteristic *pTimeCharacteristic = pService->createCharacteristic(
    TIME_CHARACTERISTIC_UUID,
    BLECharacteristic::PROPERTY_WRITE);
  pTimeCharacteristic->setCallbacks(new TimeCharacteristicCallbacks());

  pCharacteristic->setValue("Magnet Off");
  pService->start();
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();
}

void setup() {
  Serial.begin(9600);
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, HIGH);
  delay(2000);

  setupBLE();

  pinMode(hallSensorPin, INPUT);
  pinMode(buttonPin, INPUT_PULLUP);
  Serial.println("Hall Effect Sensor Test");
}

void loop() {
  int sensorState = digitalRead(hallSensorPin);

  if (sensorState == LOW) {
    highCount = 0;
    pCharacteristic->setValue("Magnet On");
    if (sensorState != lastSensorState) {
      Serial.println("Magnet Detected");
      pCharacteristic->notify();
    }
    lastSensorState = 0;
  } else {
    highCount++;
    if (highCount == 0) {
      Serial.println("First detected magnet absence");
    }
    if (highCount == 5) {
      Serial.println("No Magnet");
      pCharacteristic->setValue("Magnet Off");
      pCharacteristic->notify();
    }
    lastSensorState = 1;
  } 

  if (digitalRead(buttonPin) == LOW) {
    Serial.println("Button Pressed");
  }

  delay(100);
}