#include <SPI.h>
#include <RF24.h>
#include <nRF24L01.h>
#include <printf.h>

#define PICO

#if defined(PICO)
  #define CE_PIN  14
  #define CSN_PIN 15
  #define BUTTON_PIN 16
#else
  #define CE_PIN  9
  #define CSN_PIN 8
  #define BUTTON_PIN 4
#endif

RF24 radio(CE_PIN, CSN_PIN); // CE, CSN

const uint64_t commonAddress = 0xE8E8F0F0E1LL;

#define NODE_ID 1  // Set to 1, 2, ....
#define MSGLEN 28
struct Payload {
  int nodeID;
   int x;
  char message[MSGLEN];
};

void setup() {

  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(LED_BUILTIN, OUTPUT);

  Serial.begin(9600);
  while (!Serial) {
        delay(10); 
    }
  printf_begin();

  delay(500);

#ifdef PICO
  SPI.setSCK(6);
  SPI.setTX(7);
  SPI.setRX(4);
  SPI.begin();
#endif

  radio.begin();

  if (!radio.begin()) {
    Serial.println("nRF24L01 Hardware not found!");
    while (1); // Halt
  }

  //radio.printDetails();
  radio.openWritingPipe(commonAddress);
  radio.openReadingPipe(1, commonAddress);
  radio.setPALevel(RF24_PA_MIN);
  radio.setAutoAck(false);
  radio.startListening();
};

void loop() {  


  if (digitalRead(BUTTON_PIN) == 0 ) {
  
    char message[MSGLEN];
    strcpy(message+1, "abcd");
    message[0] = NODE_ID;
    Serial.println(message+1);
    
    
    radio.stopListening();
    for(int i=0; i<3; i++) {
      radio.write(&message, sizeof(message));
      delay(100);
    }
    delay(1000);

    radio.startListening();
    return;
  }
  
  if (radio.available()) {
    char message[MSGLEN];
    radio.read(&message, sizeof(message)); 
    if (message[0] != NODE_ID) {
      Serial.print("Node ");
      Serial.print(message[0]);
      Serial.print(" says: ");
      Serial.println(message+1);
    }
  }
}
