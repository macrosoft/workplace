#ifndef NTP_H
#define NTP_H

#include <WiFiUdp.h>
#include "az_timer.h"

class NtpClient {
  public:
    NtpClient();
    void handle();
    int getTime();
  private:
    static const int NTP_SERVER_COUNT = 5;
    static const int NTP_PACKET_SIZE = 48;
    const char* ntpServers[NTP_SERVER_COUNT] = {"0.ru.pool.ntp.org","1.ru.pool.ntp.org","2.ru.pool.ntp.org","3.ru.pool.ntp.org","time.windows.com"};
    byte packetBuffer[NTP_PACKET_SIZE];
    static const int NTP_INIT = 1
                   , NTP_PARSE = 2
                   , NTP_WAITING = 3;
    WiFiUDP udpNtp;
    int ntp_state;
    AZTimer oneHzTimer = AZTimer(1000);
    AZTimer parseTimer = AZTimer(10000);
    AZTimer resynchTimer = AZTimer(21600000UL); //6h
    byte currentNtpServer;
    int now;

    void sendNtpPacket(const char address[]);
    int parseNtpPacket();
};

NtpClient::NtpClient() {
  now = -1;
  currentNtpServer = -1;
  ntp_state = NTP_INIT;
  udpNtp.begin(2390);
}

void NtpClient::handle() {
  if (oneHzTimer.check()) {
    if (now >= 0) {
      ++now;
      if (now >= 86400L)
        now = 0;
    }
  }
  switch (ntp_state) {
  case NTP_INIT: {
    currentNtpServer = ++currentNtpServer%NTP_SERVER_COUNT;
    sendNtpPacket(ntpServers[currentNtpServer]);
    ntp_state = NTP_PARSE;
    parseTimer.start();
    break;
  }
  case NTP_PARSE: {
    if (parseTimer.check()) {
      int t = parseNtpPacket();
      if (t > 0) {
        now = t;
        ntp_state = NTP_WAITING;
      } else {
        ntp_state = NTP_INIT;
      }
    }
    break;
  }
  case NTP_WAITING: {
      if (resynchTimer.check()) { //6h
        ntp_state = NTP_INIT;
      }
    }
  }
}

int NtpClient::getTime() {
  return now;
}

void NtpClient::sendNtpPacket(const char address[]) {
  IPAddress ip;
  WiFi.hostByName(address, ip);
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  packetBuffer[0] = 0b11100011;
  packetBuffer[1] = 0;
  packetBuffer[2] = 6;
  packetBuffer[3] = 0xEC;
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;
  udpNtp.beginPacket(ip, 123);
  udpNtp.write(packetBuffer, NTP_PACKET_SIZE);
  udpNtp.endPacket();
}

int NtpClient::parseNtpPacket() {
  int cb = udpNtp.parsePacket();
  if (!cb)
    return -1;
  udpNtp.read(packetBuffer, NTP_PACKET_SIZE);
  unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
  unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
  unsigned long secsSince1900 = highWord << 16 | lowWord;
  const unsigned long seventyYears = 2208988800UL;
  unsigned long epoch = secsSince1900 - seventyYears + 10800;
  return epoch % 86400L;
}

#endif
