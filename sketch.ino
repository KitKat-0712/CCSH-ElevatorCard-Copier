#include <SPI.h>
#include <Wire.h>
#include <MFRC522.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define BTN_PIN 8
#define RST_PIN 9
#define SS_PIN 10
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define DELAY 1000
byte NEW_ID[] = {0x6A, 0x40, 0xA4, 0x36};

MFRC522 mfrc522(SS_PIN, RST_PIN);
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
bool isUID = NULL;
bool rc522_is_disc = NULL;
bool ssd1306_is_disc = NULL;

void pln(String msg, bool clear = false)
{
    if (clear)
    {
        display.clearDisplay();
    }
    display.println(msg);
    display.display();
}
void p(String msg)
{
    display.print(msg);
    display.display();
}

void setup()
{
    pinMode(BTN_PIN, INPUT_PULLUP);
    SPI.begin();
    Wire.begin();
    Serial.begin(115200);
    mfrc522.PCD_Init();
    display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
    display.clearDisplay();
    display.setCursor(0, 0);
    display.setTextSize(2);
    display.setTextColor(1);
    display.display();
}

void loop()
{
    if (!mfrc522.PCD_PerformSelfTest())
    {
        rc522_is_disc = true;
        display.clearDisplay();
        display.setCursor(0, 0);
        display.println("RC522 DISC");
        display.display();
    }
    else if (rc522_is_disc != NULL && rc522_is_disc == true)
    {
        rc522_is_disc = NULL;
        display.clearDisplay();
        display.display();
    }

    if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial())
    {
        display.setCursor(0, 0);
        pln("READ", true);

        byte origId[4] = {mfrc522.uid.uidByte[0], mfrc522.uid.uidByte[1], mfrc522.uid.uidByte[2], mfrc522.uid.uidByte[3]};
        if (mfrc522.MIFARE_SetUid(origId, 4, false))
        {
            isUID = true;
            pln("UID");
        }
        else
        {
            isUID = false;
            pln("CUID");
        }

        for (byte b : origId)
        {
            char temp[3];
            snprintf(temp, 3, "%02X", b);
            display.print(temp);
            display.setCursor(display.getCursorX() + 8, display.getCursorY());
        }
        display.display();

        mfrc522.PICC_HaltA();
        mfrc522.PCD_StopCrypto1();

        display.setCursor(0, 0);
        pln("READ-");
    }
    else if (digitalRead(BTN_PIN) == LOW && isUID != NULL)
    {
        display.setCursor(0, 0);
        pln("WRITE", true);
        while (!(mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()))
            ;

        if (isUID)
        {
            pln("UID");
            if (mfrc522.MIFARE_SetUid(NEW_ID, 4, false))
            {
                pln("Success");
            }
            else
            {
                pln("Failed");
            }
        }
        else
        {
            pln("CUID(ARW)");

            MFRC522::MIFARE_Key key({0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF});
            if ((MFRC522::StatusCode)mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, 3, &key, &(mfrc522.uid)) == MFRC522::STATUS_OK)
            {
                p("A>");
                byte blockArray[16] = {NEW_ID[0], NEW_ID[1], NEW_ID[2], NEW_ID[3], 0xB8};
                byte buffer[18];
                byte bufferSize = 18;
                if ((MFRC522::StatusCode)mfrc522.MIFARE_Read(0, buffer, &bufferSize) == MFRC522::STATUS_OK)
                {
                    p("R>");
                    for (byte i = 5; i < 16; ++i)
                    {
                        blockArray[i] = buffer[i];
                    }

                    if ((MFRC522::StatusCode)mfrc522.MIFARE_Write(0, blockArray, 16) == MFRC522::STATUS_OK)
                    {
                        pln("W");
                    }
                    else
                    {
                        pln("#");
                    }
                }
                else
                {
                    pln("#");
                }
            }
            else
            {
                pln("#");
            }
        }

        mfrc522.PICC_HaltA();
        mfrc522.PCD_StopCrypto1();
        display.setCursor(0, 0);
        isUID = NULL;
        pln("WRITE-");
    }
    delay(DELAY);
}
