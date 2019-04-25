
#include <Adafruit_HTU21DF.h>   // Biblioteca do sensor HTU21D (temp e umidade)
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <Adafruit_BMP085.h>    // Biblioteca do sensor BMP 180 (temp pressao e altitude)

const char* ssid     = "GVT-4FFB";
const char* password = "SENHA";
const int sleepTimeS = 600; //18000 for Half hour, 300 for 5 minutes etc.
int16_t utc = -3; //UTC -3:00 Brazil
float voltage;

///////////////Weather////////////////////////
char server [] = "weatherstation.wunderground.com";
char WEBPAGE [] = "GET /weatherstation/updateweatherstation.php?";
char ID [] = "ICAMPINA39";
char PASSWORD [] = "ado0qfml";


/////////////IFTTT Low Battery///////////////////////
const char* host = "maker.ifttt.com";  //dont change
const String IFTTT_Event_low = "low_battery";
const int puertoHost = 80;
const String Maker_Key = "fl-YK3430rwsRfVQqPTv8FLxgimY272S2HWXT6UmRVn";
String conexionIF_low = "POST /trigger/" + IFTTT_Event_low + "/with/key/" + Maker_Key + " HTTP/1.1\r\n" +
                    "Host: " + host + "\r\n" +
                    "Content-Type: application/x-www-form-urlencoded\r\n\r\n";
//////////////////////////////////////////


/////////////Overcharged Battery///////////////////////
const String IFTTT_Event_high = "battery_overcharged";
String conexionIF_high = "POST /trigger/" + IFTTT_Event_high + "/with/key/" + Maker_Key + " HTTP/1.1\r\n" +
                    "Host: " + host + "\r\n" +
                    "Content-Type: application/x-www-form-urlencoded\r\n\r\n";
//////////////////////////////////////////

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
  pinMode(A0, INPUT);

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

void loop() {
  
  // Calculando e imprimindo no console a voltagem da Bateria
  int level = analogRead(A0);
  voltage = level / 1060.0;
  voltage = voltage * 4.3;
  Serial.print("Voltagem 18650: ");
  Serial.println(voltage);

 
  //Get sensor data
  //Temperature
  double tempc = sensorHTU.readTemperature();                 // Lê os valores de Temperatura em Cesius nos registradores do HTU
  double humidity = sensorHTU.readHumidity();                 // Lê os valores da Umidade em Celsius nos registradores do HTU
  double tempf =  (tempc * 9.0) / 5.0 + 32.0;
  double dewptf = (dewPoint(tempf, humidity));

  //Pressure
  double pressure = BMP180.readPressure() * 0.000295300586467;       // Lê a pressão em inches de mercurio nos registradores do BMP
  double altitude = BMP180.readAltitude();                           // Lê o valor para a altitude nos registradores do BMP

  // Imprime os dados capturados no console do arduino
//  Serial.println("+++++++++++++++++++++++++");
//  Serial.print("TempCelsius: ");
//  Serial.print(tempc);
//  Serial.println(" *C");
//  Serial.print("Umidade: ");
//  Serial.println(humidity);
//  Serial.print("Pressao: ");
//  Serial.print(pressure);
//  Serial.println(" hPa");
//  Serial.print("Altitude: ");
//  Serial.print(altitude);
//  Serial.println(" m");
//  Serial.print("dewPointF: ");
//  Serial.println(dewptf);


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
  client.print(tempf);
  client.print("&dewptf=");
  client.print(dewptf);
  client.print("&humidity=");
  client.print(humidity);
  client.print("&baromin=");
  client.print(pressure);
  client.print("&softwaretype=ESP%208266O%20version1&action=updateraw&realtime=1&rtfreq=2.5");
  client.println();
  delay(2500);
  mandarNot();
  sleepMode();
}


void mandarNot() {
  // Antes de mandar notificação verifica se já enviou naquele mesmo dia
  String formattedDate = timeClient.getFormattedDate();
  String dia = formattedDate.substring(8, formattedDate.length()-10);
  int diaHoje = dia.toInt();
 
  if(diaHoje != RTCdata.dia)
  {    
    RTCdata.dia=diaHoje;
    ESP.rtcUserMemoryWrite(65, (uint32_t*) &RTCdata, sizeof(RTCdata));
    WiFiClient client;
    if (!client.connect(host, puertoHost)) //Check connection
    {
      Serial.println("Failed connection");
      return;
    }
    if(voltage < 3.0)
    {
     Serial.println("Low battery");
     delay(500);
     client.print(conexionIF_low);//Send information
     delay(10);
    
    }
    else if(voltage > 4.3)
    {
      Serial.println("Battery Overcharged");
      delay(500);
      client.print(conexionIF_low);//Send information
      delay(10);
    }
    
    while (client.available())
      {
        String line = client.readStringUntil('\r');
        Serial.print(line);
      }
  }
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
