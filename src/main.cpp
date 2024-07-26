//bibliotecas -----------------------------------------------------------------------------------------------------------
#include <SPI.h>        // SD
#include <TinyGPS++.h>  //gps
#include "axp20x.h"     //axp
#include "SD.h"         // SD
#include "FS.h"         // SD
#include <string>
#include <Arduino.h>
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
//bibliotecas -----------------------------------------------------------------------------------------------------------

//mapa de pinos SD
#define SD_MISO 35
#define SD_MOSI 13
#define SD_SCK 14
#define SD_CS 25

//velocidade serial
#define SERIAL_BAUND 9600

//intervalo de tempo para amostras
#define TEMPO 5 //em segundos

//id da rota do onibus
#define ROTA 2

//SD --------------------------------------------------------------------------------------------------------------------
SPIClass mySPI = SPIClass(HSPI);  //SPI virtual

void writeFile(fs::FS &fs, const char *path, const char *message) {
  Serial.printf("Escrevendo o arquivo: %s\n", path);

  File file = fs.open(path, FILE_WRITE);
  if (!file) {
    Serial.println("Falha ao abrir o arquivo para escrita");
    return;
  }
  if (file.print(message)) {
    Serial.println("Arquivo escrito");
  } else {
    Serial.println("Falha na escrita");
  }
  file.close();
}

void appendFile(fs::FS &fs, const char *path, const char *message) {
  digitalWrite(SD_CS, HIGH);
  digitalWrite(18, LOW);
  Serial.printf("Anexando ao arquivo: %s\n", path);
  File file = fs.open(path, FILE_APPEND);
  if (!file) {
    Serial.println("Falha ao abrir o arquivo para anexar");
    return;
  }
  if (file.print(message)) {
    Serial.println("Mensagem anexada");
  } else {
    Serial.println("Falha ao anexar");
  }
  file.close();
  digitalWrite(18, HIGH);
  digitalWrite(SD_CS, LOW);
}

void setupSD() {
  mySPI.begin(SD_SCK, SD_MISO, SD_MOSI);
  if (!SD.begin(SD_CS, mySPI, 10000000)) {
    Serial.println("Erro na leitura do arquivo não existe um cartão SD ou o módulo está conectado incorretamente...");
    return;
  }
  uint8_t cardType = SD.cardType();
  if (cardType == CARD_NONE) {
    Serial.println("Nenhum cartao SD encontrado");
    return;
  }
  Serial.println("Inicializando cartao SD...");
  //if (!SD.begin(SD_CS, spi1))
  if (!SD.begin(SD_CS)) {
    Serial.println("ERRO - SD nao inicializado!");
    return;
  }

  File file = SD.open("/data.csv");
  if (!file) {
    Serial.println("SD: arquivo data.csv nao existe");
    Serial.println("SD: Criando arquivo...");
    writeFile(SD, "/data.csv", "ROTA;LONGITUDE;LATITUDE;VELOCIDADE;ALTITUDE;HDOP;DATA;HORA \r\n");
  } else {
    Serial.println("SD: arquivo ja existe");
    //appendFile(SD, "/data.csv", "ROTA;LONGITUDE;LATITUDE;VELOCIDADE;ALTITUDE;HDOP;DATA;HORA \r\n");
  }

  file.close();
}
//SD --------------------------------------------------------------------------------------------------------------------

//axp -------------------------------------------------------------------------------------------------------------------
AXP20X_Class axp;

void setupAXP() {
  Wire.begin(21, 22);  // configurado a comunicação com o axp
  if (!axp.begin(Wire, AXP192_SLAVE_ADDRESS)) {
    Serial.println("AXP192 Begin PASS");
  } else {
    Serial.println("AXP192 Begin FAIL");
  }
  axp.setPowerOutPut(AXP192_LDO2, AXP202_ON); //Lora
  axp.setPowerOutPut(AXP192_LDO3, AXP202_ON);  //GPS
  axp.setPowerOutPut(AXP192_DCDC2, AXP202_ON); //OLED
  axp.setPowerOutPut(AXP192_EXTEN, AXP202_ON); //Lora
  axp.setPowerOutPut(AXP192_DCDC1, AXP202_ON); //GPS
}
//axp -------------------------------------------------------------------------------------------------------------------

//gps -------------------------------------------------------------------------------------------------------------------
TinyGPSPlus gps;
HardwareSerial GPS(1);

void setupGPS() {
  GPS.begin(9600, SERIAL_8N1, 34, 12);
}

void loopGPS() {
  while (GPS.available())
    gps.encode(GPS.read());
}
//gps -------------------------------------------------------------------------------------------------------------------

//packet ----------------------------------------------------------------------------------------------------------------

//Contrução de pacote dos dados para anexação no SD
void buildPacket_toSD() {
  String leitura;
  leitura.concat(String(ROTA));
  leitura.concat(";");
  leitura.concat(String(gps.location.lat(), 8));
  leitura.concat(";");
  leitura.concat(String(gps.location.lng(), 8));
  leitura.concat(";");
  leitura.concat(String(gps.speed.kmph()));
  leitura.concat(";");
  leitura.concat(String(gps.altitude.value()/100));
  leitura.concat(";");
  leitura.concat(String(gps.hdop.value()));
  leitura.concat(";");
  leitura.concat(String(gps.date.value())); //adiciona data
  leitura.concat(";");
  leitura.concat(String(gps.time.value()/100)); //adiciona hora
  leitura.concat("; \r\n");
  Serial.println(leitura);
  appendFile(SD, "/data.csv", leitura.c_str());
}

void printPacket() {
  Serial.print("Latitude: ");
  Serial.println(gps.location.lat());
  Serial.print("Longitude: ");
  Serial.println(gps.location.lng());
  Serial.print("Altitude: ");
  Serial.println(gps.altitude.meters());
  Serial.print("Hdop: ");
  Serial.println(gps.hdop.value());
}
//packet ----------------------------------------------------------------------------------------------------------------

void setup() {
  
  Serial.begin(SERIAL_BAUND);

  delay(3000);
  Serial.println("Starting");

  //iniciando módulos
  setupAXP();
  Serial.println("Axp OK");
  delay(300);
  setupGPS();
  Serial.println("GPS OK");
  delay(300);
  setupSD();
  Serial.println("SD OK");
  delay(300);
}

void loop() {
  loopGPS();            //leitura GPS
  buildPacket_toSD();   //registro
  delay(TEMPO*1000);    //intervalo
}
