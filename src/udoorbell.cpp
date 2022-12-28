#include "Particle.h"

SYSTEM_MODE(SEMI_AUTOMATIC);
SYSTEM_THREAD(ENABLED);

SerialLogHandler LogHandler(LOG_LEVEL_INFO);

const uint32_t NOTE_G6 =  1568 ;
const uint32_t NOTE_A6 = 1760 ;
const uint32_t NOTE_F6 = 1397 ;
const uint32_t NOTE_F5 = 698;
const uint32_t NOTE_C6 = 1047;

const uint32_t NOTE_G7 = 3136;
const uint32_t NOTE_A7 = 3520;
const uint32_t NOTE_F7 = 2794;
// const uint32_t NOTE_F6 = 1397;
const uint32_t NOTE_C7 = 2093;

UDP udp_sock;
const int UDP_MULTI_PORT = 1024;
IPAddress multicastAddress(224,0,0,121);
// 224.0.0.121

char packet_buf[512] = {};
static uint32_t trigger_signal_count = 0;

void play_tone(int freq) {
  analogWrite(D3, 128, freq) ; 
  delay(2000);
  analogWrite(D3, 0, freq);
}

void play_note(int freq, bool end) {
  analogWrite(D3, 127, freq);
  delay(500);
  if (end) {
    delay(500);
    analogWrite(D3, 0, freq);
  }
}

void play_tone_sequence() {
    // G A F down F up C

  play_note(NOTE_G6, false);
  play_note(NOTE_A6, false);
  play_note(NOTE_F6, false);
  play_note(NOTE_F5, false);
  play_note(NOTE_C6, true);
}


// ============================================================================
// Simulate a doorbell button press-- useful for development testing
// Cloud functions must return int and take one String
int fake_remote_signal(String extra) {
  Log.info("fake_remote_signal");
  trigger_signal_count += 5;
  return 0;
}

void init_multicast_port() {
  WiFi.on();
  bool wifi_ready = waitFor(WiFi.isOn, 30000);
  if (wifi_ready) {
    Log.info("wifi ready");
    udp_sock.begin(UDP_MULTI_PORT);
    udp_sock.joinMulticast(multicastAddress);
    // IPAddress localIP = WiFi.localIP();
    // Log.info("%u.%u.%u.%u", localIP[0], localIP[1], localIP[2], localIP[3]);
  }
  else {
    Log.error("no wifi available");
  }

}

// setup() runs once, when the device is first turned on.
void setup() {
  delay(4000);
  Log.info("begin...");


  pinMode(D3, OUTPUT);     // sets the pin as output

  Log.info("connecting...");
  Particle.connect();
  play_tone_sequence();

  waitFor(Particle.connected,90000);
  if (Particle.connected()) {
      Log.info("conneted");
      Particle.function("fakeRemote", fake_remote_signal);
      init_multicast_port();
  }
  else {
    Log.warn("could not connect!");
  }

  IPAddress localIP = WiFi.localIP();
  Log.info("%u.%u.%u.%u", localIP[0], localIP[1], localIP[2], localIP[3]);


}



void loop() {

  packet_buf[0] = 0;
  if (trigger_signal_count > 0) {
    trigger_signal_count = 0;
    play_tone_sequence();
  }

  int count = udp_sock.receivePacket((byte*)packet_buf, 127);
  if (count >= 0 && count < 128) {
    packet_buf[count] = 0;
    if (count > 0) {
      Log.info("received %u", count);
      trigger_signal_count += count;
    }
    else {
      delay(1000);
    }
  } else if (count < -1) {
    Log.error("recv err: %d", count);
    // need to re-initialize on error
    init_multicast_port();
  }

}