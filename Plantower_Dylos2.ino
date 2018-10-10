// 07/19/18
// Listens to Dylos then Plantower
// Shows both results

#include <SoftwareSerial.h>
#include <SD.h>
SoftwareSerial plantower(2, 3);
SoftwareSerial dylos(8, 9);

int nPlantowerLoops, dylosWaitTime = 2345;
String dylosShows, plantowerShows;
unsigned long cumPlantower03, cumPlantower05, cumPlantower10; 
unsigned long cumPlantower25, cumPlantower50, cumPlantower100;
unsigned long cumPlantowerPM10std, cumPlantowerPM25std, cumPlantowerPM100std;
unsigned long cumPlantowerPM10env, cumPlantowerPM25env, cumPlantowerPM100env;
unsigned long startTime, currentTime,  elapsedTime;
bool dylosOK, plantowerOK, debug = false;

struct pms5003data {
  uint16_t framelen;
  uint16_t pm10_standard, pm25_standard, pm100_standard;
  uint16_t pm10_env, pm25_env, pm100_env;
  uint16_t particles_03um, particles_05um, particles_10um, particles_25um, particles_50um, particles_100um;
  uint16_t unused;
  uint16_t checksum;
};

struct pms5003data data;


void setup() 
{
  pinMode(10, OUTPUT);   
  Serial.begin(9600);
  if(!SD.begin(4))  
  {
    Serial.println("Card failed"); Serial.println("");

  }
  plantower.begin(9600);
  dylos.begin(9600);
  dylos.listen();
  while (dylos.available() == 0)
  {
    Serial.print("*"); 
    delay(dylosWaitTime); 
  }
  Serial.println("Dylos first report done.  Now enter the loop.");
}

void loop()
{
  dylosShows = "";
  plantowerShows = "";
  plantower.listen();
  plantowerShows = getPlantower();
  dylos.listen();
 // Serial.println(); 
  while (dylos.available() == 0)
  {
    // Serial.print("*"); 
    delay(dylosWaitTime); 
  }
  String dylosSends = getDylos();
  dylosShows = convertDylos(dylosSends, ',', 100/28.3);
  
  showSave();
}

String getDylos() {
  dylos.listen();
  delay(100);
  String x;
  while (dylos.available() > 0) 
  {
    delay(100); 
    char next = dylos.read();
    if ((next != '\n') && (next != '\r')) 
    {
      x = String(x + next);
    }
    
  }
  return x;
}

String convertDylos(String data, char separator, float factor) 
{
   bool afterComma = false;
   String all = "";  String large = "";
   for(int i = 0; i<data.length(); i++)
   {   
        if(data[i]==separator) 
        {
            afterComma = true;
        } 
        else if(!afterComma) 
        {
            all.concat(data[i]);
        } 
        else if(afterComma) 
        {
            large.concat(data[i]);
        }
  }
   unsigned long pmAll   = (unsigned long) all.toInt()*factor;
   unsigned long pmLarge = (unsigned long) large.toInt()*factor;

   return String(pmAll) + "," + String(pmLarge);
}

String getPlantower()
{
  plantowerInit();
  plantower.listen();
  while (true) 
  {
    delay(100);
    plantowerOK = plantowerCheck(&plantower);
    if (plantowerOK)
    {
      cumPlantowerPM10std  += data.pm10_standard; 
      cumPlantowerPM25std  += data.pm25_standard; 
      cumPlantowerPM100std += data.pm100_standard;
      cumPlantowerPM10env  += data.pm10_env; 
      cumPlantowerPM25env  += data.pm25_env; 
      cumPlantowerPM100env += data.pm100_env;
      cumPlantower03 += data.particles_03um; 
      cumPlantower05 += data.particles_05um; 
      cumPlantower10 += data.particles_10um; 
      cumPlantower25 += data.particles_25um; 
      cumPlantower50 += data.particles_50um; 
      cumPlantower100 += data.particles_100um;
      
      nPlantowerLoops += 1;
      hint("another successful Plantower loop");
   
    }
    currentTime = millis();
    elapsedTime = currentTime - startTime;
    if (elapsedTime > 50000)
    {
        break;
    } 
  }
  plantowerShows  = String(cumPlantowerPM10std/nPlantowerLoops)  + ",";
  plantowerShows += String(cumPlantowerPM25std/nPlantowerLoops)  + ",";
  plantowerShows += String(cumPlantowerPM100std/nPlantowerLoops) + "," + ",";;
  plantowerShows += String(cumPlantowerPM10env/nPlantowerLoops)  + ",";
  plantowerShows += String(cumPlantowerPM25env/nPlantowerLoops)  + ",";
  plantowerShows += String(cumPlantowerPM100env/nPlantowerLoops) + "," + ",";;

  // multipoy by 10 to get "per liter" instead of "per 0.1 Liter"
  
  plantowerShows += String(10*cumPlantower03/nPlantowerLoops) + ",";
  plantowerShows += String(10*cumPlantower05/nPlantowerLoops) + ",";
  plantowerShows += String(10*cumPlantower10/nPlantowerLoops) + "," + ",";
  plantowerShows += String(10*cumPlantower25/nPlantowerLoops) + ",";
  plantowerShows += String(10*cumPlantower50/nPlantowerLoops) + ",";
  plantowerShows += String(10*cumPlantower100/nPlantowerLoops);
  
  return plantowerShows;
}

void showSave()
{
  String dataString = plantowerShows +  "," + "," + dylosShows;
  Serial.println(dataString);
  File comparePM = SD.open("PM.txt", FILE_WRITE);
  comparePM.println(dataString);  comparePM.close();
}

bool dylosCheck()
{
  bool result = false;
  int dylosLength = dylos.available();
  while (dylosLength < 0 )
  {
    delay(dylosWaitTime);
    dylosLength = dylos.available();
  }
  result = true;
  return result;
}

void plantowerInit()
{
  startTime = millis();
  elapsedTime = 0;
  nPlantowerLoops = 0;
  cumPlantower03 = 0; cumPlantower05 = 0; cumPlantower10 = 0; 
  cumPlantower25 = 0; cumPlantower50 = 0;cumPlantower100 = 0;
  cumPlantowerPM10std = 0; cumPlantowerPM25std = 0; cumPlantowerPM100std = 0;
  cumPlantowerPM10env = 0; cumPlantowerPM25env = 0; cumPlantowerPM100env = 0;
}

bool plantowerCheck(Stream * s)
{
  if (! s->available()) {
        hint("Plantower not available.");
        return false;
      }

      // Read a byte at a time until we get to the special '0x42' start-byte
      if (s->peek() != 0x42) {
        s->read();
        return false;
      }

      // Now read all 32 bytes
      if (s->available() < 32) {
        hint("Plantower in process.");
        return false;
      }

      uint8_t buffer[32];
      uint16_t sum = 0;
      s->readBytes(buffer, 32);

      // get checksum ready
      for (uint8_t i = 0; i < 30; i++) {
        sum += buffer[i];
      }
      //debugging
      /*
        for (uint8_t i=2; i<32; i++) {
        Serial.print("0x"); Serial.println(buffer[i], HEX); Serial.print(", ");
        }
        Serial.println();
      */

      // The data comes in endian'd, this solves it so it works on all platforms
      
      uint16_t buffer_u16[15];
      for (uint8_t i = 0; i < 15; i++) {
        buffer_u16[i] = buffer[2 + i * 2 + 1];
        buffer_u16[i] += (buffer[2 + i * 2] << 8);
        if (debug)
        {
           Serial.print("0x"); Serial.print(buffer_u16[i], HEX); Serial.print(", ");
        }
      }

      // put it into a nice struct :)
      memcpy((void *)&data, (void *)buffer_u16, 30);

      if (sum != data.checksum) {
        hint("Checksum failure");
        return false;
      }
      // success!
      hint("plantower is ok... nPlantowerLoops = ");  
      hint(String(nPlantowerLoops));
      return true;
}

void hint(String message)
{
  if (debug)
  {
    Serial.println(message);
  }
}

