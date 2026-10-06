#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <espnow.h>

// Địa chỉ MAC của ESP32 bên nhận
uint8_t receiverMac[] = {0x78, 0x1C, 0x3C, 0xB7, 0xE0, 0x78};

// Cấu trúc gói tin truyền đi
typedef struct struct_message {
  int distance;
} struct_message;

struct_message myData;

// Callback kiểm tra trạng thái gửi
void OnDataSent(uint8_t *mac_addr, uint8_t sendStatus) {
  if (sendStatus == 0) {
    Serial.println(" -> [ESP-NOW] Gui thanh cong!");
  } else {
    Serial.println(" -> [ESP-NOW] Gui that bai!");
  }
}

void setup() {
  // Cấu hình Serial 74880 baud khớp với USART1 của STM32
  Serial.begin(74880);
  delay(500);

  Serial.println("\n=== ESP8266 SENDER KHOI DONG ===");

  // Đặt chế độ Wi-Fi là Station
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  // Khởi tạo ESP-NOW
  if (esp_now_init() != 0) {
    Serial.println("Loi: Khong the khoi tao ESP-NOW!");
    return;
  }

  // Cấu hình vai trò Controller và thêm ESP32 vào danh sách ghép cặp
  esp_now_set_self_role(ESP_NOW_ROLE_CONTROLLER);
  esp_now_register_send_cb(OnDataSent);
  esp_now_add_peer(receiverMac, ESP_NOW_ROLE_SLAVE, 1, NULL, 0);

  Serial.println("San sang nhan du lieu tu STM32 va truyen sang ESP32...");
}

void loop() {
  if (Serial.available()) {
    String incomingData = Serial.readStringUntil('\n');
    incomingData.trim();

    if (incomingData.length() > 0) {
      int distance = incomingData.toInt();

      // In log xác nhận trên cổng Serial của ESP8266
      if (distance >= 8190) {
        Serial.printf("STM32 -> ESP8266: NGOAI TAM DO (%d)", distance);
      } else {
        Serial.printf("STM32 -> ESP8266: %d mm", distance);
      }

      // Đóng gói và gửi sang ESP32
      myData.distance = distance;
      esp_now_send(receiverMac, (uint8_t *)&myData, sizeof(myData));
    }
  }
}