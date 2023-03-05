// 2023/03/15 Sun.

#include <stdio.h>
#include <MFRC522.h>
#include <SPI.h>
#include <LiquidCrystal_PCF8574.h>

#define RST_PIN 9          
#define SS_PIN 10
#define DELAY_TIME 3000

byte theUID[4];
const byte CONST_UID[] = {0x6A, 0x40, 0xA4, 0x36};
const byte CONST_LO[] = {0x08, 0x04, 0x00, 0x47, 0x59, 0x55, 0xD1,0x41, 0x10, 0x36, 0x07};

bool isUID = true;
bool isCopyMode = false;
bool isRead = false;

LiquidCrystal_PCF8574 lcd(0x27);
MFRC522 mfrc522(SS_PIN, RST_PIN);
MFRC522::MIFARE_Key key;
MFRC522::StatusCode status;

void setup() {
    pinMode(8, INPUT_PULLUP);

    Serial.begin(9600); 
    SPI.begin();
    lcd.begin(16,2);
    mfrc522.PCD_Init();
    for(int i=0; i<6; i++) {
        key.keyByte[i] = 0xFF;
    }
}

void delayAnimation() {
    delay(DELAY_TIME);
    lcd.clear();
    lcd.setBacklight(0);
}

void dumpUID() {
    lcd.setCursor(0,1);
    char temp[4];
    for(int i=0; i<5; i++) {
        snprintf(temp, 4, "%02X ", i==4 ? bccCalculator(mfrc522.uid.uidByte) : mfrc522.uid.uidByte[i]);            
        lcd.print(temp);
    }
}

void cloneUID(bool isELEV) {
    lcd.setCursor(0,0);
    lcd.print(isELEV ? "ELEV " : "COPY ");
    
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
            lcd.print(" UID 0T");
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
            lcd.print("CUID 0T ");
        }
        else{
            lcd.print("CUID 0F");
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
            lcd.print("1T");
        }
        else {
            lcd.print("1F");
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
                    lcd.clear();
                    dumpUID();
                    cloneUID(false);
                    return;
                }
            }
        }
        else {
            lcd.setBacklight(255);
            lcd.setCursor(6,0);
            lcd.print("COPY");
            lcd.setCursor(6,1);
            lcd.print("MODE");
            
            while(true) {
                if(mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
                    lcd.clear();
                    lcd.setCursor(0,0);
                    lcd.print("Read ");
                    lcd.print(isUIDF() ? " UID" : "CUID");
                    
                    dumpUID();
                    for(int i=0; i<4; i++) {
                        theUID[i] = mfrc522.uid.uidByte[i];
                    }

                    while(true) {
                        if(digitalRead(8) == LOW) {
                            lcd.setCursor(13,0);
                            lcd.print("###");
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
        lcd.setBacklight(255);
        dumpUID();
        cloneUID(true);
    }
}