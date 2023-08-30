/* Cloud_Printer:Any Print*/
#include <SPI.h>
#include <Ethernet.h>
#include "html_page.h"
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>

static unsigned char CheckPassWord=0;

// Declaration for SH1106 display connected using software I2C (default case):
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define i2c_Address 0x3c //initialize with the I2C addr 0x3C Typically eBay OLED's
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#include "SoftwareSerial.h"
#define TX_PIN 28  // Arduino transmit  YELLOW WIRE  labeled RX on printer
#define RX_PIN 27  // Arduino receive   GREEN WIRE   labeled TX on printer
SoftwareSerial Printer(RX_PIN, TX_PIN); // Declare SoftwareSerial obj first

#include <WS2812FX.h>
#define LED_COUNT 1
#define LED_PIN 6
WS2812FX ws2812fx = WS2812FX(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

typedef enum 
{
  do_webserver_index = 0,
  do_print_work,
}STATE_;
STATE_ currentState;

// Enter a MAC address and IP address for your controller below.
// The IP address will be dependent on your local network:
byte mac[] = {
  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED
};
IPAddress ip(10, 0, 1, 61);

String data_String; 
uint16_t dataStart = 0;
uint16_t dataEnd = 0;
String dataStr;
uint16_t dataLen = 0;
String Anyprint_text;

EthernetClient client;
EthernetServer server_http(80);

void setup() {
  // initialize serial for debugging  
  Serial.begin(115200);
                                  
  display.begin(i2c_Address, true); // Address 0x3C default
  // Clear the buffer.
  display.clearDisplay();  
  display_logo(64,8);
  display.drawBitmap(30, 22, brower_icon_mini, 24, 24, 1);
  //display.drawBitmap(52, 22, chatgpt_icon_mini,24, 24, 1);
  display.drawBitmap(74, 22, printer_icon_mini,24, 24, 1);
  display_wifi_status(64,60);
  
  Ethernet.init(17);  // WIZnet W5100S-EVB-Pico
  // start the Ethernet connection and the server:
  Ethernet.begin(mac, ip);
  
  // print your WiFi shield's IP address
  ip = Ethernet.localIP();
  display_wifi_status(64,60);  

  Printer.begin(9600);   // Initialize SoftwareSerial
  Power_up_print();
  
  currentState = do_webserver_index;
  // start the web server on port 80
  server_http.begin();  
  //server_any_print.begin();
}

void loop(){
  ws2812fx.service();  
  switch(currentState){      
    case do_webserver_index:
       {
          display.drawRect(28, 20, 28, 28, SSD1306_WHITE);
          display.display();
          ws2812fx.setColor(GREEN);
          client = server_http.available();
          if (client) 
          {
             // an http request ends with a blank line
             boolean currentLineIsBlank = true;    
             while (client.connected()) 
             {
                if (client.available()) {
                char c = client.read();
                data_String += c;
                if(data_String.substring(0, 4) == "TEXT")
                {
                  data_String = "";
                  Anyprint_text = "";
                  while(client.available())
                  {
                    Anyprint_text += (char)client.read();
                  }
                  Serial.println(Anyprint_text);
                  // close the connection:
                  delay(10);
                  client.stop();
                  display.drawRect(28, 20, 28, 28, SSD1306_BLACK);
                  //display.display(); 
                  currentState = do_print_work;                
                }
                if (c == '\n' && currentLineIsBlank) {                                 
                  dataStr = data_String.substring(0, 4);
                  Serial.println(dataStr);
                  if(dataStr == "GET ")
                  {
                    client.print(html_page);
                  }         
                  else if(dataStr == "POST")
                  {
                    data_String = "";
                    while(client.available())
                    {
                      data_String += (char)client.read();
                    }
                    //Serial.println(data_String); 
                    dataStart = data_String.indexOf("printtext=") + strlen("printtext=");
                    Anyprint_text = data_String.substring(dataStart, data_String.length());    
                    Serial.println(Anyprint_text);   
                    Anyprint_text.replace("+", " ");            
                    client.print(html_page);        
                    // close the connection:
                    delay(10);
                    client.stop();
                    display.drawRect(28, 20, 28, 28, SSD1306_BLACK);
                    //display.display(); 
                    currentState = do_print_work;
                  }
                  data_String = "";
                  break;
                }
                if (c == '\n') {
                  // you're starting a new line
                  currentLineIsBlank = true;
                }
                else if (c != '\r') {
                  // you've gotten a character on the current line
                  currentLineIsBlank = false;
                }
              }
            }
          }
       }
      break;

      case do_print_work:
       {  
          display.drawRect(72, 20, 28, 28, SSD1306_WHITE);
          display.display(); 
          ws2812fx.setColor(ORANGE);
          InitializePrint();
          Set_Align((byte)0x00);
          Set_Bold(0x01);
          Printer.print("--------------------------------");
          Printer.print(Anyprint_text);
          Printer.print("\r\n--------------------------------");
          select_lines(2);
          display.drawRect(72, 20, 28, 28, SSD1306_BLACK);
          delay(1000);
          currentState = do_webserver_index;
       }
      break;      
  }
}

void display_logo(uint16_t x,uint16_t y)
{  
  uint8_t cloud_pixel[5*13]=
  {
    0b00111110,0b01000001,0b01000001,0b01000001,0b00100010, // C
    0b00000000,0b00000000,0b01111111,0b00000000,0b00000000, // l
    0b00001110,0b00010001,0b00010001,0b00010001,0b00001110, // o
    0b00011110,0b00000001,0b00000001,0b00000001,0b00011111, // u
    0b00001110,0b00010001,0b00010001,0b00010001,0b01111111, // d
    0b00000000,0b00000000,0b00000000,0b00000000,0b00000000, // space
    0b01111111,0b01001000,0b01001000,0b01001000,0b00110000, // P
    0b00011111,0b00010000,0b00010000,0b00001000,0b00000000, // r
    0b00000000,0b00000000,0b01011111,0b00000000,0b00000000, // i
    0b00011111,0b00010000,0b00010000,0b00010000,0b00001111, // n
    0b00010000,0b00111110,0b00010001,0b00010001,0b00000010, // t    
    0b00001110,0b00010101,0b00010101,0b00010101,0b00001100, // e
    0b00011111,0b00010000,0b00010000,0b00001000,0b00000000  // r
  };
  uint16_t _x = x - 62;
  uint16_t _y = y - 10;
  for(uint8_t i=0;i<13;i++)
  {
    if(i == 0 ||i == 1 ||i == 2 ||i ==5 ||i==6 ||i==9)
    {
       _x = _x-2;
    }
    else if(i == 8)
    {
       _x = _x-3;
    }
    else
    {
       _x = _x+2;
    }
    for(uint8_t m=0;m<5;m++)
    {
      _y = y - 8;
      for(uint8_t n=0;n<8;n++)
      {
        if((cloud_pixel[i*5+m]>>(7-n))&0x01)
        {
            display.fillRect(_x+1, _y+1, 1, 1, SSD1306_WHITE);            
        }
        _y += 2;
      }
      _x = _x + 2;
    }  
  }
  display.display();
}

uint16_t HexStr2Int(String HexStr)
{
  uint16_t ASCIIValue;
  for (int i = 0; i < HexStr.length(); i++) {
    int val = HexStr[i] > 0x39 ? (HexStr[i] - 'a' + 10) : (HexStr[i] - '0');
    ASCIIValue = ASCIIValue << 4;
    ASCIIValue += val; 
  }
  return ASCIIValue;
}

void display_wifi_status(uint8_t x,uint8_t y)
{
  if(Ethernet.linkStatus() != LinkOFF)
  {
    display.drawCircle(x,y+1,1,SSD1306_WHITE);
    display.drawArc(x,y, 3, 3, 225, 315, SSD1306_WHITE); 
    display.drawArc(x,y, 6, 6, 225, 315, SSD1306_WHITE); 
    display.drawArc(x,y, 9, 9, 225, 315, SSD1306_WHITE); 
  }
  else
  {
    display.fillRect(54, 48, 20, 20, SSD1306_BLACK);
    display.fillCircle(x-32,y+1,1,SSD1306_WHITE);
    display.drawArc(x-32,y, 3, 3, 225, 315, SSD1306_WHITE); 
    display.drawArc(x-32,y, 6, 6, 225, 315, SSD1306_WHITE); 
    display.drawArc(x-32,y, 9, 9, 225, 315, SSD1306_WHITE); 

    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(x-20, y-4);
    display.write((ip2Str(ip)).c_str());  
  }
  display.display();
}

String ip2Str(IPAddress ip) //ip地址转字符
{
  String s = "";
  for(int i=0; i<4; i++)
  {
    s+=i?"."+String(ip[i]):String(ip[i]);
  }
  return s;
}

/* 初始化打印机，清除打印缓冲 a*/
void InitializePrint(void)
{
  /* 初始化打印机，清除打印缓冲 */
  Printer.write(0x1B);
  Printer.write(0x40);
  CheckPassWord = 1;
}
/*
** 函数： void select_lines(uint8_t times)
** 参数： times  换行的次数
   功能： 换行
*/
void select_lines(uint8_t times)
{
  uint8_t i;
  for(i=1;i<=times;i++)
  {
    Printer.write(0x0A); /* 单次换行 */ 
  }  
}

/* 选择对齐方式 0:左对齐 1：居中 2：右对齐*/
void Set_Align(unsigned char opt)
{
  if(!CheckPassWord)
    return;  
  if(opt > 2)
  {
    opt = 2;
  }  
  Printer.write(0x1B);
  Printer.write(0x61);
  Printer.write(opt);
}

/* 选择/取消加粗模式 */
void Set_Bold(unsigned char opt)
{
  if(!CheckPassWord)
    return;
  if(opt>0x01)
  {
    opt=0x01;
  }
  Printer.write(0x1B);
  Printer.write(0x45);
  Printer.write(opt);
}

/* 设置下划线有无及规格  a*/
void Set_UnderLine(unsigned char opt)
{
  if(!CheckPassWord)
    return;  
  if(opt > 2)
  {
    opt=2;
  }
  Printer.write(0x1B);
  Printer.write(0x2D);
  Printer.write(opt);
}

/*    打印光栅位图 a */
void PrintGratinmap(unsigned char mode,unsigned int xDot,unsigned int yDot,unsigned char *pData)
{
  unsigned char tmp;
  unsigned int len;  
  if(!CheckPassWord)
    return;
    
  if(mode > 3)
  {
    mode = 3;
  }
  Printer.write(0x1D);
  Printer.write(0x76);
  Printer.write(0x30);
  Printer.write(mode);
  xDot = xDot / 8;
  tmp = (unsigned char)(xDot & 0x00ff);
  Printer.write(tmp);
  tmp = (unsigned char)((xDot>>8) & 0x00ff);
  Printer.write(tmp);
  tmp = (unsigned char)(yDot & 0x00ff);
  Printer.write(tmp);
  tmp = (unsigned char)((yDot>>8) & 0x00ff);
  Printer.write(tmp);
  len = xDot * yDot;
  while(len)
  {
    Printer.write(*pData++);    
    len--;
  }
}

/* 设置错误纠错等级  a*/
void Set_QRCodeAdjuLevel(unsigned char level)
{
  if(!CheckPassWord)
    return;
    
  if(level < 0x30)
  {
    level = 0x30;
  }
  else if(level > 0x33)
  {
    level = 0x33;
  }  
  Printer.write(0x1D);
  Printer.write(0x28);
  Printer.write(0x6B);
  Printer.write(0x03);
  Printer.write((byte)0x00);
  Printer.write(0x31);
  Printer.write(0x45);
  Printer.write(level);
}

/* QRCode传输数据至编码缓存 a*/
void Set_QRCodeBuffer(unsigned int Len,unsigned char *pData)
{
  unsigned char tmp;
  if(!CheckPassWord)
    return;
    
  if(Len < 3)
  {
    return;
  }  
  Len += 3;  
  Printer.write(0x1D);
  Printer.write(0x28);
  Printer.write(0x6B);
  tmp = (unsigned char)(Len & 0x00ff);
  Printer.write(tmp);
  tmp = (unsigned char)((Len >> 8)&0x00ff);
  Printer.write(tmp);
  Printer.write(0x31);
  Printer.write(0x50);
  Printer.write(0x30);
  Len -= 3;
  while(Len)
  {
    Printer.write(*pData++);
    Len--;
  }
  Printer.write((byte)0x00);
}

/* QR Code 打印编码缓存的二维条码   a*/
void PrintQRCode(void)
{
  if(!CheckPassWord)
    return;
    
  Printer.write(0x1D);
  Printer.write(0x28);
  Printer.write(0x6B);
  Printer.write(0x03);
  Printer.write((byte)0x00);
  Printer.write(0x31);
  Printer.write(0x51);
  Printer.write(0x30);
  Printer.write((byte)0x00);  
}

/*
** 函数：QR_code_print(void)
** 
   功能：二维码打印
*/
void QR_code_print(void)
{
  /*  QR_code  */
  String ip_str = String("http://")+ ip2Str(ip)+String("\r\n");
  char QrcodeUrl[28];
  memcpy(QrcodeUrl,(char*)ip_str.c_str(),ip_str.length());
  InitializePrint();                        /* 初始化打印机 */
  //Set_QRcodeMode(0x03);                   /* 设置二维码大小 */
  Set_QRCodeAdjuLevel(0x49);                /* 设置二维码的纠错水平 */
  Set_QRCodeBuffer(strlen(QrcodeUrl),(unsigned char*)QrcodeUrl);  /* 传输数据至编码缓存 */ 
  Set_Align(0x01);                          /* 居中对齐 */
  PrintQRCode();                            /* 打印编码缓存的二维条码 */
  select_lines(1);                          /* 换行 */

  InitializePrint();
  Set_Align(0x01);
  Set_Bold(0x01);
  Printer.print(ip_str);  
  // select_lines(1);
}

void Power_up_print(void)
{
  InitializePrint();
  Set_Align(0x01);
  Set_Bold(0x01);
  Printer.print("Cloud Printer:Any Print\r\n");  
  select_lines(1);
  InitializePrint();
  QR_code_print();
}
