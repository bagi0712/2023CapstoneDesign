/*
 * mega code
 */

#include <SoftwareSerial.h>
#include <Wire.h>

// modules
//#include <MsTimer2.h> // clock function interrupt
#include <Servo.h> // servo motor
#include <Keypad.h> // keypad
#include <DFRobotDFPlayerMini.h> // mp3

// lcd
#include <core_build_options.h>
#include <Adafruit_GFX.h>
#include <Adafruit_GrayOLED.h>
#include <Adafruit_SPITFT.h>
#include <Adafruit_SPITFT_Macros.h>
#include <gfxfont.h>
#include <Adafruit_ILI9341.h>



/*----------------시간 저장을 위한 global var-------------------*/
// ***minute은 1 작은 값으로 initialize
#define DAY 3;
#define MONTH 12;
#define YEAR 2023;
#define MINUTE 3;
#define HOUR 20;
volatile int day = DAY;
volatile int month = MONTH;
volatile int year = YEAR;
volatile int minute = MINUTE;
volatile int hour = HOUR;

unsigned long lcdUpdateTime = 0;
const unsigned long lcdUpdateInterval = 60000;  // 60 seconds


/*-------------------------pin 설정--------------------------------*/
SoftwareSerial co2Serial(51, 50); //CO2에 이용 : RX, TX pins

// LCD
#define TFT_RST 30
#define TFT_MOSI 33
#define TFT_MISO 34
#define TFT_CLK 35
#define TFT_DC 31
#define TFT_CS 32

// 초음파센서
#define trigPin1 38
#define echoPin1 39
#define trigPin2 40
#define echoPin2 41
#define trigPin3 42
#define echoPin3 43

int top_servoPin = 12;     ///뚜껑 서보모터 12번핀에 연결
int wheel_servoPin = 13;   ///바퀴 서보모터 13번핀에 연결
int LED1 = 48;
int LED2 = 49;
const byte ROWS = 4;
const byte COLS = 3;

char hexaKeys [ROWS][COLS] = {
  {'1','2','3'},
  {'4','5','6'},
  {'7','8','9'},
  {'*','0','#'}
};

//keypad PIN
byte rowPins[ROWS] = {10, 5, 6, 8};
byte colPins[COLS] = {9, 11, 7};        ///5,6,7,9번에 전원연결

int pressurePin1 = A0; //압전센서 pin A0~A3번 연결
int pressurePin2 = A1;
int pressurePin3 = A2;
int pressurePin4 = A3;
int mq3Pin = A4;            ///알콜센서 A3핀 연결
int cds = A5;               ///조도센서

//-------------------------------변수 설정-----------------------------------//
// co2용?
unsigned char Send_data[4] = {0x11,0x01,0x01,0xED};
unsigned char Receive_Buff[8];
unsigned char recv_cnt = 0;
unsigned int co2;

// 초음파센서
float duration, sensor1val, sensor2val, sensor3val;
int distance;

// 알콜
int alchol = 0;             ///알콜 센서 실제 측정값
int alcholVal = 0;          ///알콜 감지 시 1, 미감지 0

// 압력
int pressure1 = 0;  
int pressure2 = 0;
int pressure3 = 0;
int pressure4 = 0;
int pressureVal = 0;

// 서보모터 열린게 90, 닫힌게 0
int angle_wheel;
int angle_top;

// 비밀번호 초기값
int password_save = 0;


//-------------------------------객체 생성-----------------------------------//
Servo top_servo;
Servo wheel_servo;
DFRobotDFPlayerMini mp3;
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_MOSI, TFT_CLK, TFT_RST, TFT_MISO); //쉴드 안쓸경우
Keypad customkeypad = Keypad(makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS);




//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//
//-------------------------------FUNCTIONS-----------------------------------//
//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//




//-------------------------------키패드 함수-----------------------------------//


// 키패드에서 숫자 4자리 입력받는 함수
int read_password(void) {

  String password_string = ""; //비밀번호를 string 형태로 선언. 함수 안에서만 동작

  tft.fillRect(0, 120, 400, 200, ILI9341_WHITE);
  
  while(password_string.length() < 4) {   //비밀번호 길이가 4가 되면 반복문 탈출

    char key = 0;
    key = customkeypad.getKey();

    updateLCD();

    if (key) {
      password_string += key;            //string에 하나씩 추가 (4번 반복)

      tft.setTextSize(3);
      tft.setCursor(0,120);
      tft.print(password_string);

      Serial.print("입력한 비밀번호: ");
      Serial.println(key);
      delay(100);
    }
  }
  return password_string.toInt();       //string에 저장된 문자를 int로 변환
}

// read_password()를 호출해 비밀번호 설정하는 함수
void set_password(void) {

  String password_string = "";      //비밀번호를 string 형태로 선언. 함수 안에서만 동작

  Serial.println("set_password 함수 실행");
  Serial.println("키패드를 눌러 비밀번호를 설정하세요: ");

  tft.fillRect(0, 80, 400, 200, ILI9341_WHITE);
  tft.setCursor(0,80);
  tft.setTextSize(3);
  tft.print("SET PASSWORD: ");

  password_save = read_password();  // string형태의 비밀번호를 int형으로
  Serial.println("비밀번호 설정 완료: ");
  Serial.println(password_save);

  tft.fillRect(0, 80, 400, 200, ILI9341_WHITE);
  tft.setCursor(0,80);
  tft.setTextSize(3);
  tft.print("SETTING SUCCESS!");
  delay(500);
}

// read_password()를 호출해 비밀번호가 맞는지 확인하는 함수
bool check_password(void) {
  int password = read_password();

  if(password_save == password) {       ///비밀번호 맞으면

    Serial.println("비밀번호 일치");
    delay(100);

    return true;
  }

  else{                                 ///비밀번호 틀리면

    Serial.println("비밀번호 불일치. 다시 시도해주세요.");
    Serial.print("입력한 비밀번호: ");
    Serial.println(password);
    Serial.print("저장된 비밀번호: ");
    Serial.println(password_save);
    delay(100);

    return false;
  }
}







//-------------------------------CO2 센서-----------------------------------//
void Send_CMD(void) {
  unsigned int i;
  for(i=0; i<4; i++) {
    co2Serial.write(Send_data[i]);
    delay(1);
  }
}

unsigned char Checksum_cal(void) {
  unsigned char count, SUM=0;
  for(count=0; count<7; count++) {
     SUM += Receive_Buff[count];
  }
  return 256-SUM;
}







//------------------------------알코올 센서-----------------------------------//
int alcholCheck(void) {    ///음주 여부를 확인하는 함수
  
  alchol = analogRead(mq3Pin);
  Serial.println("음주 측정을 실행합니다.");
  delay(500);
  
  if(alchol>=700) {          ///음주상태이면
    alcholVal = 1;          ///변수에 1 저장
    Serial.print("음주상태입니다. 수치:");
    Serial.println(alchol);
    Serial.print("\t");
    delay(100);
  }
  else{
    alcholVal = 0;          ///음주 상태가 아니면 변수에 0 저장
    Serial.print("음주상태가 아닙니다. 수치:");
    Serial.println(alchol);
    Serial.print("\t");
    delay(100);
  }
  return alcholVal;         ///1 또는 0값이 저장된 상태로 반환
}





//-------------------------------압력 센서-----------------------------------//
int pressureCheck(void) {         ////압력 감지 함수
  
  pressure1 = analogRead(pressurePin1);   ///압력 센서 데이터 값 받아와 변수에 저장
  pressure2 = analogRead(pressurePin2);
  pressure3 = analogRead(pressurePin3);
  pressure4 = analogRead(pressurePin4);

  if(pressure1 + pressure2 + pressure3 + pressure4 > 250) {      //압력 감지되면
   
    Serial.print("압력 감지. Pressure value = ");
    Serial.println(pressure1 + pressure2 + pressure3 + pressure4);
    Serial.print("\t");
    delay(100);

    return pressureVal = 1;
  }

  else if (pressure1 + pressure2 + pressure3 + pressure4 <= 250) {                ///압력 감지 안되면
    Serial.print("압력 없음. Pressure value = ");
    Serial.println(pressure1 + pressure2 + pressure3 + pressure4);
    Serial.print("\t");
    delay(100);

    return pressureVal = 0;
  }

  else {
    Serial.println("CHECKSUM Error");
  }
}



//-------------------------------LCD패널 함수-----------------------------------//
void lcd_printTime() {
  updateTime();
  tft.setCursor(0, 0);
  tft.setTextSize(4);

  tft.print(month);
  tft.print("/");
  tft.print(day);
  tft.print("/");
  tft.print(year);

  tft.setCursor(0, 40);
  tft.fillRect(0, 40, 400, 41, ILI9341_WHITE);

  tft.print("Time: ");
  tft.print(hour);
  tft.print(":");
  tft.print(minute);
}

// time & date update
void updateTime(void) {
  minute += 1;  // 호출될 때마다(lcd_printTime()이 60000ms마다 호출) 1분씩 증가

  if (minute == 60) {
    ++hour;
    minute = 0;
  }

  if (hour == 24) {
    ++day;
    hour = 0;
  }

  if ((day == 29) && (month == 2)) {
    ++month;
    day = 1;
  }
  else if ((day == 31) && (month == (4 | 6 | 9 | 11))) {
    ++month;
    day = 1;
  }
  else if ((day == 32) && (month == (1 | 3 | 5 | 7 | 8 | 10))) {
    ++month;
    day = 1;
  }
  else if ((day == 32) && (month == 12)) {
    ++year;
    month = 1;
    day = 1;
  }
}

// Check if it's time to update the LCD
void updateLCD (void) {

  if (millis() - lcdUpdateTime >= lcdUpdateInterval) {
    lcd_printTime();
    lcdUpdateTime = millis();  // Reset the timer
  }
}


//-------------------------------MP3 함수-----------------------------------//
void MP3_play(int mp3_value){
  if (mp3_value == 1){
    mp3.volume(30);
    mp3.play(1); //운행을 종료합니다
    delay(1000);
  }
  if (mp3_value == 2){
    mp3.volume(30);
    mp3.play(2); //음주상태를 확인해주세요
    delay(1000);
  }
  if (mp3_value == 3){
    mp3.volume(30);
    mp3.play(3); //잠금이 해제되었습니다
    delay(1000);
  }
  if (mp3_value == 4){
    mp3.volume(30);
    mp3.play(4); //주행 불가
    delay(1000);
  }

  if (mp3_value == 5){
    mp3.volume(30);
    mp3.play(5); //다시 시도해주세요
    delay(1000);
  }

  if (mp3_value == 6){
    mp3.volume(30);
    mp3.play(6); //경고음
    delay(1000);
  }
  
  if (mp3_value == 7){
    mp3.volume(30);
    mp3.play(7); //음주상태입니다 자전거 주행이 불가합니다
    delay(1000);
  }
}




//-------------------------------초음파 센서-----------------------------------//

void driveSensor(int trigPin, int echoPin) {
  digitalWrite(trigPin, HIGH);
  delay(10);
  digitalWrite(trigPin, LOW);
  duration = pulseIn(echoPin, HIGH);
  distance = ((float)(343 * duration) / 10000) / 2; // cm로 변환
}

void car_warning_start(void){
  tft.fillRect(0, 80, 400, 200, ILI9341_WHITE);
  tft.setCursor(0,80);
  tft.setTextSize(3);
  tft.print("Car Warning Mode");

  int key = 0;
  while ((key = customkeypad.getKey()) == 0) {

    if ((key = customkeypad.getKey()) == '*') break;
    driveSensor(trigPin1, echoPin1);
    if ((key = customkeypad.getKey()) == '*') break;
    sensor1val = distance; // 센서1 측정값
    if ((key = customkeypad.getKey()) == '*') break;
    driveSensor(trigPin2, echoPin2);
    if ((key = customkeypad.getKey()) == '*') break;
    sensor2val = distance; // 센서2 측정값
    if ((key = customkeypad.getKey()) == '*') break;
    driveSensor(trigPin3, echoPin3);
    if ((key = customkeypad.getKey()) == '*') break;
    sensor3val = distance; // 센서3 측정값
    if ((key = customkeypad.getKey()) == '*') break;

    Serial.print(sensor1val);
    Serial.print("  ");
    Serial.print(sensor2val);
    Serial.print("  ");
    Serial.println(sensor3val);
    if ((key = customkeypad.getKey()) == '*') break;

    if (sensor3val < 150){  // 1.5m 미만일 때
      Serial.println("Too close! : BACK");
      if(key = customkeypad.getKey() == '*') break;
      warning_led(3);
      if(key = customkeypad.getKey() == '*') break;
      MP3_play(6); //경고음
    }
    else if (sensor1val < 150) {
      Serial.println("Too close! : LEFT");
      if(key = customkeypad.getKey() == '*') break;
      warning_led(1);
      if(key = customkeypad.getKey() == '*') break;
      MP3_play(6); //경고음
    }
    else if (sensor2val < 150) {
      Serial.println("Too close! : RIGHT");
      if(key = customkeypad.getKey() == '*') break;
      warning_led(2);
      if(key = customkeypad.getKey() == '*') break;
      MP3_play(6); //경고음
    }
    else {
      Serial.println("Safe enough.");
      if(key = customkeypad.getKey() == '*') break;
    }
    if ((key = customkeypad.getKey()) == '*') break;
    updateLCD();
    if ((key = customkeypad.getKey()) == '*') break;
  }
}




//-------------------------------초음파 센서와 연계된 LED 함수-----------------------------------//
void warning_led(int led_value){
  if (led_value == 1){
    digitalWrite(LED1,HIGH);
    delay(500);
    digitalWrite(LED1,LOW);
    delay(500);
  }
  if (led_value == 2){
    digitalWrite(LED2,HIGH);
    delay(500);
    digitalWrite(LED2,LOW);
    delay(500);
  }
  if (led_value == 3){
    digitalWrite(LED1,HIGH);
    digitalWrite(LED2,HIGH);
    delay(500);
    digitalWrite(LED1,LOW);
    digitalWrite(LED2,LOW);
    delay(500);
  }
}





//-------------------------------종료 함수-----------------------------------//
void finish(void) {
  while (!check_password()) {
    Serial.println("비밀번호 오류");
    MP3_play(5); //다시 시도해주세요.

    tft.fillRect(0, 80, 400, 200, ILI9341_WHITE);
    tft.setCursor(0,80);
    tft.setTextSize(3);
    tft.print("TRY AGAIN");
  }
  
  Serial.println("자전거 운행 종료합니다.");
  delay(100);

  MP3_play(1); //운행을 종료합니다.

  tft.fillRect(0, 80, 400, 200, ILI9341_WHITE);
  tft.setCursor(0,80);
  tft.setTextSize(3);
  tft.print("LOCKED");
  tft.setCursor(0,120);
  tft.setTextSize(2);
  tft.print("Service is finished");

  angle_wheel = 0; //바퀴 잠김
  wheel_servo.write(angle_wheel);
  Serial.println("바퀴 잠김.");
  Serial.println("뚜껑 잠김.");
  angle_top = 0; // 뚜껑 잠금
  top_servo.write(angle_top);
  alchol = 0;
  alcholVal = 0;
  pressure1 = 0; 
  pressure2 = 0;
  pressure3 = 0;
  pressure4 = 0;  
  pressureVal = 0;

  delay(3000);
}





//----------------------------------------------------------------------//
//----------------------------------------------------------------------//
//-------------------------------MAIN-----------------------------------//
//----------------------------------------------------------------------//
//----------------------------------------------------------------------//

void setup() {
  Wire.begin();                //I2C 총신을 마스터로 하겠다고 선언
  Serial.begin(9600);
  Serial1.begin(9600);  //mp3player 통신
  mp3.begin(Serial1,true);
  top_servo.attach(top_servoPin);
  wheel_servo.attach(wheel_servoPin);
  pinMode(pressurePin1, INPUT);
  pinMode(pressurePin2, INPUT);
  pinMode(pressurePin3, INPUT);
  pinMode(pressurePin4, INPUT);
  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);

  pinMode(trigPin1, OUTPUT);
  pinMode(echoPin1, INPUT);
  pinMode(trigPin2, OUTPUT);
  pinMode(echoPin2, INPUT);
  pinMode(trigPin3, OUTPUT);
  pinMode(echoPin3, INPUT);

  co2Serial.begin(9600);
  mp3.volume(30);
  Serial.println("통신 시작");

  tft.begin();
  tft.setRotation(1);
  tft.fillScreen(ILI9341_WHITE);
  tft.setTextColor(ILI9341_BLACK);

  password_save = 0;

  angle_top = 0;  //닫힌 상태
  angle_wheel = 0; //닫힌 상태
  top_servo.write(angle_top);
  wheel_servo.write(angle_wheel);

  // 시간 표시
  //MsTimer2::set(60000, lcd_printTime);  // 60초(60000ms) 주기로 lcd_printTime 함수 호출
  //MsTimer2::start();  // 타이머 시작
  lcd_printTime();
}





void loop() {

  updateLCD();

//-------------------------------잠겨있을 때-----------------------------------//
  while (angle_wheel == 0) {

    updateLCD();

    while (!check_password()) {

      updateLCD();
      
      MP3_play(5);
      tft.fillRect(0, 80, 400, 200, ILI9341_WHITE);
      tft.setCursor(0,80);
      tft.setTextSize(3);
      tft.print("TRY AGAIN");
      Serial.println("뚜껑 안열림");
      delay(100);

      updateLCD();
    }
    
    updateLCD();

    angle_top = 90; //뚜껑 열림
    MP3_play(2); //잠금이 해제되었습니다. 음주상태를 확인해주세요.
    top_servo.write(angle_top);
    Serial.println("뚜껑 열림");
    Serial.println("음주 측정을 실행해주세요.");

    updateLCD();

    tft.fillRect(0, 80, 400, 200, ILI9341_WHITE);
    tft.setCursor(0,80);
    tft.setTextSize(3);
    tft.print("UNLOCKED");
    tft.setTextSize(2);
    tft.setCursor(0,120);
    tft.print("Check Your Alcohol Level");
    co2 = 0;

    updateLCD();

    alcholCheck();        //음주상태 측정 함수 실행
    delay(5000);
    
    updateLCD();

    while(co2 <= 100) { //CO2 감지 함수 시작

      Send_CMD();
      while(1){
      
        if(co2Serial.available()) {
            Receive_Buff[recv_cnt++] = co2Serial.read();
            if(recv_cnt ==8){
              recv_cnt = 0; 
              break;}
            }
      }
      
      //CO2 수치
      if(Checksum_cal() == Receive_Buff[7]) {
        co2 = Receive_Buff[3]<<8 | Receive_Buff[4];
        Serial.print("CO2 수치 : ");
        Serial.println(co2);
      }
      else {
        Serial.println("CHECKSUM Error");
      }
      delay(500);
    }

    updateLCD();

    angle_wheel = 90;      //바퀴 열림
    MP3_play(3);
    wheel_servo.write(angle_wheel);
    Serial.println("이산화탄소 감지되어 바퀴 열림");
    delay(50);

    updateLCD();
  }

  //-------------------------------바퀴 열린 상태-----------------------------------//
  if (!alcholVal) { // 알코올 감지 안된 경우

    updateLCD();

    tft.fillRect(0, 80, 400, 200, ILI9341_WHITE);
    tft.setCursor(0, 80);
    tft.setTextSize(3);
    tft.print("BIKE UNLOCKED");
    tft.setCursor(0,120);
    tft.setTextSize(2);
    tft.print("Have a nice day");
    delay(100);

    updateLCD();

    char key = 0;
    while (key == 0) { // 키 받을때까지 계속 돌기
      key = customkeypad.getKey();
      updateLCD();
    }

    if (key == '#') {     // #이 입력되면
      set_password();     // 비밀번호를 바꿈
    }
    else if (key == '*') {
      car_warning_start(); //경고음 함수
    }
    else if (key == '0'|'1'|'2'|'3'|'4'|'5'|'6'|'7'|'8'|'9') {
      finish();
    }

    updateLCD();
    delay(100);
  }

  else { //알코올이 감지되면 (음주상태일때)
    
    updateLCD();
    
    int key = 0;

    MP3_play(7); //음주상태입니다. 자전거 주행이 불가합니다.
    tft.fillRect(0, 80, 400, 200, ILI9341_WHITE);
    tft.setCursor(0,80);
    tft.setTextSize(3);
    tft.print("YOU ARE DRUNK");
    tft.setCursor(0,120);
    tft.setTextSize(2);
    tft.print("Driving is impossible");

    updateLCD();
    
    while ((key = customkeypad.getKey()) == 0) { //키가 눌리기 전까진 계속 돌기
      if (pressureVal == 0) {
        if (pressureCheck()) {
          angle_wheel = 0;            //바퀴 다시 잠김 -> 발떼면 다시 바퀴가 풀림
          wheel_servo.write(angle_wheel);

          MP3_play(4); //주행불가 음성

          tft.fillRect(0, 80, 400, 200, ILI9341_WHITE);
          tft.setCursor(0,80);
          tft.setTextSize(3);
          tft.print("YOU ARE DRUNK");
          tft.setCursor(0,120);
          tft.setTextSize(2);
          tft.print("BIKE LOCKED");

          Serial.println("압력 감지. 바퀴 잠김");

          updateLCD();
        }
        updateLCD();
      }
      else {
        if (!pressureCheck()) { //압력 해제된 경우
          angle_wheel = 90;            //바퀴 열린 상태 유지
          wheel_servo.write(angle_wheel);
          
          tft.fillRect(0, 80, 400, 200, ILI9341_WHITE);
          tft.setCursor(0,80);
          tft.setTextSize(3);
          tft.print("YOU ARE DRUNK");
          tft.setCursor(0,120);
          tft.setTextSize(2);
          tft.print("Driving is impossible");

          Serial.println("압력 미감지. 바퀴 열림 유지");

          updateLCD();
        }
        updateLCD();
      }
    }

    updateLCD();

    if (key == '#') { //#이 입력되면
      set_password();     ///비밀번호를 바꿈
    } 
    else if (key) {
      finish();
    }

    updateLCD();
  }
  delay(100);        
}
