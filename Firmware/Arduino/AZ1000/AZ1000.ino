#include <WiFiClient.h>
#include <DNSServer.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>

#include <FS.h>

#include "index.h"

// WiFi & Network define.
char ssid[21] = "DS1_2G";
char pass[21] = "ds12ds!@";

// AP define.
const char *APssid = "AZ1000";
const char *APpassword = "12345678";
const byte DNS_PORT = 53;
IPAddress APip(192,168,5,1);
DNSServer dnsServer;

AsyncWebServer server(80);


const int EN_VO3 = 4;       // GPIO4(D2)
const int CTRL = 5;         // GPIO5(D1)

bool valid = false;
int Ctrl;
int nAddr;
int nData;

String Name(String a)
{
    String temp = "\"{v}\":";
    temp.replace("{v}", a);
    return temp;
}

String strVal(String s)
{
    String temp = "\"{v}\",";
    temp.replace("{v}", s);
    return temp;
}

void stringTo(String ssidTemp, String passTemp)
{
    int i;
    for(i=0; i<ssidTemp.length(); i++) {
        ssid[i] = ssidTemp[i];
    }
    ssid[i] = '\0';
    for(i=0; i<passTemp.length(); i++) {
        pass[i] = passTemp[i];
    }
    pass[i] = '\0';
}

bool saveConfig()
{
    String value;

    value = Name("SSID") + strVal(ssid);
    value += Name("PASS") + strVal(pass);
    File configFile = SPIFFS.open("/config.txt", "w");
    if( !configFile ) {
        Serial.println("Failed to open config file for waiting");
        return false;
    }
    configFile.println(value);
    configFile.close();
    return true;
}

String json_parser(String s, String a)
{
    String val;
    if (s.indexOf(a) != -1) {
        int st_index = s.indexOf(a);
        int val_index = s.indexOf(':', st_index);
        if (s.charAt(val_index + 1) == '"') { // 값이 스트링 형식이면
            int ed_index = s.indexOf('"', val_index + 2);
            val = s.substring(val_index + 2, ed_index);
        } else { // 값이 스트링 형식이 아니면
            int ed_index = s.indexOf(',', val_index + 1);
            val = s.substring(val_index + 1, ed_index);
        }
    } else {
        Serial.print(a);
        Serial.println(F(" is not available"));
    }
    return val;
}

bool loadConfig()
{
    File configFile = SPIFFS.open("/config.txt", "r");
    if (!configFile) {
        Serial.println("Failed to open config file");
        return false;
    }

    String line = configFile.readStringUntil('\n');
//    Serial.println(line);
    configFile.close();
    String ssidTemp = json_parser(line, "SSID");
    String passTemp = json_parser(line, "PASS");
    stringTo(ssidTemp, passTemp); // String을 배열에 저장
    return true;
}


void notFound(AsyncWebServerRequest *request) {
	int params = request->params();
    for(int i=0; i<params; i++) {
        AsyncWebParameter *p = request->getParam(i);
        if( p->isFile()) {
            Serial.printf("FILE[%s]: %s, size: %u\n", p->name().c_str(), p->value().c_str(), p->size());
        } else if(p->isPost()){
            Serial.printf("POST[%s]: %s\n", p->name().c_str(), p->value().c_str());
        } else {
            Serial.printf("GET[%s]: %s\n", p->name().c_str(), p->value().c_str());
        }
    }
    Serial.print(millis());Serial.print(" : ");Serial.println(__func__);
    Serial.println(request->url());
    request->send(404, "text/plain", "Not found");
}

void scanWiFiList()
{
    int numberOfNetworks = WiFi.scanNetworks();

    Serial.println("[ AP List ]");
    for(int i=0; i<numberOfNetworks; i++) {
        Serial.printf("%d. ", i);
        Serial.println(WiFi.SSID(i));
    }
}

void onUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final){
    data[len] = '\0';
//    Serial.println("onUpload()");
//    Serial.print("File Name : "); Serial.println(filename);
//    Serial.print("index : "); Serial.println(index);
//    Serial.print("data : "); Serial.println((char *)data);
//    Serial.print("len : "); Serial.println(len);
    parse_cmd((char *)data);
  //Handle upload
}

//char * entry = "3 0 5\n3 2 5\n3 7 50\n3 5 25";
uint8_t list_end = 0;
uint8_t list = 0;
uint8_t Cmd[20][3];
void parse_cmd(char *str) {
    char *line;
    int num = 0;

    line = strtok(str, "\n");
    while( line != NULL ) {
//      Serial.printf(" => %s -> ", line);
        String readString = String(line);
        readString += ' ';
//        Serial.println(readString);

        int _start=0, _end;
        _end = readString.indexOf(' ');
        Cmd[num][0] = readString.substring(0, _end).toInt();
        _start = _end+1;
        _end = readString.indexOf(' ', _start);
        Cmd[num][1] = readString.substring(_start, _end+1).toInt();
        _start = _end+1;
        _end = readString.indexOf(' ', _start);
        Cmd[num][2] = readString.substring(_start, _end+1).toInt();

        num++;

        line = strtok(NULL, "\n");
    }

//    for(int i=0; i<num; i++) {
//        Serial.printf("%d %d %d\n", Cmd[i][0], Cmd[i][1], Cmd[i][2]);
//    }
    list_end = num;
    list = 0;
}

void setup() {
    valid = false;
    Serial.begin(57600);
    while( !Serial ) continue;
    Serial.println("\nTest Program : CTRL ADDR DATA");
    Serial.println("\t- CTRL : 0(x), 1(Initial), 2(Reset), 3(Initial & Reset)");
    Serial.println("\t- ADDR : 0(packet-1), x(packet-2)");
    Serial.println("\t- DATA : 0(no packet), x(packet-1)");

//    pinMode(LED_BUILTIN, OUTPUT);       // GPIO2
    pinMode(EN_VO3, OUTPUT);
    pinMode(CTRL, OUTPUT);

    digitalWrite(EN_VO3, LOW);
    digitalWrite(CTRL, LOW);

    Serial.println("\nTest Start...");

    Serial.println("Mounting FS ...");
    if( !SPIFFS.begin() ) {
        Serial.println("Failed to mount file system");
        return;
    }

    if( !SPIFFS.exists("/config.txt") ) {
        saveConfig();
    }
    loadConfig();
    Serial.printf("SSID : %s, PASSWORD : %s\n", ssid, pass);
    SPIFFS.end();

    	// AP mode
	Serial.print("\nSoftrawre AP ");
	WiFi.softAPConfig(APip, APip, IPAddress(255,255,255,0));
	if(!WiFi.softAP(APssid, APpassword)){
		Serial.println("failed");  
	} else {
		IPAddress myIP = WiFi.softAPIP();
		Serial.print(": IP address(");
		Serial.print(myIP);
		Serial.println(") started");
	}

	dnsServer.setTTL(60);		// 60 sec.
	dnsServer.setErrorReplyCode(DNSReplyCode::ServerFailure);
	dnsServer.start(DNS_PORT, "www.az1000.co.kr", APip);


//    scanWiFiList();
//    Serial.println(ssid);
//    if( ssid != "" ) {
	// STATION mode
    int cnt = 0;
	WiFi.begin(ssid, pass);
	while ( WiFi.status() != WL_CONNECTED) {
		Serial.print(".");
		delay(500);
        if( ++cnt == 20 ) {
            Serial.println("");
            break;
        }
	}
	Serial.println(WiFi.localIP());
//    }

    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send_P(200, "text/html", index_html);
    });

    server.on("/Send", HTTP_GET, [](AsyncWebServerRequest *request) {
//        Serial.println("cmd = " + request->getParam("Ctrl")->value() + 
//                     " " + request->getParam("Addr")->value() + 
//                     " " + request->getParam("Data")->value());

        valid = true;
        Ctrl = request->getParam("Ctrl")->value().toInt();
        nAddr = request->getParam("Addr")->value().toInt();
        nData = request->getParam("Data")->value().toInt();
//        run_command(
//            request->getParam("Ctrl")->value().toInt(),
//            request->getParam("Addr")->value().toInt(),
//            request->getParam("Data")->value().toInt());
        request->send(204, "text/plain", "");
    });

    server.on("/uploadFile", HTTP_POST, [](AsyncWebServerRequest *request) {
//        Serial.println("uploadFile");
        request->send(204, "text/plain", "");
    }, onUpload);

    server.on("/random_command", HTTP_GET, [](AsyncWebServerRequest *request) {
        uint8_t bytes[128];
        String str;

        request->send(204, "text/plain", "");   // ack response.
        str = request->getParam("value")->value();
//        StringToByteArray(str, bytes, " ");

//        frame_parsor(bytes);
    });

    server.onNotFound(notFound);

	server.begin();
	Serial.println("HTTP server started!");

#if 0
    randomSeed(0);

    if( SPIFFS.begin() ) {
        Serial.println("SPIFFS Active");
    } else {
        Serial.println("unable to activate SPIFFS");
    }
	yield();
#endif
//    parse_cmd(entry);
}




void xdelay(int e)
{                                       // 1.009us
    for(int i=0; i<e; i++) {
//        __asm__ __volatile__ ("nop\n\t");
//        __asm__ __volatile__ ("nop\n\t");
        __asm__ __volatile__ ("nop\n\t");
    }
}  // 10(2.2us), 5(1.174us), 4(1.01us), 3(789us)

void reset(void)
{
    WRITE_PERI_REG(0x60000308, (1<<CTRL)|(1<<EN_VO3));     // Low
}

void initial(void)
{
    WRITE_PERI_REG(0x60000304, (1<<EN_VO3));     // High
    WRITE_PERI_REG(0x60000308, (1<<CTRL));     // Low
    delay(30);      // 30 ms
    WRITE_PERI_REG(0x60000304, (1<<CTRL));     // High
    delay(30);      // 30 ms
}

void packet(int addr, int data)
{
    if( addr != 0 ) {
        clock(addr);
        delayMicroseconds(1000);
    }

    clock(data);
    delayMicroseconds(1000);
}

void clock(int e)
{
    int i = 0;

    os_intr_lock();
    while( i < e ) {
        WRITE_PERI_REG(0x60000308, (1<<CTRL));     // Low
        xdelay(12);
        WRITE_PERI_REG(0x60000304, (1<<CTRL));     // High
        xdelay(11);
        i++;
    }
    os_intr_unlock();
}

void run_command(uint8_t ctrl, uint8_t addr, uint8_t data)
{
    Serial.printf("cmd : %d %d %d\n", ctrl, addr, data);

    if( ctrl & (1<<0) ) {   // INITIAL condition.
        initial();
    }

    if( data != 0 ) {   // PACKET condition.
        packet(addr, data);
    }

    if( ctrl & (1<<1) ) {   // RESET condition.
        reset();
    }
}

char buffer[32];


int idx = 0;
void loop()
{
    dnsServer.processNextRequest();

    if( Serial.available() > 0 ) {
#if 1
        char *token;
        uint8_t Bytes[4];
        int nBytes = 0;
        char *split = " :,.-";

        nBytes = Serial.readBytesUntil('\n', buffer, sizeof(buffer));
//        Serial.println(buffer);
        nBytes = 0;
        token = strtok(buffer, split);
//        Serial.println(token);

        if( strncmp(token, "getDevice", 9) == 0 ) {
            Serial.println("device:ESP8266");
            return;
//        } else if( strncmp(token, "#!/bin/av1000", 13) == 0 ) {
//            Serial.println("enter script!");
        } else if( strncmp(token, "ssid", 4) == 0 ) {
            token = strtok(NULL, split);
            String ss(token);

            token = strtok(NULL, split);
            if( strncmp(token, "pass", 4) == 0 ) {
                token = strtok(NULL, split);
                String ps(token);

                stringTo(ss, ps);
                SPIFFS.begin();
                saveConfig();
                SPIFFS.end();
                Serial.println("save complete!");
                ESP.restart();
            }
        } else {

            while( token != NULL ) {
                Bytes[nBytes++] = strtol(token, 0, 10);
                token = strtok(NULL, " :,.-");
            }

            Ctrl = Bytes[0];
            nAddr = Bytes[1];
            nData = Bytes[2];
            if( (Ctrl != 0) || (nAddr != 0) || (nData != 0) ) {
                valid = true;
            }
        }
//        run_command(Bytes[0], Bytes[1], Bytes[2]);

        for(int n=0; n<32; n++) buffer[n] = 0;
#else
        char ch = Serial.read();

        if( (ch == '\n') || (ch == '\r') ) {
            buffer[idx] = '\0';
            if( strncmp(buffer, "getDevice", 9) == 0 ) {
                Serial.println("device:ESP8266");
                for(int n=0; n<32; n++) buffer[n] = 0;
                idx = 0;
                return;
//            } else if( strncmp(buffer, "#!/bin/az1000", 13) == 0 ) {
//                Serial.println("enter script!");
            } else {
                char *token;
                uint8_t Bytes[4];
                uint8_t i=0;

                Serial.println(buffer);
                token = strtok(buffer, " ,.-");
                while( token != NULL ) {
                    Bytes[i++] = strtol(token, 0, 10);
                    token = strtok(NULL, " ,.-");
                }

                Ctrl = Bytes[0];
                nAddr = Bytes[1];
                nData = Bytes[2];
                if( (Ctrl != 0) || (nAddr != 0) || (nData != 0) ) {
                    valid = true;
                }
            }
            for(int n=0; n<32; n++) buffer[n] = 0;
            idx = 0;
        } else {
            buffer[idx++] = ch;
        }
#if 0
        if( (ch == '\n') || (ch == '\r') ) {
            int ctrl, addr, data;

            int n = sscanf(buffer, "%d %d %d", &ctrl, &addr, &data);
            Serial.print("CMD : ");
            Serial.print(ctrl);
            Serial.print(", ");
            Serial.print(addr);
            Serial.print(", ");
            Serial.println(data);

            if( ctrl & (1<<0) ) {       // INITIAL condition.
                initial();
            }

            if( data != 0 ) {           // Packet condition.
                packet(addr, data);
            }

            if( ctrl & (1<<1) ) {       // STOP condition.
                initial();
            }

            for(int n=0; n<32; n++) buffer[n] = 0;
            idx = 0;
        } else {
            buffer[idx++] = ch;
        }
#endif
#endif
    }

    if( valid == true ) {
        valid = false;
        run_command(Ctrl, nAddr, nData);
    }

    if( list < list_end ) {
        run_command(Cmd[list][0], Cmd[list][1], Cmd[list][2]);
        list++;
    }
#if 0
    initial();
    packet(4, 5);
    reset();
#endif
#if 0
//    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
    WRITE_PERI_REG(0x60000304, (1<<2));     // High
    xdelay(9);
//    digitalRead(LED_BUILTIN);               // 600ns
    WRITE_PERI_REG(0x60000308, (1<<2));     // Low
    xdelay(9);
//    {volatile int i; for(i=0; i<4; i++);}  // 10(2.2us), 5(1.174us), 4(1.01us), 3(789us)
    WRITE_PERI_REG(0x60000304, (1<<2));     // High
    xdelay(9);
//    {volatile int i; for(i=0; i<4; i++);}  // 10(2.2us), 5(1.174us), 4(1.01us), 3(789us)
    WRITE_PERI_REG(0x60000308, (1<<2));     // Low
    xdelay(9);
//    {volatile int i; for(i=0; i<4; i++);}  // 10(2.2us), 5(1.174us), 4(1.01us), 3(789us)
    WRITE_PERI_REG(0x60000304, (1<<2));     // High
    xdelay(10);             // 1.09us
//    {volatile int i; for(i=0; i<4; i++);}  // 10(2.2us), 5(1.174us), 4(1.01us), 3(789us)
    WRITE_PERI_REG(0x60000308, (1<<2));     // Low
    xdelay(9);
//    {volatile int i; for(i=0; i<4; i++);}  // 10(2.2us), 5(1.174us), 4(1.01us), 3(789us)
    WRITE_PERI_REG(0x60000304, (1<<2));     // High
    xdelay(9);
//    {volatile int i; for(i=0; i<4; i++);}  // 10(2.2us), 5(1.174us), 4(1.01us), 3(789us)
    WRITE_PERI_REG(0x60000308, (1<<2));     // Low
    xdelay(9);
//    {volatile int i; for(i=0; i<4; i++);}  // 10(2.2us), 5(1.174us), 4(1.01us), 3(789us)
    os_intr_unlock();
#endif
}

