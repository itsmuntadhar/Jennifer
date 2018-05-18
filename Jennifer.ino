/*
Project Jennifer, Jen for short, named after the great Jennifer Lawrence.
Created by Muntadhar Haydar Abd Al-Rasoul, known as "sparkist97" and previously "mrmhk97".
Mar. 9th, 2018.
Update 1, March, 18th, 2018, 0130:
    -Implement "Hold on Display"
    -Implement "Compare"
    -Implement "Set to Zero"
Update 2, May, 17th, 2018, 0100:
    -Implement LCD screen.
*/

#define DEBUG 2 //0 for no debug, 1 for normal debug strings and 2 for Data Plotting.
#define USE_LCD //Choose ...
#define USE_7SEG

#include <Arduino.h>
#include <MPU6050.h>
#include <Wire.h>
#include <EEPROM.h>
#ifdef USE_LCD
#include <LiquidCrystal_I2C.h>
#define LCD_REFRESH_MILLIS 1
#endif
#ifdef USE_7SEG
#include <TM1637Display.h>
#endif

#define READINGS_COUNT 25 //Number of reading to take their average.
#define AXISES_COUNT 6
#define AX 0
#define AY 1
#define AZ 2
#define GX 3
#define GY 4
#define GZ 5

#ifdef USE_7SEG
#define CLK 2
#define DIO 3
#define CHANGE_SELECTED_AXIS_TO_VIEW_BUTTON 4
#endif
#define VALUE_HOLD_ON_DISPLAY_BUTTON 5
#define COMPARE_BUTTON 6
#define SET_TO_ZERO_BUTTON 7

#define CHECK_OFFSET_ADDR 0xF
#define CHECK_OFFSET_VALUE 0xAA
#define GX_OFFSET_ADDR 0x10
#define GY_OFFSET_ADDR GX_OFFSET_ADDR + sizeof(int32_t)
#define GZ_OFFSET_ADDR GY_OFFSET_ADDR + sizeof(int32_t)

MPU6050 mpu;
#ifdef USE_7SEG
TM1637Display display(CLK, DIO);
#endif
#ifdef USE_LCD
LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE); // Set the LCD I2C address
#endif

int16_t ax, ay, az;
int16_t gx, gy, gz;

int32_t readings[AXISES_COUNT][READINGS_COUNT]; // the reading history
int32_t readIndex[AXISES_COUNT];                // the index of the current reading
int32_t total[AXISES_COUNT];                    // the running total
int32_t totalReadings[AXISES_COUNT];
int32_t average[AXISES_COUNT]; // the average
int16_t customOffsets[3];

double degs[3] = {0.0, 0.0, 0.0};
int holdOnDisplayButtonLastState = 1, compareButtonLastState = 1, setToZeroButtonLastState = 1,
    selectedAxisOn7Segs = 0, axisToCompare = -1;
bool holdOnDisplay = false, compareValues = false;

#ifdef USE_LCD
unsigned long lcdRefreshTimer = 0, refValues[3] = {0, 0, 0};
#endif
#ifdef USE_7SEG
int changeSelectedAxisLastState = 1, refValue = 0;
uint8_t data[] = {0xff, 0xff, 0xff, 0xff};
#endif

void setup()
{
#ifdef USE_LCD
    lcd.begin(16, 2);
    lcd.backlight();
    lcd.setCursor(0, 0);
    lcd.print("Startup...");
    delay(1000);
    lcd.clear();
#endif

#if DEBUG > 0
    Serial.begin(115200);
#endif

    // initialize device
    mpu.initialize();

    mpu.setXGyroOffset(-1100);
    mpu.setYGyroOffset(271);
    mpu.setZGyroOffset(-60);
    mpu.setXAccelOffset(-2509);
    mpu.setYAccelOffset(-101);
    mpu.setZAccelOffset(925);

#if DEBUG == 1
    // verify connection
    Serial.println(F("Testing device connections..."));
    Serial.println(mpu.testConnection() ? F("MPU connection successful") : F("MPU connection failed"));
    while (1)
        ;
#endif
#ifdef USE_7SEG
    pinMode(CHANGE_SELECTED_AXIS_TO_VIEW_BUTTON, INPUT_PULLUP);
#endif
    pinMode(VALUE_HOLD_ON_DISPLAY_BUTTON, INPUT_PULLUP);
    pinMode(COMPARE_BUTTON, INPUT_PULLUP);
    pinMode(SET_TO_ZERO_BUTTON, INPUT_PULLUP);
    pinMode(BUZZER, OUTPUT);
    pinMode(COMPARE_LED, OUTPUT);

    // zero-fill all the arrays:
    for (int axis = 0; axis < AXISES_COUNT; axis++)
    {
        readIndex[axis] = 0;
        total[axis] = 0;
        totalReadings[axis] = 0;
        average[axis] = 0;
        for (int i = 0; i < READINGS_COUNT; i++)
        {
            readings[axis][i] = 0;
        }
    }
    checkCustomOffsets();
}

void loop()
{
#ifdef USE_7SEG
    if (changeSelectedAxisLastState != digitalRead(CHANGE_SELECTED_AXIS_TO_VIEW_BUTTON))
    {
        changeSelectedAxisLastState = digitalRead(CHANGE_SELECTED_AXIS_TO_VIEW_BUTTON);
        if (changeSelectedAxisLastState)
        {
            selectedAxisOn7Segs = selectedAxisOn7Segs == 0 ? 1 : 0;
        }
    }
#endif
    if (holdOnDisplayButtonLastState != digitalRead(VALUE_HOLD_ON_DISPLAY_BUTTON))
    {
        holdOnDisplayButtonLastState = digitalRead(VALUE_HOLD_ON_DISPLAY_BUTTON);
        if (holdOnDisplayButtonLastState)
        {
            holdOnDisplay = !holdOnDisplay;
        }
    }
    if (compareButtonLastState != digitalRead(COMPARE_BUTTON))
    {
        compareButtonLastState = digitalRead(COMPARE_BUTTON);
        if (compareButtonLastState)
        {
            compareValues = !compareValues;
#ifdef USE_LCD
            lcd.clear();
#endif
            if (compareValues)
            {
#ifdef USE_LCD
                refValues[0] = degs[0];
                refValues[1] = degs[1];
                refValues[2] = degs[2];
#endif
#ifdef USE_7SEG
                axisToCompare = selectedAxisOn7Segs;
                refValue = degs[selectedAxisOn7Segs];
#endif
            }
        }
    }
    if (setToZeroButtonLastState != digitalRead(SET_TO_ZERO_BUTTON))
    {
        setToZeroButtonLastState = digitalRead(SET_TO_ZERO_BUTTON);
        if (setToZeroButtonLastState)
        {
            setToZero();
        }
    }
    //read raw accel/gyro measurements from device
    mpu.getMotion6(&gx, &gy, &gz, &ax, &ay, &az);
    gx += customOffsets[0];
    gy += customOffsets[1];
    gz += customOffsets[2];
    smooth(AX, ax);
    smooth(AY, ay);
    smooth(AZ, az);
    smooth(GX, gx);
    smooth(GY, gy);
    smooth(GZ, gz);

    degs[0] = (average[GX] / 16500.0) * 90.0, degs[1] = (average[GY] / 16500.0) * 90.0, degs[2] = (average[GZ] / 16500.0) * 90.0;
    if (compareValues)
    {
#ifdef USE_LCD
        lcd.setCursor(0, 0);
        lcd.print("(D|)" + String((int)(degs[0]) - (int)(refValues[0])) + " ");
        lcd.setCursor(0, 1);
        lcd.print("(D-)" + String((int)(degs[1]) - (int)(refValues[1])) + " ");
#endif
#ifdef USE_7SEG
        selectedAxisOn7Segs = axisToCompare;
        int value = degs[selectedAxisOn7Segs] - refValue;
        data[0] = display.encodeDigit(0xC);
        data[1] = value < 0 ? 0b01000000 : 0x0;
        data[2] = display.encodeDigit(abs(value) / 10);
        data[3] = display.encodeDigit(abs(value) % 10);
        display.setSegments(data);
        digitalWrite(BUZZER, value == 0);
        digitalWrite(COMPARE_LED, value == 0);
#endif
    }
    if (!holdOnDisplay && !compareValues)
    {
#ifdef USE_LCD
        if (millis() - lcdRefreshTimer >= LCD_REFRESH_MILLIS)
        {
            lcd.setCursor(0, 0);
            lcd.print("(|)" + String((int)degs[0]) + " ");
            lcd.setCursor(0, 1);
            lcd.print("(-)" + String((int)degs[1]) + " ");
            lcdRefreshTimer = millis();
        }
#endif
#ifdef USE_7SEG
        int b = (int)abs(degs[selectedAxisOn7Segs]);
        data[0] = display.encodeDigit(selectedAxisOn7Segs);
        data[1] = degs[selectedAxisOn7Segs] < 0 ? 0b01000000 : 0x0;
        data[2] = display.encodeDigit(b / 10);
        data[3] = display.encodeDigit(b % 10);
        display.setSegments(data);
#endif
    }

#if DEBUG == 1
    Serial.print("alpha\t");
    Serial.print(degs[0]);
    Serial.print("\tbeta\t");
    Serial.print(degs[1]);
    Serial.print("\tgamma\t");
    Serial.println(degs[2]);
#elif DEBUG == 2
    Serial.print(16500);
    Serial.print("\t");
    Serial.print(average[GX]);
    Serial.print("\t");
    Serial.print(average[GY]);
    Serial.print("\t");
    Serial.print(average[GZ]);
    Serial.print("\t");
    Serial.print(-16500);
    Serial.println("\t");
#endif
}

void smooth(int axis, int32_t val)
{
    // pop and subtract the last reading:
    total[axis] -= readings[axis][readIndex[axis]];
    total[axis] += val;

    // add value to running total
    readings[axis][readIndex[axis]] = val;
    readIndex[axis]++;

    if (readIndex[axis] >= READINGS_COUNT)
        readIndex[axis] = 0;

    // calculate the average:
    average[axis] = total[axis] / READINGS_COUNT;
}

void setToZero()
{
    for (byte i = 0; i < 3; i++)
    {
        customOffsets[i] = (-1) * (i == 0 ? gx : i == 1 ? gy : gz);
        for (byte j = 0; j < sizeof(int16_t); j++)
        {
            EEPROM.write(GX_OFFSET_ADDR + j + (i * sizeof(int16_t)), customOffsets[i] >> (sizeof(int8_t) * j));
        }
    }
}

void checkCustomOffsets()
{
    if (EEPROM.read(CHECK_OFFSET_ADDR) != CHECK_OFFSET_VALUE)
    {
        EEPROM.write(CHECK_OFFSET_ADDR, CHECK_OFFSET_VALUE);
        for (byte i = GX_OFFSET_ADDR; i < GX_OFFSET_ADDR + 12; i++)
        {
            EEPROM.write(i, 0);
        }
    }
    else
    {
        for (byte i = 0; i < 3; i++)
        {
            customOffsets[i] = 0;
            for (byte j = 0; j < sizeof(int16_t); j++)
            {
                customOffsets[i] |= (EEPROM.read(GX_OFFSET_ADDR + j + (i * sizeof(int16_t))) << (sizeof(int8_t) * j));
            }
        }
    }
}