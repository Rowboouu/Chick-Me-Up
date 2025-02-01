
Code for ESP 32 # 1 (Feeder System)

#define BLYNK_TEMPLATE_ID "TMPL6OAPaE7mU"
#define BLYNK_TEMPLATE_NAME "Soil Moisture Temperature and Humidity Monitor"
#define BLYNK_AUTH_TOKEN "cEZYyAjp8TVuDl6oBXftLm02g6PBoXA9"

#define BLYNK_PRINT Serial
#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>


char auth[] = BLYNK_AUTH_TOKEN;
char ssid[] = "Parafiber_2.4G_6AD6"; // type your wifi name
char pass[] = "082121018";           // type your wifi password

// char ssid[] = "Infinix SMART 8";  // type your wifi name
// char pass[] = "hello123";  // type your wifi password

BlynkTimer timer;

// Firebase ------------------------------------------------
#include <FirebaseESP32.h>
#define DATABASE_URL "temperature-ae783-default-rtdb.asia-southeast1.firebasedatabase.app"
#define API_KEY "AIzaSyDHC960NvN6MEZXHbc25fc8_KMziWH_XHg"
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"

FirebaseData fbdo;
FirebaseAuth auth2;
FirebaseConfig config;

bool signupOK = false;

// LCD Display ---------------------------------------------
#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x27, 16, 2);

// ultrasonic sensor ---------------------------------------
#include <HCSR04.h>
#define food_trig 19
#define food_echo 18
#define water_trig 14
#define water_echo 12
float food_height = 30;
float water_height = 30;
float food_max = 100;
float water_max = 100;

// Relay ---------------------------------------------------
#define waterRelay 32
int lowLimit = 20;
int highLimit = 80;
String pumpState = "OFF";

// Servo Motor----------------------------------------------
#include <ESP32Servo.h>
#define servoPin 13
Servo servo;

// temperature initialization ------------------------------
float temp = 0.0;

// ultrasonic: convertion to percentage --------------------
int ultrasonic(int trig, int echo, float MaxLevel, float Container)
{
    digitalWrite(trig, LOW);
    delayMicroseconds(4);
    digitalWrite(trig, HIGH);
    delayMicroseconds(10);
    digitalWrite(trig, LOW);
    long t = pulseIn(echo, HIGH);
    long distance = t / 29 / 2;

    int blynkDistance = ((MaxLevel - distance) / Container) * 100;

    if (distance <= MaxLevel)
    {
        // Blynk.virtualWrite(V4, blynkDistance);
    }
    else
    {
        // Blynk.virtualWrite(V4, 0);
        blynkDistance = 0;
    }
    return (blynkDistance);
}

// sensors doing their magic -------------------------------
void sendSensor()
{
    int foodLevel = ultrasonic(food_trig, food_echo, food_max, food_height);
    int waterLevel = ultrasonic(water_trig, water_echo, water_max, water_height);

    if (waterLevel <= lowLimit)
    {
        digitalWrite(waterRelay, LOW);
        pumpState = "ON";
    }
    else
    {
        if (waterLevel >= highLimit)
        {
            digitalWrite(waterRelay, HIGH);
            pumpState = "OFF";
        }
    }

    // Path in Firebase from where data will be read
    String path = "/sensor/temperature";

    // Read data from Firebase
    if (Firebase.ready() && signupOK)
    {
        if (Firebase.getFloat(fbdo, path))
        {
            temp = fbdo.floatData();
            // Serial.println("Successful READ from " + fbdo.dataPath() + ": " + pwmValue + " (" + fbdo.dataType() + ")");
        }
        else
        {
            Serial.println("FAILED: " + fbdo.errorReason());
        }
    }

    Blynk.virtualWrite(V1, waterLevel);
    Blynk.virtualWrite(V2, foodLevel);
    Serial.println("Water Pump: " + pumpState);
    Serial.println("Temperature:" + String(temp, 2) + "°C");
    Serial.println("Food Level: " + String(foodLevel) + "%");
    Serial.println("Water Level: " + String(waterLevel) + "%");
    Serial.println("---");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("H2O :");
    lcd.print(waterLevel);
    lcd.print("%");
    lcd.setCursor(10, 0);
    lcd.print("Temp:");
    lcd.setCursor(0, 1);
    lcd.print("Food:");
    lcd.print(foodLevel);
    lcd.print("%");
    lcd.setCursor(10, 1);
    lcd.print(temp, 1);
    lcd.print((char)223);
    lcd.print("C");
    delay(1500);
}

void setup()
{
    Serial.begin(115200);
    Serial.println("Hello, ESP32!");
    Blynk.begin(auth, ssid, pass);
    servo.attach(servoPin);
    lcd.init();
    lcd.backlight();
    lcd.setCursor(0, 0);
    pinMode(food_trig, OUTPUT);
    pinMode(food_echo, INPUT);
    pinMode(water_trig, OUTPUT);
    pinMode(water_echo, INPUT);
    pinMode(waterRelay, OUTPUT);
    digitalWrite(waterRelay, HIGH);
    timer.setInterval(100L, sendSensor);
    config.api_key = API_KEY;
    config.database_url = DATABASE_URL;
    if (Firebase.signUp(&config, &auth2, "", ""))
    {
        Serial.println("signUp OK");
        signupOK = true;
    }
    else
    {
        Serial.printf("%s\n", config.signer.signupError.message.c_str());
    }

    config.token_status_callback = tokenStatusCallback;
    Firebase.begin(&config, &auth2);
    Firebase.reconnectWiFi(true);
}

BLYNK_WRITE(V3)
{
    bool servovalue = param.asInt();
    if (servovalue == 1)
    {
        servo.write(0);
        Serial.println("Feed Food: ON");
    }
    else
    {
        servo.write(90);
        Serial.println("Feed Food: OFF");
    }
}

void loop()
{
    Blynk.run();
    timer.run();
}
