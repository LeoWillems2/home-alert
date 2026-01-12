#include <SPI.h>
#include <RF24.h>
#include <nRF24L01.h>
#include <printf.h>

// https://rf24.readthedocs.io/en/v1.4.4/classRF24.html

// change below for addresses, device and behaviour

#define sendAck true
int consoleLog = true;

#if defined(ARDUINO_ARCH_RP2040)        // PICO
  #define CE_PIN  14
  #define CSN_PIN 15
 
  short Addr1 = 5;
  char Addr2 = 'A';
#else
  #define CE_PIN  9
  #define CSN_PIN 8
  #define BUTTON_PIN 3
  #define BUTTON_HIT 0
  #define RED_PIN 4
  #define GREEN_PIN 5
  short Addr1 = 191;
  short Addr2 = '_';
#endif

const uint64_t commonAddress = 0xE9F8F0F0E1LL;

// Beyond here there be dragons....

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

  

  if (consoleLog) printf_begin();

  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(GREEN_PIN, OUTPUT);
  pinMode(RED_PIN, OUTPUT);


#ifdef ARDUINO_ARCH_RP2040
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
  radio.setPALevel(RF24_PA_LOW);			// MIN LOW HIGH MAX
  radio.setDataRate(RF24_250KBPS);
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
    delay(200);  
    digitalWrite(RED_PIN, LOW);
    delay(200);  
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
        if (consoleLog) Serial.println("Led bij buur groen, Buur reageert");
        digitalWrite(GREEN_PIN, HIGH); 
        digitalWrite(RED_PIN, LOW);  
        receivedAlert = false;
        String m = "confirmed";
        send(m, 'C');   // confirm
        delay(1000);  //second button press becomes 'B'......  geen opllossing voor bedacht.....
      } else {
        if (consoleLog) Serial.println("Led geel, HELP verzonden door mij");
        digitalWrite(RED_PIN, HIGH);  
        digitalWrite(GREEN_PIN, LOW);  

        String m = "HELP";
        send(m, 'B');
        // ledje op geel: visual feedback dat het is verzonden
      }
      return;
    }
  }  else {
      buttonPressed = false;
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
      }

      if (data.Type == 'B'){   // bij buur
        receivedAlert = true;   // piep, alarm etc. ledje op geel
        if (consoleLog) Serial.println("Led geel, HELP ontvangen");
        digitalWrite(RED_PIN, HIGH);  
        digitalWrite(GREEN_PIN, LOW);  

        return;
      }

      if (data.Type == 'C'){    // bij mij
        if (consoleLog) Serial.println("Led groen, Buur heeft gereageerd");
        digitalWrite(GREEN_PIN, HIGH);  
        digitalWrite(RED_PIN, LOW);  

        // ledje op groen:   we hebben een ack op de HELP
        return;
      }
    }
  }
}
