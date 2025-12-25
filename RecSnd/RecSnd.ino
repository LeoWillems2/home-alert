#include <SPI.h>
#include <RF24.h>
#include <nRF24L01.h>
#include <printf.h>

// change for addresses, device and behaviour
#define PICO
#define sendAck false
#define consoleLog true

#if defined(PICO)
  //#include "pico/cyw43_arch.h"
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

const uint64_t commonAddress = 0xE9F8F0F0E1LL;

// below there wait dragons....

RF24 radio(CE_PIN, CSN_PIN);

#define MSGLEN 26   // sizeof(Payload) must not exceed 32.
struct Payload {
  short AddressP1;
  char AddressP2;
  char Type;
  char message[MSGLEN];
};

void setup() {

  if (consoleLog)  {
    Serial.begin(9600);
    while (!Serial) {
        delay(10); 
    }
    printf_begin();
  }

  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(LED_BUILTIN, OUTPUT);

#ifdef PICO

   //delay(4000);
  //cyw43_arch_init();
  //cyw43_arch_deinit();

  SPI.setSCK(6);
  SPI.setTX(7);
  SPI.setRX(4);
  SPI.begin();
#endif

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
  radio.setPALevel(RF24_PA_MIN);
  radio.setAutoAck(false);
  radio.startListening();
};

void blink(int j){
  for (int i=0; i < j; ++i) {
    digitalWrite(LED_BUILTIN, HIGH);  
    delay(100);  
    digitalWrite(LED_BUILTIN, LOW);
    delay(100);  
  }
}

void send(String m, char t) {

  Payload data;
  data.AddressP1 = Addr1;
  data.AddressP2 = Addr2;
  data.Type = t;
  m.toCharArray(data.message, MSGLEN);    

  radio.stopListening();
    for(int i=0; i<3; i++) {
      radio.write(&data, sizeof(Payload));
      delay(100);
      if (t == 'A'){    // ack is done 3 time due to send normal is done 3 times, so 3 acks should be ok.
        break;
      }
    }
  radio.startListening();
}

bool buttonPressed = false;
bool receivedAlert = false;

void loop() {  

  if (digitalRead(BUTTON_PIN) == BUTTON_HIT ) {   // only send once, then ignore  a still active button.
    if (!buttonPressed) {
      buttonPressed = true;
      if (receivedAlert){
        receivedAlert = false;
        String m = "confirmed";
        send(m, 'C');   // confirm
        if (consoleLog) Serial.println("Led bij buur groen, Buur reageert");
        delay(5000);  //second button press becomes 'B'......  geen opllossing voor bedacht.....
      } else {
        String m = "HELP";
        send(m, 'B');
        // ledje op geel: visual feedback dat het is verzonden
        if (consoleLog) Serial.println("Led geel, HELP verzonden door mij");
      }
      return;
    }
  }  else {
    buttonPressed = false;
    return;
  }
  
  if (radio.available()) {
    Payload data;

    radio.read(&data, sizeof(Payload)); 

    if (data.AddressP1 == Addr1 && data.AddressP2 == Addr2) {
		  return; // me, myself and I
    } else {
      blink(3);
      if (consoleLog) {
        Serial.print("Addr ");
        Serial.print(data.AddressP1);
        Serial.print(data.AddressP2);
        Serial.print(" ");
        Serial.print(data.Type);
        Serial.print(" says: ");
        Serial.println(data.message);
      }

      if (sendAck && data.Type != 'A') {
        delay(200); // wait a little.
        send("ack", 'A');
        return;
      }

      if (data.Type == 'B'){   // bij buur
        receivedAlert = true;   // piep, alarm etc. ledje op geel
        if (consoleLog) Serial.println("Led geel, HELP ontvangen");
        return;
      }

      if (data.Type == 'C'){    // bij mij
        if (consoleLog) Serial.println("Led groen, Buur heeft gereageerd");
        // ledje op groen:   we hebben een ack op de HELP
        return;
      }
    }
  }
}
