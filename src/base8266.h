#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <EEPROM.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <ESP8266WebServer.h>

#define DEBUG_ENABLE

#ifdef DEBUG_ENABLE
#define DEBUG(...) Serial.print(##__VA_ARGS__)
#define DEBUGln(...) Serial.println(__VA_ARGS__)
#define DEBUGF(x) Serial.print(F(x))
#define DEBUGFln(x) Serial.println(F(x))
#define DEBUGf(M, ...) Serial.printf("[INFO] (%s:%d) " M "\n", __FILE__, __LINE__, ##__VA_ARGS__)
#define PRINT_ENV_VAR(x) (Serial.print(#x "="), Serial.println(x))
#else
#define DEBUG(...)
#define DEBUGln(...)
#define DEBUGF(x)
#define DEBUGFln(x)
#define DEBUGf(M, ...)
#define PRINT_ENV_VAR(x)
#endif

//==== MILLISTIMER MACRO ====
#define EVERY_MS(x)                  \
  static uint32_t tmr;               \
  bool flag = millis() - tmr >= (x); \
  if (flag)                          \
    tmr += (x);                      \
  if (flag)
//===========================

//==== DELAY MACRO ====
#define DELAY_MS(x)                  \
  static uint32_t tmr = -(x);        \
  bool flag = millis() - tmr >= (x); \
  if (flag)                          \
    tmr = millis();                  \
  if (flag)
//===========================

void callback(char *topic, byte *payload, unsigned int length);

template <typename T>
void EEPROM_get(int &address, T &t, int size);

class Base8266
{
private:
  /* data */
public:
  uint8_t deviceTypeSize;
  char deviceType[16];
  uint8_t ssidSize;
  char ssid[32];
  uint8_t wifiPassSize;
  char wifiPass[32];
  uint8_t mqttUsernameSize;
  char mqttUsername[32];
  uint8_t mqttPassSize;
  char mqttPass[32];

  char mqttHereTopic[20];
  char mqttClientId[20];
  char mqttTopic[30];

  const uint32_t MQTT_RECONNECT_DELAY = 10000;

  uint32_t millisLoop{0UL};

  IPAddress mqttServer;

  WiFiClient wifiClient;
  PubSubClient mqttClient{wifiClient};

  Base8266(/* args */);
  ~Base8266();
  void setup();
  void loop();
  void checkConnection();
  void blinker();
  String sendSettingsHTML();
  bool replaceText(String &input, const char *find, const char *replace);
};

uint8_t topicAddress(const char *topic, uint8_t number = 255);
