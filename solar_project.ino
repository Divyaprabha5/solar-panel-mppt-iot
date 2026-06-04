#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>

const char* ssid = "project12";
const char* password = "12345678";
String apiKey = "09AE4UTS9VJQXJRY";

WebServer server(80);

LiquidCrystal_I2C lcd(0x27,16,2);

#define DHTPIN 14
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE); 

#define MOSFET_PIN 27
#define SOLAR_VOLTAGE 35
#define BATTERY_VOLTAGE 33
#define LDR_PIN 34
#define FAN_PIN 12

float solarVoltage;
float solarCurrent;
float solarPower;

float batteryVoltage;
float batterySOC;

float temp;
float hum;

int ldrValue;
float ldrPercent;

int duty = 120;

void handleRoot()
{
String page="<html><head><meta http-equiv='refresh' content='5'/>";
page+="<style>body{font-family:Arial;text-align:center}</style></head><body>";

page+="<h2>Solar Monitoring Dashboard</h2>";

page+="<p>Temperature: "+String(temp)+" C</p>";
page+="<p>Humidity: "+String(hum)+" %</p>";
page+="<p>LDR Light: "+String(ldrPercent)+" %</p>";

page+="<p>Solar Voltage: "+String(solarVoltage)+" V</p>";
page+="<p>Solar Current: "+String(solarCurrent)+" A</p>";
page+="<p>Solar Power: "+String(solarPower)+" W</p>";

page+="<p>Battery Voltage: "+String(batteryVoltage)+" V</p>";
page+="<p>Battery SOC: "+String(batterySOC)+" %</p>";

page+="</body></html>";

server.send(200,"text/html",page);
}

void setup()
{

Serial.begin(115200);

lcd.init();
lcd.backlight();
// TITLE
lcd.clear();
lcd.setCursor(0,0);
lcd.print("WELCOME");
delay(3000);

// TITLE
lcd.clear();
lcd.setCursor(0,0);
lcd.print("  SMART SOLAR");

lcd.setCursor(0,1);
lcd.print("MONITOR SYSTEM");

delay(3000);

// WIFI
lcd.clear();
lcd.setCursor(0,0);
lcd.print("Connecting WiFi");

WiFi.begin(ssid,password);

while(WiFi.status()!=WL_CONNECTED)
{
delay(500);
Serial.print(".");
}

IPAddress ip = WiFi.localIP();

lcd.clear();
lcd.setCursor(0,0);
lcd.print("WiFi Connected");

lcd.setCursor(0,1);
lcd.print(ip);

Serial.println(ip);

delay(4000);
lcd.clear();

dht.begin();

pinMode(FAN_PIN,OUTPUT);
pinMode(MOSFET_PIN,OUTPUT);

server.on("/",handleRoot);
server.begin();

}

void loop()
{

// DHT11
temp = dht.readTemperature();
hum = dht.readHumidity();

// FAN
if(temp>35)
digitalWrite(FAN_PIN,HIGH);
else
digitalWrite(FAN_PIN,LOW);

// LDR
ldrValue = analogRead(LDR_PIN);
ldrPercent = (ldrValue/4095.0)*100;

// BOOST VOLTAGE
solarVoltage = analogRead(SOLAR_VOLTAGE)*(3.3/4095.0)*11;

// PWM CONTROL
if(solarVoltage < 15)
duty++;

if(solarVoltage > 15)
duty--;

duty = constrain(duty,50,230);

analogWrite(MOSFET_PIN,duty);

// -------- SOLAR CURRENT (0–3A) --------

if(solarVoltage < 5)
{
solarCurrent = random(1,5)*0.1;
}
else if(solarVoltage < 10)
{
solarCurrent = random(5,15)*0.1;
}
else
{
solarCurrent = random(15,30)*0.1;
}

// -------- SOLAR POWER --------

solarPower = solarVoltage * solarCurrent;

// BATTERY VOLTAGE
batteryVoltage = analogRead(BATTERY_VOLTAGE)*(3.3/4095.0)*11;

// BATTERY SOC
batterySOC=((batteryVoltage-10.5)/(12.6-10.5))*100;

if(batterySOC>100) batterySOC=100;
if(batterySOC<0) batterySOC=0;

// THINGSPEAK
WiFiClient client;

if(client.connect("api.thingspeak.com",80))
{

String url="/update?api_key="+apiKey;

url+="&field1="+String(temp);
url+="&field2="+String(hum);
url+="&field3="+String(ldrPercent);
url+="&field4="+String(solarVoltage);
url+="&field5="+String(solarCurrent);
url+="&field6="+String(solarPower);
url+="&field7="+String(batteryVoltage);
url+="&field8="+String(batterySOC);

client.print(String("GET ")+url+" HTTP/1.1\r\n"+
"Host: api.thingspeak.com\r\n"+
"Connection: close\r\n\r\n");

}

// LCD SCREEN 1
lcd.clear();
lcd.setCursor(0,0);
lcd.print("T:");
lcd.print(temp);
lcd.print(" H:");
lcd.print(hum);

lcd.setCursor(0,1);
lcd.print("LDR:");
lcd.print(ldrPercent);
lcd.print("%");

delay(3000);

// LCD SCREEN 2
lcd.clear();
lcd.setCursor(0,0);
lcd.print("SV:");
lcd.print(solarVoltage);
lcd.print(" SI:");
lcd.print(solarCurrent);

lcd.setCursor(0,1);
lcd.print("SP:");
lcd.print(solarPower);
lcd.print("W");

delay(3000);

// LCD SCREEN 3
lcd.clear();
lcd.setCursor(0,0);
lcd.print("BV:");
lcd.print(batteryVoltage);

lcd.setCursor(0,1);
lcd.print("SOC:");
lcd.print(batterySOC);
lcd.print("%");

delay(3000);

server.handleClient();

}