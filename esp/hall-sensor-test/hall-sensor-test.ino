#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>

#define SERVICE_UUID "96bde720-973d-4f43-820b-0cd2ff8b666c"
#define CHARACTERISTIC_UUID "d5c94e7e-47e3-484d-897a-ea417b91b77a"

const int ledPin = 2;
const int hallSensorPin = 27;
const int buttonPin = 26;
int highCount = 0;

BLECharacteristic *pCharacteristic;
int lastSensorState = -1;

class ServerCallbacks : public BLEServerCallbacks {
  void onDisconnect(BLEServer *pServer) {
    Serial.println("Client disconnected, resuming advertising");
    pServer->startAdvertising();
  }
};

void setup() {
  Serial.begin(9600);

  BLEDevice::init("Zyncounter");
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new ServerCallbacks());
  BLEService *pService = pServer->createService(SERVICE_UUID);
  pCharacteristic = pService->createCharacteristic(
    CHARACTERISTIC_UUID,
    BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_NOTIFY);
  pCharacteristic->addDescriptor(new BLE2902());

  pCharacteristic->setValue("Magnet Off");
  pService->start();
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();

  pinMode(hallSensorPin, INPUT);
  pinMode(buttonPin, INPUT_PULLUP);
  pinMode(ledPin, OUTPUT);

  Serial.println("Hall Effect Sensor Test");
}

void loop() {
  int sensorState = digitalRead(hallSensorPin);

  if (sensorState == LOW) {
    highCount = 0;
    digitalWrite(ledPin, HIGH);
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
      digitalWrite(ledPin, LOW);
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