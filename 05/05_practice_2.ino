#define PIN_LED 7
unsigned int count;

void setup(){
  pinMode(PIN_LED, OUTPUT);
  Serial.begin(115200); //Intialize serial port for monitor
  while(!Serial){
    ; //wait for serial port to connect
  }
  count = 0;
}

void loop(){
  digitalWrite(PIN_LED, 0);
  Serial.println("test");
  delay(1000);
  
  for(; count < 10; count++){
    digitalWrite(PIN_LED, count % 2 == 1);
    delay(1000 / 10);
  }

  digitalWrite(PIN_LED, 1);

  while(true){
    ;
  }
}
