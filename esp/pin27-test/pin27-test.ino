const int testPin = 27;

void setup()
{
  Serial.begin(9600);

  pinMode(testPin, INPUT_PULLUP);

  Serial.println("Pin 27 sanity check");
  Serial.println("Idle state should read 1. Touch a jumper from this pin to GND and it should read 0.");
}

void loop()
{
  Serial.println(digitalRead(testPin));
  delay(200);
}
