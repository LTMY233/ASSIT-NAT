#pragma once
#include <Arduino.h>
#include <stdint.h>

// Event types
enum EventType : uint8_t {
    EVENT_NONE              = 0,
    EVENT_WIFI_SCAN_DONE    = 1,   // WiFi scan done
    EVENT_PACKET_RX         = 2,   // Promisc packet RX
    EVENT_PACKET_RX_BEACON  = 3,   // Beacon frame RX
    EVENT_PACKET_RX_PROBE   = 4,   // Probe Req RX
    EVENT_PACKET_RX_EAPOL   = 5,   // EAPOL frame RX
    EVENT_PACKET_RX_ARP     = 6,   // ARP pkt RX
    EVENT_PACKET_RX_DEAUTH  = 7,   // Deauth frame RX
    EVENT_GATEWAY_CHANGE    = 8,   // GW MAC changed
    EVENT_DEVICE_JOIN       = 9,   // Device joined
    EVENT_DEVICE_LEAVE      = 10,  // Device left
    EVENT_NTP_SYNC          = 11,  // NTP synced
    EVENT_WIFI_CONNECTED    = 12,  // WiFi connected
    EVENT_WIFI_DISCONNECTED = 13,  // WiFi disconnected
    EVENT_HEAP_LOW          = 14,  // Heap low
};

struct EventMessage {
    EventType type;
    uint8_t   source;      // module ID
    uint16_t  dataLen;
    void*     data;        // data ptr (consume immediately)
};

// Event callback
typedef void (*EventCallback)(const EventMessage& msg);

class EventBus {
public:
    static const uint8_t MAX_LISTENERS = 8;

    void init();
    void subscribe(EventType type, EventCallback cb);
    void unsubscribe(EventType type, EventCallback cb);
    void publish(const EventMessage& msg);
    void clear();

private:
    struct Listener {
        EventType     type;
        EventCallback callback;
        bool          active;
    };
    Listener listeners[MAX_LISTENERS];
    uint8_t  count;
};

extern EventBus eventBus;
