// 2023/04/04 Tue.

#include <stdio.h>
#include <string.h>
#include <SPI.h>
#include <Wire.h> 
#include <MFRC522.h>
#include <Adafruit_GFX.h> 
#include <Adafruit_SSD1306.h> 
#define DELAY_TIME 3000

byte theUID[4];
const byte CONST_UID[] = {0x6A, 0x40, 0xA4, 0x36};
const byte CONST_LO[] = {0x08, 0x04, 0x00, 0x47, 0x59, 0x55, 0xD1,0x41, 0x10, 0x36, 0x07};

bool isUID = true;
bool isCopyMode = false;
bool isRead = false;

Adafruit_SSD1306 display(128, 64, &Wire, -1);
MFRC522 mfrc522(10, 9);
MFRC522::MIFARE_Key key;
MFRC522::StatusCode status;

void displayPrint
(
    const bool clearDisplay,
    const uint8_t fontSize, 
    const char str[],
    uint16_t x,
    uint16_t y
)
{
    if(clearDisplay) displayClear();
    
    if(x == -1)  // default
        x = display.getCursorX();
    else if(x == -2) // align horizontal center
        x = int((128-(6*fontSize*strlen(str)-fontSize))/2);

    if(y == -1) // default
        y = display.getCursorY(); 
    else if(y == -2) // align vertical center (for blue area)
        y = 39-int(7*fontSize/2);

    display.setCursor(x, y);
    display.setTextSize(fontSize);
    display.print(str);
    display.display();
}

void displayClear() {
    display.clearDisplay();
    display.display();
}

void setup() {
    pinMode(8, INPUT_PULLUP);

    Serial.begin(9600); 
    SPI.begin();
    mfrc522.PCD_Init();
    for(int i=0; i<6; i++) {
        key.keyByte[i] = 0xFF;
    }
    display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
    displayClear();
    display.setTextColor(1);
    display.setCursor(0, 0);
}

void delayAnimation() {
    delay(DELAY_TIME);
    displayClear();
}

void dumpUID() {
    display.setCursor(0, 22);
    char temp[4];
    for(int i=0; i<4; i++) {
        snprintf(temp, 4, "%02X ", mfrc522.uid.uidByte[i]);
        if(i==2) display.setCursor(0, 40);
        displayPrint(false, 2, temp, -1, -1);
    }
}

void cloneUID(bool isELEV) {
    if(!isRead) displayPrint(true, 2, isELEV?"":"COPY", -2, 0);
    
    byte uidArray[4];
    for(int i=0; i<4; i++) {
        if(isELEV) {
            uidArray[i] = CONST_UID[i];
        }
        else {
            uidArray[i] = theUID[i];
        }
    }
    
    if(isUID) {
        if(mfrc522.MIFARE_SetUid(uidArray, 0x04, true)) {
            displayPrint(true, 2, "UID 0T", 0, 0);
            displayPrint(false, 3, "Success", -2, -2);
            delayAnimation();
            isCopyMode = false;
            isRead = false;
        }
        else {
            // 不是UID 可能是CUID
            isUID = false;
        }
        return;
    }
    else {
        status = (MFRC522::StatusCode) mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, 3, &key, &(mfrc522.uid));
        if(status == MFRC522::STATUS_OK) {
            displayPrint(true, 2, "CUID 0T ", 0, 0);
        }
        else{
            displayPrint(true, 2, "CUID 0F", 0, 0);
            displayPrint(false, 3, "Fail", -2, -2);
            delayAnimation();
            return;
        }

        byte blockArray[16];
        for(int i=0; i<16; i++) {
            if(i <= 3) {
                blockArray[i] = uidArray[i];
            }
            else if(i == 4) {
                blockArray[i] = bccCalculator(uidArray);
            }
            else {
                blockArray[i] = CONST_LO[i-5];
            }
        }

        status = (MFRC522::StatusCode) mfrc522.MIFARE_Write(0, blockArray, 16);
        if (status == MFRC522::STATUS_OK) {
            displayPrint(false, 2, "1T", -1, -1);
            displayPrint(false, 3, "Success", -2, -2);
        }
        else {
            displayPrint(false, 2, "1F", -1, -1);
            displayPrint(false, 3, "Fail", -2, -2);
        }
        isUID = true;
        isCopyMode = false;
        isRead = false;
        delayAnimation();
    }
}

bool isUIDF() {
    if(mfrc522.MIFARE_SetUid(mfrc522.uid.uidByte, 0x04, true)) {
        return true;
    }
    else {
        return false;
    }
}

byte bccCalculator(byte uidArray[]) {
    byte temp = 0x00;
    for(int i=0; i<4; i++) {
        temp ^= uidArray[i];
    }
    return temp;
}

void loop() {
    mfrc522.PCD_Init();

    if(digitalRead(8) == LOW || isCopyMode) {
        if(isRead) {
            while(true) {
                if(mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
                    displayClear();
                    cloneUID(false);
                    return;
                }
            }
        }
        else {
            displayPrint(true, 2, "COPY MODE", -2, 0);
            
            while(true) {
                if(mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
                    displayPrint(true, 2, isUIDF()?"READ UID":"READ CUID", -2, 0);
                    
                    dumpUID();
                    for(int i=0; i<4; i++) {
                        theUID[i] = mfrc522.uid.uidByte[i];
                    }

                    while(true) {
                        if(digitalRead(8) == LOW) {
                            displayPrint(false, 6, "#", 76, -2);
                            isCopyMode = true;
                            isRead = true;
                            return;
                        }
                    }
                }
            }
        }
    }
    else {
        if(!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial()) {
            delay(50);
            return;
        }
        cloneUID(true);
    }
}
