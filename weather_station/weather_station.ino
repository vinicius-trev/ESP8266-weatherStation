
#include <math.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <Adafruit_BMP085.h>    // Biblioteca do sensor BMP 180 (temp pressao e altitude)
#include <Adafruit_HTU21DF.h>   // Biblioteca do sensor HTU21D (temp e umidade) 

const char* ssid     = "YOUR_SSID";
const char* password = "TOUR_WIFI_PSWD";
const int sleepTimeS = 60; //18000 for Half hour, 300 for 5 minutes etc.
const int16_t utc = -3; //UTC -3:00 Brazil

///////////////Weather////////////////////////
const char server [] = "weatherstation.wunderground.com";
const char WEBPAGE [] = "GET /weatherstation/updateweatherstation.php?";
const char ID [] = "WEATHER_STATION_ID";
const char PASSWORD [] = "WEATHER_STATION_KEY";
const double R2 = 10000;            // 10k ohm series resistor
const double VCC = 3.3;

Adafruit_HTU21DF sensorHTU;      // Objeto para representar o sensor HTU
Adafruit_BMP085 BMP180;         // Objeto para representar o sensor BMP180
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "a.st1.ntp.br", utc*3600, 60000);

// Estrutura que contem os dados que serao armazenados na memoria do RTC
typedef struct data {      //stting up the variables to be stored
  int dia;
};
data RTCdata;

void setup()
{
  Serial.begin(115200);
  delay(1000);

  // Conectando ao WiFi
  Serial.println();
  Serial.print("Conectando em ");
  Serial.print(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("Conectado!!");

  // Inicializando os objetos dos sensores
  sensorHTU.begin();
  BMP180.begin();          

  // Capturando hora via NTP
  timeClient.begin();
  timeClient.update();

  // Lendo o dia na memoria do RTC
  ESP.rtcUserMemoryRead(65, (uint32_t*) &RTCdata, (sizeof(data)));
  if(RTCdata.dia > 31 || RTCdata.dia < 1)
  {
    String formattedDate = timeClient.getFormattedDate();
    String dia = formattedDate.substring(8, formattedDate.length()-10);
    RTCdata.dia = dia.toInt();
    ESP.rtcUserMemoryWrite(65, (uint32_t*) &RTCdata, sizeof(RTCdata));
  }
}

void loop() 
{ 
  // Get sensor data
  // NTC
  double tempNTC_c = Thermister(AnalogReadFiltered());
  double tempNTC_f = (tempNTC_c * 9.0) / 5.0 + 32.0;

  // HTU
  double tempHTU_c = sensorHTU.readTemperature();             // Lê os valores de Temperatura em Cesius nos registradores do HTU
  double tempHTU_f =  (tempHTU_c * 9.0) / 5.0 + 32.0;
  double humidity = sensorHTU.readHumidity();                 // Lê os valores da Umidade em Celsius nos registradores do HTU

  // Pressure
  double tempBMP_c = BMP180.readTemperature();
  double tempBMP_f =(tempBMP_c * 9.0) / 5.0 + 32.0;
  double pressure = BMP180.readPressure() * 0.000295300586467;       // Lê a pressão em inches de mercurio nos registradores do BMP
  double altitude = BMP180.readAltitude();                           // Lê o valor para a altitude nos registradores do BMP

  // Mean TEMP and Dew Point
  double mean_temp_c = (tempNTC_c + tempHTU_c + tempBMP_c) / 3.0;
  double mean_temp_f = (mean_temp_c * 9.0) / 5.0 + 32.0;
  double dewpt_f = (dewPoint(mean_temp_f, humidity));


  // Imprime os dados capturados no console do arduino
  Serial.println("+++++++++++++++++++++++++");
  Serial.print("TempNTC-C: ");
  Serial.print(tempNTC_c);
  Serial.print(" *C ");
  Serial.print("TempNTC-F: ");
  Serial.print(tempNTC_f);
  Serial.println(" F");
  Serial.print("tempHTU-C: ");
  Serial.print(tempHTU_c);
  Serial.print(" *C ");
  Serial.print("TempHTU-F: ");
  Serial.print(tempHTU_f);
  Serial.println(" F");
  Serial.print("tempBMP-C: ");
  Serial.print(tempBMP_c);
  Serial.print(" *C ");
  Serial.print("TempBMP-F: ");
  Serial.print(tempBMP_f);
  Serial.println(" F");
  Serial.print("meanTemp-C: ");
  Serial.print(mean_temp_c);
  Serial.print(" *C ");
  Serial.print("meanTemp-F: ");
  Serial.print(mean_temp_f);
  Serial.println(" F");
  Serial.print("Umidade: ");
  Serial.println(humidity);
  Serial.print("Pressao: ");
  Serial.print(pressure);
  Serial.println(" hPa");
  Serial.print("Altitude: ");
  Serial.print(altitude);
  Serial.println(" m");
  Serial.print("dewPointF: ");
  Serial.println(dewpt_f);


  // Enviando as informações para o Wunderground
  Serial.print("connecting to ");
  Serial.println(server);
  WiFiClient client;
  if(!client.connect(server, 80)){
    Serial.println("Conection Fail");
    delay(1000);
    return;
  }
  
  client.print(WEBPAGE);
  client.print("ID=");
  client.print(ID);
  client.print("&PASSWORD=");
  client.print(PASSWORD);
  client.print("&dateutc=now");
  client.print("&tempf=");
  client.print(mean_temp_f);
  client.print("&dewptf=");
  client.print(dewpt_f);
  client.print("&humidity=");
  client.print(humidity);
  client.print("&baromin=");
  client.print(pressure);
  client.print("&softwaretype=ESP%208266O%20version1&action=updateraw&realtime=1&rtfreq=2.5");
  client.println();
  delay(2500);
  sleepMode();
}

void sleepMode() {
  Serial.print(F("Sleeping..."));
  ESP.deepSleep(sleepTimeS * 1000000);
}

double dewPoint(double celsius, double humidity)
{
  // (1) Saturation Vapor Pressure = ESGG(T)
  double RATIO = 373.15 / (273.15 + celsius);
  double RHS = -7.90298 * (RATIO - 1);
  RHS += 5.02808 * log10(RATIO);
  RHS += -1.3816e-7 * (pow(10, (11.344 * (1 - 1 / RATIO ))) - 1) ;
  RHS += 8.1328e-3 * (pow(10, (-3.49149 * (RATIO - 1))) - 1) ;
  RHS += log10(1013.246);

  // factor -3 is to adjust units - Vapor Pressure SVP * humidity
  double VP = pow(10, RHS - 3) * humidity;

  // (2) DEWPOINT = F(Vapor Pressure)
  double T = log(VP / 0.61078); // temp var
  return (241.88 * T) / (17.558 - T);
}

int AnalogReadFiltered() {
  int val = 0;
  for(int i = 0; i < 20; i++) {
    val += analogRead(A0);
    delay(1);
  }

  val = val / 20;
  return val;
}

double Thermister(int val) {

  double V_NTC = ((double)val * VCC) / 1024;
  double R_NTC = (VCC * R2 / V_NTC) - R2;
  R_NTC = log(R_NTC);
  double Temp = 1 / (0.001129148 + (0.000234125 + (0.0000000876741 * R_NTC * R_NTC ))* R_NTC );
  Temp = Temp - 273.15;        
  return Temp;

}
