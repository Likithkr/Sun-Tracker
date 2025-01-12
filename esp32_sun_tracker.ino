#include <math.h>
#include <ESP32Servo.h>
#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <string.h>
#include <DHT.h>
//initialize DTH sensor
DHT dht(15, DHT11);
BLEServer *pServer = NULL;
BLECharacteristic
  *voltCharacteristic = NULL,
  *tempCharacteristic = NULL,
  *humidCharacteristic = NULL,
  *LDR1Characteristic = NULL,
  *LDR2Characteristic = NULL,
  *LDR3Characteristic = NULL,
  *LDR4Characteristic = NULL,
  *CPUTempCharacteristic=NULL;
//bluetooth config
bool deviceConnected = false;
bool oldDeviceConnected = false;
#define SERVICE_UUID "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define VOLT_CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"
#define TEMP_CHARACTERSTICS_UUID "ad0e6e31-6647-4a50-bf5c-8ed866db2d39"
#define HUMID_CHARACTERSTICS_UUID "bcf8eb60-59e0-42a5-a6e6-3d38d54692ca"
#define CPU_TEMP_CHARACTERSTICS_UUID "cef8fa83-9843-4d27-875a-1b09ec250937"
//ldr sensor char
#define LDR1_CHARACTERSTICS_UUID "f9e89d47-fcbe-492a-b6c1-ec4f2832a97a"
#define LDR2_CHARACTERSTICS_UUID "f4f000bd-ae16-487e-a450-9ccffbd57dcb"
#define LDR3_CHARACTERSTICS_UUID "b2661cfe-014c-4a17-9bb3-333c0127d126"
#define LDR4_CHARACTERSTICS_UUID "0d8ab83d-65e6-47dd-bc83-707fa70f80c8"
// Define the ADC pin
const int adcPin = 4;                // Use an ADC-capable GPIO pin of the ESP32
const float adcResolution = 4096.0;  // 12-bit resolution
const float referenceVoltage = 3.3;  // ESP32's ADC reference voltage
float temp() {
  float temp = dht.readTemperature();
  return temp;
}
float humidity() {
  float humid = dht.readHumidity();
  return humid;
}
class MyServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer *pServer) {
    deviceConnected = true;
    BLEDevice::startAdvertising();
  };

  void onDisconnect(BLEServer *pServer) {
    deviceConnected = false;
  }
};
float voltRead() {
  int rawAdcValue = analogRead(adcPin);  // Read raw ADC value
  //float voltage = (rawAdcValue / adcResolution) * referenceVoltage; // Convert to voltage
  float voltage = map(rawAdcValue, 0, 4095, 0, 6500);
  Serial.print("Solar Panel Voltage: ");
  //Serial.print(rawAdcValue);
  voltage = voltage / 1000;
  Serial.print(voltage);  // Display voltage with 2 decimal places
  Serial.println(" V");
  return voltage;
}

// Servo configuration
Servo servoHorizontal, servoVertical;
const int SERVO_HORIZONTAL_PIN = 32;
const int SERVO_VERTICAL_PIN = 13;
int servoHorizontalAngle = 90;  // Initial horizontal servo angle
int servoVerticalAngle = 45;    // Initial vertical servo angle

// LDR pin connections
const int LDR_TOP_LEFT = 14;      // Top-left LDR
const int LDR_TOP_RIGHT = 27;     // Top-right LDR
const int LDR_BOTTOM_LEFT = 26;   // Bottom-left LDR
const int LDR_BOTTOM_RIGHT = 33;  // Bottom-right LDR

// Servo angle limits
const int HORIZONTAL_LEFT_LIMIT = 20;
const int HORIZONTAL_RIGHT_LIMIT = 170;
const int VERTICAL_UP_LIMIT = 25;
const int VERTICAL_DOWN_LIMIT = 150;

// Sensitivity threshold
const int TOLERANCE = 11;

void setup() {
  dht.begin();
  Serial.begin(9600);  // Start serial communication
  // Allocate all timers for ESP32 PWM
  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  ESP32PWM::allocateTimer(2);
  ESP32PWM::allocateTimer(3);
  // Configure servos
  servoHorizontal.setPeriodHertz(50);  // Standard 50 Hz servo
  servoVertical.setPeriodHertz(50);
  servoHorizontal.attach(SERVO_HORIZONTAL_PIN, 500, 2500);
  servoVertical.attach(SERVO_VERTICAL_PIN, 500, 2500);

  // Initialize servos to default positions
  servoHorizontal.write(servoHorizontalAngle);
  delay(100);
  servoVertical.write(servoVerticalAngle);
  delay(100);

  //Solar panel voltage read
  analogReadResolution(12);        // Set ADC resolution to 12-bit
  analogSetAttenuation(ADC_11db);  // Extend the ADC range to cover the full 3.3V

  // Create the BLE Device
  BLEDevice::init("ESP32");

  // Create the BLE Server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create the BLE Service
  BLEService *pService = pServer->createService(BLEUUID(SERVICE_UUID),30,0);

  // Create a BLE Characteristic
  CPUTempCharacteristic = pService->createCharacteristic(
    CPU_TEMP_CHARACTERSTICS_UUID,
    BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY | BLECharacteristic::PROPERTY_INDICATE);
  CPUTempCharacteristic->addDescriptor(new BLE2902());
  voltCharacteristic = pService->createCharacteristic(
    VOLT_CHARACTERISTIC_UUID,
    //BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_NOTIFY | BLECharacteristic::PROPERTY_INDICATE);
    BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY | BLECharacteristic::PROPERTY_INDICATE);
  voltCharacteristic->addDescriptor(new BLE2902());

  humidCharacteristic = pService->createCharacteristic(
    HUMID_CHARACTERSTICS_UUID,
    BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY | BLECharacteristic::PROPERTY_INDICATE);
  humidCharacteristic->addDescriptor(new BLE2902());

  tempCharacteristic = pService->createCharacteristic(
    TEMP_CHARACTERSTICS_UUID,
    BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY | BLECharacteristic::PROPERTY_INDICATE);
  tempCharacteristic->addDescriptor(new BLE2902());

  LDR1Characteristic = pService->createCharacteristic(
    LDR1_CHARACTERSTICS_UUID,
    BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY | BLECharacteristic::PROPERTY_INDICATE);
  LDR1Characteristic->addDescriptor(new BLE2902());

  LDR2Characteristic = pService->createCharacteristic(
    LDR2_CHARACTERSTICS_UUID,
    BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY | BLECharacteristic::PROPERTY_INDICATE);
  LDR2Characteristic->addDescriptor(new BLE2902());

  LDR3Characteristic = pService->createCharacteristic(
    LDR3_CHARACTERSTICS_UUID,
    BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY | BLECharacteristic::PROPERTY_INDICATE);
  LDR3Characteristic->addDescriptor(new BLE2902());

  LDR4Characteristic = pService->createCharacteristic(
    LDR4_CHARACTERSTICS_UUID,
    BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY | BLECharacteristic::PROPERTY_INDICATE);
  LDR4Characteristic->addDescriptor(new BLE2902());

  // Start the service
  pService->start();

  // Start advertising
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(false);
  pAdvertising->setMinPreferred(0x0);  // set value to 0x00 to not advertise this parameter
  BLEDevice::startAdvertising();
  Serial.println("Waiting a client connection to notify...");
}

void loop() {
  // Read LDR values
  int ldrTopLeft = analogRead(LDR_TOP_LEFT);
  int ldrTopRight = analogRead(LDR_TOP_RIGHT);
  int ldrBottomLeft = analogRead(LDR_BOTTOM_LEFT);
  int ldrBottomRight = analogRead(LDR_BOTTOM_RIGHT);

  // Map LDR values to percentage (0-100)
  ldrTopLeft = map(ldrTopLeft, 0, 4095, 0, 100);
  ldrTopRight = map(ldrTopRight, 0, 4095, 0, 100);
  ldrBottomLeft = map(ldrBottomLeft, 0, 4095, 0, 100);
  ldrBottomRight = map(ldrBottomRight, 0, 4095, 0, 100);

  // Print LDR readings for debugging
  Serial.print("Top Left: ");
  Serial.print(ldrTopLeft);
  Serial.print(" | Top Right: ");
  Serial.print(ldrTopRight);
  Serial.print(" | Bottom Left: ");
  Serial.print(ldrBottomLeft);
  Serial.print(" | Bottom Right: ");
  Serial.println(ldrBottomRight);

  // Calculate average light intensity
  int avgTop = (ldrTopLeft + ldrTopRight) / 2;
  int avgBottom = (ldrBottomLeft + ldrBottomRight) / 2;
  int avgLeft = (ldrTopLeft + ldrBottomLeft) / 2;
  int avgRight = (ldrTopRight + ldrBottomRight) / 2;

  // Horizontal movement
  if (avgLeft > avgRight + TOLERANCE && servoHorizontalAngle > HORIZONTAL_LEFT_LIMIT) {
    servoHorizontalAngle = servoHorizontalAngle - 1;
    servoHorizontal.write(servoHorizontalAngle);
    Serial.println("Moving Left");
  } else if (avgRight > avgLeft + TOLERANCE && servoHorizontalAngle < HORIZONTAL_RIGHT_LIMIT) {
    servoHorizontalAngle = servoHorizontalAngle + 1;
    servoHorizontal.write(servoHorizontalAngle);
    Serial.println("Moving Right");
  }

  // Vertical movement
  if (avgTop > avgBottom + TOLERANCE && servoVerticalAngle > VERTICAL_UP_LIMIT) {
    servoVerticalAngle = servoVerticalAngle - 1;
    servoVertical.write(servoVerticalAngle);
    Serial.println("Moving Up");
  } else if (avgBottom > avgTop + TOLERANCE && servoVerticalAngle < VERTICAL_DOWN_LIMIT) {
    servoVerticalAngle = servoVerticalAngle + 1;
    servoVertical.write(servoVerticalAngle);
    Serial.println("Moving Down");
  }
  //for sending data over bluetooth to client
  if (deviceConnected) {
    String valueString = String(voltRead());
    String humidString = String(humidity());
    String tempString = String(temp());
    CPUTempCharacteristic->setValue(String(temperatureRead()).c_str());
    CPUTempCharacteristic->notify();
    delay(25);
    voltCharacteristic->setValue(valueString.c_str());
    voltCharacteristic->notify();
    delay(10);
    humidCharacteristic->setValue(humidString.c_str());
    humidCharacteristic->notify();
    delay(10);
    tempCharacteristic->setValue(tempString.c_str());
    tempCharacteristic->notify();
    delay(10);
    LDR1Characteristic->setValue(String(ldrTopLeft).c_str());
    LDR1Characteristic->notify();
    delay(10);
    LDR2Characteristic->setValue(String(ldrTopRight).c_str());
    LDR2Characteristic->notify();
    delay(10);
    LDR3Characteristic->setValue(String(ldrBottomLeft).c_str());
    LDR3Characteristic->notify();
    delay(10);
    LDR4Characteristic->setValue(String(ldrBottomRight).c_str());
    LDR4Characteristic->notify();
    delay(10);
  }
  // disconnecting
  if (!deviceConnected && oldDeviceConnected) {
    delay(100);                   // give the bluetooth stack the chance to get things ready
    pServer->startAdvertising();  // restart advertising
    Serial.println("start advertising");
    oldDeviceConnected = deviceConnected;
  }
  // connecting
  if (deviceConnected && !oldDeviceConnected) {
    // do stuff here on connecting
    oldDeviceConnected = deviceConnected;
  }
}
