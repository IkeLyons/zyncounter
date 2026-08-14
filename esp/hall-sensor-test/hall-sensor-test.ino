#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>

#define SERVICE_UUID          "96bde720-973d-4f43-820b-0cd2ff8b666c"
#define CHARACTERISTIC_UUID   "d5c94e7e-47e3-484d-897a-ea417b91b77a"

const int ledPin = 2;
const int hallSensorPin = 27;
const int buttonPin = 26;

BLECharacteristic *pCharacteristic;
int lastSensorState = -1;

void setup()
{
  Serial.begin(9600);

  BLEDevice::init("Zyncounter");
  BLEServer *pServer = BLEDevice::createServer();
  BLEService *pService = pServer->createService(SERVICE_UUID);
  pCharacteristic = pService->createCharacteristic(
    CHARACTERISTIC_UUID,
    BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_NOTIFY
  );
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

void loop()
{
  int sensorState = digitalRead(hallSensorPin);

  if(sensorState != lastSensorState) {
    lastSensorState = sensorState;

    if(sensorState == LOW) {
      Serial.println("Magnet Detected");
      digitalWrite(ledPin, HIGH);
      pCharacteristic->setValue("Magnet On");
    } else {
      Serial.println("No Magnet");
      digitalWrite(ledPin, LOW);
      pCharacteristic->setValue("Magnet Off");
    }

    pCharacteristic->notify();
  }

  if(digitalRead(buttonPin) == LOW){
    Serial.println("Button Pressed");
  }

  delay(500);
}