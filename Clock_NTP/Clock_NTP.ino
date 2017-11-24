/*
 * Public domain
 * Written by Markus Uhlin
 */

#include <Adafruit_NeoPixel.h>
#include <RtcDS3231.h>
#include <SoftwareSerial.h>
#include <U8glib.h>
#include <WiFiEsp.h>
#include <WiFiEspUdp.h>
#include <Wire.h>

Adafruit_NeoPixel     ring;
RtcDS3231             rtcModule;
SoftwareSerial        Serial1(6, 7);
U8GLIB_SSD1306_128X64 oled(U8G_I2C_OPT_NONE);

/* A UDP instance to let us send and receive packets over UDP */
WiFiEspUDP Udp;

byte hours      = 0;
byte hours_save = 255;
byte minutes    = 0;
byte seconds    = 0;

/* NTP time stamp is in the first 48 bytes of the message */
const int NTP_PACKET_SIZE = 48;

/* buffer to hold incoming and outgoing packets */
byte packetBuffer[NTP_PACKET_SIZE];

/* NTP server */
char time_server[] = "ntp1.chalmers.se";

const byte neoBright = 5;
const byte neoPin    = 9;
const byte neoPixels = 24;

namespace network {
  char ssid[] = "ChangeMe";
  char pass[] = "ChangeMe";
  int status = WL_IDLE_STATUS;
}

struct colors_tag {
  byte red;
  byte green;
  byte blue;
} colors[] = {
  {   0,   0, 160 },
  {   0,   0, 255 },
  {   0,  64, 128 },
  {   0, 128,   0 },
  {   0, 128,  64 },
  {   0, 128, 128 },
  {   0, 128, 192 },
  {   0, 128, 255 },
  {   0, 255,   0 },
  {   0, 255,  64 },
  {   0, 255, 128 },
  { 128,   0,   0 },
  { 128,   0,  64 },
  { 128,   0, 128 },
  { 128,   0, 255 },
  { 128,  64,  64 },
  { 128, 128, 192 },
  { 128, 128, 255 },
  { 128, 255,   0 },
  { 128, 255, 128 },
  { 128, 255, 255 },
  { 255,   0,   0 },
  { 255,   0, 128 },
  { 255,   0, 255 },
  { 255, 128,   0 },
  { 255, 128,  64 },
  { 255, 128, 128 },
  { 255, 128, 192 },
  { 255, 128, 255 },
  { 255, 255,   0 },
  { 255, 255,   0 },
  { 255, 255, 128 },
};

const long int numColors =
  (long int) (sizeof colors / sizeof (struct colors_tag));

/* local port to listen for UDP packets */
unsigned int local_port = 2390;

void
Print_Wifi_Status()
{
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);
}

/* send an NTP request to the time server at the given address */
void
Send_Packet(char *server)
{
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  packetBuffer[0] = 0b11100011;
  packetBuffer[1] = 0;
  packetBuffer[2] = 6;
  packetBuffer[3] = 0xEC;
  packetBuffer[12] = 49;
  packetBuffer[13] = 0x4E;
  packetBuffer[14] = 49;
  packetBuffer[15] = 52;
  Udp.beginPacket(server, 123);
  Udp.write(packetBuffer, NTP_PACKET_SIZE);
  Udp.endPacket();
}

boolean
Get_Seconds(unsigned long *secs)
{
  Send_Packet(time_server);
  if (!Udp.parsePacket()) {
    *secs = 0;
    return false;
  }
  Serial.println("packet received");
  Udp.read(packetBuffer, NTP_PACKET_SIZE);
  unsigned long highWord           = word(packetBuffer[40], packetBuffer[41]);
  unsigned long lowWord            = word(packetBuffer[42], packetBuffer[43]);
  unsigned long secsSince1900      = highWord << 16 | lowWord;
  const unsigned long seventyYears = 2208988800UL;
  *secs = secsSince1900 - seventyYears;
  *secs += 7200;
  return true;
}

void
setup(void)
{
  unsigned long secs = 0;

  Serial.begin(115200);
  Serial1.begin(9600);
  WiFi.init(&Serial1);
  if (WiFi.status() == WL_NO_SHIELD) {
    Serial.println("WiFi shield not present");
    while (true)
      delay(1000); /* don't continue */
  }
  while (network::status != WL_CONNECTED) {
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(network::ssid);
    network::status = WiFi.begin(network::ssid, network::pass);
  }
  Serial.println("You're connected to the network!");
  Print_Wifi_Status();
  Serial.println("Starting connection to server...");
  Udp.begin(local_port);
  while (!Get_Seconds(&secs))
    delay(1000); /* don't continue */
  Wire.begin();
  rtcModule.SetDateTime(RtcDateTime(secs));
  ring = Adafruit_NeoPixel(neoPixels, neoPin, NEO_GRB);
  ring.begin();
  ring.setBrightness(neoBright);
  ring.show();
}

void
Turn_Pixels_On(const byte pixels)
{
  for (int i = 0; i < pixels; i++) {
#if 0
    const long int ar_index = random(0, numColors);
#endif
    const long int ar_index = 0;
    byte red   = colors[ar_index].red;
    byte green = colors[ar_index].green;
    byte blue  = colors[ar_index].blue;

    ring.setPixelColor(i, ring.Color(red, green, blue));
    ring.show();
  }

  if (minutes >= 30) {
    ring.setPixelColor(pixels, ring.Color(0, 160, 0));
    ring.show();
  }
}

void
Turn_Pixels_Off(void)
{
  for (int i = 0; i < neoPixels; i++) {
    ring.setPixelColor(i, ring.Color(0, 0, 0));
    ring.show();
  }
}

void
Draw(void)
{
  char buf[50] = { '\0' };

  oled.setFont(u8g_font_helvB24);
  snprintf(buf, sizeof buf, "%02u:%02u:%02u", hours, minutes, seconds);
  oled.setPrintPos(0, 45);
  oled.print(buf);
}

void
Draw_Ring(void)
{
  Turn_Pixels_Off();

  switch (hours) {
  case 1: case 13:
    Turn_Pixels_On(2);
    break;
  case 2: case 14:
    Turn_Pixels_On(4);
    break;
  case 3: case 15:
    Turn_Pixels_On(6);
    break;
  case 4: case 16:
    Turn_Pixels_On(8);
    break;
  case 5: case 17:
    Turn_Pixels_On(10);
    break;
  case 6: case 18:
    Turn_Pixels_On(12);
    break;
  case 7: case 19:
    Turn_Pixels_On(14);
    break;
  case 8: case 20:
    Turn_Pixels_On(16);
    break;
  case 9: case 21:
    Turn_Pixels_On(18);
    break;
  case 10: case 22:
    Turn_Pixels_On(20);
    break;
  case 11: case 23:
    Turn_Pixels_On(22);
    break;
  case 12: case 24:
  default:
    Turn_Pixels_On(0);
    break;
  }
}

void
Update_Time(void)
{
  RtcDateTime now = rtcModule.GetDateTime();

  hours   = now.Hour();
  minutes = now.Minute();
  seconds = now.Second();
}

void
loop(void)
{
  oled.firstPage();

  do {
    Draw();
  } while (oled.nextPage());

  if (hours != hours_save || (minutes == 30 && seconds == 0))
    Draw_Ring();

  if (hours_save != hours)
    hours_save = hours;

  delay(20);
  Update_Time();
}
