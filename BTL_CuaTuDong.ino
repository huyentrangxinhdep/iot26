#define BLYNK_TEMPLATE_ID "TMPL6H2H_IzqT"
#define BLYNK_TEMPLATE_NAME "BTL"
#define BLYNK_AUTH_TOKEN "gKKpwE3DkAYhCnb2rzhJJNW9eCIOaFH6"

// Khai báo thư viện
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <ESP32Servo.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>
#include <time.h>

char auth[] = "gKKpwE3DkAYhCnb2rzhJJNW9eCIOaFH6";
char ssid[] = "Wokwi-GUEST";
char pass[] = "";

const int trigPin = 5;
const int echoPin = 18;

const int trigPin2 = 14;
const int echoPin2 = 27;

#define ledPin 12
#define BUZZER_PIN 26
#define THRESHOLD 20
#define max 5


struct tm timeinfo;
Servo servo;

// Khai báo biến
int buzzState;  // trạng thái của Buzzer
int dem = 0;    // đếm số lượng khách
int p;          // biến xác định trạng thái bảo mật
int k;          // biến thực hiện reset biến đếm
int hour;       // giờ hiện tại, thời gian thực
int state;      // trạng thái auto, luôn mở cửa vào

// 4 biến để thực hiện đo của SR04
long duration;
float distance;
long duration2;
float distance2;

// Khởi tại màn hình LCD
LiquidCrystal_I2C LCD(0x27, 16, 2);

void setup() {
  Serial.begin(115200);  // Khởi tạo màn Serial Monitor

  // Cấu hình chân input, output cho các thiết bị
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  pinMode(trigPin2, OUTPUT);
  pinMode(echoPin2, INPUT);

  pinMode(BUZZER_PIN, OUTPUT);
  noTone(BUZZER_PIN);
  pinMode(ledPin,OUTPUT);

  digitalWrite(ledPin,LOW);
  servo.attach(13 );

  // Khởi tạo bật sáng màn
  LCD.init();
  LCD.backlight();
  servo.write(90);

  // Kết nối wifi
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");

  // Kết nối "pool.ntp.org", "time.nist.gov" để lấy múi giờ
  configTime(7 * 3600, 0, "pool.ntp.org", "time.nist.gov");  //Múi giờ GMT+7 của Việt Nam
  Serial.print("Waiting for NTP time sync: ");
  time_t now = time(nullptr);
  while (now < 8 * 3600 * 2) {
    delay(500);
    Serial.print(".");
    now = time(nullptr);
  }
  Serial.println("");

  // Kết nối với blynk
  Blynk.begin(auth, ssid, pass);
  if (Blynk.connected()) {
    Serial.println("Connected to Blynk!");
    LCD.setCursor(0, 0);
    LCD.print("Da san sang!");
    delay(2000);
    LCD.clear();
  }
  Blynk.virtualWrite(V0, 0);
  p = 0;
  Blynk.virtualWrite(V3, 0);
  state = 0;
}

// Các hàm của hệ thống
// Đo của SR04 cửa vào
float ultrasonic() {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  duration = pulseIn(echoPin, HIGH);
  return distance = duration * 0.034 / 2;
}

// Đo của SR04 cửa ra
float ultrasonic2() {
  digitalWrite(trigPin2, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin2, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin2, LOW);
  duration2 = pulseIn(echoPin2, HIGH);
  return distance2 = duration2 * 0.034 / 2;
}

// In thông điệp lên màn LCD
void printLCD(int a) {
  if (a == 1) {
    LCD.setCursor(0, 0);
    LCD.print("Kinh chao");
    digitalWrite(ledPin,HIGH);
    Blynk.virtualWrite(V4, 1);
    servo.write(0);
    dem++;
    Blynk.virtualWrite(V2, dem);
    //buzON(500);
  }
  if (a == 2) {
    LCD.setCursor(0, 0);
    LCD.print("Tam biet");
    digitalWrite(ledPin,HIGH);
    Blynk.virtualWrite(V4, 1);
    //buzON(500);
  }
  LCD.setCursor(0, 1);
  LCD.print("Quy khach!<3");
  delay(2000);
  digitalWrite(ledPin,LOW);
  Blynk.virtualWrite(V4, 0);
  LCD.clear();
}

// Điều khiển còi Buzzer
void buzON(int x) {
  tone(BUZZER_PIN, x);  //phát tiếng bíp 1000Hz
  delay(x);
  noTone(BUZZER_PIN);  //dừng tiếng bíp
  buzzState = 0;
}

// Kiểm tra thời gian để reset biến đếm
void checkTime() {
  if (hour == 12 && timeinfo.tm_min >= 0 && timeinfo.tm_min < 1) {
    dem = 0;
    Blynk.virtualWrite(V2, dem);
  }
  if (hour == 18 && timeinfo.tm_min >= 0 && timeinfo.tm_min < 1) {
    dem = 0;
    Blynk.virtualWrite(V2, dem);
  }
  if (hour == 23 && timeinfo.tm_min >= 0 && timeinfo.tm_min < 1) {
    dem = 0;
    Blynk.virtualWrite(V2, dem);
  }
}

// dữ liệu từ Blynk
BLYNK_WRITE(V0) {
  p = param.asInt();
}
BLYNK_WRITE(V1) {
  k = param.asInt();
}
BLYNK_WRITE(V3) {
  state = param.asInt();
}

void loop() {
  // Kiểm tra biến reset đếm
  if (k == 1) {
    dem = 0;
    Blynk.virtualWrite(V2, dem);
  }

  Blynk.run();
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    return;
  }
  hour = timeinfo.tm_hour;

  // Tự bật bảo mật từ 23h đến 7h sáng hôm sau
  if (hour >= 23 || hour <= 7) {
    p = 1;
    dem = 0;
    Blynk.virtualWrite(V0, p);
  }
  checkTime();

  // Kiểm tra chế độ bảo mật
  if (p == 1) {
    servo.write(90);
    Blynk.virtualWrite(V3, 0);
    state = 0;
    while (ultrasonic() <= max) {
      Serial.println("Canh bao");
      LCD.setCursor(0, 0);
      LCD.print("Canh bao!!! ");
      digitalWrite(ledPin,HIGH);
      delay(1000);
      digitalWrite(ledPin,LOW);
      LCD.setCursor(0, 1);
      LCD.print("Trom!!! ");
      buzON(1000);
      delay(500);
      LCD.clear();
      Blynk.logEvent("canh_bao", String("Bao dong"));
    }
  } 
  else {
    if (state == 1) {
      servo.write(0);
      buzzState = 1;
      while (ultrasonic() <= max) {
        if (buzzState == 1) {
          printLCD(1);
          Serial.println("Chao quy khach!");
          buzzState = 0;
        }
        if (ultrasonic() > max) {
          break;
        }
      }
      buzzState = 1;
      while (ultrasonic2() <= max) {
        if (buzzState == 1) {
          printLCD(2);
          Serial.println("Tam biet quy khach!");
          buzzState = 0;
        }
      }
    } 
    else {
      servo.write(90);
      buzzState = 1;
      while (ultrasonic() <= max) {
        if (buzzState == 1) {
          printLCD(1);
          Serial.println("Chao quy khach!");
          buzzState = 0;
        }
        if (ultrasonic() > max) {
          delay(4000);
          servo.write(90);
        }
      }

      buzzState = 1;
      while (ultrasonic2() <= max) {
        if (buzzState == 1) {
          printLCD(2);
          servo.write(180);
          Serial.println("Tam biet quy khach!");
          buzzState = 0;
        }
        if (ultrasonic2() > max) {
          delay(4000);
          servo.write(90);
        }
      }
    }
  }
  delay(500);
}
