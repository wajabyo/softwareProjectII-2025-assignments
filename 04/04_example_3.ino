#define PIN_LED 13
unsigned int count, toggle;

void setup(){
  pinMode(PIN_LED, OUTPUT);
  Serial.begin(115200); //Intialize serial port for monitor
  while(!Serial){
    ; //wait for serial port to connect
  }
  Serial.println("HelloWorld!");
  count = toggle = 0;
  digitalWrite(PIN_LED, toggle);  //turn off LED
}

void loop(){
  Serial.println(++count);
  toggle = !toggle_state(toggle); //toggle LED value(0->1, 1->0)
  digitalWrite(PIN_LED, toggle);  //update LED status
  delay(1000);  //wait for 1000ms
}

int toggle_state(int toggle){
  return toggle;
}
