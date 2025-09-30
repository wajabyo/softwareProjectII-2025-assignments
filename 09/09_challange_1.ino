// Arduino pin assignment
#define PIN_LED  9
#define PIN_TRIG 12
#define PIN_ECHO 4  //13->4(can't use 13th port)

// configurable parameters
#define SND_VEL 346.0     // sound velocity at 24 celsius degree (unit: m/sec)
#define INTERVAL 25       // sampling interval (unit: msec)
#define PULSE_DURATION 10 // ultra-sound Pulse Duration (unit: usec)
#define _DIST_MIN 100     // minimum distance to be measured (unit: mm)
#define _DIST_MAX 300     // maximum distance to be measured (unit: mm)

#define SCALE (0.001 * 0.5 * SND_VEL)     // coefficent to convert duration to distance

#define _EMA_ALPHA 0.25    // EMA weight of new sample (range: 0 to 1)
                          // Setting EMA to 1 effectively disables EMA filter.

#define SAMPLE_COUNT 30  //challange: N(case: 3, 10, 30)

// global variables
unsigned long last_sampling_time;   // unit: msec
float dist_ema;                     // EMA distance

float m_samples[SAMPLE_COUNT];

void setup() {
  // initialize GPIO pins
  pinMode(PIN_LED,OUTPUT);
  pinMode(PIN_TRIG,OUTPUT);
  pinMode(PIN_ECHO,INPUT);
  digitalWrite(PIN_TRIG, LOW);

  // initialize serial port
  Serial.begin(57600);

  InitSample();
}

void loop() {
  float dist_raw, dist_median;
  
  // wait until next sampling time. 
  // millis() returns the number of milliseconds since the program started. 
  // will overflow after 50 days.
  if (millis() < last_sampling_time + INTERVAL)
    return;

  // get a distance reading from the USS
  dist_raw = USS_measure(PIN_TRIG,PIN_ECHO);

  // Modify the below if-else statement to implement the range filter
  if ((dist_raw == 0.0) || (dist_raw > _DIST_MAX)) {
      dist_median = GetSampleMedian();
  } else if (dist_raw < _DIST_MIN) {
      dist_median = GetSampleMedian();
  } else {    // In desired Range
      dist_median = dist_raw;
      AddSample(dist_raw);
  }

  // Modify the below line to implement the EMA equation
  dist_ema = _EMA_ALPHA * dist_median + (1 - _EMA_ALPHA) * dist_ema;

  // output the distance to the serial port
  Serial.print("Min:"); Serial.print(_DIST_MIN);
  Serial.print(",raw:");  Serial.print(dist_raw);
  Serial.print(",ema:");  Serial.print(dist_ema);
  Serial.print(",median:"); Serial.print(dist_median);
  Serial.print(",Max:");  Serial.print(_DIST_MAX);
  Serial.println("");

  // do something here
  if ((dist_raw < _DIST_MIN) || (dist_raw > _DIST_MAX))
    digitalWrite(PIN_LED, 1);       // LED OFF
  else
    digitalWrite(PIN_LED, 0);       // LED ON

  // update last sampling time
  last_sampling_time += INTERVAL;
}

// get a distance reading from USS. return value is in millimeter.
float USS_measure(int TRIG, int ECHO)
{
  digitalWrite(TRIG, HIGH);
  delayMicroseconds(PULSE_DURATION);
  digitalWrite(TRIG, LOW);
  
  //return pulseIn(ECHO, HIGH, TIMEOUT) * SCALE; // unit: mm
  return pulseIn(ECHO, HIGH) * SCALE;

  // Pulse duration to distance conversion example (target distance = 17.3m)
  // - pulseIn(ECHO, HIGH, timeout) returns microseconds (음파의 왕복 시간)
  // - 편도 거리 = (pulseIn() / 1,000,000) * SND_VEL / 2 (미터 단위)
  //   mm 단위로 하려면 * 1,000이 필요 ==>  SCALE = 0.001 * 0.5 * SND_VEL
  //
  // - 예, pusseIn()이 100,000 이면 (= 0.1초, 왕복 거리 34.6m)
  //        = 100,000 micro*sec * 0.001 milli/micro * 0.5 * 346 meter/sec
  //        = 100,000 * 0.001 * 0.5 * 346
  //        = 17,300 mm  ==> 17.3m
}

void InitSample(){
  for(int i = 0; i < SAMPLE_COUNT; i++){
    m_samples[i] = _DIST_MAX + 1;
  }
}

void AddSample(float pDistance){
  int i = 0;
  for(; i < SAMPLE_COUNT; i++){
    if(m_samples[i] > _DIST_MAX){
      m_samples[i] = pDistance;
      SortSample();
      return;
    }
  }
  for(i = 0; i < SAMPLE_COUNT - 1; i++){
    m_samples[i] = m_samples[i + 1];
  }
  m_samples[SAMPLE_COUNT - 1] = pDistance;
  SortSample();
  return;
}

float GetSampleMedian(){
  int count = 0;
  for(int i = 0; i < SAMPLE_COUNT; i++){
    if(m_samples[i] > _DIST_MAX){
      break;
    }
    count++;
  }
  if(count == 0){
    return 0;
  }
  if(count % 2 == 0){
    return (m_samples[count - 2] + m_samples[count - 1]) / 2;
  }
  else{
    return m_samples[count - 1];
  }
}

void SortSample(){
  float temp;
  int targetIndex;
  for(int i = 0; i < SAMPLE_COUNT - 1; i++){
    temp = m_samples[i];
    targetIndex = i;
    for(int j = i + 1; j < SAMPLE_COUNT; j++){
      if(temp > m_samples[j]){
        temp = m_samples[j];
        targetIndex = j;
      }
    }
    if(targetIndex != i){
      m_samples[targetIndex] = m_samples[i];
      m_samples[i] = temp;
    }
  }
}
