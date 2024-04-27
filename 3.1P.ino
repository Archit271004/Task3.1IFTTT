#include <WiFiNINA.h>
#include "secrets.h"
#include <BH1750.h>
#include <Wire.h>


char ssid[] = SECRET_SSID;
char pass[] = SECRET_PASS;

WiFiClient client;
BH1750 light_measurement;

char HOST_NAME[] = "maker.ifttt.com";
String PATH_NAME = "/trigger/light_detected/with/key/df7wUCXV-KPTiU54zzg_KJ";
String queryString;

unsigned long startOfDay;
bool isSunlightFalling = false;
const unsigned long twoHoursInMilliseconds = 7200000;
unsigned long startTimeOfSunlight = 0;
const long totalTimeInDayInMilliseconds = (24 * 3600000); // 1 hour = 60 min. 1 min = 60 seconds. 1 hour = 3600 seconds. 1 second = 1000ms. 1 hour = 3600 * 1000.
unsigned long endTimeOfSunlight = 0;
unsigned long totalTimeOfSunlight = 0;

void setup() 
{
  Serial.begin(9600);
  while (!Serial);  // Wait for Serial Montior. If there is no Serial Monitor then this condition will always be true and we cannot move furhter into the code.
  startOfDay = millis();
  // Track the count of the Attempts made to connect to Wifi.
  int maxAttempts = 10; // The Max number of Attempts Allowed.
  int attemptCount = 0; // The present attempt count.

  // Checks that if Wifi status is still not connected and we have remaining attempts to connect it we enter the loop.
  while (WiFi.status() != WL_CONNECTED && attemptCount < maxAttempts) 
  {
    Serial.print("Attempting to connect to WiFi network, attempt ");
    Serial.println(attemptCount + 1);
    WiFi.begin(ssid, pass);

    // Waits for some time to let it connect. 
    int waitTime = 10000; // 10 seconds
    unsigned long startTime = millis(); // unsigned is used when we want all positive values and here millis() function returns the time in milliseconds since when Arduino started running. 

    // This condition calculates the elapsed time by subtracting the start time from the present time since when arduino has been running.
    // If this condition is true and also wFi status is not connected then enter the loop and display a "."(dot) symbol.
    while (millis() - startTime < waitTime && WiFi.status() != WL_CONNECTED) 
    {
      delay(500);
      Serial.print(".");
    }
    attemptCount++;
    Serial.println();
  }

  // Check if connected successfully
  if (WiFi.status() == WL_CONNECTED) 
  {
    Serial.println("Connected to the WiFi network");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
  
    // connect to web server on port 80:
    if (client.connect(HOST_NAME, 80)) 
    {
      Serial.println("Connected to server");
    } 
    else
    {
      Serial.println("Connection to server failed");
    }
  } else 
    {
      Serial.println("Failed to connect to WiFi network");
    }

  Wire.begin();
  light_measurement.begin();

}


void loop() 
{
  // put your main code here, to run repeatedly:
  unsigned long currentTime = millis(); // since this is in loop() this will run again and again and current time will be updated again and again.
  int luxReading = light_measurement.readLightLevel();
  if(luxReading > 500 && (!isSunlightFalling == true)) // if sunlight is not falling already is true, which means isSunlightFalling needs to be false to make !isSunlightFalling equal to true.
  {
    isSunlightFalling = true; // then that means sunlight is falling and this needs to be true then.
    startTimeOfSunlight = currentTime; // Initial time will be stored in the startTimeOfSunlight.
    sendMessage("Sunlight falling", luxReading, (totalTimeOfSunlight/1000.0));

  }
  else if(luxReading < 500 && (isSunlightFalling == true)) // it means that lux < 500 and isSunlightFalling variable is true which is contradictory then we need to turn of isSunlightFalling.
  {
    isSunlightFalling = false;
    endTimeOfSunlight = currentTime - startTimeOfSunlight; // millis() will give the current time and if we subtract the start time we can get end time of the sunlight.
    totalTimeOfSunlight = totalTimeOfSunlight + endTimeOfSunlight;
    sendMessage("Sunlight Stopped",luxReading, (totalTimeOfSunlight/1000.0));

  }

  if(currentTime - startOfDay >= totalTimeInDayInMilliseconds)
  {
    if(totalTimeOfSunlight >= twoHoursInMilliseconds)
    {
      sendMessage("Sufficient Light for the day. You can keep the Terrarium inside.",luxReading, (totalTimeOfSunlight/1000.0));
    }
    else
    {
      sendMessage("Goal not Reached. It was less sunlight for the day.", luxReading, (totalTimeOfSunlight/1000.0));
    }

    startOfDay = currentTime;
    totalTimeOfSunlight = 0;
  }

  delay(10000); // delay for some time before checking again.

}

void sendMessage(const char* status, float lightIntensityInLux, unsigned long totalTimeOfLight)
{
  if(client.connect(HOST_NAME,80))
  {
    queryString = "?value1=" + String(status) + "&value2=" + String(lightIntensityInLux) + "&value3=" + String(totalTimeOfLight);

    client.println("GET " + PATH_NAME + queryString + " HTTP/1.1");
    client.println("Host: " + String(HOST_NAME));
    client.println("Connection: close");
    client.println(); // end HTTP header

    // print things on serial monitor as well.
    Serial.println("Sending data to IFTTT:");
    Serial.println("Status " + String(status));
    Serial.println("Lux Intensity: " + String(lightIntensityInLux));
    Serial.println("Total Duration of Sunlight: " + String((totalTimeOfLight)) + " seconds");
    while (!client.available()) 
    {
      delay(100);
    }

    // Read and print response from server
    while (client.available()) 
    {
      Serial.write(client.read());
    }
  } 
  else 
  {
    Serial.println("Connection to IFTTT failed");
  }
  
  client.stop();
}

