/* vim: set ts=2 sw=2 sts=2 et! : */
//
// BoilerMake Fall 2014 Badge Code
//
// These boards are equivalent to an Arduino Leonardo
// And contain an ATMega32U4 microprocessor and an nRF24L01 radio
//
// Lior Ben-Yehoshua (admin@Lior9999.com)
// Viraj Sinha       (virajosinha@gmail.com)
// Scott Opell       (me@scottopell.com)
//
// 10/15/2014
//
// Happy Hacking!
//

#define MAX_TERMINAL_LINE_LEN 40
#define MAX_TERMINAL_WORDS     7

// 14 is strlen("send FFFF -m X")
// the max message length able to be sent from the terminal is
// total terminal line length MINUS the rest of the message
#define MAX_TERMINAL_MESSAGE_LEN  MAX_TERMINAL_LINE_LEN - 14

#include <RF24.h>
#include <SPI.h>
#include <EEPROM.h>

void welcomeMessage(void);
void printHelpText(void);
void printPrompt(void);

void networkRead(void);
void serialRead(void);
void handleSerialData(char[], byte);

void setValue(word);
void handlePayload(struct payload *);

void ledDisplay(byte);
void displayDemo();

// Maps commands to integers
const byte PING   = 0; // Ping
const byte LED    = 1; // LED pattern
const byte MESS   = 2; // Message
const byte DEMO   = 3; // Demo Pattern

// Global Variables
int SROEPin = 3; // using digital pin 3 for SR !OE
int SRLatchPin = 8; // using digital pin 4 for SR latch
boolean terminalConnect = false; // indicates if the terminal has connected to the board yet

// nRF24L01 radio static initializations
RF24 radio(9,10); // Setup nRF24L01 on SPI bus using pins 9 & 10 as CE & CSN, respectively

uint16_t this_node_address = (EEPROM.read(0) << 8) | EEPROM.read(1); // Radio address for this node

struct payload{ // Payload structure
  byte command;
  byte led_pattern;
  char message[30];
};

// This runs once on boot
void setup() {
  Serial.begin(9600);

  // SPI initializations
  SPI.begin();
  SPI.setBitOrder(MSBFIRST); // nRF requires MSB first
  SPI.setClockDivider(16); // Set SPI clock to 16 MHz / 16 = 1 MHz

  // nRF radio initializations
  radio.begin();
  radio.setDataRate(RF24_1MBPS); // 1Mbps transfer rate
  radio.setCRCLength(RF24_CRC_16); // 16-bit CRC
  radio.setChannel(80); // Channel center frequency = 2.4005 GHz + (Channel# * 1 MHz)
  radio.setRetries(200, 5); // set the delay and number of retries upon failed transmit
  radio.openReadingPipe(0, this_node_address); // Open this address
  radio.startListening(); // Start listening on opened address
  radio.enableDynamicAck();

  // Shift register pin initializations
  pinMode(SROEPin, OUTPUT);
  pinMode(SRLatchPin, OUTPUT);
  digitalWrite(SROEPin, HIGH);
  digitalWrite(SRLatchPin, LOW);

  // Display welcome message
  welcomeMessage();

  // make the pretty LEDs happen
  ledDisplay(2);
}


// This loops forever
void loop() {
  // Displays welcome message if serial terminal connected after program setup
  if (Serial && !terminalConnect) { 
    welcomeMessage();
    terminalConnect = true;
  } else if (!Serial && terminalConnect) {
    terminalConnect = false;
  }

  networkRead(); // Read from network
  serialRead(); // Read from serial  
}


// Handle reading from the radio
void networkRead() {
  while (radio.available()) {
    struct payload * current_payload = (struct payload *) malloc(sizeof(struct payload));

    // Fetch the payload, and see if this was the last one.
    radio.read( current_payload, sizeof(struct payload) );
    handlePayload(current_payload);
  }
}


// Get user input from serial terminal
void serialRead() {
  char inData[MAX_TERMINAL_LINE_LEN]; // allocate space for incoming serial string
  byte index = 0; // Index into array, where to store chracter
  char inChar; // Where to store the character to read

  // fills up characters from the terminal, leaving room for null terminator
  while ( index < MAX_TERMINAL_LINE_LEN - 1 && Serial.available() > 0){
    inChar = Serial.read();
    if (inChar == '\r'){
      inData[index] = '\0';
      break;
    } else {
      inData[index] = inChar;
      index++;
    }
  }

  if (index > 0){ // if we read some data, then process it
    Serial.println(inData);
    handleSerialData(inData, index);
    printPrompt();
  }

}


// Handle received commands from user obtained via the serial terminal
void handleSerialData(char inData[], byte index) {
  // tokenize the input from the terminal by spaces
  char * words[MAX_TERMINAL_WORDS];
  byte current_word_index = 0;
  char * p = strtok(inData, " ");
  while(p != NULL) {
    words[current_word_index++] = p;
    p = strtok(NULL, " ");
  }

  if (strcmp(words[0], "help") == 0) {
    printHelpText();

  } else if (strcmp(words[0], "send") == 0) {
    // checks if address field was given valid characters
    if ((strspn(words[1], "1234567890AaBbCcDdEeFf") <= 4)
        && (strspn(words[1], "1234567890AaBbCcDdEeFf") > 0)) {

      uint16_t TOaddr = strtol(words[1], NULL, 16);

      if (strncmp(words[2], "-p", 2) == 0) { // Send ping
        struct payload myPayload = {PING, '\0', {'\0'}};
        size_t len = sizeof(PING) + sizeof('\0') * 2;

        radio.stopListening();
        radio.openWritingPipe(TOaddr);
        radio.write(&myPayload, len);
        radio.startListening();

      } else if (strcmp(words[2], "-l") == 0) { // Send LED pattern
        if (strspn(words[3], "1234567890") == 1) {
          byte led_patt = (byte) atoi(words[3]);
          struct payload myPayload = {LED, led_patt, {'\0'}};
          size_t len = sizeof(LED) + sizeof(led_patt) + sizeof('\0');

          radio.stopListening();
          radio.openWritingPipe(TOaddr);
          radio.write(&myPayload, len);
          radio.startListening();
        }

        else {
          Serial.println("  Invalid LED pattern field.");
        }

      } else if (strcmp(words[2], "-m") == 0) { // Send message
        char str_msg[MAX_TERMINAL_MESSAGE_LEN];

        char * curr_pos = str_msg;
        for (int i = 3; i < current_word_index; i++){
          byte curr_len = strlen(words[i]);
          strncpy(curr_pos, words[i], curr_len);
          curr_pos += curr_len;

          // this will add in the space characters that tokenizing removes
          if (i < current_word_index - 1){
            strncpy(curr_pos, " ", 1);
            curr_pos++;
          }
        }

        struct payload myPayload = {MESS, '\0', {}};

        // the end of the string minus the start of the string gives the length
        memcpy(&myPayload.message, str_msg, curr_pos - str_msg);
        Serial.println(myPayload.message);
        radio.stopListening();
        radio.openWritingPipe(TOaddr);
        radio.write(&myPayload, sizeof(myPayload));
        radio.startListening();
      }
      
      else if (strcmp(words[2], "-d") == 0) { // Send LED pattern
        struct payload myPayload = {DEMO, ' ', {}};
        
        radio.stopListening();
        radio.openWritingPipe(TOaddr);
        radio.write(&myPayload, sizeof(myPayload));
        radio.startListening();
      }

      else {
        Serial.println("  Invalid command field.");
      }
    }

    else {
      Serial.println("  Invalid address field.");
    }
    
  } else if (strcmp(words[0], "self") == 0) {
    
    if (strcmp(words[1], "-l") == 0) { // Send LED pattern
      if (strspn(words[2], "1234567890") == 1) {
        byte led_patt = (byte) atoi(words[2]);
        struct payload selfPayload = {LED, led_patt, {'\0'}};
        size_t len = sizeof(LED) + sizeof(led_patt) + sizeof('\0');

        handlePayload(&selfPayload);
      }

      else {
        Serial.println("  Invalid self LED pattern field.");
      }
    }
    
    else if (strcmp(words[1], "-d") == 0) { // Send LED pattern
        struct payload selfPayload = {DEMO, ' ', {}};
        
        handlePayload(&selfPayload);
    }
    
    else {
      Serial.println("  Invalid self field.");
    }
    
  } else if (strcmp(words[0], "search") == 0) {
    
    if (strcmp(words[1], "-a") == 0) { // 
      
      radio.setAutoAck(1);                    // Ensure autoACK is enabled
      //radio.setRetries(15,15);
      
      const int hexStart = 0x0000;
      const int hexEnd = 0x01FF;
      const int addrRange = hexEnd - hexStart;
      struct payload myPayload = {LED, '1', {'\0'}};
      
      for (int TOaddr = 0; TOaddr < addrRange; TOaddr++) {
        radio.stopListening();
        radio.openWritingPipe(TOaddr);
        radio.write(&myPayload, sizeof(myPayload));
        int* crapbuffer = new int(0);
        radio.read(crapbuffer, 10);
        if (radio.isAckPayloadAvailable()){
          Serial.println(TOaddr, HEX);
        }
        radio.startListening();
      }      
      
    }
    
  } else if (strcmp(words[0], "channel") == 0) {

    // Set radio channel
    byte chn = (byte) atoi(words[1]);

    if (chn >= 0 && chn <= 83) {
      Serial.print("Channel is now set to ");
      Serial.println(chn);
      radio.setChannel(chn);
    } else {
      Serial.println(" Invalid channel number. Legal channels are 0 - 83.");
    }

  } else if (strcmp(words[0], "radio") == 0) {
    // Turn radio listening on/off
    if (strcmp(words[1], "on") == 0) {
      Serial.println("Radio is now in listening mode");
      radio.startListening();
    } else if (strcmp(words[1], "off") == 0) {
      Serial.println("Radio is now NOT in listening mode");
      radio.stopListening();
    } else {
      Serial.println(" Invalid syntax.");
    }
  }
  
  else {
    Serial.println("Unrecognized command.  Type help for help."); 
  }
}


// Grab message received by nRF for this node
void handlePayload(struct payload * myPayload) {
  switch(myPayload->command) {

    case PING:
      Serial.println("Someone pinged us!");
      printPrompt();
      break;

    case LED:
      ledDisplay(myPayload->led_pattern);
      break;

    case MESS:
      Serial.print("Message:\r\n  ");
      Serial.println(myPayload->message);
      printPrompt();
      break;

    case DEMO:
      displayDemo();
      break;

    default:
      Serial.println(" Invalid command received.");
      break;
  }
  free(myPayload); // Deallocate payload memory block
}

void printPrompt(void){
  Serial.print("> ");
}

/*
// Display LED pattern

// LED numbering:

           9
       8       10
     7           11
   6               12
  5                 13
   4               14
     3           15
       2       16
           1

shift register 1-8: AA
shift register 9-16: BB

setValue data in hex: 0xAABB
where AA in binary = 0b[8][7][6][5][4][3][2][1]
      BB in binary = 0b[16][15][14][13][12][11][10][9]

*/
void ledDisplay(byte pattern) {
  setValue(0x0000);
  digitalWrite(SROEPin, LOW);
  if(pattern == 0) {
    word pattern = 0x0000; // variable used in shifting process
    int del = 62; // ms value for delay between LED toggles

    for(int i=0; i<16; i++) {
      pattern = (pattern << 1) | 0x0001;
      setValue(pattern);
      delay(del);
    }

    for(int i=0; i<16; i++) {
      pattern = (pattern << 1);
      setValue(pattern);
      delay(del);
    }
  }
  else if(pattern == 1) {
    word pattern = 0x0000; // variable used in shifting process
    int del = 62; // ms value for delay between LED toggles

    for (int i = 0; i < 16; i++) {
      pattern = (pattern >> 1) | 0x8000;
      setValue(pattern);
      delay(del);
    }
    for (int i=0; i<16; i++) {
      pattern = (pattern >> 1);
      setValue(pattern);
      delay(del);
    }
  }
  else if(pattern == 2) {
    int del = 100;
    setValue(0x1010);
    delay(del);
    setValue(0x3838);
    delay(del);
    setValue(0x7C7C);
    delay(del);
    setValue(0xFEFE);
    delay(del);
    setValue(0xFFFF);
    delay(del);
    setValue(0xEFEF);
    delay(del);
    setValue(0xC7C7);
    delay(del);
    setValue(0x8383);
    delay(del);
    setValue(0x0101);
    delay(del);
    setValue(0x0000);
    delay(del);
  }
  else if(pattern == 3) {
    word pattern = 0x0101;
    int del = 125;
    setValue(pattern);
    for(int i=0; i<8; i++) {
      delay(del);
      pattern = (pattern << 1);
      setValue(pattern);
    }
  }
  else if(pattern == 4) {
    for (int i = 0; i < 4; i++) {
      setValue(0xFFFF);
      delay(125);
      setValue(0x0000);
      delay(125);
    }
  }
  digitalWrite(SROEPin, HIGH);
}


// LED display demo
void displayDemo() {
  digitalWrite(SROEPin, LOW);
  for (int i = 0; i < 100; i++) {
    setValue(0xAAAA);
    delay(125);
    setValue(0x5555);
    delay(125);
  }
  digitalWrite(SROEPin, HIGH);
}

// Sends word sized value to both SRs & latches output pins
void setValue(word value) {
  byte Hvalue = value >> 8;
  byte Lvalue = value & 0x00FF;
  SPI.transfer(Lvalue);
  SPI.transfer(Hvalue);
  digitalWrite(SRLatchPin, HIGH);
  digitalWrite(SRLatchPin, LOW);
}

// Prints 'help' command
void printHelpText() {
  Serial.println("Available commands:");
  Serial.println("  help          - displays commands list.");
  Serial.println();
  Serial.println("  send [addr] [command] [data] - send packets to other node.");
  Serial.println("      [addr]    - address of destination node, as a 4 digit hex value");
  Serial.println("      [command] - command field.");
  Serial.println("        -p - ping destination node.");
  Serial.println("        -l - send LED pattern to destination node.");
  Serial.println("           - [data] - LED pattern. Valid range: 0-4.");
  Serial.println("        -m - send message to destination node.");
  Serial.println("           - [data] - message to be sent. Max 26 characters.");
  Serial.println();
  Serial.println("  self [command] [data] - simulate receiving a packet of data.");
  Serial.println("        -l - send LED pattern to yourself.");
  Serial.println();
  Serial.println("  search [command] - search for peers...  ");
  Serial.println();
  Serial.println("  channel [val] - change channel of your node.");
  Serial.println("                - [val] - new channel. Valid range: 0-83. Default: 80.");
  Serial.println();
  Serial.println("  radio [on | off] - turn radio on or off");
}

void welcomeMessage(void) {
  char hex_addr[10];
  sprintf(hex_addr, "%04x", this_node_address);
  Serial.print("\r\nWelcome to the BoilerMake Hackathon Badge Network...\r\n\n");
  Serial.print("Your address: ");
  Serial.println(hex_addr);
  Serial.print("\nAll commands must be terminated with a carriage return.\r\n"
      "Type 'help' for a list of available commands.\r\n\n> ");
}
