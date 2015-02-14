#include <Mailbox.h>
#include "SPI.h"
#include "PN532_SPI.h"
#include "snep.h"
#include "NdefMessage.h"
#include "SoftwareSerial.h"
#include "Adafruit_Thermal.h"
#include "NfcAdapter.h"
#include "med_logo.h"

#define PRINTER_RX 6  // This is the green wire
#define PRINTER_TX 7  // This is the yellow wire

#define BUZZER 11 // BUZZZ
#define NFC_TIMEOUT 20000

#define NFC 10

PN532_SPI pn532spi(SPI, NFC);
SNEP nfc(pn532spi);
uint8_t ndefBuf[128];
uint8_t recordBuf[128];

NfcAdapter nfc_adapter = NfcAdapter(pn532spi); // create an NFC adapter object

Adafruit_Thermal printer(PRINTER_RX, PRINTER_TX);

String mailMsg;

void makeNoise()
{
  // we are on
  digitalWrite(BUZZER, HIGH);
  delay(500);
  digitalWrite(BUZZER, LOW);
}


void setup() {
  // Initialize Bridge and Mailbox
  Bridge.begin();
  Mailbox.begin();
  
  // Initialize Printer
  printer.begin();
  printer.setSize('L');
  //Version 1.0 of the Arduino IDE introduced the F() syntax for storing strings in flash memory rather than RAM
  printer.println(F("I am Configured"));
  printer.feed(2);
  printer.printBitmap(med_logo_width, med_logo_height, med_logo_data);
  
  printer.feed(5);
  makeNoise();
}

void printerPrint(const char* print_buffer) {
  printer.printBitmap(med_logo_width, med_logo_height, med_logo_data);
  printer.feed(2);
  printer.setSize('M');
  printer.println(print_buffer);
  printer.feed(5);
  delay(2000);
}

String hex2char(const byte * data, const uint32_t numBytes)
{
  uint32_t szPos;
  String result;
  result.concat(' '); //Concat characters http://arduino.cc/en/Reference/StringConcat
  for (szPos=0; szPos < numBytes; szPos++) {
    if (data[szPos] > 0x1F) result.concat((char)data[szPos]);
  }
  return result;
}

void checkMailbox() {
  // if there is a message in the Mailbox
  if (Mailbox.messageAvailable())
  {
    makeNoise();
    // read all the messages present in the queue
    while (Mailbox.messageAvailable())
    {
      Mailbox.readMessage(mailMsg);
      printerPrint(mailMsg.c_str()); //Convert string to char array http://arduino.cc/en/Reference/CStr
    }
  }
  return;
}

void checkforNFC(){
    // nfc.read is a blocking function.. adding timeout
    int msgSize = nfc.read(ndefBuf, sizeof(ndefBuf), NFC_TIMEOUT);
    if (msgSize > 0) {
        makeNoise();
        NdefMessage msg  = NdefMessage(ndefBuf, msgSize);
        msg.print();
        NdefRecord record = msg.getRecord(0);
        record.print();
        int recordLength = record.getPayloadLength();
        if (recordLength <= sizeof(recordBuf)) {
            record.getPayload(recordBuf);
            printerPrint((char*)recordBuf);
            
        }
    }
    return;
}

void checkRegularCard() {
  if (nfc_adapter.tagPresent()) // Do an NFC scan to see if an NFC tag is present
    {
        int i = 0;
        makeNoise();
        NfcTag tag = nfc_adapter.read(); // read the NFC tag into an object, nfc.read() returns an NfcTag object.
        NdefMessage msg = tag.getNdefMessage();
        for (i=0; i<msg.getRecordCount(); i++)
        {
          NdefRecord tmp = msg.getRecord(i);
          int record_length = tmp.getPayloadLength();
          byte storage[record_length];
          tmp.getPayload(storage);
          String respBuffer = hex2char(storage, record_length);
          printerPrint(respBuffer.c_str());
        }
        
    }
    delay(500); // wait half a second (500ms) before scanning again (you may increment or decrement the wait time)
    return;
}

void loop() {
  checkforNFC();
  checkMailbox();
  checkRegularCard();
}
