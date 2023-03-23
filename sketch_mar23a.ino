// Подключаем библиотеки
#include "WiFi.h"
#include "AsyncUDP.h"
#include "ESPmDNS.h"

#include <GyverPortal.h>
GyverPortal portal;

#include <Wire.h>

#define HW_RESET_PIN 0  // Только программный сброс

#define NBOARDS 17

// Определяем номер этой платы
const unsigned int NUM = 2;
struct multidata {
  /* Номер платы (необходим для быстрого доступа по индексу
    в массиве структур) */
  uint8_t num;
  /* В структуру можно добавлять элементы
    например, ip-адрес текущей платы:*/
  IPAddress boardIP;

  String nameTeam;
  bool dina_Base;
  bool dina_TS;
  bool dina_St;
  bool dina_R;
  bool alertTeam;
  GPcolor color_Res;
  byte vol_s_colb;
  byte vol_en_colb;
};
multidata data[NBOARDS]{ 0 };

/* Определяем имена для mDNS */
// для ведущей платы
const char* master_host = "esp32master";
// приставка имени ведомой платы
const char* slave_host = "esp32slave";
// Определяем название и пароль точки доступа
const char* SSID = "TP-Link_4F90";
const char* PASSWORD = "00608268";

// Определяем порт
const uint16_t PORT = 8888;

// Создаём объект UDP соединения
AsyncUDP udp;

bool arm;

// Определяем callback функцию обработки пакета
void parsePacket(AsyncUDPPacket packet) {
  const multidata* tmp = (multidata*)packet.data();

  // Вычисляем размер данных
  const size_t len = packet.length() / sizeof(data[0]);

  // Если адрес данных не равен нулю и размер данных больше нуля...
  if (tmp != nullptr && len > 0) {
    // Обновляем данные массива структур
    data[2].num = tmp[2].num;
    data[2].boardIP = tmp[2].boardIP;
    // Записываем данные станции
    data[2].nameTeam = tmp[2].nameTeam;
    data[2].dina_Base = tmp[2].dina_Base;
    data[2].dina_TS = tmp[2].dina_TS;
    data[2].dina_St = tmp[2].dina_St;
    data[2].dina_R = tmp[2].dina_R;
    data[2].alertTeam = tmp[2].alertTeam;
    data[2].color_Res = tmp[2].color_Res;
    data[2].vol_s_colb = tmp[2].vol_s_colb;
    data[2].vol_en_colb = tmp[2].vol_en_colb;

    //эти данные должна отправлять динамика
    data[2].dina_St = tmp[2].dina_St;
    data[2].dina_Base = tmp[2].dina_Base;
    data[2].dina_TS = tmp[2].dina_TS;
  }
}

void setup() {
  Serial.begin(100);
  // Инициируем WiFi
  WiFi.begin(SSID, PASSWORD);
  // Ждём подключения WiFi
  Serial.print("Подключаем к WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(100);
  }
  Serial.println();


  // Добавляем номер этой платы в массив структур
  data[NUM].num = NUM;
  //добавляем название команды
  // Записываем адрес текущей платы в элемент структуры
  data[NUM].boardIP = WiFi.localIP();
  data[NUM].nameTeam = "Super";
  data[NUM].vol_s_colb = 200;
  GPcolor color(255,0,0);
  data[NUM].color_Res = color;
  data[NUM].dina_R = true; 
  data[NUM].alertTeam = true;  

  // Инициируем mDNS с именем "esp32slave" + номер платы
  if (!MDNS.begin(String(slave_host + NUM).c_str())) {
    Serial.println("не получилось инициировать mDNS");
  }
  // Узнаём IP адрес платы с UDP сервером
  IPAddress server = MDNS.queryHost(master_host);

  // Если удалось подключиться по UDP
  if (udp.connect(server, PORT)) {

    Serial.println("UDP подключён");

    // вызываем callback функцию при получении пакета
    udp.onPacket(parsePacket);
  }

  Serial.println("Веб страница запущена");

  // Инициализация модуля
  Wire.begin();
}

void action() {
  if (portal.click()) {
    // первый цвет, выведем в порт
    if (portal.click("c1")) Serial.println(portal.getString());

    // второй перепишем в буфер и выведем каналами
    GPcolor buf;
    if (portal.copyColor("c2", buf)) {
      Serial.print(buf.r);
      Serial.print(',');
      Serial.print(buf.g);
      Serial.print(',');
      Serial.println(buf.b);
    }
  }
}


void loop() {
  // Отправляем данные этой платы побайтово
  udp.broadcastTo((uint8_t*)&data[NUM], sizeof(data[0]), PORT);

  // Выводим значения элементов в последовательный порт
  Serial.print("IP адрес платы: ");
  Serial.print(data[2].boardIP);
  Serial.print(", порядковый номер: ");
  Serial.print(data[2].num);
  Serial.print("; ");
  Serial.println();
  Serial.println("----------------------------");
  portal.attach(action);
  delay(500);
}