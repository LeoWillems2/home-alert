#include <SPI.h>
#include <RF24.h>
#include <nRF24L01.h>
#include <printf.h>

#define PICO

#if defined(PICO)
  #define CE_PIN  14
  #define CSN_PIN 15
  #define BUTTON_PIN 16
  #define BUTTON_HIT 0
  short Addr1 = 5;
  char Addr2 = 'A';
#else
  #define CE_PIN  9
  #define CSN_PIN 8
  #define BUTTON_PIN 4
  #define BUTTON_HIT 1
  short Addr1 = 191;
  short Addr2 = '_';
#endif

RF24 radio(CE_PIN, CSN_PIN);

const uint64_t commonAddress = 0xE8E8F0F0E1LL;

#define MSGLEN 26
struct Payload {
  short AddressP1;
  char AddressP2;
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


  if (digitalRead(BUTTON_PIN) == BUTTON_HIT ) {
  
    Payload data;
    data.AddressP1 = Addr1;
    data.AddressP2 = Addr2;
    String m = "bcde";
    m.toCharArray(data.message, MSGLEN);    
    
    radio.stopListening();
    for(int i=0; i<3; i++) {
      radio.write(&data, sizeof(Payload));
      delay(100);
    }
    delay(1000);

    radio.startListening();
    return;
  }
  
  if (radio.available()) {
    Payload data;

    radio.read(&data, sizeof(Payload)); 
    if (data.AddressP1 == Addr1 && data.AddressP2 == Addr2) {
		// me myself && I
    } else {
      Serial.print("Addr ");
      Serial.print(data.AddressP1);
      Serial.print(data.AddressP2);
      Serial.print(" says: ");
      Serial.println(data.message);
    }
  }
}
