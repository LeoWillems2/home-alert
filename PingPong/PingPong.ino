#include <SPI.h>
#include <RF24.h>
#include <nRF24L01.h>
#include <printf.h>

// https://rf24.readthedocs.io/en/v1.4.4/classRF24.html

// change below for addresses, device and behaviour

#define sendAck true
int consoleLog = true;

#define CE_PIN  9
#define CSN_PIN 8
#define BUTTON_PIN 3
#define BUTTON_HIT 0
#define RED_PIN 4
#define GREEN_PIN 5

const uint64_t commonAddress = 0xE9F8F0F0E1LL;

// Beyond here there be dragons....

RF24 radio(CE_PIN, CSN_PIN);

#define MSGLEN 26   // sizeof(Payload) must not exceed 32.
struct Payload {
  char Cmd[6];
};

void setup() {
  if (consoleLog)  {
    int n = 0;
    Serial.begin(9600);
    while (!Serial) {
        if (n > 1500) {
          consoleLog = false;
          break;          // give up, Pico only.....  Serial fails if not connected to something.
        }
        n++;
        delay(10); 
    }
    
  }

    // prevent interference:
    pinMode(0, INPUT_PULLUP);
    pinMode(1, INPUT_PULLUP);
    pinMode(2, INPUT_PULLUP);
    pinMode(6, INPUT_PULLUP);
    pinMode(7, INPUT_PULLUP);
    pinMode(10, INPUT_PULLUP);
    pinMode(14, INPUT_PULLUP);
    pinMode(15, INPUT_PULLUP);
    pinMode(16, INPUT_PULLUP);
    pinMode(17, INPUT_PULLUP);
    pinMode(18, INPUT_PULLUP);
    pinMode(19, INPUT_PULLUP);

  

  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(GREEN_PIN, OUTPUT);
  pinMode(RED_PIN, OUTPUT);



  digitalWrite(LED_BUILTIN, LOW); 

  if (consoleLog) printf_begin();



  radio.begin();

  if (!radio.begin()) {
    if (consoleLog) Serial.println("nRF24L01 Hardware not found!");
    while (1){ // Halt
      blink(3);
    }
  }

  //if (consoleLog) radio.printDetails();
  radio.openWritingPipe(commonAddress);
  radio.openReadingPipe(1, commonAddress);
  radio.setPALevel(RF24_PA_MAX);			// MIN LOW HIGH MAX
  radio.setDataRate(RF24_250KBPS);
  radio.setChannel(110);
  radio.setAutoAck(false);
  radio.startListening();
};

void blink(int j){
  for (int i=0; i < j; ++i) {
    digitalWrite(LED_BUILTIN, HIGH);  
    delay(100);  
    digitalWrite(LED_BUILTIN, LOW);
    delay(1000);  
  }
}

void blink_green(int j){
  Serial.println("blink_green");
  for (int i=0; i < j; ++i) {
    digitalWrite(GREEN_PIN, HIGH);  
    delay(200);  
    digitalWrite(GREEN_PIN, LOW);
    delay(200);  
  }
}

void blink_red(int j){
  Serial.println("blink_red");
  for (int i=0; i < j; ++i) {
    digitalWrite(RED_PIN, HIGH);  
    delay(300);  
    digitalWrite(RED_PIN, LOW);
    delay(300);  
  }
}


void send(String m) {
  Payload data;
  m.toCharArray(data.Cmd, MSGLEN);   

  radio.stopListening();
    for(int i=0; i<1; i++) {
      radio.write(&data, sizeof(Payload));
      delay(100);
    }
  radio.startListening();
}

bool buttonPressed = false;

void loop() {  

  if (digitalRead(BUTTON_PIN) == BUTTON_HIT ) {   
    	send("ping");
      delay(100);
      digitalWrite(GREEN_PIN, LOW);  
      digitalWrite(RED_PIN, LOW);  
      Serial.println("Sending ping");
	    send("ping");
      delay(700);
	    return;
  }
  

  if (radio.available()) {
    Payload data;

    if (radio.testRPD() ){
      digitalWrite(GREEN_PIN, HIGH);  
      digitalWrite(RED_PIN, LOW);  
    } else {
      digitalWrite(GREEN_PIN, LOW);  
      digitalWrite(RED_PIN, HIGH);  
    }

    radio.read(&data, sizeof(Payload)); 

    if (consoleLog) {
        Serial.print("Incoming Cmd ");
        Serial.println(data.Cmd); 
    }  

    if (data.Cmd[1] == 'i'){
      Serial.println("Answering pong");
      send("pong");
    }
    delay(1000); // wait a little.

    digitalWrite(GREEN_PIN, LOW);  
    digitalWrite(RED_PIN, LOW);  
  }
}
