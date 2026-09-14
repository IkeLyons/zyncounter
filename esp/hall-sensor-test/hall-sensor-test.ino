#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>
#include <sys/time.h>
#include <esp_sleep.h>

#define SERVICE_UUID "96bde720-973d-4f43-820b-0cd2ff8b666c"
#define CHARACTERISTIC_UUID "d5c94e7e-47e3-484d-897a-ea417b91b77a"
#define TIME_CHARACTERISTIC_UUID "7677590e-7808-4e26-84e1-da269b480206"
#define ACK_CHARACTERISTIC_UUID "1b1e6e3a-8f36-4c7b-9a2b-7a6e2d9c4f10"
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
RTC_DATA_ATTR bool timeIsSynced = false;
bool ignoreNextEvent = false;

const unsigned long AWAKE_DURATION_MS = 60000;
unsigned long wakeMillis;

portMUX_TYPE stateMux = portMUX_INITIALIZER_UNLOCKED;

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
    wakeMillis = millis();
  }
};

void sendNextEvent(BLECharacteristic *pCharacteristic) {
  int64_t nextEvent = 0;

  portENTER_CRITICAL(&stateMux);
  if (eventReadCursor < popCount) {
    nextEvent = popLog[eventReadCursor];
    eventReadCursor++;
  } else {
    eventReadCursor = 0;
  }
  portEXIT_CRITICAL(&stateMux);

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

class AckCharacteristicCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pAckCharacteristic) {
    if (pAckCharacteristic->getLength() != sizeof(int64_t)) {
      return;
    }

    int64_t ackedCount;
    memcpy(&ackedCount, pAckCharacteristic->getData(), sizeof(ackedCount));
    if (ackedCount <= 0) {
      return;
    }

    int remaining;
    portENTER_CRITICAL(&stateMux);
    remaining = ackedCount >= popCount ? 0 : popCount - ackedCount;
    for (int i = 0; i < remaining; i++) {
      popLog[i] = popLog[ackedCount + i];
    }
    popCount = remaining;
    eventReadCursor = eventReadCursor > ackedCount ? eventReadCursor - ackedCount : 0;
    portEXIT_CRITICAL(&stateMux);

    Serial.printf("Acked %lld events, %d remaining\n", (long long)ackedCount, remaining);
    wakeMillis = millis();
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

  BLECharacteristic *pAckCharacteristic = pService->createCharacteristic(
    ACK_CHARACTERISTIC_UUID,
    BLECharacteristic::PROPERTY_WRITE);
  pAckCharacteristic->setCallbacks(new AckCharacteristicCallbacks());

  pService->start();
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();
}

void logWakeReason() {
  switch (esp_sleep_get_wakeup_cause()) {
    case ESP_SLEEP_WAKEUP_EXT0:
      Serial.println("Woke up: hall sensor");
      break;
    case ESP_SLEEP_WAKEUP_EXT1:
      Serial.println("Woke up: button");
      break;
    default:
      Serial.println("Fresh boot");
      break;
  }
}

void setup() {
  Serial.begin(9600);
  logWakeReason();

  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, HIGH);
  pinMode(hallSensorPin, INPUT);
  pinMode(buttonPin, INPUT_PULLUP);

  setupBLE();

  wakeMillis = millis();
  Serial.println("Hall Effect Sensor Test");
}

void goToSleep() {
  Serial.println("Waiting for sensor/button to clear before sleeping...");
  while (digitalRead(hallSensorPin) == HIGH || digitalRead(buttonPin) == LOW) {
    delay(100);
  }

  Serial.println("Going to sleep");
  Serial.flush();

  esp_sleep_enable_ext0_wakeup((gpio_num_t)hallSensorPin, 1);
  esp_sleep_enable_ext1_wakeup(1ULL << buttonPin, ESP_EXT1_WAKEUP_ALL_LOW);
  esp_deep_sleep_start();
}

void loop() {
  int sensorState = digitalRead(hallSensorPin);
  bool buttonPressed = digitalRead(buttonPin) == LOW;

  if (buttonPressed) {
    ignoreNextEvent = true;
    wakeMillis = millis();
  }

  if (sensorState == LOW) {
    highCount = 0;
    if (sensorState != lastSensorState) {
      Serial.println("Magnet Detected");
      wakeMillis = millis();
    }
    lastSensorState = 0;
  } else {
    highCount++;
    if (highCount == 0) {
      Serial.println("First detected magnet absence");
      wakeMillis = millis();
    }
    if (highCount == 5) {
      Serial.println("No Magnet");
      wakeMillis = millis();

      if (ignoreNextEvent) {
        Serial.println("Event ignored due to button press");
        ignoreNextEvent = false;
      } else if (!timeIsSynced) {
        Serial.println("Time not synced yet, dropping event");
      } else {
        time_t newEvent = time(nullptr);
        bool logged = false;

        portENTER_CRITICAL(&stateMux);
        if (popCount < MAX_EVENTS) {
          popLog[popCount] = newEvent;
          popCount++;
          eventReadCursor = popCount;
          logged = true;
        }
        portEXIT_CRITICAL(&stateMux);

        if (logged) {
          Serial.println(newEvent);
          printPopLog();
          pCharacteristic->setValue((uint8_t *)&newEvent, sizeof(newEvent));
          pCharacteristic->notify();
        }
      }
    }
    lastSensorState = 1;
  } 

  if (buttonPressed) {
    Serial.println("Button Pressed");
  }

  if (millis() - wakeMillis >= AWAKE_DURATION_MS) {
    goToSleep();
  }

  delay(100);
}