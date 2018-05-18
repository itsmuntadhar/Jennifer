# 1 "d:\\_Engineering\\Projects\\Jennifer\\Jennifer.ino"
# 1 "d:\\_Engineering\\Projects\\Jennifer\\Jennifer.ino"
/*
Project Jennifer, Jen for short, named after the great Jennifer Lawrence.
Created by Muntadhar Haydar Abd Al-Rasoul, known as "sparkist97" and previously "mrmhk97".
Mar. 9th, 2018.
Update 1, March, 18th, 2018, 0130:
    -Implement "Hold on Display"
    -Implement "Compare"
    -Implement "Set to Zero"
*/





# 16 "d:\\_Engineering\\Projects\\Jennifer\\Jennifer.ino" 2
# 17 "d:\\_Engineering\\Projects\\Jennifer\\Jennifer.ino" 2
# 18 "d:\\_Engineering\\Projects\\Jennifer\\Jennifer.ino" 2
# 19 "d:\\_Engineering\\Projects\\Jennifer\\Jennifer.ino" 2

# 21 "d:\\_Engineering\\Projects\\Jennifer\\Jennifer.ino" 2



# 25 "d:\\_Engineering\\Projects\\Jennifer\\Jennifer.ino" 2
# 58 "d:\\_Engineering\\Projects\\Jennifer\\Jennifer.ino"
MPU6050 mpu;

TM1637Display display(2, 3);


LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE); // Set the LCD I2C address


int16_t ax, ay, az;
int16_t gx, gy, gz;

int32_t readings[6][25 /*Number of reading to take their average.*/]; // the reading history
int32_t readIndex[6]; // the index of the current reading
int32_t total[6]; // the running total
int32_t totalReadings[6];
int32_t average[6]; // the average
int16_t customOffsets[3];
double degs[3] = {0.0, 0.0, 0.0};
int holdOnDisplayButtonLastState = 1, compareButtonLastState = 1, setToZeroButtonLastState = 1,
    selectedAxisOn7Segs = 0, axisToCompare = -1;
bool holdOnDisplay = false, compareValues = false;


unsigned long lcdRefreshTimer = 0, refValues[3] = {0, 0, 0};


int changeSelectedAxisLastState = 1, refValue = 0;
uint8_t data[] = {0xff, 0xff, 0xff, 0xff};


void setup()
{

    lcd.begin(16, 2);
    lcd.backlight();
    lcd.setCursor(0, 0);
    lcd.print("System was started.");
    delay(1000);
    lcd.clear();



    Serial.begin(115200);


    // initialize device
    mpu.initialize();

    mpu.setXGyroOffset(-1100);
    mpu.setYGyroOffset(271);
    mpu.setZGyroOffset(-60);
    mpu.setXAccelOffset(-2509);
    mpu.setYAccelOffset(-101);
    mpu.setZAccelOffset(925);







    pinMode(4, 0x2);

    pinMode(5, 0x2);
    pinMode(6, 0x2);
    pinMode(7, 0x2);
    pinMode(8, 0x1);
    pinMode(13, 0x1);

    // zero-fill all the arrays:
    for (int axis = 0; axis < 6; axis++)
    {
        readIndex[axis] = 0;
        total[axis] = 0;
        totalReadings[axis] = 0;
        average[axis] = 0;
        for (int i = 0; i < 25 /*Number of reading to take their average.*/; i++)
        {
            readings[axis][i] = 0;
        }
    }
    checkCustomOffsets();
}

void loop()
{
    digitalWrite(8, compareValues ? digitalRead(8) : 0x0);
    digitalWrite(13, compareValues ? digitalRead(13) : 0x0);


    if (changeSelectedAxisLastState != digitalRead(4))
    {
        changeSelectedAxisLastState = digitalRead(4);
        if (changeSelectedAxisLastState)
        {
            selectedAxisOn7Segs = selectedAxisOn7Segs == 0 ? 1 : 0;
        }
    }

    if (holdOnDisplayButtonLastState != digitalRead(5))
    {
        holdOnDisplayButtonLastState = digitalRead(5);
        if (holdOnDisplayButtonLastState)
        {
            holdOnDisplay = !holdOnDisplay;
        }
    }
    if (compareButtonLastState != digitalRead(6))
    {
        compareButtonLastState = digitalRead(6);
        if (compareButtonLastState)
        {
            compareValues = !compareValues;

            lcd.clear();

            if (compareValues)
            {

                refValues[0] = degs[0];
                refValues[1] = degs[1];
                refValues[2] = degs[2];


                axisToCompare = selectedAxisOn7Segs;
                refValue = degs[selectedAxisOn7Segs];

            }
        }
    }
    if (setToZeroButtonLastState != digitalRead(7))
    {
        setToZeroButtonLastState = digitalRead(7);
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
    smooth(0, ax);
    smooth(1, ay);
    smooth(2, az);
    smooth(3, gx);
    smooth(4, gy);
    smooth(5, gz);
    //average[GX] += customOffsets[0];
    //average[GY] += customOffsets[1];
    //average[GZ] += customOffsets[2];

    degs[0] = (average[3] / 16500.0) * 90.0, degs[1] = (average[4] / 16500.0) * 90.0, degs[2] = (average[5] / 16500.0) * 90.0;
    if (compareValues)
    {

        lcd.setCursor(0, 0);
        lcd.print("(D|)" + String((int)(degs[0]) - (int)(refValues[0])) + " ");
        lcd.setCursor(0, 1);
        lcd.print("(D-)" + String((int)(degs[1]) - (int)(refValues[1])) + " ");


        selectedAxisOn7Segs = axisToCompare;
        int value = degs[selectedAxisOn7Segs] - refValue;
        data[0] = display.encodeDigit(0xC);
        data[1] = value < 0 ? 0b01000000 : 0x0;
        data[2] = display.encodeDigit(((value)>0?(value):-(value)) / 10);
        data[3] = display.encodeDigit(((value)>0?(value):-(value)) % 10);
        display.setSegments(data);
        digitalWrite(8, value == 0);
        digitalWrite(13, value == 0);

    }
    if (!holdOnDisplay && !compareValues)
    {

        if (millis() - lcdRefreshTimer >= 1)
        {
            //lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("(|)" + String((int)degs[0]) + " ");
            lcd.setCursor(0, 1);
            lcd.print("(-)" + String((int)degs[1]) + " ");
            lcdRefreshTimer = millis();
        }


        int b = (int)((degs[selectedAxisOn7Segs])>0?(degs[selectedAxisOn7Segs]):-(degs[selectedAxisOn7Segs]));
        data[0] = display.encodeDigit(selectedAxisOn7Segs);
        data[1] = degs[selectedAxisOn7Segs] < 0 ? 0b01000000 : 0x0;
        data[2] = display.encodeDigit(b / 10);
        data[3] = display.encodeDigit(b % 10);
        display.setSegments(data);

    }
# 263 "d:\\_Engineering\\Projects\\Jennifer\\Jennifer.ino"
    Serial.print(16500);
    Serial.print("\t");
    Serial.print(average[3]);
    Serial.print("\t");
    Serial.print(gx);
    Serial.print("\t");
    Serial.print(-16500);
    Serial.println("\t");

}

void smooth(int axis, int32_t val)
{
    // pop and subtract the last reading:
    total[axis] -= readings[axis][readIndex[axis]];
    total[axis] += val;

    // add value to running total
    readings[axis][readIndex[axis]] = val;
    readIndex[axis]++;

    if (readIndex[axis] >= 25 /*Number of reading to take their average.*/)
        readIndex[axis] = 0;

    // calculate the average:
    average[axis] = total[axis] / 25 /*Number of reading to take their average.*/;
}

void setToZero()
{
    for (byte i = 0; i < 3; i++)
    {
        customOffsets[i] = (-1) * (i == 0 ? gx : i == 1 ? gy : gz);
        for (byte j = 0; j < sizeof(int16_t); j++)
        {
            EEPROM.write(0x10 + j + (i * sizeof(int16_t)), customOffsets[i] >> (sizeof(int8_t) * j));
        }
    }
}

void checkCustomOffsets()
{
    if (EEPROM.read(0xF) != 0xAA)
    {
        EEPROM.write(0xF, 0xAA);
        for (byte i = 0x10; i < 0x10 + 12; i++)
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
                customOffsets[i] |= (EEPROM.read(0x10 + j + (i * sizeof(int16_t))) << (sizeof(int8_t) * j));
            }
        }
    }
}
