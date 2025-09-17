#define PIN_LED 7
unsigned int period = 100;  //100, 1000, 10000의 case 녹화
unsigned int onTime, offTime; //unit: us

void setup() {
  pinMode(PIN_LED, OUTPUT);
  
  Serial.begin(115200); //Intialize serial port for monitor
  while(!Serial){
    ; //wait for serial port to connect
  }
  
  set_period(10000);
}

void loop() {
  Serial.println("ON"); //log to check the speed
  //increasing brighness
  for(int i=0;i<100;i++)
  {
    turnon_light(i);
  }
  //decreasing brightness
  for(int i=100;i>0;i--)
  {
    turnon_light(i);
  }
}

void turnon_light(int pDuty){
  //if period is larger than 7500: its duration became larger => skip 1/2 steps
  if(period > (5000 + 10000) / 2 && pDuty % 2 == 1){
    return;
  }
  int periodSum = 0;
  //500000(1/2 sec.) / 100(steps): each steps will be filled by periods
  while(periodSum < 500000 / 100)
  {
    set_duty(pDuty);
    periodSum += period;
  }
}

//period: 100 to 10000 (unit: us, 1ms = 1000us)
void set_period(int pPeriod){
  if(pPeriod < 100 || pPeriod > 10000){
    return;
  }
  period = pPeriod;
  Serial.println(period); //log to record period
}

//duty: 0(off) to 100(Maximum brightness) (unit: %)
void set_duty(int pDuty){
  if(pDuty < 0 || pDuty > 100){
    return;
  }
  onTime = (double)period * pDuty / 100;
  offTime = period - onTime;
  //turn on for duty
  digitalWrite(PIN_LED, 0);
  delayMicroseconds(onTime);
  //turn off for ramnant time
  digitalWrite(PIN_LED, 1);
  delayMicroseconds(offTime);
}
