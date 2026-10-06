#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>

// 1. Cấu trúc gói tin phải khớp 100% với bên ESP8266
typedef struct struct_message {
  int distance;
} struct_message;

struct_message incomingData;

// 2. Callback nhận dữ liệu (tương thích cả ESP32 Core v2.x và v3.x)
#if defined(ESP_IDF_VERSION_MAJOR) && (ESP_IDF_VERSION_MAJOR >= 5)
void onDataRecv(const esp_now_recv_info_t *esp_now_info, const uint8_t *incomingDataPtr, int len) {
#else
void onDataRecv(const uint8_t *mac_addr, const uint8_t *incomingDataPtr, int len) {
#endif
  // Sao chép dữ liệu nhận được vào biến incomingData
  memcpy(&incomingData, incomingDataPtr, sizeof(incomingData));

  // In kết quả ra Serial Monitor
  Serial.print("[ESP-NOW] Nhan tu ESP8266: ");
  if (incomingData.distance >= 8190) {
    Serial.printf("NGOAI TAM DO (Out of range: %d)\n", incomingData.distance);
  } else {
    Serial.printf("%d mm\n", incomingData.distance);
  }
}

void setup() {
  // Cổng Serial in ra máy tính của ESP32
  Serial.begin(9600);
  delay(1000);

  // Đặt chế độ Wi-Fi là Station
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  // Khởi tạo ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Loi: Khong the khoi tao ESP-NOW!");
    return;
  }

  // Đăng ký hàm nhận dữ liệu
  esp_now_register_recv_cb(onDataRecv);

  Serial.println("\n=== ESP32 RECEIVER DA SAN SANG ===");
  Serial.print("Dia chi MAC cua board: ");
  Serial.println(WiFi.macAddress());
  Serial.println("Dang cho du lieu khoang cach tu ESP-12F...");
}

void loop() {
  // Để trống vì ESP-NOW xử lý bằng ngắt phần cứng,
  // khi có dữ liệu tới nó sẽ tự động kích hoạt hàm onDataRecv
}