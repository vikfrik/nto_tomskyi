#include "WiFi.h"
#include "AsyncUDP.h"
#include "ESPmDNS.h"

#include <GyverPortal.h>
GyverPortal portal;

#include <Wire.h>
#include "TLC59108.h"

#define HW_RESET_PIN 0  // Только программный сброс
#define I2C_ADDR TLC59108::I2C_ADDR::BASE

TLC59108 leds(I2C_ADDR + 7);

// Определяем количество плат
#define NBOARDS 17

// Определяем номер этой платы
const unsigned int NUM = 2;

struct PlotData {
  int16_t vals[2][PLOT_SIZE];
  uint32_t unix[PLOT_SIZE];
};

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

// Массив структур для обмена
multidata data[NBOARDS]{ 0 };

/* Определяем имена для mDNS */
// для ведущей платы
const char* master_host = "esp32master";
// приставка имени ведомой платы
const char* slave_host = "esp32slave";

// Определяем название и пароль точки доступа
#define AP_SSID "MGBot" 
#define AP_PASS "password" 

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

    // Проходим по элементам массива
    for (size_t i = 0; i < len; i++) {

      // Если это не ESP на которой выполняется этот скетч
      if (i != NUM) {
        // Обновляем данные массива структур
        data[i].num = tmp[i].num;
        data[i].boardIP = tmp[i].boardIP;
        // Записываем данные станции
        data[i].nameTeam = tmp[i].nameTeam;
        data[i].dina_Base = tmp[i].dina_Base;
        data[i].dina_TS = tmp[i].dina_TS;
        data[i].dina_St = tmp[i].dina_St;
        data[i].dina_R = tmp[i].dina_R;
        data[i].alertTeam = tmp[i].alertTeam;
        data[i].color_Res = tmp[i].color_Res;
        data[i].vol_s_colb = tmp[i].vol_s_colb;
        data[i].vol_en_colb = tmp[i].vol_en_colb;
      }else{
        //эти данные должна отправлять динамика
        data[i].dina_St = tmp[i].dina_St;
        data[i].dina_Base = tmp[9].dina_Base;
        data[i].dina_TS = tmp[9].dina_TS;
      }
    }
  }
}

void loginPage() { //страница с авторизацией 
  GP_MAKE_BLOCK_TAB(
    "Авторизация"
    GP_MAKE_BOX(GP.LABEL("Логин"); GP.NUMBER("", "", "Ivan");      );    
    GP_MAKE_BOX(GP.LABEL("Пароль"); GP.NUMBER("", "", "123456");         );

void mainPage() { //страница с графиком и управлением 
  GP_MAKE_BLOCK_TAB(
    "Управление станцией",
    GP_MAKE_BOX(GP.LABEL("Элемент 1");     GP.SWITCH("x");      );
    GP_MAKE_BOX(GP.LABEL("Элемент 2");     GP.SLIDER("0","y");  );
    GP_MAKE_BOX(GP.LABEL("Элемент 3");     GP.SPINNER("0","z"); );
  );

  GP.PLOT_STOCK_DARK<2, PLOT_SIZE>("plot", names, data.unix, data.vals, int dec = 0, int height = 400, bool local = 0);

void build() {
  GP.BUILD_BEGIN();
  GP.THEME(GP_DARK);
  GP.UPDATE("x,y,z"); //x,y,z - названия показателей измерения

  GP.TITLE("Станция Томск");
  GP.HR();
  if (setup()) {
    loginPage()
    if () {          //логин и пароль должны быть верными  
      mainPage()   
  GP.BUILD_END();
}

void setup() {
  // Добавляем номер этой платы в массив структур
  data[NUM].num = NUM;
  // Инициируем WiFi
  WiFi.begin(AP-SSID, AP-PASS);
  // Ждём подключения WiFi
  Serial.print("Подключаем к WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
    Serial.print(".");
  }
  Serial.println();
  
  // Записываем адрес текущей платы в элемент структуры
  data[NUM].boardIP = WiFi.localIP();

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
  Serial.println(WiFi.localIP());
  Serial.println("Веб страница запущена");
  portal.attachBuild(build);
  portal.attach(action);
  portal.start();
  // Инициализация модуля
  Wire.begin();
  //здесь место для датчиков

}
}

void loop() {
  // Отправляем данные этой платы побайтово
  udp.broadcastTo((uint8_t*)&data[NUM], sizeof(data[0]), PORT);

  // Выводим значения элементов в последовательный порт
  for (size_t i = 0; i < NBOARDS; i++) {
    Serial.print("IP адрес платы: ");
    Serial.print(data[i].boardIP);
    Serial.print(", порядковый номер: ");
    Serial.print(data[i].num);
    Serial.print("; ");
    Serial.println();
  }
  Serial.println("----------------------------");

  delay(100);
}
}
