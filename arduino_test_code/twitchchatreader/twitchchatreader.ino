#include <WiFi.h>
#include <WiFiClientSecure.h>

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// --------------------------------------------------
// WLAN
// --------------------------------------------------

const char* WIFI_SSID     = "Internet";
const char* WIFI_PASSWORD = "244466666";

// --------------------------------------------------
// Twitch
// --------------------------------------------------

const char* TWITCH_CHANNEL = "thebrutzler";

// Twitch OAuth Token
// Format: oauth:xxxxxxxxxxxxxxxxxxxxxxxx
const char* TWITCH_OAUTH = "oauth:DEIN_OAUTH_TOKEN";

// Benutzername des Twitch-Accounts,
// zu dem der OAuth-Token gehört
const char* TWITCH_USER = "justinfan0815";

// --------------------------------------------------
// OLED
// --------------------------------------------------

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

#define OLED_SDA 2
#define OLED_SCL 3

#define OLED_RESET -1
#define OLED_ADDRESS 0x3C

Adafruit_SSD1306 display(
  SCREEN_WIDTH,
  SCREEN_HEIGHT,
  &Wire,
  OLED_RESET
);

// --------------------------------------------------
// Twitch IRC
// --------------------------------------------------

WiFiClientSecure twitch;

String currentUser = "";
String currentMessage = "";

unsigned long lastDisplayUpdate = 0;

// --------------------------------------------------
// Display
// --------------------------------------------------

void displayText(String username, String message)
{
  display.clearDisplay();

  display.setTextColor(SSD1306_WHITE);

  // Benutzername
  display.setTextSize(1);
  display.setCursor(0, 0);

  display.print(username);

  // Trennlinie
  display.drawLine(0, 10, 127, 10, SSD1306_WHITE);

  // Nachricht
  display.setCursor(0, 14);

  // Text automatisch auf mehrere Zeilen verteilen
  int charsPerLine = 21;

  for (int i = 0; i < message.length(); i += charsPerLine)
  {
    String line = message.substring(
      i,
      min(i + charsPerLine, (int)message.length())
    );

    display.println(line);

    if ((i / charsPerLine) >= 4)
      break;
  }

  display.display();
}

// --------------------------------------------------
// Twitch verbinden
// --------------------------------------------------

bool connectTwitch()
{
  Serial.println("Verbinde mit Twitch...");

  // TLS-Zertifikat nicht prüfen
  // Für einen privaten ESP32-Aufbau einfach.
  twitch.setInsecure();

  if (!twitch.connect("irc.chat.twitch.tv", 6697))
  {
    Serial.println("Twitch Verbindung fehlgeschlagen!");
    return false;
  }

  Serial.println("Mit Twitch verbunden.");

  // IRC Login
  twitch.println("PASS " + String(TWITCH_OAUTH));
  twitch.println("NICK " + String(TWITCH_USER));

  // Chat-Kanal aktivieren
  twitch.println("JOIN #" + String(TWITCH_CHANNEL));

  Serial.println("JOIN #" + String(TWITCH_CHANNEL));

  return true;
}

// --------------------------------------------------
// IRC Nachricht verarbeiten
// --------------------------------------------------

void processIRC(String line)
{
  Serial.println(line);

  // Twitch sendet regelmäßig PING
  if (line.startsWith("PING"))
  {
    twitch.println("PONG :tmi.twitch.tv");
    Serial.println("PONG");
    return;
  }

  // Nur PRIVMSG interessiert uns
  if (line.indexOf("PRIVMSG") == -1)
    return;

  // -----------------------------------------------
  // Username extrahieren
  //
  // Beispiel:
  //
  // :username!username@username.tmi.twitch.tv
  // PRIVMSG #thebrutzler :Hallo
  // -----------------------------------------------

  int exclamation = line.indexOf('!');

  if (exclamation < 2)
    return;

  String username = line.substring(1, exclamation);

  // Nachricht beginnt nach " :"
  int messageStart = line.indexOf(" :", line.indexOf("PRIVMSG"));

  if (messageStart == -1)
    return;

  messageStart += 2;

  String message = line.substring(messageStart);

  // -----------------------------------------------
  // Ausgabe
  // -----------------------------------------------

  Serial.println();
  Serial.println("USER: " + username);
  Serial.println("MSG : " + message);
  Serial.println();

  displayText(username, message);
  delay(2000);
}

// --------------------------------------------------
// Setup
// --------------------------------------------------

void setup()
{
  Serial.begin(115200);

  delay(1000);

  // OLED starten
  Wire.begin(OLED_SDA, OLED_SCL);

  if (!display.begin(
        SSD1306_SWITCHCAPVCC,
        OLED_ADDRESS))
  {
    Serial.println("OLED nicht gefunden!");
    while (1)
      delay(1000);
  }

  display.clearDisplay();

  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);

  display.setCursor(0, 0);
  display.println("THEBRUTZLER");
  display.println();
  display.println("Verbinde WLAN...");

  display.display();

  // ------------------------------------------------
  // WLAN
  // ------------------------------------------------

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.print("WLAN");

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("WLAN verbunden");

  Serial.print("IP: ");
  Serial.println(WiFi.localIP());

  display.clearDisplay();

  display.setCursor(0, 0);
  display.println("THEBRUTZLER");
  display.println();
  display.println("WLAN OK");
  display.println();
  display.println("Twitch...");

  display.display();

  // ------------------------------------------------
  // Twitch
  // ------------------------------------------------

  connectTwitch();

  delay(1000);

  display.clearDisplay();

  display.setCursor(0, 0);
  display.println("THEBRUTZLER");
  display.drawLine(0, 10, 127, 10, SSD1306_WHITE);
  display.setCursor(0, 20);
  display.println("Warte auf Chat...");

  display.display();
}

// --------------------------------------------------
// Loop
// --------------------------------------------------

void loop()
{
  // WLAN-Verbindung prüfen
  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.println("WLAN getrennt!");

    WiFi.reconnect();

    delay(5000);

    return;
  }

  // Twitch-Verbindung prüfen
  if (!twitch.connected())
  {
    Serial.println("Twitch getrennt!");

    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("Twitch");
    display.println();
    display.println("Verbindung");
    display.println("getrennt...");
    display.display();

    delay(3000);

    connectTwitch();

    return;
  }

  // IRC-Daten vorhanden?
  if (twitch.available())
  {
    String line = twitch.readStringUntil('\n');

    line.trim();

    if (line.length() > 0)
    {
      processIRC(line);
    }
  }
}