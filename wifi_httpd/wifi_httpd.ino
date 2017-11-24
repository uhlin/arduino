/* Copyright (C) 2017 Markus Uhlin. ALL RIGHTS RESERVED. */

#include <SoftwareSerial.h>
#include <WiFiEsp.h>

/* pin 6 (RX) and 7 (TX) */
SoftwareSerial Serial1(6, 7);

namespace network {
  char ssid[] = "retaildata_unifi";
  const char pass[] = "Change Me!";

  /* "radio status" */
  int status = WL_IDLE_STATUS;
}

/*
 * Create a HTTPD on port 80
 */
WiFiEspServer server(80);

const char *header[] = {
  "HTTP/1.1 200 OK\r\n",
  "Content-Type: text/html\r\n",
  "Connection: close\r\n",
  "\r\n",
};

const char *html[] = {
  "<!DOCTYPE html>",
  "<html>",
  "<head>",
  "<title>Retail Data Arduino</title>",
  "<meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\">",
  "<style>",
  "#logo { color: rgb(64, 192, 192); display: block; font-family: monospace; margin: 1em 0; white-space: pre; }",
  "a { color: rgb(64, 192, 192); text-decoration: underline; }",
  "a:hover { color: rgb(64, 192, 192); text-decoration: none; }",
  "body { background-color: black; color: white; font-family: sans-serif; }",
  "</style>",
  "</head>",
  "<body>",
  "<span id=\"logo\">",
  "  ___     _        _ _   ___       _",
  " | _ \\___| |_ __ _(_) | |   \\ __ _| |_ __ _",
  " |   / -_)  _/ _` | | | | |) / _` |  _/ _` |",
  " |_|_\\___|\\__\\__,_|_|_| |___/\\__,_|\\__\\__,_|",
  "</span>",
  "<p>Powered by <a href=\"https://www.arduino.cc/\">ARDUINO</a></p>",
  "</body>",
  "</html>",
};

void
printWifiStatus(void)
{
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);
}

void
setup(void)
{
  Serial.begin(115200);
  Serial1.begin(9600);
  WiFi.init(&Serial1);

  if (WiFi.status() == WL_NO_SHIELD) {
    Serial.println("WiFi shield not present");
    while (true) /* don't continue */
      delay(500);
  }

  while (network::status != WL_CONNECTED) {
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(network::ssid);
    network::status = WiFi.begin(network::ssid, network::pass);
  }

  Serial.println("You are connected to the network!");
  printWifiStatus();

  /* Start the server */
  server.begin();
}

void
loop(void)
{
  /* Listen for incoming clients... */
  WiFiEspClient client = server.available();

  if (!client)
    return;

  Serial.println("New client");
  boolean currentLineIsBlank = true;

  while (client.connected()) {
    if (!client.available())
      continue;
    char c = client.read();

    Serial.write(c);

    if (c == '\n' && currentLineIsBlank) {
      int i;

      Serial.println("Sending response...");

      for (i = 0; i < (sizeof header / sizeof (char *)); i++)
	client.print(header[i]);
      for (i = 0; i < (sizeof html / sizeof (char *)); i++) {
	client.print(html[i]);
	client.print("\r\n");
      }
      break;
    }

    if (c == '\n')
      currentLineIsBlank = true;
    else if (c != '\r')
      currentLineIsBlank = false;
  }

  delay(100);
  client.stop();
  Serial.println("Client disconnected");
}
