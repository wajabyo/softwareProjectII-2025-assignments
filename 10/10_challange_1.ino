#include <Servo.h>

// Arduino pin assignment
#define PIN_LED  9
#define PIN_TRIG 12
#define PIN_ECHO 4  //13->4(can't use 13th port)
#define PIN_SERVO 10

// configurable parameters
#define SND_VEL 346.0     // sound velocity at 24 celsius degree (unit: m/sec)
#define INTERVAL 25       // sampling interval (unit: msec)
#define PULSE_DURATION 10 // ultra-sound Pulse Duration (unit: usec)
#define _DIST_MIN 20     // minimum distance to be measured (unit: mm)
#define _DIST_MAX 200     // maximum distance to be measured (unit: mm)

#define _TARGET_MIN 50
#define _TARGET_MAX 100

#define TIMEOUT ((INTERVAL / 2) * 1000.0) // maximum echo waiting time (unit: usec)
#define SCALE (0.001 * 0.5 * SND_VEL)     // coefficent to convert duration to distance

#define _EMA_ALPHA 0.25    // EMA weight of new sample (range: 0 to 1)
                          // Setting EMA to 1 effectively disables EMA filter.

#define _LIFTING_TIME 1000
#define _WAITING_TIME 2000
#define _MULTIPLIER 10000

#define _OPENED_ANGLE 120
#define _CLOSED_ANGLE 0

// global variables
unsigned long last_sampling_time;   // unit: msec
unsigned long last_signaled_time;    // unit: msec
float dist_prev = _DIST_MAX;        // Distance last-measured
float dist_ema;                     // EMA distance

Servo myServo;
unsigned long moveStartTime;

// angles will be swap as the direction changed
int startAngle = 0; // 0°
int stopAngle  = 110; // 110°

int liftingDir = 0;

void setup() {
  // initialize GPIO pins
  pinMode(PIN_LED,OUTPUT);
  pinMode(PIN_TRIG,OUTPUT);
  pinMode(PIN_ECHO,INPUT);
  digitalWrite(PIN_TRIG, LOW);

  myServo.attach(PIN_SERVO);
  moveStartTime = millis(); // start moving

  myServo.write(startAngle); // Set position

  Serial.begin(57600);
  
  delay(500);
}

void loop() {
  float dist_raw, dist_filtered;

  if (millis() >= last_sampling_time + INTERVAL){
    // get a distance reading from the USS
    dist_raw = USS_measure(PIN_TRIG,PIN_ECHO);

    // Modify the below if-else statement to implement the range filter
    if ((dist_raw == 0.0) || (dist_raw > _DIST_MAX)) {
      dist_filtered = dist_prev;
    } else if (dist_raw < _DIST_MIN) {
      dist_filtered = dist_prev;
    } else {    // In desired Range
      dist_filtered = dist_raw;
      dist_prev = dist_raw;
    }

    // Modify the below line to implement the EMA equation
    dist_ema = _EMA_ALPHA * dist_filtered + (1 - _EMA_ALPHA) * dist_ema;

    // in target distant
    if((dist_ema > _TARGET_MIN) && (dist_ema < _TARGET_MAX)){
      digitalWrite(PIN_LED, 0);       // LED ON
      last_signaled_time = millis();

      // if it isn't opened
      if(liftingDir != 1){
        startAngle = _CLOSED_ANGLE;
        stopAngle = _OPENED_ANGLE;
        moveStartTime = millis();
        liftingDir = 1;
      }
    }
    // out from target distant
    else{
      digitalWrite(PIN_LED, 1);       // LED OFF

      // it passed watingTime since it sensed near(=>need to lift lower)
      if(liftingDir == 1 && millis() >= last_signaled_time + _WAITING_TIME){
        startAngle = _OPENED_ANGLE;
        stopAngle = _CLOSED_ANGLE;
        moveStartTime = millis();
        liftingDir = -1;
      }
    }
    // update last sampling time
    last_sampling_time += INTERVAL;
  }

  unsigned long progress = millis() - moveStartTime;

  if(progress <= _LIFTING_TIME){
    //long angle = map(sigmoid(progress), 0, _MULTIPLIER, startAngle, stopAngle);
    long angle = map(modifiedSin(progress), 0, _MULTIPLIER, startAngle, stopAngle);
    myServo.write(angle);
    Serial.println(angle);
  }
}

// get a distance reading from USS. return value is in millimeter.
float USS_measure(int TRIG, int ECHO)
{
  digitalWrite(TRIG, HIGH);
  delayMicroseconds(PULSE_DURATION);
  digitalWrite(TRIG, LOW);
  
  return pulseIn(ECHO, HIGH, TIMEOUT) * SCALE; // unit: mm
}

// 도전과제 1-1
double sigmoid(double z){
  double x = (z / _LIFTING_TIME) * 12 - 6;
  return 1 / (1 + exp(-x)) * _MULTIPLIER;
}

// 도전과제 1-2
double modifiedSin(double z){
  double x = (z / _LIFTING_TIME) * PI - 0.5 * PI;
  return 0.5 * (sin(x) + 1) * _MULTIPLIER;
}
