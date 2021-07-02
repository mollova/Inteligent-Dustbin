#include <ESP32Servo.h>
#include <WiFi.h>
#include <Arduino.h>
#include <analogWrite.h>

#define DRV_A_IN1 26
#define DRV_A_IN2 12
#define DRV_A_PWM 14

#define DRV_B_IN1 32
#define DRV_B_IN2 27
#define DRV_B_PWM 25

const char* ssid = "Xiaomi Mi A2 lite";
const char* password = "Mmagito2";

WiFiServer server(80); // Port 80

int wait30 = 30000; // time to reconnect when connection is lost.

int coordinates [3];

const int CLOCKWISE = -1;
const int COUNTER_CLOCKWISE = 1;
const int NONE = 0;

int motorSpeedA = 120;
int motorSpeedB = 120;

#define echoPinFull 5
#define trigPinFull 4
#define ledPinFull 23

#define echoPinOpen 2
#define trigPinOpen 15
#define servoPinOpen 19

const unsigned long DURATION = 8000;
const int LOOP_DELAY = 500;

Servo servo;

bool isFull;

bool isOff = true;

void setPins () {
  pinMode(DRV_A_IN1, OUTPUT);
  pinMode(DRV_A_IN2, OUTPUT);
  pinMode(DRV_A_PWM, OUTPUT);
  
  pinMode(DRV_B_IN1, OUTPUT);
  pinMode(DRV_B_IN2, OUTPUT);
  pinMode(DRV_B_PWM, OUTPUT);

  pinMode(trigPinFull, OUTPUT);
  pinMode(echoPinFull, INPUT);
  pinMode(ledPinFull, OUTPUT);

  pinMode(trigPinOpen, OUTPUT);
  pinMode(echoPinOpen, INPUT);
}

void setup() {
  Serial.begin(115200);

  setPins();

// Connect WiFi net.
  Serial.println();
  Serial.print("Connecting with ");
  Serial.println(ssid);
 
  WiFi.begin(ssid, password);
 
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Connected with WiFi.");
 
  // Start Web Server.
  server.begin();
  Serial.println("Web Server started.");
 
  Serial.print("This is IP to connect to the WebServer: ");
  Serial.print("http://");
  Serial.println(WiFi.localIP());

  servo.setPeriodHertz(50);
  servo.attach(servoPinOpen, 500, 2400);

  isFull = false;
}

void loop() {
  controlFull();
  controlOpen();
  
  // If disconnected, try to reconnect every 30 seconds.
  if ((WiFi.status() != WL_CONNECTED) && (millis() > wait30)) {
    Serial.println("Trying to reconnect WiFi...");
    WiFi.disconnect();
    WiFi.begin(ssid, password);
    wait30 = millis() + 30000;
  } 
    
  // Check if a client has connected..
  WiFiClient client = server.available();
  if (!client) {
    return;
  }
     
  Serial.print("New client: ");
  Serial.println(client.remoteIP());

  String req = client.readStringUntil('\r');
  Serial.println(req);
  req.replace("+", " ");
  req.replace(" HTTP/1.1", "");
  req.replace("GET /", "");
  Serial.print(req);
  Serial.print(" -> ");
  getCoordinates(req);
     
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/html");
  client.println(""); //  Important.

  navigate();
}

void getCoordinates (String req) {
  int reqLen = req.length() + 1;
  char chars[reqLen]; 
  req.toCharArray(chars, reqLen);
  
  int i = 0;
  char* command = strtok(chars, "/");
  while (command != 0){
    coordinates[i] = atoi(command);
    i++;
    command = strtok(NULL, "/");
  }

  for (int j = 0; j < 3; j++) {
    Serial.print(coordinates[j]);
    Serial.print(" ");
  }
  Serial.println();
}

void navigate () {
  int moveInLine = 0; // 0 = stop, 1 = forward, 2 = backward
  
  // Y-axis used for forward and backward control
  if (coordinates[1] < -5) {
    moveInLine = 1;
  }
  else if (coordinates[1] > 5) {
    moveInLine = 2;
  }
  else {
    moveInLine = 0;
  }

  int moveLeftRight = 0; // 0 = stop, 1 = left, 2 = right;
  
  // X-axis used for left and right control
  if (coordinates[0] > 5) {
    moveLeftRight = 1;
  }
  else if (coordinates[0] < -5) {
    moveLeftRight = 2;
  }
  else {
    moveLeftRight = 0;
  }

  Serial.print("move in line = ");
  Serial.println(moveInLine);
  Serial.print("move left/right = ");
  Serial.println(moveLeftRight);
  Serial.println();

  if (moveInLine == 1) {
    moveForward(120, 2);
  } else if (moveInLine == 2) {
    moveBackward(120, 2);
  }

  if (moveLeftRight == 1) {
    turnLeft(120, 1);
  } else if (moveLeftRight == 2) {
    turnRight(120, 1);
  }

  if (moveInLine == 0 && moveLeftRight == 0) {
    turnMotorsOff();
  }
}

void moveForward (int speed, int timeMs) {
  Serial.println("forward");
  setMotorDirection(1, CLOCKWISE);
  setMotorDirection(2, CLOCKWISE);

  setMotorSpeed(1, speed);
  setMotorSpeed(2, speed);
}

void moveBackward (int speed, int timeMs) {
  Serial.println("backward");
  setMotorDirection(1, COUNTER_CLOCKWISE);
  setMotorDirection(2, COUNTER_CLOCKWISE);

  setMotorSpeed(1, speed);
  setMotorSpeed(2, speed);
}

void turnRight (int speed, int timeMs) {
  Serial.println("right");
  setMotorDirection(1, NONE);
  setMotorDirection(2, CLOCKWISE);

  setMotorSpeed(1, 0);
  setMotorSpeed(2, speed);
}

void turnLeft (int speed, int timeMs) {
  Serial.println("left");
  setMotorDirection(1, CLOCKWISE);
  setMotorDirection(2, NONE);

  setMotorSpeed(1, speed);
  setMotorSpeed(2, 0);
}

void setMotorDirection(int motor, int direction)
{
  switch(direction)
  {
    case CLOCKWISE:
      digitalWrite(motor == 1 ? DRV_A_IN1 : DRV_B_IN1, HIGH);
      digitalWrite(motor == 1 ? DRV_A_IN2 : DRV_B_IN2, LOW);
      break;
    case COUNTER_CLOCKWISE:
      digitalWrite(motor == 1 ? DRV_A_IN1 : DRV_B_IN1, LOW);
      digitalWrite(motor == 1 ? DRV_A_IN2 : DRV_B_IN2, HIGH);
      break;
    case NONE:
      digitalWrite(motor == 1 ? DRV_A_IN1 : DRV_B_IN1, LOW);
      digitalWrite(motor == 1 ? DRV_A_IN2 : DRV_B_IN2, LOW);
      break;
  }
}

void setMotorSpeed(int motor, int speed) {
  switch (motor) {
    case 1: 
      analogWrite(DRV_A_PWM, speed); 
      break;
    case 2: 
      analogWrite(DRV_B_PWM, speed); 
      break;
  }
}

void turnMotorsOff () {
  digitalWrite(DRV_A_IN1, LOW);
  digitalWrite(DRV_A_IN2, LOW);
  
  digitalWrite(DRV_B_IN1, LOW);
  digitalWrite(DRV_B_IN2, LOW);
}

void controlFull () {
  double distance = getDistance(trigPinFull, echoPinFull, false);

  if (distance > 1 && distance <= 15) {
    blink(500);
    isFull = true;
  } else {
    isFull = false;
  }
}

void controlOpen () {
  double distance = getDistance(trigPinOpen, echoPinOpen, true);

  if (distance > 1 && distance <= 20) {
    if (isFull == false) {
      openBin();
      delay(10000); // 10 seconds
      closeBin();
    } else {
      blink(200);
    }
  }
}

double getDistance (int trigPin, int echoPin, bool what) {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);

  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  long duration = pulseIn(echoPin, HIGH);
  double distance = duration * 0.034 / 2; // duration * speed of light / go and back time

  printDistanceInfo(duration, distance, what);
  
  return distance;
}

void printDistanceInfo (long duration, double distance, bool what) {
  if (what == true) {
    Serial.print("Open ->   ");
  } else {
    Serial.print("Full ->   ");
  }
  
  Serial.print("Duration: ");
  Serial.print(duration);
  Serial.print("   Distance: ");
  Serial.print(distance);
  Serial.println(" cm");
}

void blink (int delayTime) {
  if (isFull == false) {
    digitalWrite(ledPinFull, HIGH);
    delay(delayTime);
    digitalWrite(ledPinFull, LOW);
    delay(delayTime);
  }
}

void openBin () {
  for (int position = 0; position <= 180; position++) {
    servo.write(position);
    delay(15);
  }
}

void closeBin () {
  for (int position = 180; position >= 0; position--) {
    servo.write(position);
    delay(15);
  }
}
