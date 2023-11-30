#include <WiFi.h>
#include <ESPAsyncWebSrv.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>
#include <LiquidCrystal.h>

LiquidCrystal lcd(D0, D1, D2, D3, D4, D5);
int buttonState = 0;
int lastButtonState = 0;

int lastNumClients = 0;
bool ClientConnectAP = false;
bool ChatConnectSTA = false;
bool OnChat = false;

const unsigned long LCD_MTBS = 100; // mean time between scan bouton
unsigned long lcd_lasttime;

// Tableau des caractères personnalisés
byte hg[8] = { B00000, B00000, B00010, B00101, B00100, B00110, B00111, B00010 };
byte hm[8] = { B00000, B00000, B00000, B00000, B11111, B00000, B00000, B00000 };
byte hd[8] = { B00000, B00000, B01000, B10100, B00100, B01100, B11100, B01000 };
byte bg[8] = { B00110, B00100, B00111, B00010, B00001, B00000, B00001, B00010 };
byte bm1[8] = { B10001, B10001, B00100, B00000, B00000, B11111, B00100, B00000 };
byte bm2[8] = { B10001, B11011, B10001, B00100, B00000, B11111, B00100, B00000 };
byte bm3[8] = { B10001, B11001, B10000, B00100, B00000, B11111, B00100, B00000 };
byte bd[8] = { B01100, B00100, B11100, B01000, B10000, B00000, B10000, B01000 };

const char* OPENAI_CERTIFICATE_ROOT = R"(
  -----BEGIN CERTIFICATE-----
MIIDdzCCAl+gAwIBAgIEAgAAuTANBgkqhkiG9w0BAQUFADBaMQswCQYDVQQGEwJJ
RTESMBAGA1UEChMJQmFsdGltb3JlMRMwEQYDVQQLEwpDeWJlclRydXN0MSIwIAYD
VQQDExlCYWx0aW1vcmUgQ3liZXJUcnVzdCBSb290MB4XDTAwMDUxMjE4NDYwMFoX
DTI1MDUxMjIzNTkwMFowWjELMAkGA1UEBhMCSUUxEjAQBgNVBAoTCUJhbHRpbW9y
ZTETMBEGA1UECxMKQ3liZXJUcnVzdDEiMCAGA1UEAxMZQmFsdGltb3JlIEN5YmVy
VHJ1c3QgUm9vdDCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBAKMEuyKr
mD1X6CZymrV51Cni4eiVgLGw41uOKymaZN+hXe2wCQVt2yguzmKiYv60iNoS6zjr
IZ3AQSsBUnuId9Mcj8e6uYi1agnnc+gRQKfRzMpijS3ljwumUNKoUMMo6vWrJYeK
mpYcqWe4PwzV9/lSEy/CG9VwcPCPwBLKBsua4dnKM3p31vjsufFoREJIE9LAwqSu
XmD+tqYF/LTdB1kC1FkYmGP1pWPgkAx9XbIGevOF6uvUA65ehD5f/xXtabz5OTZy
dc93Uk3zyZAsuT3lySNTPx8kmCFcB5kpvcY67Oduhjprl3RjM71oGDHweI12v/ye
jl0qhqdNkNwnGjkCAwEAAaNFMEMwHQYDVR0OBBYEFOWdWTCCR1jMrPoIVDaGezq1
BE3wMBIGA1UdEwEB/wQIMAYBAf8CAQMwDgYDVR0PAQH/BAQDAgEGMA0GCSqGSIb3
DQEBBQUAA4IBAQCFDF2O5G9RaEIFoN27TyclhAO992T9Ldcw46QQF+vaKSm2eT92
9hkTI7gQCvlYpNRhcL0EYWoSihfVCr3FvDB81ukMJY2GQE/szKN+OMY3EU/t3Wgx
jkzSswF07r51XgdIGn9w/xZchMB5hbgF/X++ZRGjD8ACtPhSNzkE1akxehi/oCr0
Epn3o0WC4zxe9Z2etciefC7IpJ5OCBRLbf1wbWsaY71k5h+3zvDyny67G7fyUIhz
ksLi4xaNmjICq44Y3ekQEe5+NauQrz4wlHrQMz2nZQ/1/I6eYs9HRCwBXbsdtTLS
R9I4LtD+gdwyah617jzV/OeBHRnDJELqYzmp-----END CERTIFICATE-----)";

//update ssl certificate on 2024 !

//YOUR CREDENTIALS :

#define BOTtoken "YOUR_TELEGRAM_BOT_TOKEN_BOT_FATHER"
const char *apiKey = "YOUR_OPENAI_API";
const char *assistant_id = "YOUR_OPENAI_ASSISTANT_ID"

WiFiClientSecure telegramClient;
WiFiClientSecure openaiClient;
UniversalTelegramBot bot(BOTtoken, telegramClient);
String threadId;

const unsigned long BOT_MTBS = 1000; // mean time between scan messages
unsigned long bot_lasttime;

const char *ssid = "Le ChatBot de Schrödinger";
const char *password = NULL;  // Mot de passe NULL pour ouvrir le réseau

String availableNetworks;  // Déclaration en dehors de la fonction setup
String selectedSSID;  // SSID sélectionné pour la connexion

AsyncWebServer server(80);

void setup() {
  Serial.begin(115200);

  lcd.createChar(0, hg);
  lcd.createChar(1, hm);
  lcd.createChar(2, hd);
  lcd.createChar(3, bg);
  lcd.createChar(4, bm1);
  lcd.createChar(5, bm2);
  lcd.createChar(6, bm3);
  lcd.createChar(7, bd);
  lcd.begin(16, 2);

  // Mode AP avec mot de passe NULL et nombre maximal de connexions
  WiFi.softAP(ssid, password, 1);
  IPAddress IP = WiFi.softAPIP();
  Serial.println("AP IP address: " + IP.toString());

  // Recherche des réseaux WiFi disponibles
  availableNetworks = scanNetworks();

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    String html = "<html><head>";
    html += "<meta charset='UTF-8'>";
    html += "<style>";
    html += "body { font-family: Calibri, sans-serif; text-align: center; }";
    html += "h1 { color: #333; font-size: 4vw; }";
    html += "h2 { color: #2b5353; font-size: 3vw; }";
    html += "a { color: #0066cc; text-decoration: none; font-size: 3vw; }";
    html += "b { color: #2b5353; font-size: 3vw; }";
    html += "form { display: flex; flex-direction: column; align-items: center; margin: 2vh auto; max-width: 30vw; }";
    html += "</style>";
    html += "</head><body>";
    html += "<h1>Bienvenue sur le webserveur de Schrödinger !</h1>";
    html += "<h2>Connecte le chat sur l'un des réseaux WiFi disponible pour poursuivre l'exploration :</h2>";
    html += availableNetworks;
    html += "</body></html>";
    request->send(200, "text/html", html);
  });

  server.on("/connect", HTTP_GET, [](AsyncWebServerRequest *request){
    selectedSSID = request->arg("ssid");
    String html = "<html><head>";
    html += "<meta charset='UTF-8'>";
    html += "<style>";
    html += "body { font-family: Calibri, sans-serif; font-size: 3vw; text-align: center; }";
    html += "h1 { color: #333; font-size: 3.5vw; }";
    html += "h2 { color: #a42020; font-size: 3vw; }";
    html += "form { display: flex; flex-direction: column; align-items: center; margin: 2vh auto; max-width: 30vw; }";  
    html += "input { padding: 1.5vh; font-family: Calibri, sans-serif; font-size: 3vw; margin-bottom: 1.5vh; }";  
    html += "input[type='password'], input[type='submit'] { font-family: Calibri, sans-serif; font-size: 3vw; }";
    html += "</style>";
    html += "</head><body>";
    html += "<h1>Connexion au réseau : " + selectedSSID + "</h1>";
    html += "<form method='get' action='/doconnect'>";
    html += "Mot de passe: <input type='password' name='password'>";
    html += "<input type='hidden' name='ssid' value='" + selectedSSID + "'>";
    html += "<input type='submit' value='Connecter le chat'></form>";
    html += "<h2>Minute sécurité : le formulaire est envoyé en http. Toutefois, une seule connexion simultanée est autorisée sur l'ESP32. Personne ne devrait être en capacité de l'intercepter pour récupérer ton mot de passe. Les données de connexion sont enregistrées dans la mémoire flash, elles seront effacées lors du reboot de l'ESP32.</h2>";
    html += "</body></html>";
    request->send(200, "text/html", html);
  });

  server.on("/doconnect", HTTP_GET, [](AsyncWebServerRequest *request){
    String password = request->arg("password");
    connectToWiFi(selectedSSID, password);
    // Rediriger vers la page de succès après la connexion réussie
    request->redirect("/success");
  });

  server.on("/success", HTTP_GET, [](AsyncWebServerRequest *request){
    String html = "<html><head>";
    html += "<meta charset='UTF-8'>";
    html += "<style>";
    html += "body { font-family: Calibri, sans-serif; font-size: 3vw; text-align: center; }";
    html += "h1 { color: #00cc00; }";
    html += "h2 { color: #2b5353; font-size: 3vw; }";  
    html += "</style>";
    html += "</head><body>";
    html += "<h1>Connexion réussie!</h1>";
    html += "<p>Le chat de Schrödinger est maintenant connecté à " + selectedSSID + " !</p>";
    html += "<h2>Tu peux venir discuter avec lui <a href='/disconnect'>par ici</a> ! Have fun !</h2>";
    html += "</body></html>";
    request->send(200, "text/html", html);
  });

  server.on("/disconnect", HTTP_GET, [](AsyncWebServerRequest *request){
    // Déconnectez l'utilisateur de l'AP ESP
    request->redirect("tg://resolve?domain=ChatDeSchrodingerBot");
    delay(500);
    WiFi.softAPdisconnect(true);
  }); 

  server.begin();
}

String scanNetworks() {
  String networks;
  int numNetworks = WiFi.scanNetworks();
  for (int i = 0; i < numNetworks; i++) {
    networks += "<p>";
    networks += "<a href=\"/connect?ssid=" + WiFi.SSID(i) + "\">" + WiFi.SSID(i) + "</a>";
    networks += "<b> (Force du signal: " + String(WiFi.RSSI(i)) + " dBm, Sécurité: " + getEncryptionType(WiFi.encryptionType(i)) + ")</b>";
    networks += "</p>";
  }
  return networks;
}

String getEncryptionType(wifi_auth_mode_t encryptionType) {
  switch (encryptionType) {
    case WIFI_AUTH_OPEN: return "Open"; break;
    case WIFI_AUTH_WEP: return "WEP"; break;
    case WIFI_AUTH_WPA_PSK: return "WPA"; break;
    case WIFI_AUTH_WPA2_PSK: return "WPA2"; break;
    case WIFI_AUTH_WPA_WPA2_PSK: return "WPA/WPA2"; break;
    case WIFI_AUTH_WPA2_ENTERPRISE: return "WPA2 Enterprise"; break;
    case WIFI_AUTH_WPA3_PSK: return "WPA3"; break;
    default: return "Unknown";
  }
}

void connectToWiFi(String ssid, String password)
{
  Serial.println("Tentative de connexion à " + ssid);
  WiFi.setMinSecurity(WIFI_AUTH_WPA_PSK);
  WiFi.begin(ssid.c_str(), password.c_str());
  telegramClient.setCACert(TELEGRAM_CERTIFICATE_ROOT);
  openaiClient.setCACert(OPENAI_CERTIFICATE_ROOT);
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nConnecté à " + ssid);
    Serial.println("Adresse IP: " + WiFi.localIP().toString());
    ChatConnectSTA = true;
  } 
  else {
    Serial.println("\nÉchec de la connexion à " + ssid);
  }
}

String createThread() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(" create thread");
  Serial.println("createThread appelé");
  String apiUrl = "https://api.openai.com/v1/threads";
  String apiKeyHeader = "Bearer " + String(apiKey);
  HTTPClient http;
  http.begin(openaiClient, apiUrl);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Authorization", apiKeyHeader);
  http.addHeader("OpenAI-Beta", "assistants=v1");
  int httpResponseCode = http.POST("");
  String response = "";
  if (httpResponseCode == 200) {
    response = http.getString();
  } else {
    Serial.printf("Error %i: %s\n", httpResponseCode, http.errorToString(httpResponseCode).c_str());
  }
  http.end();
  DynamicJsonDocument jsonResponse(1024);
  deserializeJson(jsonResponse, response);
  Serial.println(jsonResponse["id"].as<String>());
  return jsonResponse["id"].as<String>();
}

int addMessageToThread(const String &threadId, const String &role, const String &content) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(" addMsgToThread");
  Serial.println("addMessageToThread appelé");
  String apiUrl = "https://api.openai.com/v1/threads/" + threadId + "/messages";
  String apiKeyHeader = "Bearer " + String(apiKey);
  DynamicJsonDocument jsonDoc(1024);
  jsonDoc["role"] = role;
  jsonDoc["content"] = content;
  String payload;
  serializeJson(jsonDoc, payload);
  HTTPClient http;
  http.begin(openaiClient, apiUrl);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Authorization", apiKeyHeader);
  http.addHeader("OpenAI-Beta", "assistants=v1");
  int httpResponseCode = http.POST(payload.c_str());
  if (httpResponseCode != 200) {
    Serial.printf("Error %i: %s\n", httpResponseCode, http.errorToString(httpResponseCode).c_str());
  }
  http.end();
  return httpResponseCode;
}

String runAssistantOnThread(const String &threadId) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("  runAssistant");
  lcd.setCursor(0, 1);
  lcd.print("    OnThread");
  Serial.println("runAssistantOnThread appelé");
  String apiUrl = "https://api.openai.com/v1/threads/" + threadId + "/runs";
  String apiKeyHeader = "Bearer " + String(apiKey);
  DynamicJsonDocument jsonDoc(1024);
  jsonDoc["assistant_id"] = assistant_id
  String payload;
  serializeJson(jsonDoc, payload);
  HTTPClient http;
  http.begin(openaiClient, apiUrl);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Authorization", apiKeyHeader);
  http.addHeader("OpenAI-Beta", "assistants=v1");
  int httpResponseCode = http.POST(payload.c_str());
  String response = "";
  if (httpResponseCode == 200) {
    response = http.getString();
  } else {
    Serial.printf("Error %i: %s\n", httpResponseCode, http.errorToString(httpResponseCode).c_str());
  }
  http.end();
  DynamicJsonDocument jsonResponse(1024);
  deserializeJson(jsonResponse, response);
  Serial.println(jsonResponse["id"].as<String>());
  return jsonResponse["id"].as<String>();
}

String getRunStatus(const String &threadId, const String &runId) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("  getRunStatus");
  Serial.println("getRunStatus appelé");
  String apiUrl = "https://api.openai.com/v1/threads/" + threadId + "/runs/" + runId;
  String apiKeyHeader = "Bearer " + String(apiKey);
  HTTPClient http;
  http.begin(openaiClient, apiUrl);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Authorization", apiKeyHeader);
  http.addHeader("OpenAI-Beta", "assistants=v1");
  int httpResponseCode = http.GET();
  String response = "";
  if (httpResponseCode == 200) {
    response = http.getString();
  } else {
    Serial.printf("Error %i: %s\n", httpResponseCode, http.errorToString(httpResponseCode).c_str());
  }
  http.end();
  DynamicJsonDocument jsonResponse(1024);
  deserializeJson(jsonResponse, response);
  Serial.println(jsonResponse["status"].as<String>());
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(jsonResponse["status"].as<String>());
  return jsonResponse["status"].as<String>();
}

String getAssistantMessagesFromThread(const String &threadId) {
  String apiUrl = "https://api.openai.com/v1/threads/" + threadId + "/messages";
  String apiKeyHeader = "Bearer " + String(apiKey);
  HTTPClient http;
  http.begin(openaiClient, apiUrl);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Authorization", apiKeyHeader);
  http.addHeader("OpenAI-Beta", "assistants=v1");
  int httpResponseCode = http.GET();
  String response = "";
  if (httpResponseCode == 200) {
    response = http.getString();
  } else {
    Serial.printf("Error %i: %s\n", httpResponseCode, http.errorToString(httpResponseCode).c_str());
  }
  http.end();
  DynamicJsonDocument jsonResponse(1024);
  deserializeJson(jsonResponse, response);
  return jsonResponse["data"][0]["content"][0]["text"]["value"].as<String>();
}

void handleNewMessages(int numNewMessages) {
  OnChat = true;
  for (int i = 0; i < numNewMessages; i++) {
    String chat_id = bot.messages[i].chat_id;
    String text = bot.messages[i].text;
    String from_name = bot.messages[i].from_name;
    bot.sendChatAction(chat_id, "typing");
    if (text == "/start") {
      threadId = createThread(); // create the thread when /start is received
      addMessageToThread(threadId, "user", "Salut !");
      String runId = runAssistantOnThread(threadId);
      while (getRunStatus(threadId, runId) != "completed") {
        delay(1000); // wait for 1 second before checking again
      }
      String chatGPTResponse = getAssistantMessagesFromThread(threadId);
      bot.sendMessage(chat_id, chatGPTResponse);
    } else {
      if (addMessageToThread(threadId, "user", text) == 400) {
        threadId = createThread(); // create the thread when /start is received
        addMessageToThread(threadId, "user", "Salut !");
      }
      else {
      String runId = runAssistantOnThread(threadId);
      while (getRunStatus(threadId, runId) != "completed") {
        bot.sendChatAction(chat_id, "typing");
        delay(1000); // wait for 1 second before checking again
      }
      String chatGPTResponse = getAssistantMessagesFromThread(threadId);
      bot.sendMessage(chat_id, chatGPTResponse);
      }
    }
  }
}

void loop() {
  if (millis() - bot_lasttime > BOT_MTBS) {
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    while (numNewMessages) {
      handleNewMessages(numNewMessages);
      numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    }
    int numClients = WiFi.softAPgetStationNum();
    ClientConnectAP = (numClients == 1);
    if (!ChatConnectSTA && ClientConnectAP) {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(" Bienvenue !");
      lcd.setCursor(0, 1);
      lcd.print("> 192.168.4.1");
    }
    if (ChatConnectSTA && !OnChat) {
      // Le nombre de clients a changé !
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("    Je suis");
      lcd.setCursor(0, 1);
      lcd.print("   en ligne !");
    }
    bot_lasttime = millis();
  }

  if ((!ClientConnectAP || !ChatConnectSTA) && millis() - lcd_lasttime > LCD_MTBS) {
    buttonState = digitalRead(D6);
    if (buttonState != lastButtonState) {
      lcd.clear();
      if (buttonState == HIGH) {
        int aleatoire = random(2);
        if (aleatoire == 0) {
          lcd.setCursor(0, 0);
          lcd.write(byte(0));
          lcd.write(byte(1));
          lcd.write(byte(2));
          lcd.print(" Le chat est");

          lcd.setCursor(0, 1);
          lcd.write(byte(3));
          lcd.write(byte(5));
          lcd.write(byte(7));
          lcd.print("    mort !");
        } else {
          lcd.setCursor(0, 0);
          lcd.write(byte(0));
          lcd.write(byte(1));
          lcd.write(byte(2));
          lcd.print(" Le chat est");

          lcd.setCursor(0, 1);
          lcd.write(byte(3));
          lcd.write(byte(4));
          lcd.write(byte(7));
          lcd.print("   vivant !");
        }
      }
      // Sinon, si le bouton est relâché
      else {
        lcd.setCursor(0, 0);
        lcd.write(byte(0));
        lcd.write(byte(1));
        lcd.write(byte(2));
        lcd.print(" Le chat est");

        lcd.setCursor(0, 1);
        lcd.write(byte(3));
        lcd.write(byte(6));
        lcd.write(byte(7));
        lcd.print("mort & vivant");
      }
    }
  lastButtonState = buttonState;
  lcd_lasttime = millis();
  }

}
