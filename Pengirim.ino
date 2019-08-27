#include <SPI.h>
#include <LoRa.h>
#include <OneWire.h>
#include <DallasTemperature.h>

#define led_merah 32
#define led_hijau 33

char buff[40]; // variabel penampung untuk mengirim
/*
    LoRa Communation sender
    declarate pin use
*/
//define the pins used by the transceiver module
#define ss 5      // pin SS
#define rst 14    // pin reset
#define dio0 2    // pin di02

// tempC for temperature ds18b20
float tempC;
float phValue;

// use for dummy sender using lora
//float phvalue = 7.1;
//float tempe = 29;

void LoRa_sender() {
  while (!Serial);
  Serial.println("LoRa Sender");
  //setup LoRa transceiver module
  LoRa.setPins(ss, rst, dio0);
  //replace the LoRa.begin(---E-) argument with your location's frequency
  //433E6 for Asia
  //866E6 for Europe
  //915E6 for North America
  while (!LoRa.begin(433E6)) {
    Serial.println(".");
    delay(500);
  }
  // Change sync word (0xF3) to match the receiver
  // The sync word assures you don't get LoRa messages from other LoRa transceivers
  // ranges from 0-0xFF
  LoRa.setSyncWord(0xBA);
  Serial.println("LoRa Initializing OK!");
}

/*
    PIR sensor inisialisasi
    Use pin
    +5V
    GND
    36
*/
// Data wire is plugged into pin
#define pir_pin 36 // PIR pin connected to GPIO 36 VP
int pirState = LOW;             // we start, assuming no motion detected
int val = 0;                    // variable for reading the pin status


/*
    PH Sensor inisialisasi
    Using pin
    +5V
    GND board
    GND Ph meter
    Po
*/
#define ph_pin 25// PH pin connected to GPIOO 26
float calibration = 121.07 ; //change this value to calibrate
int sensorValue = 0;
unsigned long int avgValue;
float b;
int buf[10], temp;

/* DS18B20 Sensor inisialisasi */
// Data wire is plugged into pin 27
#define ONE_WIRE_BUS 27

// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature.
DallasTemperature sensors(&oneWire);

// arrays to hold device address
DeviceAddress insideThermometer;

void insiasi_ds18b20() {
  Serial.println("Dallas Temperature IC Control Library Demo");

  // locate devices on the bus
  Serial.print("Locating devices...");
  sensors.begin();
  Serial.print("Found ");
  Serial.print(sensors.getDeviceCount(), DEC);
  Serial.println(" devices.");

  // report parasite power requirements
  Serial.print("Parasite power is: ");
  if (sensors.isParasitePowerMode()) Serial.println("ON");
  else Serial.println("OFF");

  // Method
  // Search for devices on the bus and assign based on an index. Ideally,
  // you would do this to initially discover addresses on the bus and then
  // use those addresses and manually assign them (see above) once you know
  // the devices on your bus (and assuming they don't change).
  if (!sensors.getAddress(insideThermometer, 0)) Serial.println("Unable to find address for Device 0");
  // show the addresses we found on the bus
  Serial.print("Device 0 Address: ");
  printAddress(insideThermometer);
  Serial.println();

  // set the resolution to 9 bit (Each Dallas/Maxim device is capable of several different resolutions)
  sensors.setResolution(insideThermometer, 9);

  Serial.print("Device 0 Resolution: ");
  Serial.print(sensors.getResolution(insideThermometer), DEC);
  Serial.println();
}

void setup() {
  //initialize Serial Monitor
  Serial.begin(115200);
  Serial.println(" DIYA ");
  // void lora tx
  LoRa_sender();

  // declare sensor as input
  pinMode(pir_pin, INPUT);
  pinMode(led_merah, OUTPUT);
  pinMode(led_hijau, OUTPUT);

  // void setup ds18b20
  insiasi_ds18b20();
}

void loop() {
    ds18b20_sensor();
    ph_sensor();

    Serial.print("*");Serial.print(tempC); Serial.print(",");Serial.print("0");Serial.print(phValue);Serial.println("#");

  // LoRa Send to received
  LoRa.beginPacket();
  LoRa.print("*");
  LoRa.print(tempC); // tempe is parameter for debug
  LoRa.print(",");
  pir_sensor();
  LoRa.print(",");
  LoRa.print(phValue); //phvalue is parameter for debug
  LoRa.print("#");
  LoRa.endPacket();
  //  delay(5000);
}

void pir_sensor() {
  val = digitalRead(pir_pin);  // read input value
  if (val == HIGH) {            // check if the input is HIGH
    digitalWrite(led_hijau, LOW);
    digitalWrite(led_merah, HIGH);
    if (pirState == LOW) {
      // we have just turned on
      Serial.println("Motion detected!");
      //      LoRa.print(1);
      //      LoRa.print("Motion Detected!");
      // We only want to print on the output change, not state
      pirState = HIGH;
    }
    LoRa.print(1);
  } else {
    digitalWrite(led_merah, LOW);
    digitalWrite(led_hijau, HIGH);
    if (pirState == HIGH) {
      // we have just turned of
      Serial.println("Motion ended!");
      //      LoRa.print(0);
      //      LoRa.print("Motion ended!");
      // We only want to print on the output change, not state
      pirState = LOW;
    }
    LoRa.print(0);
  }
}

void ph_sensor() {
  for (int i = 0; i < 10; i++)
  {
    buf[i] = analogRead(ph_pin);
    delay(30);
  }
  for (int i = 0; i < 9; i++)
  {
    for (int j = i + 1; j < 10; j++)
    {
      if (buf[i] > buf[j])
      {
        temp = buf[i];
        buf[i] = buf[j];
        buf[j] = temp;
      }
    }
  }
  avgValue = 0;
  for (int i = 2; i < 8; i++)
    avgValue += buf[i];
  float pHVol = (float)avgValue * 5.0 / 1024 / 6;
  phValue = -5.70 * pHVol + calibration;
  Serial.print("PHvalue "); Serial.print("sensor = ");
  Serial.println(phValue);

  delay(500);
}


// function to print the temperature for a device
void printTemperature(DeviceAddress deviceAddress)
{
  // method - faster
  tempC = sensors.getTempC(deviceAddress);
  Serial.print("Temp C: ");
  Serial.println(tempC);
  //  Serial.print(" Temp F: ");
  //  Serial.println(DallasTemperature::toFahrenheit(tempC)); // Converts tempC to Fahrenheit
}

void ds18b20_sensor() {
  // call sensors.requestTemperatures() to issue a global temperature
  // request to all devices on the bus
  //  Serial.print("Requesting temperatures...");
  sensors.requestTemperatures(); // Send the command to get temperatures
  //  Serial.println("DONE");

  // It responds almost immediately. Let's print out the data
  printTemperature(insideThermometer); // Use a simple function to print out the data
}

// function to print a device address
void printAddress(DeviceAddress deviceAddress) {
  for (uint8_t i = 0; i < 8; i++)
  {
    if (deviceAddress[i] < 16) Serial.print("0");
    Serial.print(deviceAddress[i], HEX);
  }
}
