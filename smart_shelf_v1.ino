//oled          //XH711         //SENS
//SDA-D2        //DT-D5         //13-D7
//SCL-D1        //SCK-D6    
    
    #include <ESP8266WiFi.h>
    #include <ESPAsyncTCP.h>
    #include <ESPAsyncWebServer.h>
    #include <ESP8266HTTPClient.h>
    #include "ArduinoJson.h"
    #include <EEPROM.h>
    #include <DNSServer.h>
    #include <GyverHX711.h>
    #include <SPI.h>
    #include <Wire.h>
    #include <GyverFilters.h>

GMedian<13, int> testFilter;

GyverHX711 sensor(14, 12, HX_GAIN64_A);
int weight = 0;

int zeroWeigth = 0;
// const char* ssid     = "[MikroTik]";
// const char* password = "19591983";

const int output = 4;
const int buttonPin = 5; // 2
const int connectPin = 13;
const int ipPort = 80;
const int ledPin = 2;
// DNS SETUP
const byte DNS_PORT = 53;
DNSServer dnsServer;

bool goToUpdate = false;
int updateShelfByIndex = 0;
String shelfMessage;
String shelfParam;

// Variables will change:
int ledState = LOW;
int buttonState;
int lastButtonState = LOW;

bool restart;
bool onConfig = false;

String qsid = "";
String qpass = "";
String qeventhost = "";
String qevents = "";
String spiff_cont = "";
String inputSSID = "";
String inputPASS = "";
String wifilist = "";
String jsonStr;
String postMessage;
char host[] = "smart_shelf";

AsyncWebServer server(ipPort);
const char *PARAM_INPUT = "inputdata";

// HTML
const char index_html[] PROGMEM = R"rawliteral(
  <!DOCTYPE html>
  <html lang="en">
  <head>
      <meta name="viewport" content="width=device-width, initial-scale=1">
      <meta charset="UTF-8">
      <title>smart_shelf config page</title>
      <style>
          html{ color: #333333; font-size: 16px}
          .container{ background: #565f60; display: flex; flex-direction: column; align-items: center;}
          .wifi-credentials{ display: flex; flex-direction: row; justify-content: center; }
          .main-container{display: flex; flex-direction: column; width: 20vw; margin: 1rem}
          .control {
              display: block;
              width: 100%;
              min-width: 220px;
              padding: .375rem .75rem;
              font-size: 1rem;
              line-height: 1.5;
              color: #495057;
              background-color: #fff;
              background-clip: padding-box;
              border: 1px solid #ced4da;
              border-radius: .25rem;
              transition: border-color .15s ease-in-out,box-shadow .15s ease-in-out;
          }
          .btn {
              color: #fff;
              background-color: #cb1d38;
              display: inline-block;
              font-weight: 400;
              text-align: center;
              white-space: nowrap;
              vertical-align: middle;
              -webkit-user-select: none;
              -moz-user-select: none;
              -ms-user-select: none;
              user-select: none;
              border: 1px solid #cb1d38;
              padding: .375rem .75rem;
              font-size: 1rem;
              line-height: 1.5;
              border-radius: .25rem;
              transition: color .15s ease-in-out,background-color .15s ease-in-out,border-color .15s ease-in-out,box-shadow .15s ease-in-out;
          }
          .btn:hover{
            background-color: #a6132f;
            transition: all 200ms;
          }
          .label {
              color: white;
              font-size: 1rem;
              display: inline-block;
              margin-bottom: .5rem;
              align-self: flex-start;
          }
          option {
              font-size: 1rem;
          }
          button, input, optgroup, select, textarea {
              margin: 0;
              font-family: inherit;
              font-size: inherit;
              line-height: inherit;
          }
          *, ::after, ::before {
              box-sizing: border-box;
          }
          button, select {
              text-transform: none;
          }
          select.control:not([size]):not([multiple]) {
              height: calc(2.25rem + 2px);
          }
          @media screen and (max-width: 1080px) {
              .wifi-credentials{flex-direction: column}
              .main-container{width: 60vw}
          }
      </style>

  </head>
  <body class="container">
      <h1 style="color:#ddf2ff">smart_shelf Setup Page</h1>

      <div class="wifi-credentials">
        %BUTTONPLACEHOLDER%
      </div>

      <div class="main-container" style="width: 60vw;">
          <textarea id="saved" cols=120 rows=15  class="control" style="margin-bottom: 1rem"></textarea>
          <button type="button" class="btn" onclick="sentcontent()">GET Save And Reboot</button>
          <p></p>
          <button type="button" class="btn" onclick="sentPOSTcontent()">POST Save And Reboot</button>
      </div>
  </body>
  </html>

  <script>
      function sentcontent(){
          var name = document.getElementById('wifiname').value;
          var pass = document.getElementById('wifipass').value;

          data='/get?inputdata={ "inputdata" :{"wifiname":"' + name + '","wifipass":"'+ pass +'","eventdata":'+ document.getElementById('saved').value + '}';
          data.replace(/" /g, '');
          data.replace(/ "/g, '');
          window.location.href=data;
      }

          function sentPOSTcontent(){
          var name = document.getElementById('wifiname').value;
          var pass = document.getElementById('wifipass').value;

          data='{ "inputdata" :{"wifiname":"' + name + '","wifipass":"'+ pass +'","eventdata":'+ document.getElementById('saved').value + '}';
          data.replace(/" /g, '');
          data.replace(/ "/g, '');
          srvURL=window.location.protocol + "//" + window.location.host + "/";
            var pst = new XMLHttpRequest();
            pst.open("POST", srvURL, true);
            pst.setRequestHeader("Content-Type", "application/x-www-form-urlencoded; charset=UTF-8");
            pst.send(data);
            pst.responseType = 'text';
              pst.onreadystatechange = function() {
                if (pst.readyState === pst.DONE) {
          if (pst.status === 200) {
              console.log(pst.response);
              window.location.href="/save";
              //console.log(pst.responseText);
          }
      }
            }
          //window.location.href="/save";
      }

      function showJSON() {
            var wifiname = document.getElementById('wifiname').value;
          var wifipass = document.getElementById('wifipass').value;
          var h1 = document.getElementById('host1').value;
          var h2 = document.getElementById('host2').value;
          var h3 = document.getElementById('host3').value;
          var e1 = document.getElementById('event1').value;
          var e2 = document.getElementById('event2').value;
          var e3 = document.getElementById('event3').value;
              var ugly ='{ "inputdata" :{"wifiname":"' + wifiname + '","wifipass":"'+ wifipass +'","eventdata": {"'+
              h1 +'":"' + e1 +'",'+
              '"' + h2 +'":"'  + e2 +'",'+
              '"' + h3 +'":"' + e3 +'"}}}';
              var obj = JSON.parse(ugly);
              var pretty = JSON.stringify(obj, undefined, 4);
              document.getElementById('myTextArea').value = pretty;
          }
      function showSaved() {

              var obj = JSON.parse(document.getElementById('savedJSON').innerHTML);
              var showJSON = JSON.stringify(obj, undefined, 4);
              document.getElementById('saved').value = showJSON;
        }

        function update() {
          var select = document.getElementById('networks');
          var option = select.options[select.selectedIndex];
          document.getElementById('wifiname').value = option.value;
        }
          window.onload = showSaved;
  </script>)rawliteral";

const char when_product_scan_html[] PROGMEM = R"rawliteral(
  <!DOCTYPE html><html lang=\"en\">
  <head> 
    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\"> 
    <title>Title</title> 
    <style> html{ color: #333333; font-size: 18px} .container{ background: #565f60; display: flex; flex-direction: column; align-content: center; align-items: center;} 
    @media screen and (max-width: 1080px) { .wifi-credentials{flex-direction: column} .main-container{width: 60vw} } 
    </style>
  </head>
  <body class=\"container\"> 
    <h1 style=\"color:#ced4da\">Product was scaned!</h1>
    <h2 style=\"color:#ced4da\">scan QR on shelf to add it</h2>
  </body></html>
)rawliteral";

const char when_product_add_html[] PROGMEM = R"rawliteral(
  <!DOCTYPE html><html lang=\"en\">
  <head> 
    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\"> 
    <title>Title</title> 
    <style> html{ color: #333333; font-size: 18px} .container{ background: #565f60; display: flex; flex-direction: column; align-content: center; align-items: center;} 
    @media screen and (max-width: 1080px) { .wifi-credentials{flex-direction: column} .main-container{width: 60vw} } 
    </style>
  </head>
  <body class=\"container\"> 
    <h1 style=\"color:#ced4da\">Product added to smart shelf</h1>
  </body></html>
)rawliteral";

String wificredits(const String &var)
{
  if (var == "BUTTONPLACEHOLDER")
  {
    String buttons = "";
    buttons += "<div class=\"main-container\">";
    buttons += "<label class=\"label\" for=\"wifiname\">WIFI name</label>";
    buttons += "<input class=\"control\" type=\"text\" id=\"wifiname\" value=\"" + inputSSID + "\"></div>";
    buttons += "<div class=\"main-container\">";
    buttons += "<label class=\"label\" for=\"wifipass\">WIFI password</label>";
    buttons += "<input class=\"control\" type=\"password\" id=\"wifipass\" value=\"" + inputPASS + "\"></div>";
    buttons += "<label style=\"display:none;\" id=\"savedJSON\">" + jsonStr + "</label>";
    buttons += wifilist;
    return buttons;
  }
  return String();
  }

  void scanwifinetwork()
  {
    wifilist = "<div class=\"main-container\">";
    wifilist += "<label class=\"label\" for=\"wifiname\">WIFI List</label>";
    wifilist += "<select class=\"control\" id=\"networks\" onChange=\"update()\">";
    Serial.print("Scan start ... ");
    int n = WiFi.scanNetworks();
    Serial.print(n);
    Serial.println(" network(s) found");
    for (int i = 0; i < n; i++)
    {
      Serial.println(WiFi.SSID(i));
      wifilist += "<option value=\"" + WiFi.SSID(i) + "\">" + WiFi.SSID(i) + "</option>";
    }
    wifilist += "</select></div>";
  }

  void notFound(AsyncWebServerRequest *request) {
    request->send(404, "text/plain", "Not found");
  }

  void saveConfig()
  {
    StaticJsonDocument<900> doc_eeprom;
    deserializeJson(doc_eeprom, postMessage);
    JsonObject data = doc_eeprom["inputdata"];
    Serial.println(data);
    const char *wifiname = data["wifiname"];
    const char *wifipass = data["wifipass"];
    const char *eventjson = data["eventdata"];
    Serial.println("POST//json large: ");
    Serial.println(data.size());
    /////////////////EEPROM write////////////////////////////////
    qsid = wifiname;
    qpass = wifipass;
    // qeventhost = eventhost;
    qevents = eventjson;

    if (qsid.length() > 0 && qpass.length() > 0)
    {
      Serial.println("POST//clearing eeprom");
      for (int i = 0; i < 1024; ++i)
      {
        EEPROM.write(i, 0);
      }
      Serial.print(qsid);
      Serial.print("/");
      Serial.println(qpass);
      Serial.println("POST//writing eeprom ssid:");
      for (int i = 0; i < qsid.length(); ++i)
      {
        EEPROM.write(i, qsid[i]);
        Serial.print("POST//Wrote: ");
        Serial.println(qsid[i]);
      }
      Serial.println("POST//writing eeprom pass:");
      for (int i = 0; i < qpass.length(); ++i)
      {
        EEPROM.write(32 + i, qpass[i]);
        Serial.print("POST//Wrote: ");
        Serial.println(qpass[i]);
      }
      EEPROM.commit();
    };

    ////////////////////WRITE SPIFS
    File file = SPIFFS.open("/post.json", "w");
    int bytesWritten = file.print(postMessage);
    file.close();
    // Print values.
    Serial.println(wifiname);
    Serial.println(wifipass);
    Serial.println("POST//bytesWritten:");
    Serial.println(bytesWritten);
    //////end JSON
  }

  void updateContent(bool secure, String messageUpdate)
  {
    if (secure)
    {
      Serial.println("secure");
      std::unique_ptr<BearSSL::WiFiClientSecure> client(new BearSSL::WiFiClientSecure);
      client->setInsecure();
      HTTPClient http;
      http.begin(*client, "https://webhook.site/adc734bb-861d-4acf-846f-38259f79a13b");
      http.addHeader("Content-Type", "application/json");
      http.setUserAgent("smart_shelf");
      int httpCode = http.POST(messageUpdate);
      Serial.println(httpCode);
      http.end();
    }
    else
    {
      WiFiClient client;
      HTTPClient http;
      http.begin(client, "http://192.168.77.77:55554/event");
      http.addHeader("Content-Type", "application/json");
      http.setUserAgent("smart_shelf");
      int httpCode = http.POST(messageUpdate);
      Serial.println(httpCode);
      http.end();
    }
  }

  void setup()
  {
    pinMode(connectPin, INPUT_PULLUP);
    pinMode(output, OUTPUT);
    digitalWrite(output, LOW);
    pinMode(ledPin, OUTPUT);
    digitalWrite(ledPin, LOW);
    pinMode(buttonPin, INPUT);
    delay(1000);

    Serial.begin(115200);
    Serial.flush();
    Serial.println("filterd, shifted");
    EEPROM.begin(1024);
    Serial.println("start SPIFFS");
    bool success = SPIFFS.begin();
    if (success)
    {
      Serial.println("SPIFFS// File system mounted with success");
    }
    else
    {
      Serial.println("SPIFFS// Error mounting the file system");
    }
    // File file = SPIFFS.open("/file.txt", "w");
    File file = SPIFFS.open("/post.json", "r");

    if (!file)
    {
      Serial.println("SPIFFS// Error opening file for writing");
      Serial.println("SPIFFS// creating new file...");
      File file = SPIFFS.open("/post.json", "w");
      int bytesWritten = file.print("");
      file.close();
    }
    else
    {

      while (file.available())
      {
        spiff_cont += char(file.read());
      }
      Serial.println("SPIFFS// file ./post.json:");
      Serial.println(spiff_cont);
      file.close();
    }
    // deserialize
    Serial.println("deserializeJson// post.json");
    StaticJsonDocument<900> eep;
    deserializeJson(eep, spiff_cont);
    JsonObject data = eep["inputdata"];
    Serial.println("Json// {inputdata}:");
    const char *wifiname = data["wifiname"];
    const char *wifipass = data["wifipass"];
    const char *eventdata = data["eventdata"];
    Serial.println(data.size());

    StaticJsonDocument<900> spif;
    deserializeJson(spif, spiff_cont);
    JsonObject evdata = spif["inputdata"]["eventdata"];
    Serial.println("Json// {inputdata}:");
    // String jsonStr;
    serializeJson(evdata, jsonStr);
    Serial.println(jsonStr);

    Serial.println("SPIIFFS>Json// {inputdata} size: ");
    int evdatasize = evdata.size();

    Serial.println(evdatasize);
    Serial.println("SPIIFFS>Json// {inputdata} entris: ");

    ///////////
    for (JsonPair keyValue : evdata)
    {
      Serial.println(keyValue.key().c_str());
    }
    ////////////////////

    Serial.println("Reading EEPROM ssid");
    String esid;
    for (int i = 0; i < 32; ++i)
    {
      if (EEPROM.read(i) != 0)
      {
        esid += char(EEPROM.read(i));
      }
    }
    inputSSID = esid;
    Serial.println();
    Serial.print("SSID: ");
    Serial.println(esid);
    Serial.println("Reading EEPROM pass");

    String epass = "";
    for (int i = 32; i < 96; ++i)
    {
      if (EEPROM.read(i) != 0)
      {
        epass += char(EEPROM.read(i));
      }
    }
    inputPASS = epass;
    Serial.print("PASS: ");
    Serial.println(epass);

    //==========if btn is presed then >CONFIG MODE and wifi hotspot is active untill reboot

    int reading = digitalRead(buttonPin);

    if (reading == 0)
    { // set to 0!!!!!
      onConfig = true;
      digitalWrite(output, HIGH);
      Serial.println(">>>CONFIG MODE<<<");
      scanwifinetwork();
      Serial.println("Setting soft-AP ... ");
      WiFi.softAP("BUTTON_CONFIG", "12345678");
      IPAddress IP = WiFi.softAPIP();
      Serial.print("AP IP address: ");
      Serial.println(IP);
      // SET DNS
      dnsServer.setTTL(300);
      dnsServer.setErrorReplyCode(DNSReplyCode::ServerFailure);
      dnsServer.start(DNS_PORT, "www.btn.net", IP);
    }
    else
    {
      // Connect to Wi-Fi
      // WiFi.hostname(hostname.c_str()); //define hostname

      WiFi.mode(WIFI_STA);
      wifi_station_set_hostname(host);
      WiFi.setAutoConnect(false);
      Serial.printf("hostname: %s\n", WiFi.hostname().c_str());
      WiFi.begin(esid, epass);
      while (WiFi.status() != WL_CONNECTED)
      {
        delay(1000);
        Serial.println("Connecting to WiFi..");
        digitalWrite(output, !digitalRead(output));
      }
      digitalWrite(output, LOW);
    }
    // Print ESP Local IP Address
    Serial.println(WiFi.localIP());
    Serial.printf("hostname: %s\n", WiFi.hostname().c_str());
    // HTML

    // Route for root / web page
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send_P(200, "text/html", index_html, wificredits);
    });

    server.on("/save", HTTP_GET, [](AsyncWebServerRequest *request){
            request->send(200, "text/html", "<!DOCTYPE html><html lang=\"en\"><head> <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\"> <title>Title</title> <style> html{ color: #333333; font-size: 18px} .container{ background: #565f60; display: flex; flex-direction: column; align-content: center; align-items: center;} @media screen and (max-width: 1080px) { .wifi-credentials{flex-direction: column} .main-container{width: 60vw} } </style></head><body class=\"container\"> <h1 style=\"color:#ced4da\">hooray, dude!</h1> <h2 style=\"color:#ddf2ff\">Button was successfully configured!</h2> <p></p> <textarea cols=120 rows=15 class=\"control\" style=\"margin-bottom: 1rem\">"
      + postMessage +  "</textarea></body></html>");
      Serial.println("restart ESP...");
      delay(2000);
      restart=true;
    });

    server.on("/set_update_data", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send_P(200, "text/html", when_product_scan_html);
        if (request->hasParam(PARAM_INPUT)) {
          shelfMessage = request->getParam(PARAM_INPUT)->value();
          shelfParam = PARAM_INPUT;
          Serial.println(shelfMessage);
        }
    });

    server.on("/update_shelf_1", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send_P(200, "text/html", when_product_add_html);
          goToUpdate = true;
          updateShelfByIndex = 1;
    });

    server.on("/update_shelf_2", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send_P(200, "text/html", when_product_add_html);
          goToUpdate = true;
          updateShelfByIndex = 2;
    });

    server.on("/update_shelf_3", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send_P(200, "text/html", when_product_add_html);
          goToUpdate = true;
          updateShelfByIndex = 3;
    });

      // Route for POST / 
  server.on("/", HTTP_POST, [](AsyncWebServerRequest *request){
    Serial.println("recive POST");
      int params = request->params();
      Serial.println(params);
      if (params>=0){
        AsyncWebParameter* p = request->getParam(0);
        postMessage=p->value().c_str();
        Serial.println(postMessage);
      }
      saveConfig();
      Serial.println("send request...");
      request->send(200, "text/html", "done"); 
      //request->redirect("/save");
    });

      server.on("/getID", HTTP_POST, [](AsyncWebServerRequest *request){
      Serial.println("recive ID POST");
      Serial.println("send ID via post request...");
      request->send(200, "text/html", WiFi.hostname().c_str());
    });

    // Send a GET request to <host_IP>/get?
    server.on("/get", HTTP_GET, [] (AsyncWebServerRequest *request) {
      String inputMessage;
      String inputParam;
      ///////////////GET DATA SAMPLE////////////////
      // {
      //   "inputdata": {
      //     "wifiname": "micro",
      //     "wifipass": "pass",
      //     "eventdata": {
      //       "event": "{\"eventName\":\"DYNAMIC_EVENT\"}",
      //       "event1": "{\"eventName\":\"DYNAMIC_EVENT\"}",
      //       "event2": "{\"eventName\":\"DYNAMIC_EVENT\"}"
      //     }
      //   }
      // }
      ////////////////////////////////////////

      // GET input value on <ESP_IP>/get?input
      if (request->hasParam(PARAM_INPUT))
      {
        inputMessage = request->getParam(PARAM_INPUT)->value();
        inputParam = PARAM_INPUT;

        StaticJsonDocument<900> doc_eeprom;
        deserializeJson(doc_eeprom, inputMessage);
        JsonObject data = doc_eeprom["inputdata"];
        Serial.println(data);
        const char *wifiname = data["wifiname"];
        const char *wifipass = data["wifipass"];
        const char *eventjson = data["eventdata"];

        // const char* eventhost = data["eventhost"];
        // const char* events = data["events"];
        // Serial.println(events);

        Serial.println("json large: ");
        Serial.println(data.size());
        /////////////////EEPROM write////////////////////////////////
        qsid = wifiname;
        qpass = wifipass;
        // qeventhost = eventhost;
        qevents = eventjson;

        if (qsid.length() > 0 && qpass.length() > 0)
        {
          Serial.println("clearing eeprom");
          for (int i = 0; i < 1024; ++i)
          {
            EEPROM.write(i, 0);
          }
          Serial.println(qsid);
          Serial.println("");
          Serial.println(qpass);
          Serial.println("");

          Serial.println("writing eeprom ssid:");
          for (int i = 0; i < qsid.length(); ++i)
          {
            EEPROM.write(i, qsid[i]);
            Serial.print("Wrote: ");
            Serial.println(qsid[i]);
          }
          Serial.println("writing eeprom pass:");
          for (int i = 0; i < qpass.length(); ++i)
          {
            EEPROM.write(32 + i, qpass[i]);
            Serial.print("Wrote: ");
            Serial.println(qpass[i]);
          }
          EEPROM.commit();
        };
        ////////////////////WRITE SPIFS

        File file = SPIFFS.open("/post.json", "w");
        int bytesWritten = file.print(inputMessage);
        file.close();

        // Print values.
        Serial.println(wifiname);
        Serial.println(wifipass);
        Serial.println("bytesWritten:");
        Serial.println(bytesWritten);
        //////end JSON
      }

      else
      {
        inputMessage = "GET// unvalid setting request";
        inputParam = "none";
      }

      Serial.println(inputMessage);
      // request->send(200, "text/html", "<h2>button will reset now with parameters:</h2><h6>" 
      //                                 + inputMessage + "</h6>");
    request->send(200, "text/html", "<h2>button will reset now with parameters:</h2><h6>" 
                                      + inputMessage + "</h6>");


      Serial.println("restart ESP...");
      delay(1000);
      restart=true;
      
    });
    server.onNotFound(notFound);
    server.begin();

  zeroWeigth=sensor.read()/11;
  delay(1000);
  zeroWeigth=sensor.read()/11;
  Serial.print("zero weigth:");
  Serial.println(zeroWeigth);
  }

void oldupdate(){
  
  if (restart){
    unsigned long timing= millis ();
    Serial.println("restart in 3 sec...");
        delay(3000);
        Serial.println("restart...");
        ESP.restart(); 
        //setup();
    }
  if (onConfig!=false){
    dnsServer.processNextRequest();
  }
  int reading = digitalRead(buttonPin);

  if (onConfig==false){

    if (reading ==0) {
      Serial.println("btn");
          StaticJsonDocument<900> main;
          deserializeJson(main, spiff_cont);
          JsonObject maindata = main["inputdata"]["eventdata"]; 
          String keyval;  
        
        for (JsonPair keyValue : maindata) {
              keyval=keyValue.key().c_str();
              Serial.println(keyValue.key().c_str());
                
            String postname = maindata[keyValue.key().c_str()];
                //serializeJson(postname, jsonStr);
                Serial.println(postname);

            Serial.println(">HTTP DATA: " + keyval + "/" + postname );
              char toHost[128];
              keyval.toCharArray(toHost, 128);
              char toEvent[128];
              postname.toCharArray(toEvent, 128);

              String ifsecure = keyval.substring(4,5);

              if (ifsecure=="s"){ 
                  Serial.println("secure");
                    std::unique_ptr<BearSSL::WiFiClientSecure>client(new BearSSL::WiFiClientSecure);
                    client->setInsecure();                 
                    HTTPClient http;                  
                    http.begin(*client, toHost);           //////qeventhost or "https://webhook.site/71c2f7ef-1dff-492b-9377-5757859cb3c4"
                    http.addHeader("Content-Type", "application/json");  
                    http.setUserAgent( "smart_shelf");                 
                    int httpCode = http.POST(toEvent);    ///////// "{\"eventName\":\"DYNAMIC_EVENT\"}"             
                    Serial.println(httpCode);                    
                    http.end(); 
              }
              else{
                  Serial.println("not secure");
                    WiFiClient client;
                    HTTPClient http;                                      
                    http.begin(client, toHost);         //http://192.168.50.143:55554/event
                    http.addHeader("Content-Type", "application/json"); 
                    http.setUserAgent( "smart_shelf");                   
                    int httpCode = http.POST(postname);                 
                    Serial.println(httpCode);                    
                    http.end(); 
              }
        }
 
        }
        if (WiFi.status() != WL_CONNECTED){
          Serial.println(">>>disconected...");
          Serial.println("restart in 3 sec...");
            delay(3000);
          Serial.println("restart...");
            ESP.restart(); 
       }
    }
}


unsigned long millisStart=0;
bool waiting=false;
int deltaWeight=0;
long incomingByte;

int tempData=0;
int curWeight=0;
int memWeight=1;
int i=0;

const int redLable=570;   int l_redLable=555;   int h_redLable=585;
const int cocaCola=872;   int l_cocaCola=832;   int h_cocaCola=900;
const int beer=588;       int l_beer=578;       int h_beer=598;
const int lviv_ipa=350;   int l_lviv_ipa=330;   int h_lviv_ipa=370;



int getProduct(int val){
  if(val > l_redLable & val < h_redLable)       {return 1;}
    else if(val > l_cocaCola & val <h_cocaCola ){return 2;}
    else if(val > l_beer & val < h_beer)        {return 3;}
    else if(val > l_beer & val < h_beer)        {return 4;}
  else {return 0;}
}

int incomingData;
int filtered;

int dataArray[20];
int maxVal;
int minVal;
int outVal;

bool addShelfFlag=false;
int counter=0;
int deltaVal=0;
int stableValue=0;
int emptyShelfValue = 2730;
bool noProducts = false;
String eventData;

const String bootle_LvivIpa = "{\"name\":\"bootle_info\",\"value\":\"749ce450-1abf-4847-97bf-99eeb5b0302c.cnt\"}";
const String bootle_Grimbergen = "{\"name\":\"bootle_info\",\"value\":\"f4f5f136-3129-4692-8ad8-15b25e36bbc9.cnt\"}";
const String bootle_RedLabel_promo = "{\"name\":\"bootle_info\",\"value\":\"e2fc08a6-f9ba-452d-b2d2-b23d93d2916c.cnt\"}";
const String bootle_RedLabel = "{\"name\":\"bootle_info\",\"value\":\"7a126644-a0ea-486c-9726-4f891bb65f24.cnt\"}";

const String bootle_Cola = "{\"name\":\"bootle_info\",\"value\":\"b0de2396-2e53-40f4-a56d-ed064ea6d9a0.cnt\"}";
const String bootle_Blank = "{\"name\":\"bootle_info\",\"value\":\"291c120a-139b-4276-97f0-ceeaa1d07946.cnt\"}";

const String shelf_NoProducts = "{\"name\":\"shelf_change\",\"value\":\"7eb60178-f8c9-4bc5-87a7-69d7278e2e04.cnt\"}";
const String shelf_TwoProducts = "{\"name\":\"shelf_change\",\"value\":\"51a7729a-af1c-434a-8e7d-07ff1f5b16cc.cnt\"}";
const String shelf_TwoPlusOne = "{\"name\":\"shelf_change\",\"value\":\"45614522-1f4c-4f96-b821-a2188c3a8709.cnt\"}";

const String shelfOneQR = "ea9b157c-ce1f-4eb4-bc78-39c42ba28434.cnt";
const String shelfTwoQR = "311f765d-82a0-4729-90bc-6b392ea43d5b.cnt";
const String shelfThreQR = "dbb2c128-9b16-4a4b-aa08-532dc19e0926.cnt";

String shelf_addProduct(String index, String product, String price){
  String data = "{\"name\":\"lable_" + index + "_name\",\"value\":\"" + product + "\"},{\"name\":\"lable_" + index + "_price\",\"value\":\"" + price + "\"}";
  if (price == ""){data += ",{\"name\":\"lable_" + index + "_qr\",\"value\":\"b5e207c5-6b7b-4e75-b89b-5b2f40bf0c64.cnt\"}";}
  return data;
}

String hide_Price_Lable(String index){   // hide all lable by index
  String data = "{\"name\":\"lable_" + index + "_name\",\"value\":\"" + "" + "\"}";
  data += ",{\"name\":\"lable_" + index + "_price\",\"value\":\"" + "" + "\"}";
  data += ",{\"name\":\"lable_" + index + "_qr\",\"value\":\"" + "43081260-c292-47ab-9557-d9d6fd8c1fb7.cnt" + "\"}";
  data += ",{\"name\":\"lable_" + index + "_background\",\"value\":\"" + "43081260-c292-47ab-9557-d9d6fd8c1fb7.cnt" + "\"}";
  return data;
}

String show_Price_Lable(String index){   // hide all lable by index
  String data = "{\"name\":\"lable_" + index + "_name\",\"value\":\"" + "Add product" + "\"}";
  data += ",{\"name\":\"lable_" + index + "_price\",\"value\":\"" + "$ 0" + "\"}";
  data += ",{\"name\":\"lable_" + index + "_background\",\"value\":\"" + "28b545ed-bfe0-41bf-9185-2d544b845d6f.cnt" + "\"}";
  return data;
}

String display_Price_qr(String index, bool isHide)
{ // only when lable is visible
  String data;
  String key;
  if (isHide == true)
  {
    data = ",{\"name\":\"lable_" + index + "_qr\",\"value\":\"" + "28b545ed-bfe0-41bf-9185-2d544b845d6f.cnt" + "\"}";
  }
  else
  {
    switch (index.toInt())
    {
    case 1:
      key = shelfOneQR;
      break;
    case 2:
      key = shelfTwoQR;
      break;
    case 3:
      key = shelfThreQR;
      break;
    default:
      key = "43081260-c292-47ab-9557-d9d6fd8c1fb7.cnt";
      Serial.println("no shelf lable key parsed! use gray blank key");
      break;
    }
    data = ",{\"name\":\"lable_" + index + "_qr\",\"value\":\"" + key + "\"}";
  }
  return data;
}

int newShelfConnected;

void loop() {
    incomingData=sensor.read();
    filtered = testFilter.filtered(incomingData);
    memcpy(dataArray, &dataArray[1], sizeof(dataArray) - sizeof(int));
    dataArray[19] = filtered/11;

    // get MAX and MIN in buffer
      int maxValue = dataArray[0];
      int minValue = dataArray[0];
      int average = 0; 
      for (int i = 0; i < (sizeof(dataArray) / sizeof(dataArray[0])); i++) {
          maxValue = max(dataArray[i],maxValue);
          minValue = min(dataArray[i],minValue);
      }
      for (int i = 0; i < 20; i++) {
          average = average + dataArray[i];
      }
      average = average/20;  //get AVERAGE of buffer  
    //get stable value
    if ((maxValue < (average+20)) & (minValue > (average-20))){
        deltaVal = stableValue-average;

          //serial print:
          if (deltaVal!=0 | stableValue != average){
            Serial.print("DELTA:>");Serial.print(deltaVal);Serial.print("<  ");    
            Serial.print("stable:>");Serial.print(stableValue);Serial.println("<");
          }
          
        stableValue = average;
          if(deltaVal<-5) {
            Serial.println(getProduct(abs(deltaVal)));}
          else if(deltaVal > 5) {updateContent(false, "{\"eventName\":\"DYNAMIC_EVENT\",\"eventData\":[" + bootle_Blank + "]}");}  // if put pruduct on shelf then clear info on screen
    }

      // product detection
      if (deltaVal<0){
        switch (getProduct(abs(deltaVal))){
        case 1:
            updateContent(false, "{\"eventName\":\"DYNAMIC_EVENT\",\"eventData\":[" + bootle_RedLabel_promo + "]}");
          break;
        case 2:
            updateContent(false, "{\"eventName\":\"DYNAMIC_EVENT\",\"eventData\":[" + bootle_Cola + "]}");
          break;
        case 3:
            updateContent(false, "{\"eventName\":\"DYNAMIC_EVENT\",\"eventData\":[" + bootle_Grimbergen + "]}");
          break;
        case 4:
            updateContent(false, "{\"eventName\":\"DYNAMIC_EVENT\",\"eventData\":[" + bootle_LvivIpa + "]}");
          break;
        default:
          break;
        }
      }
      
      if (newShelfConnected!=digitalRead(connectPin)){Serial.println("Change new shelf state");}      

      newShelfConnected = digitalRead(connectPin);
      //new shelf detection
      if ((newShelfConnected==0) & addShelfFlag==false){                               // another shelf was added
        addShelfFlag = true;
        //updateContent(false, "{\"eventName\":\"DYNAMIC_EVENT\",\"eventData\":[" + shelf_TwoPlusOne + "]}");
         updateContent(false, "{\"eventName\":\"DYNAMIC_EVENT\",\"eventData\":[" + show_Price_Lable("3") + display_Price_qr("3", false) + "]}");

      } else if ((newShelfConnected==1) & addShelfFlag==true){                          // shelf was removed
        //updateContent(false, "{\"eventName\":\"DYNAMIC_EVENT\",\"eventData\":[" + shelf_TwoProducts +  "," + bootle_Blank + "]}");
         updateContent(false, "{\"eventName\":\"DYNAMIC_EVENT\",\"eventData\":[" + hide_Price_Lable("3") + "]}");
        addShelfFlag = false;
      }

      // no items on shelf
     if (stableValue > emptyShelfValue & noProducts == false){
       delay(200);
       Serial.println("+++++++++++++++++no items on shelf++++++++++++++++");
       Serial.println("{\"eventName\":\"DYNAMIC_EVENT\",\"eventData\":[" + bootle_Blank + "," + shelf_addProduct("1", "Add product", "$ 0.0") + display_Price_qr("1",  false) + "," +shelf_addProduct("2", "Add product", "$ 0.0") + display_Price_qr("2",  false) + "]}");
        updateContent(false, "{\"eventName\":\"DYNAMIC_EVENT\",\"eventData\":[" + bootle_Blank + "," + shelf_addProduct("1", "Add product", "$ 0.0") + display_Price_qr("1",  false) + "," +shelf_addProduct("2", "Add product", "$ 0.0") + display_Price_qr("2",  false) + "]}");

        noProducts = true;
     } else if (stableValue < emptyShelfValue & noProducts == true){
        updateContent(false, "{\"eventName\":\"DYNAMIC_EVENT\",\"eventData\":[{\"name\":\"count\",\"value\":\"0\"},{\"name\":\"shelf_change\",\"value\":\"51a7729a-af1c-434a-8e7d-07ff1f5b16cc.cnt\"},{\"name\":\"bootle_info\",\"value\":\"291c120a-139b-4276-97f0-ceeaa1d07946.cnt\"}]}");
        noProducts = false;
     }
     

//shelfMessage

    if(updateShelfByIndex != 0){
       delay(200);
       // create data set of product for shelf lable (name, price, demo picture)
       String prodName, prodPrice, prodDemo;
        if(shelfMessage == "product1"){prodName = "Grimbergen beer"; prodPrice = "$ 1.99"; prodDemo = bootle_Grimbergen;}
        else if(shelfMessage == "product2"){prodName = "Coca Cola 1L"; prodPrice = "$ 0.59"; prodDemo = bootle_Cola;}
        else if(shelfMessage == "product3"){prodName = "Red Lable"; prodPrice = "$ 15"; prodDemo = bootle_RedLabel;}

      // update shelf lable
       switch (updateShelfByIndex){
       case 1:
          updateContent(false, "{\"eventName\":\"DYNAMIC_EVENT\",\"eventData\":[" + shelf_addProduct("1", prodName, prodPrice) + "," + prodDemo +  display_Price_qr("1", true) + "]}");
         break;
        case 2:
          updateContent(false, "{\"eventName\":\"DYNAMIC_EVENT\",\"eventData\":[" + shelf_addProduct("2", prodName, prodPrice) + "," + prodDemo + display_Price_qr("2", true) + "]}");
         break; 
        case 3:
          updateContent(false, "{\"eventName\":\"DYNAMIC_EVENT\",\"eventData\":[" + shelf_addProduct("3", prodName, prodPrice) + "," + prodDemo + display_Price_qr("3", true) + "]}");
         break;
       default:
         break;
       }
      updateShelfByIndex = 0;


      // if(shelfMessage == "product1"){updateContent(false, "{\"eventName\":\"DYNAMIC_EVENT\",\"eventData\":[{\"name\":\"shelf_change\",\"value\":\"04d4ec16-626e-4edb-9d9b-d52be96b3003.cnt\"}," + bootle_Grimbergen + "]}");}
      //   else if(shelfMessage == "product2"){updateContent(false, "{\"eventName\":\"DYNAMIC_EVENT\",\"eventData\":[{\"name\":\"shelf_change\",\"value\":\"04d4ec16-626e-4edb-9d9b-d52be96b3003.cnt\"},{\"name\":\"bootle_info\",\"value\":\"b0de2396-2e53-40f4-a56d-ed064ea6d9a0.cnt\"}]}");}
      //   else if(shelfMessage == "product3"){updateContent(false, "{\"eventName\":\"DYNAMIC_EVENT\",\"eventData\":[{\"name\":\"count\",\"value\":\"" + shelfMessage + "\"}]}");}
      //   // else(shelfMessage == "product2"){updateContent(false, "{\"eventName\":\"DYNAMIC_EVENT\",\"eventData\":[{\"name\":\"count\",\"value\":\"" + shelfMessage + "\"}]}");}
            
    }
      delay(50);
      digitalWrite(ledPin,!digitalRead(ledPin));
  }

//{"eventName":"DYNAMIC_EVENT", "eventData": [{"name": "count", "value": "10"},{"name": "shelf_change", "value": "51a7729a-af1c-434a-8e7d-07ff1f5b16cc.cnt"}]}

  //shelf's:
  //51a7729a-af1c-434a-8e7d-07ff1f5b16cc.cnt   -> 2 labels
  //45614522-1f4c-4f96-b821-a2188c3a8709.cnt   -> 2+1
  //04d4ec16-626e-4edb-9d9b-d52be96b3003.cnt   -> 3 labels
  //7eb60178-f8c9-4bc5-87a7-69d7278e2e04.cnt   -> no products
  //291c120a-139b-4276-97f0-ceeaa1d07946.cnt   -> blank beer info   ->> bootle_info
  //749ce450-1abf-4847-97bf-99eeb5b0302c.cnt   -> lviv ipa info
  //e2fc08a6-f9ba-452d-b2d2-b23d93d2916c.cnt   -> RED LABEL promo
  //f4f5f136-3129-4692-8ad8-15b25e36bbc9.cnt   -> Grmibergen DA
  //b0de2396-2e53-40f4-a56d-ed064ea6d9a0.cnt   -> coca cola

  // QR List for shelfs: 
  // http://192.168.77.76/update_shelf_1    ea9b157c-ce1f-4eb4-bc78-39c42ba28434.cnt
  // http://192.168.77.76/update_shelf_2    311f765d-82a0-4729-90bc-6b392ea43d5b.cnt
  // http://192.168.77.76/update_shelf_3    dbb2c128-9b16-4a4b-aa08-532dc19e0926.cnt
  // blank QR white                         28b545ed-bfe0-41bf-9185-2d544b845d6f.cnt
  // blank QR gray                          43081260-c292-47ab-9557-d9d6fd8c1fb7.cnt


