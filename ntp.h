#ifndef NTP_H
#define NTP_H

#include <WiFiUdp.h>
#include <time.h> 

class NtpClient {
  public:
    NtpClient();
    void begin();
    void handle();
    unsigned long getTime(); 
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
    
    unsigned long parseTimeoutStart = 0;
    unsigned long lastSyncTime = 0;
    
    byte currentNtpServer;
    
    unsigned long baseUnixTime; // Точное время NTP в момент синхронизации
    unsigned long syncMillis;   // Значение millis() в момент синхронизации

    void sendNtpPacket(const char address[]);
    unsigned long parseNtpPacket();
};

NtpClient::NtpClient() {
  baseUnixTime = 0;
  syncMillis = 0;
  currentNtpServer = -1;
  ntp_state = NTP_INIT;
}

void NtpClient::begin() {
  int res = udpNtp.begin(2390); 
  Serial.print("[NTP DBG] UDP Socket on port 2390: ");
  Serial.println(res == 1 ? "SUCCESS" : "FAILED");
}

void NtpClient::handle() {
  if (WiFi.status() == WL_CONNECTED) {
    switch (ntp_state) {
      case NTP_INIT: {
        currentNtpServer = ++currentNtpServer % NTP_SERVER_COUNT;
        sendNtpPacket(ntpServers[currentNtpServer]);
        ntp_state = NTP_PARSE;
        parseTimeoutStart = millis(); 
        break;
      }
      case NTP_PARSE: {
        if (millis() - parseTimeoutStart >= 3000) { 
          unsigned long t = parseNtpPacket();
          if (t > 0) {
            baseUnixTime = t;         // Запоминаем время из интернета
            syncMillis = millis();    // Запоминаем время работы процессора в этот миг
            
            ntp_state = NTP_WAITING;
            lastSyncTime = millis(); 
            Serial.println("[NTP DBG] Success! Time synced.");
          } else {
            ntp_state = NTP_INIT;
            Serial.println("[NTP DBG] Sync failed. Retrying next server...");
          }
        }
        break;
      }
      case NTP_WAITING: {
        // Синхронизируемся раз в 6 часов (21600000 мс)
        if (millis() - lastSyncTime >= 21600000UL) { 
          ntp_state = NTP_INIT;
        }
        break;
      }
    }
  } else {
    if (ntp_state != NTP_INIT) {
      ntp_state = NTP_INIT;
    }
  }
}

unsigned long NtpClient::getTime() {
  // Если синхронизации еще ни разу не было - возвращаем 0
  if (baseUnixTime == 0) return 0;
  
  unsigned long elapsedSecs = (millis() - syncMillis) / 1000;
  return baseUnixTime + elapsedSecs;
}

void NtpClient::sendNtpPacket(const char address[]) {
  IPAddress ip;
  WiFi.hostByName(address, ip);
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  packetBuffer[0] = 0b11100011;
  packetBuffer[1] = 0;
  packetBuffer[2] = 6;
  packetBuffer[3] = 0xEC;
  packetBuffer[12] = 49;
  packetBuffer[13] = 0x4E;
  packetBuffer[14] = 49;
  packetBuffer[15] = 52;
  
  udpNtp.beginPacket(ip, 123);
  udpNtp.write(packetBuffer, NTP_PACKET_SIZE);
  udpNtp.endPacket();
}

unsigned long NtpClient::parseNtpPacket() {
  int cb = udpNtp.parsePacket();
  if (!cb) return 0;
    
  udpNtp.read(packetBuffer, NTP_PACKET_SIZE);

  unsigned long secsSince1900 = (unsigned long)packetBuffer[40] << 24 |
                                (unsigned long)packetBuffer[41] << 16 |
                                (unsigned long)packetBuffer[42] << 8  |
                                (unsigned long)packetBuffer[43];

  const unsigned long seventyYears = 2208988800UL;
  return secsSince1900 - seventyYears + 10800; // Возвращаем UTC+3
}

#endif