#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>
#include <sys/time.h>

#define SERVICE_UUID "96bde720-973d-4f43-820b-0cd2ff8b666c"
#define CHARACTERISTIC_UUID "d5c94e7e-47e3-484d-897a-ea417b91b77a"
#define TIME_CHARACTERISTIC_UUID "7677590e-7808-4e26-84e1-da269b480206"
#define MAX_EVENTS 200

const int ledPin = 2;
const int hallSensorPin = 27;
const int buttonPin = 26;
int highCount = 0;

BLECharacteristic *pCharacteristic;
int lastSensorState = -1;

RTC_DATA_ATTR time_t popLog[MAX_EVENTS];
RTC_DATA_ATTR int popCount = 0;
int eventReadCursor = 0;
bool timeIsSynced = false;
bool ignoreNextEvent = false;

void printPopLog() {
  Serial.println(popCount);
  for (int i = 0; i < popCount; i++) {
    Serial.print(popLog[i]);
    Serial.print(" ");
  }
  Serial.println();
}

class ServerCallbacks : public BLEServerCallbacks {
  void onDisconnect(BLEServer *pServer) {
    Serial.println("Client disconnected, resuming advertising");
    digitalWrite(ledPin, HIGH);
    pServer->startAdvertising();
  }

  void onConnect(BLEServer *pServer) {
    Serial.println("Client connected");
    digitalWrite(ledPin, LOW);
    eventReadCursor = 0;
  }
};

void sendNextEvent(BLECharacteristic *pCharacteristic) {
  if (eventReadCursor >= popCount) {
    eventReadCursor = 0;
    int64_t empty = 0;
    pCharacteristic->setValue((uint8_t *)&empty, sizeof(empty));
    return;
  }

  int64_t nextEvent = popLog[eventReadCursor];
  eventReadCursor++;
  pCharacteristic->setValue((uint8_t *)&nextEvent, sizeof(nextEvent));
}

class EventsCharacteristicCallbacks : public BLECharacteristicCallbacks {
  void onRead(BLECharacteristic *pCharacteristic) {
    sendNextEvent(pCharacteristic);
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
    timeIsSynced = true;
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
    BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);
  pCharacteristic->addDescriptor(new BLE2902());
  pCharacteristic->setCallbacks(new EventsCharacteristicCallbacks());

  BLECharacteristic *pTimeCharacteristic = pService->createCharacteristic(
    TIME_CHARACTERISTIC_UUID,
    BLECharacteristic::PROPERTY_WRITE);
  pTimeCharacteristic->setCallbacks(new TimeCharacteristicCallbacks());

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
  bool buttonPressed = digitalRead(buttonPin) == LOW;

  if (buttonPressed) {
    ignoreNextEvent = true;
  }

  if (sensorState == LOW) {
    highCount = 0;
    if (sensorState != lastSensorState) {
      Serial.println("Magnet Detected");
    }
    lastSensorState = 0;
  } else {
    highCount++;
    if (highCount == 0) {
      Serial.println("First detected magnet absence");
    }
    if (highCount == 5) {
      Serial.println("No Magnet");

      if (ignoreNextEvent) {
        Serial.println("Event ignored due to button press");
        ignoreNextEvent = false;
      } else if (!timeIsSynced) {
        Serial.println("Time not synced yet, dropping event");
      } else if (popCount < MAX_EVENTS) {
        time_t newEvent = time(nullptr);
        Serial.println(newEvent);
        popLog[popCount++] = newEvent;
        printPopLog();
        pCharacteristic->setValue((uint8_t *)&newEvent, sizeof(newEvent));
        pCharacteristic->notify();
        eventReadCursor = popCount;
      }
    }
    lastSensorState = 1;
  } 

  if (buttonPressed) {
    Serial.println("Button Pressed");
  }

  delay(100);
}