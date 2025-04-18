#include <RH_RF95.h>
#include "messages.pb.h"
#include "pb_decode.h"
#include "pb_encode.h"

#define SCRATCH_SIZE RH_RF95_MAX_MESSAGE_LEN


RH_RF95 rfm95(8, 3);

const char * callsign = "KN4MXH-3";

static uint8_t packet_scratch_buffer[SCRATCH_SIZE];

struct Command {
  const char* phrase;
  void (*callback)();
};

bool fill_basepacket_command(messaging_BasePacket * packet) {
  strncpy(packet->callsign, callsign, sizeof(packet->callsign));
  packet->flags = static_cast<messaging_PacketFlags>(messaging_PacketFlags_RELATIVE_TIME | packet->flags);
  packet->timestamp = millis();
  packet->nonce_id = random();
  packet->which_packet_data = messaging_BasePacket_command_packet_tag;
  return true;
}

bool send_packet(const messaging_BasePacket * packet) {
  static uint8_t packet_buffer[RH_RF95_MAX_MESSAGE_LEN];
  pb_ostream_t output_stream = pb_ostream_from_buffer(packet_buffer, sizeof(packet_buffer));
  if (!pb_encode(&output_stream, messaging_BasePacket_fields, packet)) {
    Serial.println("!!! FAILED TO ENCODE COMMAND PACKET TO SEND !!!");
    return false;
  }

  return rfm95.send(packet_buffer, output_stream.bytes_written);
}

void cutdownBalloon() {
  Serial.println("[CMD] Cutting down the balloon!");
  messaging_BasePacket packet = messaging_BasePacket_init_zero;
  fill_basepacket_command(&packet);
  packet.packet_data.command_packet.command = messaging_Command_CUTDOWN_BALLOON;
  packet.packet_data.command_packet.which_command_data = messaging_CommandPacket_boolean_data_tag;
  packet.packet_data.command_packet.command_data.boolean_data = true;
  if (!send_packet(&packet)) {
    Serial.println("!!! FAILED TO SEND COMMAND PACKET !!!");
  }
}

void cutdownParachute() {
  Serial.println("[CMD] Cutting down the parachute!");
  messaging_BasePacket packet = messaging_BasePacket_init_zero;
  fill_basepacket_command(&packet);
  packet.packet_data.command_packet.command = messaging_Command_CUTDOWN_PARACHUTE;
  packet.packet_data.command_packet.which_command_data = messaging_CommandPacket_boolean_data_tag;
  packet.packet_data.command_packet.command_data.boolean_data = true;
  if (!send_packet(&packet)) {
    Serial.println("!!! FAILED TO SEND COMMAND PACKET !!!");
  }
}


Command commands[] = {
  { "cutdown balloon", cutdownBalloon },
  { "cutdown parachute", cutdownParachute }
};
const size_t commandCount = sizeof(commands) / sizeof(commands[0]);

const size_t MAX_LINE_LEN = 128;
static char lineBuffer[MAX_LINE_LEN];
static size_t lineIndex = 0;


void setup() {
    Serial.begin(115200);
    if (!rfm95.init()) {
        Serial.println("RFM9x Initialization Failed");
    }
    rfm95.setTxPower(20, false);
    delay(1000);
    Serial.println("Initialized Controller!");
}

void loop() {
    while (Serial.available()) {
    char c = Serial.read();

    // If newline, terminate string and process
    if (c == '\n' || c == '\r') {
      if (lineIndex > 0) {
        lineBuffer[lineIndex] = '\0';  // Null-terminate

        // Trim whitespace (optional)
        while (lineIndex > 0 && isspace(lineBuffer[lineIndex - 1])) {
          lineBuffer[--lineIndex] = '\0';
        }

        Serial.print("[INPUT] ");
        Serial.println(lineBuffer);

        // Search commands
        for (size_t i = 0; i < commandCount; ++i) {
          if (strcmp(lineBuffer, commands[i].phrase) == 0) {
            commands[i].callback();
            break;
          }
        }

        // Reset for next line
        lineIndex = 0;
      }
    } else {
      // Store character if space allows
      if (lineIndex < MAX_LINE_LEN - 1) {
        lineBuffer[lineIndex++] = c;
      }
    }
  }


    
    if (rfm95.available()) {
        uint8_t len = sizeof(packet_scratch_buffer);
        if (rfm95.recv(packet_scratch_buffer, &len)) {
            parse_radio_data(packet_scratch_buffer, len);
        } else {
            Serial.println("Radio said a message was available, but failed to read. Weird...");
            
        }
    }
}

void parse_radio_data(uint8_t * buffer, int size) {
    pb_istream_t stream = pb_istream_from_buffer(buffer, size);
    messaging_BasePacket packet = messaging_BasePacket_init_zero;
    if (!pb_decode(&stream, messaging_BasePacket_fields, &packet)) {
        Serial.println("Radio received a packet we could not decode. Ignoring.");
        return;
    }
    pretty_print_packet(&packet);
    switch (packet.which_packet_data) {
        case messaging_BasePacket_telemetry_packet_tag:
            parse_telemetry_packet(&packet);
            break;
        case messaging_BasePacket_command_packet_tag:
            parse_command_packet(&packet);
            break;
        case messaging_BasePacket_ack_packet_tag:
            parse_ack_packet(&packet);
            break;
        default:
            Serial.println("Received Packet matched a type we don't decode. Ignoring...");
            break;
    };
}

void parse_telemetry_packet(messaging_BasePacket * packet) {
  // not needed atm...
}

void parse_command_packet(messaging_BasePacket * packet) {
  //not expected
}

void parse_ack_packet(messaging_BasePacket * packet) {
  //todo
}

void pretty_print_packet(const messaging_BasePacket * packet) {
  // --- Line 1: [timestamp] @ CALLSIGN, NONCE [TEST?] ---
  Serial.print("[");
  if (packet->flags & messaging_PacketFlags_RELATIVE_TIME) {
    Serial.print("Rel:");
  } else {
    Serial.print("Epoch:");
  }
  Serial.print(packet->timestamp);
  Serial.print("] @ ");
  Serial.print(packet->callsign);
  Serial.print(", NONCE ");
  Serial.print(packet->nonce_id);
  if (packet->flags & messaging_PacketFlags_TEST_PACKET) {
    Serial.print(" [TEST]");
  }
  Serial.println();
  
  // --- Line 2: command, telemetry, or ack data ---
  switch (packet->which_packet_data) {
    case messaging_BasePacket_telemetry_packet_tag: {
      messaging_TelemetryPacket* tp = (messaging_TelemetryPacket*)&packet->packet_data.telemetry_packet;
      Serial.print("Telemetry: T=");
      Serial.print(tp->temperature / 1000000.0, 6);
      Serial.print("C, H=");
      Serial.print(tp->humidity / 1000000.0, 6);
      Serial.print("%, P=");
      Serial.print(tp->pressure / 1000000.0, 6);
      Serial.print("hPa");
      break;
    }
    case messaging_BasePacket_command_packet_tag: {
      messaging_CommandPacket* cp = (messaging_CommandPacket*)&packet->packet_data.command_packet;
      Serial.print("Command: ");
      switch (cp->command) {
        case messaging_Command_SET_NICHROME:
          Serial.print("SET_NICHROME");
          break;
        case messaging_Command_SET_MOTOR:
          Serial.print("SET_MOTOR");
          break;
        case messaging_Command_ADJUST_TRANSMIT_POWER:
          Serial.print("ADJUST_TRANSMIT_POWER");
          break;
        case messaging_Command_CUTDOWN_PARACHUTE:
          Serial.print("CUTDOWN_PARACHUTE");
          break;
        case messaging_Command_CUTDOWN_BALLOON:
          Serial.print("CUTDOWN_BALLOON");
          break;
        default:
          Serial.print("Unknown");
          break;
      }
      Serial.print(", Data:");
      if (cp->which_command_data == 0) {
        Serial.print(cp->command_data.boolean_data ? "true" : "false");
      } else if (cp->which_command_data == 1) {
        Serial.print("<string>");
      } else if (cp->which_command_data == 2) {
        Serial.print(cp->command_data.int_data);
      } else {
        Serial.print("N/A");
      }
      break;
    }
    case messaging_BasePacket_ack_packet_tag: {
      messaging_AckPacket* ap = (messaging_AckPacket*)&packet->packet_data.ack_packet;
      Serial.print("Ack: ");
      switch (ap->command) {
        case messaging_Command_SET_NICHROME:
          Serial.print("SET_NICHROME");
          break;
        case messaging_Command_SET_MOTOR:
          Serial.print("SET_MOTOR");
          break;
        case messaging_Command_ADJUST_TRANSMIT_POWER:
          Serial.print("ADJUST_TRANSMIT_POWER");
          break;
        case messaging_Command_CUTDOWN_PARACHUTE:
          Serial.print("CUTDOWN_PARACHUTE");
          break;
        case messaging_Command_CUTDOWN_BALLOON:
          Serial.print("CUTDOWN_BALLOON");
          break;
        default:
          Serial.print("Unknown");
          break;
      }
      Serial.print(", Nonce:");
      Serial.print(ap->your_nonce);
      break;
    }
    default:
      Serial.print("Unknown Packet Type");
      break;
  }
  Serial.println();
  
  // --- Line 3: GPS if applicable (only for telemetry packets) ---
  if (packet->which_packet_data == messaging_BasePacket_telemetry_packet_tag) {
    messaging_TelemetryPacket* tp = (messaging_TelemetryPacket*)&packet->packet_data.telemetry_packet;
    if (tp->has_gps_data) {
      Serial.print("GPS: Lat=");
      Serial.print(tp->gps_data.latitude / 1000000.0, 6);
      Serial.print(", Lon=");
      Serial.print(tp->gps_data.longitude / 1000000.0, 6);
      Serial.print(", Alt=");
      Serial.print(tp->gps_data.altitude / 1000000.0, 6);
    } else {
      Serial.print("GPS: N/A");
    }

    bool balloonActive = tp->motor;
    bool parachuteActive = tp->relay_1 || tp->relay_2 || tp->relay_3 || tp->relay_4;
    if (balloonActive || parachuteActive) {
      Serial.print(" | CUTDOWNS ACTIVE - ");
      bool printed = false;
      if (balloonActive) {
        Serial.print("BALLOON");
        printed = true;
      }
      if (parachuteActive) {
        if (printed) Serial.print(", ");
        Serial.print("PARACHUTE");
      }
    }
    Serial.println();

    Serial.print("Relays: ");
    Serial.print(tp->relay_1 ? "ON " : "OFF ");
    Serial.print(tp->relay_2 ? "ON " : "OFF ");
    Serial.print(tp->relay_3 ? "ON " : "OFF ");
    Serial.print(tp->relay_4 ? "ON " : "OFF");
    Serial.print(" | Motor: ");
    Serial.println(tp->motor ? "ON" : "OFF");
    Serial.println();
    Serial.println();

  } else {
    Serial.print("GPS: N/A");
  }
  Serial.println();
  // Line 4 for relays and motors
  
}





