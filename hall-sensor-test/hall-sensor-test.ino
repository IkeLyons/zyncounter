const int ledPin = 2;
const int hallSensorPin = 27;

void setup()
{
  Serial.begin(9600);

  pinMode(hallSensorPin, INPUT_PULLUP);
  pinMode(ledPin, OUTPUT);

  Serial.println("Hall Effect Sensor Test");
}

void loop()
{
  int sensorState = digitalRead(hallSensorPin);

  if(sensorState == LOW) {
    Serial.println("Magnet Detected");
    digitalWrite(ledPin, HIGH);
  } else {
    Serial.println("No Magnet");
    digitalWrite(ledPin, LOW);
  }

  delay(500);
}