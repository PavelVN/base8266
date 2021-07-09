#include <base8266.h>

template <typename T>
void EEPROM_get(int &address, T &t, int size)
{
  for (uint16_t i = 0; i < size; i++)
  {
    t[i] = EEPROM.read(address + i);
  }
  address += size;
}

Base8266::Base8266(/* args */)
{
}

Base8266::~Base8266()
{
}

uint8_t topicAddress(const char *topic, uint8_t number)
{
  uint16_t size;
  uint16_t start = 0;
  uint16_t end = 0;
  uint8_t matches = 0;
  size = strlen(topic);
  for (uint8_t i = 0; i < size; i++)
  {
    if (topic[i] == '/')
    {
      matches++;
      if (i == 0)
      {
        matches = 0;
      }
      else if (i == size - 1)
      {
        end = i;
        break;
      }

      if (matches == number)
      {
        end = i;
        break;
      }
      else
      {
        start = i;
      }
    }
  }
  if (!end)
  {
    end = size - 1;
  }
  if (end - start > 0)
  {
    String newWiFiPassStr;
    for (uint16_t i = start + 1; i <= end; i++)
    {
      if (topic[i] >= '0' && topic[i] <= '9')
      {
        newWiFiPassStr += topic[i];
      }
    }
    return newWiFiPassStr.toInt();
  }
  return 255;
}

String Base8266::sendSettingsHTML()
{
  String settingsHtml = String("<head>") +
                        "<meta http-equiv='Content-Type' content='text/html; charset=utf-8' />" +
                        "</head>" +
                        "<div>" +
                        "<div>Сохраненное название сети: %%WIFI_NAME%%</div>" +
                        "<div>Сохраненный пароль: %%LAST_PASS%%</div>" +
                        "<form method='POST' action='/'>" +
                        "<input type='checkbox' checked style='display: none;' name='refresh' value='on'>" +
                        "<input class='just__button' id='refresh' type='submit' value='Refresh wifi list'>" +
                        "</form>" +
                        "<form method='POST' action='/'>" +
                        "<table class='setting__table'>" +
                        "<tr>" +
                        "<td></td><td>Номер пп</td><td>Название</td><td>Канал</td><td>Мощность</td><td>Шифрование</td>" +
                        "</tr>" +
                        "%%WIFI_LIST%%" +
                        "</table>" +
                        "<div>" +
                        "<input type='checkbox' id='connection' name='connection' value='on'>" +
                        "<label for='connection'>Подключаться к новой сети после сохранения</label>" +
                        "</div>" +
                        "<div>" +
                        "<input type='checkbox' id='reset' name='reset' value='on'><label for='reset'>Перезагрузить для работы mDNS в новой сети</label>" +
                        "</div>" +
                        "<input type='checkbox' id='setpass' name='setpass' value='on'><label for='setpass'>Установить новый пароль</label>" +
                        "<label for='password'>Новый пароль:</label><input type='text' placeholder='password' id='password'" +
                        "name='password' value='' onchange='setpass.value=password.value'><input class='just__button' type='submit' value='Выполнить'>" +
                        "</form>" +
                        "</div>";
  String wifiList;
  uint8_t length;
  replaceText(settingsHtml, "%%WIFI_NAME%%", ssid);
  replaceText(settingsHtml, "%%LAST_PASS%%", wifiPass);
  //replaceText(htmlPage, "%%LAST_PASS%%", "");
  for (int8_t i = 0; i < WiFi.scanComplete(); i++)
  {
    char newWiFiPassBuffer[256];
    sprintf(newWiFiPassBuffer, "<tr><td><input type='radio' id='%s' name='network' value='%s'></td><td>%d</td><td>%s</td><td>%d</td><td>%d dBm</td><td>%s</td></tr>\r\n", WiFi.SSID(i).c_str(), WiFi.SSID(i).c_str(), i + 1, WiFi.SSID(i).c_str(), WiFi.channel(i), WiFi.RSSI(i), WiFi.encryptionType(i) == ENC_TYPE_NONE ? "open" : "encrypted");
    Serial.print(newWiFiPassBuffer);
    wifiList += String(newWiFiPassBuffer);
  }
  settingsHtml.replace("%%WIFI_LIST%%", wifiList);
  return settingsHtml;
}

bool Base8266::replaceText(String &input, const char *find, const char *replace)
{
  input.replace(find, replace);
  return true;
}

void Base8266::setup()
{
  Serial.begin(74880);
  delay(100);
  Serial.println("Start");
  EEPROM.begin(512);
  if (EEPROM.read(0) == 1)
  {
    // deviceTypeSize = EEPROM.read(10);
    // ssidSize = EEPROM.read(11);
    // wifiPassSize = EEPROM.read(12);
    // mqttUsernameSize = EEPROM.read(13);
    // mqttPassSize = EEPROM.read(14);
    int memptr = 16;
    EEPROM.get(memptr, deviceType);
    memptr += sizeof(deviceType);
    EEPROM.get(memptr,ssid);
    memptr += sizeof(ssid);
    EEPROM.get(memptr, wifiPass);
    memptr += sizeof(wifiPass);
    EEPROM.get(memptr, mqttUsername);
    memptr += sizeof(mqttUsername);
    EEPROM.get(memptr, mqttPass);
    memptr += sizeof(mqttPass);
  }
  else
  {
    strcpy(deviceType, "default");
    strcpy(ssid, "ASU");
    strcpy(wifiPass, "1030103010");
    strcpy(mqttUsername, "arduino1");
    strcpy(mqttPass, "1234qwerASDF");
  }

  uint32_t chipId = ESP.getChipId();
  sprintf(mqttHereTopic, "here/%s/%06x", deviceType, chipId);
  sprintf(mqttClientId, "%s-%06x", deviceType, chipId);
  sprintf(mqttTopic, "%s/%06x/#", deviceType, chipId);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, wifiPass);
  if (WiFi.waitForConnectResult() != WL_CONNECTED)
  {
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP(mqttClientId, "1030103010");
    ESP8266WebServer webServer(80);
    // webServer.on("/", [&webServer]()
    //              { webServer.send(200, "text/html", "<h1>You are connected</h1>"); });
    webServer.on("/", HTTP_POST, [&]()
                 {
                   Serial.print("args: ");
                   Serial.println(webServer.args());
                   for (uint8_t i = 0; i < webServer.args(); i++)
                   {
                     Serial.print(webServer.argName(i));
                     Serial.print(" ");
                     Serial.println(webServer.arg(i));

                     if (!strcmp(webServer.argName(i).c_str(), "refresh"))
                     {
                       Serial.print("1\n");
                       WiFi.scanNetworks(false, true);
                     }
                     else
                     {
                       if (!strcmp(webServer.argName(i).c_str(), "network"))
                       {
                         Serial.print(webServer.arg(i).length());
                         strcpy(ssid,webServer.arg(i).c_str());
                         if (ssid != nullptr)
                         {
                           Serial.print("resize ok");
                         }
                         delay(100);
                         
                         strcpy(ssid, webServer.arg(i).c_str());
                       }
                       if (!strcmp(webServer.argName(i).c_str(), "password"))
                       {
                         String newWiFiPass;
                         newWiFiPass = webServer.arg(i);
                         if (newWiFiPass.length() > 8)
                         {
                           strcpy(wifiPass, newWiFiPass.c_str());
                         }
                       }
                       if (!strcmp(webServer.argName(i).c_str(), "reset") && !strcmp(webServer.arg(i).c_str(), "on"))
                       {
                         webServer.send(200, "text/html", "<h2>Настройки сохранены. Перезагрузка</h2>");
                         delay(500);
                         ESP.restart();
                       }
                       if (!strcmp(webServer.argName(i).c_str(), "connection") && !strcmp(webServer.arg(i).c_str(), "on"))
                       {
                         Serial.println("connection=on");
                         WiFi.mode(WIFI_AP_STA);
                         WiFi.disconnect();
                         delay(1);
                         WiFi.begin(ssid, wifiPass);
                       }
                     }
                   }
                   webServer.send(200, "text/html", sendSettingsHTML());
                   Serial.println(millis());
                 });
    webServer.on("/", HTTP_GET, [&]()
                 { webServer.send(200, "text/html", sendSettingsHTML()); });
    webServer.begin();
    while (1)
    {
      webServer.handleClient();
    }
  }

  // ArduinoOTA.setPort(8266);

  // Hostname defaults to esp8266-[ChipID]
  // ArduinoOTA.setHostname("kettleone");

  // No authentication by default
  /*
  char chipID_char[15];
  sprintf(chipID_char, "%06x", chipId);
  ArduinoOTA.setPassword(chipID_char);
  */
  // Password can be set with it's md5 value as well
  // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
  // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");

  ArduinoOTA.onStart([]()
                     {
                       String type;
                       if (ArduinoOTA.getCommand() == U_FLASH)
                       {
                         type = "sketch";
                       }
                       else
                       { // U_FS
                         type = "filesystem";
                       }

                       // NOTE: if updating FS this would be the place to unmount FS using FS.end()
                       DEBUG("Start updating " + type);
                     });
  ArduinoOTA.onEnd([]()
                   { DEBUG("\nEnd"); });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total)
                        {
    /*Serial.printf("Progress: %u%%\n", (progress / (total / 100)));*/ });
  ArduinoOTA.onError([](ota_error_t error)
                     {
                       //Serial.printf("Error[%u]: ", error);
                       if (error == OTA_AUTH_ERROR)
                       {
                         DEBUG("Auth Failed");
                       }
                       else if (error == OTA_BEGIN_ERROR)
                       {
                         DEBUG("Begin Failed");
                       }
                       else if (error == OTA_CONNECT_ERROR)
                       {
                         DEBUG("Connect Failed");
                       }
                       else if (error == OTA_RECEIVE_ERROR)
                       {
                         DEBUG("Receive Failed");
                       }
                       else if (error == OTA_END_ERROR)
                       {
                         DEBUG("End Failed");
                       }
                     });
  ArduinoOTA.begin();
  DEBUG("IP address: ");
  DEBUGln(WiFi.localIP());

  this->mqttClient.setCallback(callback);
  Serial.println(mqttClientId);

  //DEBUG(ArduinoOTA.getHostname());
  Serial.println("End setup");
}

void Base8266::loop()
{
  millisLoop = millis();
  // подключаемся к MQTT серверу
  checkConnection();
  ArduinoOTA.handle();
}

void Base8266::checkConnection()
{
  static uint32_t lastRunMillis(-MQTT_RECONNECT_DELAY);
  if (!mqttClient.loop())
  {
    if (millisLoop >= lastRunMillis + MQTT_RECONNECT_DELAY)
    {
      if (WiFi.status() != WL_CONNECTED)
      {
        WiFi.mode(WIFI_STA);
        WiFi.begin(ssid, wifiPass);
      }

      if (WiFi.status() == WL_CONNECTED)
      {
        IPAddress localIP;
        localIP = WiFi.localIP();
        if (mqttServer[0] != localIP[0] || mqttServer[1] != localIP[1] || mqttServer[2] != localIP[2] || mqttServer[3] != 2)
        {
          mqttServer = WiFi.localIP();
          mqttServer[3] = 2;
          mqttClient.setServer(mqttServer, 1883);
        }
        if (!mqttClient.connected())
        {
          if (mqttClient.connect(mqttClientId, mqttUsername, mqttPass, mqttHereTopic, 0, true, "0"))
          {
            mqttClient.publish(mqttHereTopic, "1", true);
            mqttClient.subscribe(mqttTopic);
            mqttClient.subscribe("all/+");
          }
        }
      }
      else
      {
        DEBUGln("Check WiFi connection");
      }

      if (WiFi.status() == WL_NO_SSID_AVAIL)
      {
        DEBUGln("Run scanning...");
        if (WiFi.scanNetworks(false, true))
        {
          DEBUGln("Founded networks");
          for (uint8_t i = 0; i < WiFi.scanComplete(); i++)
          {
            Serial.printf("%d: %-32sCh:%2d (%ddBm) %s\n", i + 1, WiFi.SSID(i).c_str(), WiFi.channel(i), WiFi.RSSI(i), WiFi.encryptionType(i) == ENC_TYPE_NONE ? "open" : "");
            //Serial.printf("%d : %s\n", i + 1, WiFi.SSID(i).c_str());
          }
        }
        else
        {
          DEBUGln("WiFi Networks not founded.");
        }
      }
      lastRunMillis = millisLoop;
    }
  }
}

// sub callback function
void callback(char *topic, byte *payload, unsigned int length)
{
  DEBUG("[sub: " + String(topic) + "] ");
  char message[length + 1];
  for (unsigned int i = 0; i < length; i++)
    message[i] = (char)payload[i];
  message[length] = '\0';
  DEBUG(message);

  if (!strncmp(topic, "test", 17))
  {
    Serial.println("receive topic test");
  }
  else if (!strncmp(topic, "relay/getPulseWidth/", 20))
  {
    uint8_t newWiFiPassRelayAddr = topicAddress(topic);
  }
}

void Base8266::blinker()
{
  static uint32_t millisLED = 0;
  static uint32_t millisLEDDelay;
  static uint8_t counter = 0;
  static uint16_t leddelay = 100;

  if (millis() > millisLED + millisLEDDelay)
  {
    switch (counter)
    {
    case 0:
      digitalWrite(LED_BUILTIN, LOW);
      millisLEDDelay = leddelay;
      break;
    case 1:
      digitalWrite(LED_BUILTIN, HIGH);
      millisLEDDelay = leddelay;
      break;
    case 2:
      digitalWrite(LED_BUILTIN, LOW);
      millisLEDDelay = leddelay;
      break;
    case 3:
      digitalWrite(LED_BUILTIN, HIGH);
      millisLEDDelay = leddelay;
      break;
    case 4:
      digitalWrite(LED_BUILTIN, LOW);
      millisLEDDelay = leddelay;
      break;
    case 5:
      digitalWrite(LED_BUILTIN, HIGH);
      millisLEDDelay = leddelay * 5;
      break;
    }
    millisLED = millis();
    counter++;
    counter %= 6;
  }
}