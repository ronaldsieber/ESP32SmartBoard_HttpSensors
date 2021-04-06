/****************************************************************************

  Copyright (c) 2021 Ronald Sieber

  Project:      ESP32SmartBoard / HTTP Sensors
  Description:  WebServer Firmware to interact with ESP32SmartBoard

  -------------------------------------------------------------------------

    Arduino IDE Settings:

    Board:              "ESP32 Dev Module"
    Upload Speed:       "115200"
    CPU Frequency:      "240MHz (WiFi/BT)"
    Flash Frequency:    "80Mhz"
    Flash Mode:         "QIO"
    Flash Size:         "4MB (32Mb)"
    Partition Scheme:   "No OTA (2MB APP/2MB SPIFFS)"
    PSRAM:              "Disabled"

  -------------------------------------------------------------------------

  Revision History:

  2021/01/22 -rs:   V1.00 Initial version

****************************************************************************/

#define DEBUG                                                           // Enable/Disable TRACE
// #define DEBUG_DUMP_BUFFER


#include <WiFi.h>
#include <AsyncTCP.h>                                                   // Requires Library from https://github.com/me-no-dev/AsyncTCP
#include <ESPAsyncWebServer.h>                                          // Requires Library from https://github.com/me-no-dev/ESPAsyncWebServer
#include <SPIFFS.h>
#include <DHT.h>                                                        // Requires Library "DHT sensor library" by Adafruit
#include <MHZ19.h>                                                      // Requires Library "MH-Z19" by Jonathan Dempsey
#include <SoftwareSerial.h>                                             // Requires Library "EspSoftwareSerial" by Dirk Kaar, Peter Lerup
#include <esp_wifi.h>
#include "Trace.h"





/***************************************************************************/
/*                                                                         */
/*                                                                         */
/*          G L O B A L   D E F I N I T I O N S                            */
/*                                                                         */
/*                                                                         */
/***************************************************************************/

//---------------------------------------------------------------------------
//  Application Configuration
//---------------------------------------------------------------------------

const int       APP_VERSION                         = 1;                // 1.xx
const int       APP_REVISION                        = 0;                // x.00
const char      APP_BUILD_TIMESTAMP[]               = __DATE__ " " __TIME__;

const int       CFG_ENABLE_NETWORK_SCAN             = 1;
const int       CFG_ENABLE_DI_DO                    = 1;
const int       CFG_ENABLE_DHT_SENSOR               = 1;
const int       CFG_ENABLE_MHZ_SENSOR               = 1;
const int       CFG_ENABLE_STATUS_LED               = 1;

const int       CFG_WIFI_AP_MODE                    = 1;                // 0 = Device is Station/Client, 1 = Device is AccessPoint



//---------------------------------------------------------------------------
//  WLAN/WIFI Configuration
//---------------------------------------------------------------------------

// WIFI Station/Client Configuration
const char*     WIFI_STA_SSID                       = "YourNetworkName";            // <--- ADJUST HERE!
const char*     WIFI_STA_PASSWORD                   = "YourNetworkPassword";        // <--- ADJUST HERE!
IPAddress       WIFI_STA_LOCAL_IP(192,168,42,201);                      // DHCP: IP(0,0,0,0), otherwise static IP Address if running in Station Mode

// WIFI AccessPoint Configuration
const char*     WIFI_AP_SSID                        = "ESP32SmartBoard";
const char*     WIFI_AP_PASSWORD                    = "HttpSensors";
IPAddress       WIFI_AP_LOCAL_IP(192,168,200,1);                        // IP Address for Server if running in AP Mode
const int       WIFI_AP_CHANNEL                     = 1;                // WIFI Channel, from 1 to 13, default channel = 1
const bool      WIFI_AP_HIDDE_SSID                  = false;            // true = Hide SSID
const int       WIFI_AP_MAX_CONNECTIONS             = 4;                // max. simultaneous connected stations, from 0 to 8, default = 4

// PowerSavingMode for WLAN Interface
wifi_ps_type_t  WIFI_POWER_SAVING_MODE              = WIFI_PS_NONE;     // PowerSaving is not appropriate for a Server

const uint32_t  WIFI_TRY_CONNECT_TIMEOUT            = 30000;            // Timeout for trying to connect to WLAN [ms]

const String    astrWiFiStatus_g[]                  = { "WL_IDLE_STATUS", "WL_NO_SSID_AVAIL", "WL_SCAN_COMPLETED", "WL_CONNECTED", "WL_CONNECT_FAILED", "WL_CONNECTION_LOST", "WL_DISCONNECTED" };



//---------------------------------------------------------------------------
//  Runtime Configuration
//---------------------------------------------------------------------------

// Toggle Time Periods for Status LED
const uint32_t  STATUS_LED_PERIOD_NET_SCAN          = 50;               // 10Hz   = 50ms On + 50ms Off
const uint32_t  STATUS_LED_PERIOD_NET_CONNECT       = 100;              // 5Hz    = 100ms On + 100ms Off
const uint32_t  STATUS_LED_PERIOD_MAIN_LOOP         = 2000;             // 0.25Hz = 2000ms On + 2000ms Off

// Data Sources for LED Bar Indicator
typedef enum
{
    kLedBarNone             = 0,
    kLedBarDht22Temperature = 1,
    kLedBarDht22Humidity    = 2,
    kLedBarMhz19Co2Level    = 3

} tLedBar;

// Default Board Configuration Settings
const int       DEFAULT_STATUS_LED_HEARTBEAT        = true;
const tLedBar   DEFAULT_LED_BAR_INDICATOR           = kLedBarMhz19Co2Level;
const bool      DEFAULT_PRINT_SENSOR_VALUES         = true;

// MH-Z19 Settings
const bool      DEFAULT_MHZ19_AUTO_CALIBRATION      = true;
const bool      DEFAULT_DBG_MHZ19_PRINT_COMM        = false;



//---------------------------------------------------------------------------
//  Hardware/Pin Configuration
//---------------------------------------------------------------------------

const int       PIN_KEY_BLE_CFG                     = 36;               // PIN_KEY_BLE_CFG      (GPIO36 -> Pin02)
const int       PIN_KEY0                            = 35;               // KEY0                 (GPIO35 -> Pin05)
const int       PIN_KEY1                            = 34;               // KEY1                 (GPIO34 -> Pin04)

const int       PIN_LED0                            = 13;               // LED0 (green)         (GPIO13 -> Pin13)
const int       PIN_LED1                            = 12;               // LED1 (green)         (GPIO12 -> Pin12)
const int       PIN_LED2                            = 14;               // LED2 (green)         (GPIO14 -> Pin11)
const int       PIN_LED3                            =  4;               // LED3 (yellow)        (GPIO04 -> Pin20)
const int       PIN_LED4                            =  5;               // LED4 (yellow)        (GPIO05 -> Pin23)
const int       PIN_LED5                            = 18;               // LED5 (yellow)        (GPIO18 -> Pin24)
const int       PIN_LED6                            = 19;               // LED6 (red)           (GPIO19 -> Pin25)
const int       PIN_LED7                            = 21;               // LED7 (red)           (GPIO21 -> Pin26)
const int       PIN_LED8                            = 22;               // LED8 (red)           (GPIO22 -> Pin29)
const int       PIN_STATUS_LED                      =  2;               // On-board LED (blue)  (GPIO02 -> Pin19)

const int       DHT_TYPE                            = DHT22;            // DHT11, DHT21 (AM2301), DHT22 (AM2302,AM2321)
const int       DHT_PIN_SENSOR                      = 23;               // PIN used for DHT22 (AM2302/AM2321)
const uint32_t  DHT_SENSOR_SAMPLE_PERIOD            = 5000;             // Sample Period for DHT-Sensor in [ms]

const int       MHZ19_PIN_SERIAL_RX                 = 32;               // ESP32 Rx pin which the MH-Z19 Tx pin is attached to
const int       MHZ19_PIN_SERIAL_TX                 = 33;               // ESP32 Tx pin which the MH-Z19 Rx pin is attached to
const int       MHZ19_BAUDRATE_SERIAL               = 9600;             // Serial baudrate for communication with MH-Z19 Device
const uint32_t  MHZ19_SENSOR_SAMPLE_PERIOD          = 5000;             // Sample Period for MH-Z19-Sensor in [ms]



//---------------------------------------------------------------------------
//  Local Variables
//---------------------------------------------------------------------------

static  AsyncWebServer* pWebServer_g                = NULL;
static  uint16_t        ui16WifiOwnPortNum_g        = 80;

static  String          strChipID_g;
static  unsigned int    uiMainLoopProcStep_g        = 0;

static  DHT             DhtSensor_g(DHT_PIN_SENSOR, DHT_TYPE);
static  uint32_t        ui32LastTickDhtRead_g       = 0;
static  float           flDhtTemperature_g          = 0;
static  float           flDhtHumidity_g             = 0;

static  MHZ19           Mhz19Sensor_g;
static  SoftwareSerial  Mhz19SoftSerial_g(MHZ19_PIN_SERIAL_RX, MHZ19_PIN_SERIAL_TX);
static  uint32_t        ui32LastTickMhz19Read_g     = 0;
static  int             iMhz19Co2Value_g            = 0;
static  int             iMhz19Co2SensTemp_g         = 0;
static  bool            fMhz19AutoCalibration_g     = DEFAULT_MHZ19_AUTO_CALIBRATION;
static  bool            fMhz19DbgPrintComm_g        = DEFAULT_DBG_MHZ19_PRINT_COMM;

static  bool            fStatusLedHeartbeat_g       = DEFAULT_STATUS_LED_HEARTBEAT;
static  hw_timer_t*     pfnOnTimerISR_g             = NULL;
static  volatile byte   bStatusLedState_g           = LOW;
static  uint32_t        ui32LastTickStatLedToggle_g = 0;

static  tLedBar         LedBarIndicator_g           = DEFAULT_LED_BAR_INDICATOR;
static  bool            fPrintSensorValues_g        = DEFAULT_PRINT_SENSOR_VALUES;



//---------------------------------------------------------------------------
//  Static HTML Page (Redirect to 'index.html')
//---------------------------------------------------------------------------

// If only the IPAddress of the ESP32SmartBoard is entered in the browser, this
// leads to the call of the AsyncWebServer callback function 'onNotFound()' with
// the path '/'. In 'onNotFound()' the path '/' is resolved with the delivery of
// the field 'root_html []' (this corresponds with the delivery of an HTML page
// 'root.html'). The HTML code in 'root_html[]' leads to a redirect to the page
// 'index.html'. The regular call of 'index.html' then also resolves embedded
// symbols there (see 'pWebServer_g-> on ("/ index.html", HTTP_GET)').
//
//  <ESP32SmartBoard_IP> ---> '/' ---> 'root_html' ---(Redirect)---> 'index.html'
//                                                                         ^
//                                                                         |
//                                                                  incl. running
//                                                                 SymbolProcessor
const char root_html[] PROGMEM = R"rawliteral(
    <!DOCTYPE HTML>
    <html>
        <head>
            <title>Page Redirection</title>
            <meta http-equiv="refresh" content="0; url='/index.html'" />
            <script type="text/javascript">
                window.location.href = "/index.html"
            </script>
        </head>
        <body>
            <!-- <a href="/index.html">Start</a> -->
        </body>
    </html>
)rawliteral";





//=========================================================================//
//                                                                         //
//          S K E T C H   P U B L I C   F U N C T I O N S                  //
//                                                                         //
//=========================================================================//

//---------------------------------------------------------------------------
//  Application Setup
//---------------------------------------------------------------------------

void setup()
{

char      szTextBuff[64];
size_t    iFsTotalBytes;
size_t    iFsUsedBytes;
uint16_t  ui16PortNum;
char      acMhz19Version[4];
int       iMhz19Param;
bool      fMhz19AutoCalibration;
bool      fStateKey0;
bool      fStateKey1;
bool      fSensorCalibrated;
bool      fResult;


    // Serial console
    Serial.begin(115200);
    Serial.println();
    Serial.println();
    Serial.println("======== APPLICATION START ========");
    Serial.println();
    Serial.flush();


    // Application Version Information
    snprintf(szTextBuff, sizeof(szTextBuff), "App Version:      %u.%02u", APP_VERSION, APP_REVISION);
    Serial.println(szTextBuff);
    snprintf(szTextBuff, sizeof(szTextBuff), "Build Timestamp:  %s", APP_BUILD_TIMESTAMP);
    Serial.println(szTextBuff);
    Serial.println();
    Serial.flush();


    // Device Identification
    strChipID_g = GetChipID();
    Serial.print("Unique ChipID:    ");
    Serial.println(strChipID_g);
    Serial.println();
    Serial.flush();


    // Initialize Workspace
    flDhtTemperature_g  = 0;
    flDhtHumidity_g     = 0;
    iMhz19Co2Value_g    = 0;
    iMhz19Co2SensTemp_g = 0;


    // Setup DI/DO (Key/LED)
    if ( CFG_ENABLE_DI_DO )
    {
        Serial.println("Setup DI/DO...");
        pinMode(PIN_KEY0, INPUT);
        pinMode(PIN_KEY1, INPUT);
        pinMode(PIN_LED0, OUTPUT);
        pinMode(PIN_LED1, OUTPUT);
        pinMode(PIN_LED2, OUTPUT);
        pinMode(PIN_LED3, OUTPUT);
        pinMode(PIN_LED4, OUTPUT);
        pinMode(PIN_LED5, OUTPUT);
        pinMode(PIN_LED6, OUTPUT);
        pinMode(PIN_LED7, OUTPUT);
        pinMode(PIN_LED8, OUTPUT);
    }


    // Setup DHT22 Sensor (Temerature/Humidity)
    if ( CFG_ENABLE_DHT_SENSOR )
    {
        Serial.println("Setup DHT22 Sensor...");
        DhtSensor_g.begin();
        snprintf(szTextBuff, sizeof(szTextBuff), "  Sample Period:    %d [sec]", (DHT_SENSOR_SAMPLE_PERIOD / 1000));
        Serial.println(szTextBuff);
        AppProcessDhtSensor(0, fPrintSensorValues_g);                   // get initial values
    }


    // Setup MH-Z19 Sensor (CO2)
    if ( CFG_ENABLE_MHZ_SENSOR )
    {
        Serial.println("Setup MH-Z19 Sensor...");
        Mhz19SoftSerial_g.begin(MHZ19_BAUDRATE_SERIAL);                 // Initialize Software Serial Device to communicate with MH-Z19 sensor
        Mhz19Sensor_g.begin(Mhz19SoftSerial_g);                         // Initialize MH-Z19 Sensor (using Software Serial Device)
        if ( fMhz19DbgPrintComm_g )
        {
            Mhz19Sensor_g.printCommunication(false, true);              // isDec=false, isPrintComm=true
        }
        fStateKey0 = !digitalRead(PIN_KEY0);                            // Keys are inverted (1=off, 0=on)
        if ( fStateKey0 )                                               // KEY0 pressed?
        {
            fSensorCalibrated = AppMhz19CalibrateManually();
            if ( fSensorCalibrated )
            {
                fStateKey0 = false;                                     // if calibration was done, don't invert AutoCalibration mode
            }
        }
        fStateKey1 = !digitalRead(PIN_KEY1);                            // Keys are inverted (1=off, 0=on)
        if ( fStateKey1 )                                               // KEY1 pressed?
        {
            AppMhz19CalibrateUnattended();
            ESP.restart();
        }
        fMhz19AutoCalibration = fMhz19AutoCalibration_g ^ fStateKey0;
        Mhz19Sensor_g.autoCalibration(fMhz19AutoCalibration);           // Set AutoCalibration Mode (true=ON / false=OFF)
        Mhz19Sensor_g.getVersion(acMhz19Version);
        snprintf(szTextBuff, sizeof(szTextBuff), "  Firmware Version: %c%c.%c%c", acMhz19Version[0], acMhz19Version[1], acMhz19Version[2], acMhz19Version[3]);
        Serial.println(szTextBuff);
        iMhz19Param = (int)Mhz19Sensor_g.getABC();
        snprintf(szTextBuff, sizeof(szTextBuff), "  AutoCalibration:  %s", (iMhz19Param) ? "ON" : "OFF");
        Serial.println(szTextBuff);
        iMhz19Param = Mhz19Sensor_g.getRange();
        snprintf(szTextBuff, sizeof(szTextBuff), "  Range:            %d", iMhz19Param);
        Serial.println(szTextBuff);
        iMhz19Param = Mhz19Sensor_g.getBackgroundCO2();
        snprintf(szTextBuff, sizeof(szTextBuff), "  Background CO2:   %d", iMhz19Param);
        Serial.println(szTextBuff);
        snprintf(szTextBuff, sizeof(szTextBuff), "  Sample Period:    %d [sec]", (MHZ19_SENSOR_SAMPLE_PERIOD / 1000));
        Serial.println(szTextBuff);
        AppProcessMhz19Sensor(0, fPrintSensorValues_g);                 // get initial values
    }


    // Status LED Setup
    if ( CFG_ENABLE_STATUS_LED )
    {
        pinMode(PIN_STATUS_LED, OUTPUT);
        ui32LastTickStatLedToggle_g = 0;
        bStatusLedState_g = LOW;
    }


    // Default Board Configuration Settings
    fStatusLedHeartbeat_g = DEFAULT_STATUS_LED_HEARTBEAT;
    LedBarIndicator_g     = DEFAULT_LED_BAR_INDICATOR;
    fPrintSensorValues_g  = DEFAULT_PRINT_SENSOR_VALUES;


    // Mount SPI Flash File System (SPIFFS)
    Serial.println("Mounting SPI Flash File System (SPIFFS):");
    fResult = SPIFFS.begin(true);
    if ( fResult )
    {
        Serial.println("  -> successfully mounted.");

        iFsTotalBytes = SPIFFS.totalBytes();
        snprintf(szTextBuff, sizeof(szTextBuff), "     FsTotalBytes: %7d", iFsTotalBytes);
        Serial.println(szTextBuff);

        iFsUsedBytes  = SPIFFS.usedBytes();
        snprintf(szTextBuff, sizeof(szTextBuff), "     FsUsedBytes:  %7d", iFsUsedBytes);
        Serial.println(szTextBuff);
    }
    else
    {
        Serial.println();
        Serial.println("ERROR: Mounting SPIFFS failed!");
        return;
    }


    // Network Scan
    if ( CFG_ENABLE_NETWORK_SCAN )
    {
        Serial.println();
        WifiScanNetworks();
    }


    // Network Setup
    if ( CFG_WIFI_AP_MODE )
    {
        WifiConnectAccessPointMode(WIFI_AP_SSID, WIFI_AP_PASSWORD, WIFI_AP_LOCAL_IP, WIFI_AP_CHANNEL, WIFI_AP_HIDDE_SSID, WIFI_AP_MAX_CONNECTIONS, WIFI_POWER_SAVING_MODE);
    }
    else
    {
        WifiConnectStationMode(WIFI_STA_SSID, WIFI_STA_PASSWORD, WIFI_STA_LOCAL_IP, WIFI_POWER_SAVING_MODE);
    }


    // Create WEB Server Runtime Object
    Serial.println("Creating WEB Server:");
    ui16PortNum = ui16WifiOwnPortNum_g;
    if (ui16PortNum == 0)
    {
        ui16PortNum = 80;       // use default HTTP Server PortNum
    }
    Serial.print("  PortNum:         ");   Serial.println(ui16PortNum);
    pWebServer_g = new AsyncWebServer(ui16PortNum);
    if (pWebServer_g != NULL)
    {
        Serial.println("  -> WEB Server successfully created.");
    }
    else
    {
        Serial.println("ERROR: Creating WEB Server failed!");
        return;
    }


    //-----------------------------------------------------------------------
    // Register WEB Server File Access Handler
    //-----------------------------------------------------------------------

    //---------------------------------------------------------------
    // WebServer Route: root '/index.html' Web Page
    //---------------------------------------------------------------
    {
        pWebServer_g->on("/index.html", HTTP_GET, [](AsyncWebServerRequest* pServerRequest_p)
        {
            TRACE0("+ pWebServer_g->on('/index.html', HTTP_GET)\n");
            pServerRequest_p->send(SPIFFS, "/index.html", String(), false, [](const String& strSymbol_p)
            {
                char  szVersionBuff[64];
                if (strSymbol_p == "VERSION_INFO")
                {
                    snprintf(szVersionBuff, sizeof(szVersionBuff), "%u.%02u (Build: %s)", APP_VERSION, APP_REVISION, APP_BUILD_TIMESTAMP);
                }
                return ( String(szVersionBuff) );
            });
            TRACE0("- pWebServer_g->on('/index.html', HTTP_GET)\n");
        });
    }


    //---------------------------------------------------------------
    // WebServer Route: '/board_settings.html' Web Page
    //---------------------------------------------------------------
    {
        pWebServer_g->on("/board_settings.html", HTTP_GET, [](AsyncWebServerRequest* pServerRequest_p)
        {
            TRACE0("+ pWebServer_g->on('/board_settings.html', HTTP_GET)\n");
            pServerRequest_p->send(SPIFFS, "/board_settings.html", String(), false, AppCbHdlrHtmlResolveSymbolBoradSettings);
            TRACE0("- pWebServer_g->on('/board_settings.html', HTTP_GET)\n");
        });

        // Handle a POST request to <ESP_IP>/settings?option=<OptionType>&value=<SetValue>
        pWebServer_g->on("/settings", HTTP_POST, AppCbHdlrHtmlOnTellBoardSettings);
    }



    //---------------------------------------------------------------
    // WebServer Route: '/dht22_as_*.html' Web Page
    //---------------------------------------------------------------
    {
        pWebServer_g->on("/dht22_as_text.html", HTTP_GET, [](AsyncWebServerRequest* pServerRequest_p)
        {
            TRACE0("+ pWebServer_g->on('/dht22_as_text.html', HTTP_GET)\n");
            pServerRequest_p->send(SPIFFS, "/dht22_as_text.html", String(), false, AppCbHdlrHtmlResolveSymbolDht22Value);
            TRACE0("- pWebServer_g->on('/dht22_as_text.html', HTTP_GET)\n");
        });

        pWebServer_g->on("/dht22_as_gauge.html", HTTP_GET, [](AsyncWebServerRequest* pServerRequest_p)
        {
            TRACE0("+ pWebServer_g->on('/dht22_as_gauge.html', HTTP_GET)\n");
            pServerRequest_p->send(SPIFFS, "/dht22_as_gauge.html", String(), false, AppCbHdlrHtmlResolveSymbolDht22Value);
            TRACE0("- pWebServer_g->on('/dht22_as_gauge.html', HTTP_GET)");
        });

        // Handle a GET request to <ESP_IP>/get_dht22?<ValueType>
        pWebServer_g->on("/get_dht22", HTTP_GET, AppCbHdlrHtmlOnRequestDht22);
    }


    //---------------------------------------------------------------
    // WebServer Route: '/mhz19_as_*.html' Web Page
    //---------------------------------------------------------------
    {
        pWebServer_g->on("/mhz19_as_text.html", HTTP_GET, [](AsyncWebServerRequest* pServerRequest_p)
        {
            TRACE0("+ pWebServer_g->on('/mhz19_as_text.html', HTTP_GET)\n");
            pServerRequest_p->send(SPIFFS, "/mhz19_as_text.html", String(), false, AppCbHdlrHtmlResolveSymbolMhz19Value);
            TRACE0("- pWebServer_g->on('/mhz19_as_text.html', HTTP_GET)\n");
        });

        pWebServer_g->on("/mhz19_as_gauge.html", HTTP_GET, [](AsyncWebServerRequest* pServerRequest_p)
        {
            TRACE0("+ pWebServer_g->on('/mhz19_as_gauge.html', HTTP_GET)\n");
            pServerRequest_p->send(SPIFFS, "/mhz19_as_gauge.html", String(), false, AppCbHdlrHtmlResolveSymbolMhz19Value);
            TRACE0("- pWebServer_g->on('/mhz19_as_gauge.html', HTTP_GET)\n");
        });

        // Handle a GET request to <ESP_IP>/get_mhz19?<ValueType>
        pWebServer_g->on("/get_mhz19", HTTP_GET, AppCbHdlrHtmlOnRequestMhz19);
    }


    //---------------------------------------------------------------
    // WebServer Route: '/board_input_keys.html' Web Page
    //---------------------------------------------------------------
    {
        pWebServer_g->on("/board_input_keys.html", HTTP_GET, [](AsyncWebServerRequest* pServerRequest_p)
        {
            TRACE0("+ pWebServer_g->on('/board_input_keys.html', HTTP_GET)\n");
            pServerRequest_p->send(SPIFFS, "/board_input_keys.html", String(), false, AppCbHdlrHtmlResolveSymbolInputKeys);
            TRACE0("- pWebServer_g->on('/board_input_keys.html', HTTP_GET)\n");
        });

        // Handle a GET request to <ESP_IP>/get_input?key=<KeyNum>
        pWebServer_g->on("/get_input", HTTP_GET, AppCbHdlrHtmlOnRequestGetInputKeys);
    }


    //---------------------------------------------------------------
    // WebServer Route: '/board_output_leds.html' Web Page
    //---------------------------------------------------------------
    {
        pWebServer_g->on("/board_output_leds.html", HTTP_GET, [](AsyncWebServerRequest* pServerRequest_p)
        {
            TRACE0("+ pWebServer_g->on('/board_output_leds.html', HTTP_GET)\n");
            pServerRequest_p->send(SPIFFS, "/board_output_leds.html", String(), false, AppCbHdlrHtmlResolveSymbolOutputLeds);
            TRACE0("- pWebServer_g->on('/board_output_leds.html', HTTP_GET)\n");
        });

        // Handle a POST request to <ESP_IP>/set_output?led=<LedType>&state=<State>
        pWebServer_g->on("/set_output", HTTP_POST, AppCbHdlrHtmlOnTellOutputLeds);
    }


    //---------------------------------------------------------------
    // Common Route for all not explicit handled Files ('/*.js, /*.png')
    //---------------------------------------------------------------
    {
        pWebServer_g->onNotFound([](AsyncWebServerRequest* pServerRequest_p)
        {
            String  strPath;
            bool    fFileExists;

            strPath = pServerRequest_p->url();
            TRACE1("+ pWebServer_g->onNotFound('%s')\n", strPath.c_str());
            if (strPath == String("/"))
            {
                // Resolve <ESP32SmartBoard_IP> to 'index.html' (incl. running SymbolProcessor)
                // <ESP32SmartBoard_IP> ---> '/' ---> 'root_html' ---(Redirect)---> 'index.html'
                TRACE0("Redirect '/' to 'root_html'\n");
                pServerRequest_p->send_P(200, "text/html", root_html);
            }
            else
            {
                TRACE1("  Check File: '%s' -> ", strPath.c_str());
                fFileExists = SPIFFS.exists(strPath.c_str());
                if ( fFileExists )
                {
                    TRACE0("exists\n");
                    pServerRequest_p->send(SPIFFS, strPath.c_str());
                }
                else
                {
                    TRACE0("NOT FOUND\n");
                    pServerRequest_p->send(404, "text/plain", "ERROR 404 - File not found.");
                }
            }
            TRACE1("- pWebServer_g->onNotFound('%s')\n", strPath.c_str());
        });
    }
    //---------------------------------------------------------------


    //-----------------------------------------------------------------------
    // Start WEB Server
    //-----------------------------------------------------------------------
    pWebServer_g->begin();

    return;

}



//---------------------------------------------------------------------------
//  Application Main Loop
//---------------------------------------------------------------------------

void loop()
{

unsigned int  uiProcStep;
uint32_t      ui32CurrTick;


    // ensure that WLAN/WIFI is available
    WifiEnsureNetworkAvailability();

    // process local periphery (sensors)
    uiProcStep = uiMainLoopProcStep_g++ % 10;
    switch (uiProcStep)
    {
        case 0:
        {
            if ( CFG_ENABLE_DHT_SENSOR )
            {
                // process DHT Sensor (Temperature/Humidity)
                AppProcessDhtSensor(DHT_SENSOR_SAMPLE_PERIOD, fPrintSensorValues_g);
            }
            break;
        }

        case 1:
        {
            if ( CFG_ENABLE_MHZ_SENSOR )
            {
                // process MH-Z19 Sensor (CO2Value/SensorTemperature)
                AppProcessMhz19Sensor(MHZ19_SENSOR_SAMPLE_PERIOD, fPrintSensorValues_g);
            }
            break;
        }

        case 2:
        case 3:
        case 4:
        case 5:
        case 6:
        case 7:
        case 8:
        {
            break;
        }

        case 9:
        {
            // process LED Bar Indicator depending on its data source
            AppProcessLedBarIndicator();
            break;
        }

        default:
        {
            break;
        }
    }

    // toggle Status LED
    if ( CFG_ENABLE_STATUS_LED )
    {
        if ( fStatusLedHeartbeat_g )
        {
            ui32CurrTick = millis();
            if ((ui32CurrTick - ui32LastTickStatLedToggle_g) >= STATUS_LED_PERIOD_MAIN_LOOP)
            {
                bStatusLedState_g = !bStatusLedState_g;
                digitalWrite(PIN_STATUS_LED, bStatusLedState_g);

                ui32LastTickStatLedToggle_g = ui32CurrTick;
            }
        }
    }

    delay(50);

    return;

}





//=========================================================================//
//                                                                         //
//          W I F I   N E T W O R K   F U N C T I O N S                    //
//                                                                         //
//=========================================================================//

//---------------------------------------------------------------------------
//  Scan WLAN/WiFi Networks
//---------------------------------------------------------------------------

void  WifiScanNetworks()
{

int  iNumOfWlanNets;
int  iIdx;


    if ( CFG_ENABLE_STATUS_LED )
    {
        Esp32TimerStart(STATUS_LED_PERIOD_NET_SCAN);                // 10Hz = 50ms On + 50ms Off
    }


    Serial.println("Scanning for WLAN Networks...");
    iNumOfWlanNets = WiFi.scanNetworks();
    if (iNumOfWlanNets > 0)
    {
        Serial.print("Networks found within Range: ");
        Serial.println(iNumOfWlanNets);
        Serial.println("---------------------------------+------+----------------");
        Serial.println("               SSID              | RSSI |    AUTH MODE   ");
        Serial.println("---------------------------------+------+----------------");
        for (iIdx=0; iIdx<iNumOfWlanNets; iIdx++)
        {
            Serial.printf("%32.32s | ", WiFi.SSID(iIdx).c_str());
            Serial.printf("%4d | ", WiFi.RSSI(iIdx));
            Serial.printf("%15s\n", WifiAuthModeToText(WiFi.encryptionType(iIdx)).c_str());
        }
    }
    else
    {
        Serial.println("No Networks found.");
    }

    Serial.println("");

    if ( CFG_ENABLE_STATUS_LED )
    {
        Esp32TimerStop();
    }

    return;

}



//---------------------------------------------------------------------------
//  Setup WLAN/WiFi SubSystem in AccessPoint Mode
//---------------------------------------------------------------------------

void  WifiConnectAccessPointMode (const char* pszWifiSSID_p, const char* pszWifiPassword_p, IPAddress LocalIP_p, int iWiFiChannel_p, bool fHideSSID_p, int iMaxConnections_p, wifi_ps_type_t WifiPowerSavingMode_p)
{

IPAddress  WIFI_AP_GATEWAY(0,0,0,0);                                // no Gateway functionality supported
IPAddress  WIFI_AP_SUBNET(255,255,255,0);
uint32_t   ui32ConnectStartTicks;
bool       fResult;


    if ( CFG_ENABLE_STATUS_LED )
    {
        Esp32TimerStart(STATUS_LED_PERIOD_NET_CONNECT);             // 5Hz = 100ms On + 100ms Off
    }

    Serial.println("Setup WiFi Interface as ACCESSPOINT");

    // Set PowerSavingMode for WLAN Interface
    Serial.print("Set PowerSavingMode for WLAN Interface: ");
    switch (WifiPowerSavingMode_p)
    {
        case WIFI_PS_NONE:          Serial.println("OFF (WIFI_PS_NONE)");                   break;
        case WIFI_PS_MIN_MODEM:     Serial.println("ON / using DTIM (WIFI_PS_MIN_MODEM)");  break;
        case WIFI_PS_MAX_MODEM:     Serial.println("ON (WIFI_PS_MAX_MODEM)");               break;
        default:                    Serial.println("???unknown???");                        break;
    }
    esp_wifi_set_ps(WifiPowerSavingMode_p);

    // Set WiFi Mode to SoftAP
    Serial.println("Set WiFi Mode to SoftAP:");
    WiFi.disconnect();
    WiFi.mode(WIFI_OFF);
    WiFi.mode(WIFI_AP);
    Serial.println("  -> done.");

    // Create WiFi SoftAP
    Serial.println("Create WiFi SoftAP:");
    Serial.print("  SSID:            ");   Serial.println(pszWifiSSID_p);
    Serial.print("  Passwd:          ");   Serial.println(pszWifiPassword_p);
    Serial.print("  Channel:         ");   Serial.println(iWiFiChannel_p);
    Serial.print("  Hide SSID:       ");   Serial.println(fHideSSID_p);
    Serial.print("  Max Connections: ");   Serial.println(iMaxConnections_p);
    fResult = WiFi.softAP(pszWifiSSID_p, pszWifiPassword_p, iWiFiChannel_p, fHideSSID_p, iMaxConnections_p);
    if ( fResult )
    {
        Serial.println("  -> WiFi SoftAP successfully created.");
    }
    else
    {
        Serial.println("ERROR: Creating WiFi SoftAP failed!");
        return;
    }

    // Wait for System Event 'AP_START'
    Serial.print("Wait for System Event 'AP_START': ");
    delay(200);                                                     // wait for SYSTEM_EVENT_AP_START
    Serial.println("done.");

    // Set WiFi SoftAP Configuration
    Serial.println("Set WiFi SoftAP Configuration:");
    Serial.print("  Local IP:        ");   Serial.println(LocalIP_p);
    Serial.print("  Gateway:         ");   Serial.println(WIFI_AP_GATEWAY);
    Serial.print("  Subnet:          ");   Serial.println(WIFI_AP_SUBNET);
    fResult = WiFi.softAPConfig(LocalIP_p, WIFI_AP_GATEWAY, WIFI_AP_SUBNET);
    if ( fResult )
    {
        Serial.println("  -> WiFi SoftAP successfully configured.");
    }
    else
    {
        Serial.println("ERROR: Configuring WiFi SoftAP failed!");
        return;
    }

    Serial.print("IP Address: ");
    Serial.println(WiFi.softAPIP());
    Serial.print("   (MAC: ");
    Serial.print(GetChipMAC());
    Serial.println(")");

    if ( CFG_ENABLE_STATUS_LED )
    {
        Esp32TimerStop();
    }

    return;

}



//---------------------------------------------------------------------------
//  Setup WLAN/WiFi SubSystem in Station Mode
//---------------------------------------------------------------------------

void  WifiConnectStationMode (const char* pszWifiSSID_p, const char* pszWifiPassword_p, IPAddress LocalIP_p, wifi_ps_type_t WifiPowerSavingMode_p)
{

IPAddress  WIFI_STA_GATEWAY(0,0,0,0);                               // no Gateway functionality supported
IPAddress  WIFI_STA_SUBNET(255,255,255,0);
uint32_t   ui32ConnectStartTicks;
bool       fResult;


    if ( CFG_ENABLE_STATUS_LED )
    {
        Esp32TimerStart(STATUS_LED_PERIOD_NET_CONNECT);             // 5Hz = 100ms On + 100ms Off
    }

    Serial.println("Setup WiFi Interface as STATION (Client)");

    // Set PowerSavingMode for WLAN Interface
    Serial.print("Set PowerSavingMode for WLAN Interface: ");
    switch (WifiPowerSavingMode_p)
    {
        case WIFI_PS_NONE:          Serial.println("OFF (WIFI_PS_NONE)");                   break;
        case WIFI_PS_MIN_MODEM:     Serial.println("ON / using DTIM (WIFI_PS_MIN_MODEM)");  break;
        case WIFI_PS_MAX_MODEM:     Serial.println("ON (WIFI_PS_MAX_MODEM)");               break;
        default:                    Serial.println("???unknown???");                        break;
    }
    esp_wifi_set_ps(WifiPowerSavingMode_p);

    // Set WiFi Mode to Station
    Serial.println("Set WiFi Mode to Station:");
    WiFi.disconnect();
    WiFi.mode(WIFI_OFF);
    WiFi.mode(WIFI_STA);
    Serial.println("  -> done.");

    // Set IPAddress configuration
    // 0.0.0.0 = DHCP, otherwise set static IPAddress
    Serial.print("IP Address configuration: ");
    if (LocalIP_p != IPAddress(0,0,0,0))
    {
        Serial.print("static ");
        Serial.println(LocalIP_p.toString());
        WiFi.config(LocalIP_p, WIFI_STA_GATEWAY, WIFI_STA_SUBNET);
        Serial.println("  -> done.");
    }
    else
    {
        Serial.println("DHCP");
    }

    // Connecting to WLAN/WiFi
    Serial.print("Connecting to WLAN '");
    Serial.print(pszWifiSSID_p);
    Serial.print("' ");
    WiFi.begin(pszWifiSSID_p, pszWifiPassword_p);

    ui32ConnectStartTicks = millis();
    while (WiFi.status() != WL_CONNECTED)
    {
        if ((millis() - ui32ConnectStartTicks) > WIFI_TRY_CONNECT_TIMEOUT)
        {
            Serial.println("");
            Serial.print("ERROR: Unable to connect to WLAN since more than ");
            Serial.print(WIFI_TRY_CONNECT_TIMEOUT/1000);
            Serial.println(" seconds.");
            Serial.println("-> REBOOT System now...");
            Serial.println("");

            ESP.restart();
        }

        delay(500);
        Serial.print(".");
    }
    Serial.println(" -> connected.");

    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
    Serial.print("   (MAC: ");
    Serial.print(GetChipMAC());
    Serial.println(")");

    if ( CFG_ENABLE_STATUS_LED )
    {
        Esp32TimerStop();
    }

    return;

}



//---------------------------------------------------------------------------
//  Get WLAN/WiFi Authentication Mode as Text
//---------------------------------------------------------------------------

String  WifiAuthModeToText (wifi_auth_mode_t WifiAuthMode_p)
{

String  strWifiAuthMode;


    switch (WifiAuthMode_p)
    {
        case WIFI_AUTH_OPEN:
        {
            strWifiAuthMode = "Open";
            break;
        }

        case WIFI_AUTH_WEP:
        {
            strWifiAuthMode = "WEP";
            break;
        }

        case WIFI_AUTH_WPA_PSK:
        {
            strWifiAuthMode = "WPA PSK";
            break;
        }

        case WIFI_AUTH_WPA2_PSK:
        {
            strWifiAuthMode = "WPA2 PSK";
            break;
        }

        case WIFI_AUTH_WPA_WPA2_PSK:
        {
            strWifiAuthMode = "WPA/WPA2 PSK";
            break;
        }

        case WIFI_AUTH_WPA2_ENTERPRISE:
        {
            strWifiAuthMode = "WPA2 ENTERPRISE";
            break;
        }

        case WIFI_AUTH_MAX:
        {
            strWifiAuthMode = "MAX";
            break;
        }

        default:
        {
            strWifiAuthMode = "Unknown";
            break;
        }
    }

    return (strWifiAuthMode);

}



//---------------------------------------------------------------------------
//  Ensure that Network (WLAN/WiFi) is available
//---------------------------------------------------------------------------

void  WifiEnsureNetworkAvailability()
{

int  iWiFiStatus;


    if ( !(CFG_WIFI_AP_MODE) )
    {
        // verify that WLAN/WiFi Connection is still active
        iWiFiStatus = WiFi.status();
        if (iWiFiStatus != WL_CONNECTED)
        {
            Serial.println();
            Serial.println("ERROR: WLAN Connection lost, try to Reconnet...");
            Serial.print("       (-> '");
            Serial.print(astrWiFiStatus_g[iWiFiStatus]);
            Serial.println("'");

            WiFi.disconnect();
            WiFi.mode(WIFI_OFF);

            WifiConnectStationMode(WIFI_STA_SSID, WIFI_STA_PASSWORD, WIFI_STA_LOCAL_IP, WIFI_POWER_SAVING_MODE);
        }
    }


    return;

}





//=========================================================================//
//                                                                         //
//          S K E T C H   P R I V A T E   F U N C T I O N S                //
//                                                                         //
//=========================================================================//

//-------------------------------------------------------------------------//
//                                                                         //
//          Handlers for Web Page '/board_settings.html'                   //
//                                                                         //
//-------------------------------------------------------------------------//

//---------------------------------------------------------------------------
//  Application Callback Handler: Resolve Settings Symbol in HTML Document
//---------------------------------------------------------------------------

String  AppCbHdlrHtmlResolveSymbolBoradSettings (const String& strSymbol_p)
{

String  strSymbol;
String  strSettingType;
String  strValue;


    TRACE1("+ CallBack Handler 'AppCbHdlrHtmlResolveSymbolBoradSettings()': strSymbol_p=%s\n", strSymbol_p.c_str());

    strValue = "";

    strSymbol = strSymbol_p;
    strSymbol.toUpperCase();
    if ( strSymbol.startsWith("SETTING_") )
    {
        strSettingType = strSymbol.substring(sizeof("SETTING_")-1);
        TRACE1("  SettingType: %s\n", strSettingType.c_str());

        if (strSettingType == String("HEARTBEAT"))
        {
            strValue = ( fStatusLedHeartbeat_g ) ? "1" : "0";
        }
        else if (strSettingType == String("LED_BAR_INDICATOR"))
        {
            strValue = AppGetLedBarIndicator();
        }
        else if (strSettingType == String("PRINT_SENSOR_VALUES"))
        {
            strValue = ( fPrintSensorValues_g ) ? "1" : "0";
        }
        else
        {
            strValue = "{unknown}";
        }
    }

    TRACE1("- CallBack Handler 'AppCbHdlrHtmlResolveSymbolBoradSettings()': strValue=%s\n", strValue.c_str());

    return (strValue);

}



//---------------------------------------------------------------------------
//  Application Callback Handler: Handle Post for ('/settings', HTTP_POST)
//---------------------------------------------------------------------------

void  AppCbHdlrHtmlOnTellBoardSettings (AsyncWebServerRequest* pServerRequest_p)
{

#define PARAM_NAME_OPTION   "option"
#define PARAM_NAME_VALUE    "value"

String  strOptionType;
String  strValue;
int     iValue;


    TRACE0("+ AppCbHdlrHtmlOnTellBoardSettings('/settings', HTTP_POST)\n");
    TRACE1("  URL:  %s\n", pServerRequest_p->url().c_str());
    TRACE1("  Host: %s\n", pServerRequest_p->host().c_str());

    if (pServerRequest_p != NULL)
    {
        // get values on <ESP_IP>/settings?option=<OptionType>&value=<SetValue>
        if (pServerRequest_p->hasParam(PARAM_NAME_OPTION) && pServerRequest_p->hasParam(PARAM_NAME_VALUE))
        {
            strOptionType = pServerRequest_p->getParam(PARAM_NAME_OPTION)->value();
            strOptionType.toUpperCase();
            TRACE1("  OptionType: %s\n", strOptionType.c_str());

            strValue = pServerRequest_p->getParam(PARAM_NAME_VALUE)->value();
            iValue = strValue.toInt();
            TRACE1("  Value: %d\n", iValue);

            if (strOptionType == String("SYSLED_HEARTBEAT"))
            {
                fStatusLedHeartbeat_g = (iValue == 0) ? false : true;
                if ( !fStatusLedHeartbeat_g )
                {
                    digitalWrite(PIN_STATUS_LED, LOW);
                }
            }
            else if (strOptionType == String("PRINT_SENSOR_VALUES"))
            {
                fPrintSensorValues_g = (iValue == 0) ? false : true;
            }
            else
            {
                AppSetLedBarIndicator(strOptionType, iValue);
                AppPresentLedBar(0);                                // clear all LEDs in LED Bar
            }
        }
        else
        {
            strOptionType = "{not sent}";
            strValue      = "{not sent}";
        }
    }

    pServerRequest_p->send(200);

    TRACE2("  Option: %s - Set to: %s\n", strOptionType.c_str(), strValue.c_str());
    TRACE0("- AppCbHdlrHtmlOnTellBoardSettings('/settings', HTTP_POST)\n");

    return;

}



//---------------------------------------------------------------------------
//  Get Configuration Setting for LED Bar Indicator Source
//---------------------------------------------------------------------------

String  AppGetLedBarIndicator()
{

String  strLedBarIndicator;


    TRACE0("+ AppGetLedBarIndicator\n");

    switch (LedBarIndicator_g)
    {
        case kLedBarNone:               strLedBarIndicator = "ledbar_none";                 break;
        case kLedBarDht22Temperature:   strLedBarIndicator = "ledbar_dht22_temperature";    break;
        case kLedBarDht22Humidity:      strLedBarIndicator = "ledbar_dht22_humidity";       break;
        case kLedBarMhz19Co2Level:      strLedBarIndicator = "ledbar_mhz19_co2_level";      break;
        default:                        strLedBarIndicator = "???unknown???";               break;
    }

    TRACE1("- AppGetLedBarIndicator: strLedBarIndicator=%s\n", strLedBarIndicator.c_str());

    return (strLedBarIndicator);

}



//---------------------------------------------------------------------------
//  Set Configuration Setting for LED Bar Indicator Source
//---------------------------------------------------------------------------

void  AppSetLedBarIndicator (String strLedBarIndicator_p, int iValue_p)
{

    TRACE2("+ AppSetLedBarIndicator: strLedBarIndicator_p=%s, iValue=%d\n", strLedBarIndicator_p.c_str(), iValue_p);

    strLedBarIndicator_p.toLowerCase();

    if (iValue_p == 0)
    {
        LedBarIndicator_g = kLedBarNone;
    }
    else if (strLedBarIndicator_p == String("ledbar_dht22_temperature"))
    {
        LedBarIndicator_g = kLedBarDht22Temperature;
    }
    else if (strLedBarIndicator_p == String("ledbar_dht22_humidity"))
    {
        LedBarIndicator_g = kLedBarDht22Humidity;
    }
    else if (strLedBarIndicator_p == String("ledbar_mhz19_co2_level"))
    {
        LedBarIndicator_g = kLedBarMhz19Co2Level;
    }

    TRACE0("- AppSetLedBarIndicator\n");

    return;

}





//-------------------------------------------------------------------------//
//                                                                         //
//          Handlers for Web Page '/dht22_as_*.html'                       //
//                                                                         //
//-------------------------------------------------------------------------//

//---------------------------------------------------------------------------
//  Application Callback Handler: Resolve DHT22 Symbol in HTML Document
//---------------------------------------------------------------------------

String  AppCbHdlrHtmlResolveSymbolDht22Value (const String& strSymbol_p)
{

String  strSymbol;
String  strDht22ValueType;
String  strValue;


    TRACE1("+ CallBack Handler 'AppCbHdlrHtmlResolveSymbolDht22Value()': strSymbol_p=%s\n", strSymbol_p.c_str());

    strValue = "";

    strSymbol = strSymbol_p;
    strSymbol.toUpperCase();
    if ( strSymbol.startsWith("DHT22_") )
    {
        strDht22ValueType = strSymbol.substring(sizeof("DHT22_")-1);
        TRACE1("  Dht22ValueType: %s\n", strDht22ValueType.c_str());

        if (strDht22ValueType == String("FEATURE_ENABLE_STATE"))
        {
            strValue = ( CFG_ENABLE_DHT_SENSOR ) ? "1" : "0";
        }
        else
        {
            strValue = AppGetDht22Value(strDht22ValueType);
        }
    }

    TRACE1("- CallBack Handler 'AppCbHdlrHtmlResolveSymbolDht22Value()': strValue=%s\n", strValue.c_str());

    return (strValue);

}



//---------------------------------------------------------------------------
//  Application Callback Handler: Handle Request for ('/get_dht22', HTTP_GET)
//---------------------------------------------------------------------------

void  AppCbHdlrHtmlOnRequestDht22 (AsyncWebServerRequest* pServerRequest_p)
{

#define PARAM_NAME_TEMPERATURE      "temperature"
#define PARAM_NAME_HUMIDITY         "humidity"

String  strDht22ValueType;
String  strValue;


    TRACE0("+ AppCbHdlrHtmlOnRequestDht22('/get_dht22', HTTP_GET)\n");
    TRACE1("  URL:  %s\n", pServerRequest_p->url().c_str());
    TRACE1("  Host: %s\n", pServerRequest_p->host().c_str());

    if (pServerRequest_p != NULL)
    {
        // get values on <ESP_IP>/get_dht22?<ValueType>
        if (pServerRequest_p->hasParam(PARAM_NAME_TEMPERATURE))
        {
            strDht22ValueType = PARAM_NAME_TEMPERATURE;
            strValue = AppGetDht22Value(strDht22ValueType);
        }
        else if (pServerRequest_p->hasParam(PARAM_NAME_HUMIDITY))
        {
            strDht22ValueType = PARAM_NAME_HUMIDITY;
            strValue = AppGetDht22Value(strDht22ValueType);
        }
        else
        {
            strDht22ValueType = "{unknown}";
            strValue = "{unknown}";
        }
    }

    pServerRequest_p->send(200, "text/plain", strValue);

    TRACE2("  ValueType: %s - Value: %s\n", strDht22ValueType.c_str(), strValue.c_str());
    TRACE0("- AppCbHdlrHtmlOnRequestDht22('/get_dht22', HTTP_GET)\n");

    return;

}



//---------------------------------------------------------------------------
//  Get current DHT22 Temperature or Humidity Value
//---------------------------------------------------------------------------

String  AppGetDht22Value (String strDht22ValueType_p)
{

char    szValue[32];
String  strDht22ValueType;
String  strValue;


    TRACE1("+ AppGetDht22Value: strDht22ValueType_p=%s\n", strDht22ValueType_p.c_str());

    strDht22ValueType = strDht22ValueType_p;
    strDht22ValueType.toUpperCase();

    if (strDht22ValueType == String("TEMPERATURE"))
    {
        snprintf(szValue, sizeof(szValue), "%.1f", flDhtTemperature_g);
        strValue = szValue;
    }
    else if (strDht22ValueType == String("HUMIDITY"))
    {
        snprintf(szValue, sizeof(szValue), "%.1f", flDhtHumidity_g);
        strValue = szValue;
    }
    else
    {
        strValue = "???";
    }

    TRACE1("- AppGetDht22Value: strState=%s\n", strValue.c_str());

    return (strValue);

}





//-------------------------------------------------------------------------//
//                                                                         //
//          Handlers for Web Page '/mhz19_as_*.html'                       //
//                                                                         //
//-------------------------------------------------------------------------//

//---------------------------------------------------------------------------
//  Application Callback Handler: Resolve MH-Z19 Symbol in HTML Document
//---------------------------------------------------------------------------

String  AppCbHdlrHtmlResolveSymbolMhz19Value (const String& strSymbol_p)
{

String  strSymbol;
String  strMhz19ValueType;
String  strValue;


    TRACE1("+ CallBack Handler 'AppCbHdlrHtmlResolveSymbolMhz19Value()': strSymbol_p=%s\n", strSymbol_p.c_str());

    strValue = "";

    strSymbol = strSymbol_p;
    strSymbol.toUpperCase();
    if ( strSymbol.startsWith("MHZ19_") )
    {
        strMhz19ValueType = strSymbol.substring(sizeof("MHZ19_")-1);
        TRACE1("  Mhz19ValueType: %s\n", strMhz19ValueType.c_str());

        if (strMhz19ValueType == String("FEATURE_ENABLE_STATE"))
        {
            strValue = ( CFG_ENABLE_MHZ_SENSOR ) ? "1" : "0";
        }
        else
        {
            strValue = AppGetMhz19Value(strMhz19ValueType);
        }
    }

    TRACE1("- CallBack Handler 'AppCbHdlrHtmlResolveSymbolMhz19Value()': strValue=%s\n", strValue.c_str());

    return (strValue);

}



//---------------------------------------------------------------------------
//  Application Callback Handler: Handle Request for ('/get_mhz19', HTTP_GET)
//---------------------------------------------------------------------------

void  AppCbHdlrHtmlOnRequestMhz19 (AsyncWebServerRequest* pServerRequest_p)
{

#define PARAM_NAME_CO2              "co2"
#define PARAM_NAME_SENSTEMP         "senstemp"

String  strMhz19ValueType;
String  strValue;


    TRACE0("+ AppCbHdlrHtmlOnRequestMhz19('/get_mhz19', HTTP_GET)\n");
    TRACE1("  URL:  %s\n", pServerRequest_p->url().c_str());
    TRACE1("  Host: %s\n", pServerRequest_p->host().c_str());

    if (pServerRequest_p != NULL)
    {
        // get values on <ESP_IP>/get_mhz19?<ValueType>
        if (pServerRequest_p->hasParam(PARAM_NAME_CO2))
        {
            strMhz19ValueType = PARAM_NAME_CO2;
            strValue = AppGetMhz19Value(strMhz19ValueType);
        }
        else if (pServerRequest_p->hasParam(PARAM_NAME_SENSTEMP))
        {
            strMhz19ValueType = PARAM_NAME_SENSTEMP;
            strValue = AppGetMhz19Value(strMhz19ValueType);
        }
        else
        {
            strMhz19ValueType = "{unknown}";
            strValue = "{unknown}";
        }
    }

    pServerRequest_p->send(200, "text/plain", strValue);

    TRACE2("  ValueType: %s - Value: %s\n", strMhz19ValueType.c_str(), strValue.c_str());
    TRACE0("- AppCbHdlrHtmlOnRequestMhz19('/get_mhz19', HTTP_GET)\n");

    return;

}



//---------------------------------------------------------------------------
//  Get current MH-Z19 CO2 Value or SensorTemp Value
//---------------------------------------------------------------------------

String  AppGetMhz19Value (String strMhz19ValueType_p)
{

char    szValue[32];
String  strMhz19ValueType;
String  strValue;


    TRACE1("+ AppGetMhz19Value: strMhz19ValueType_p=%s\n", strMhz19ValueType_p.c_str());

    strMhz19ValueType = strMhz19ValueType_p;
    strMhz19ValueType.toUpperCase();

    if (strMhz19ValueType == String("CO2"))
    {
        snprintf(szValue, sizeof(szValue), "%d", iMhz19Co2Value_g);
        strValue = szValue;
    }
    else if (strMhz19ValueType == String("SENSTEMP"))
    {
        snprintf(szValue, sizeof(szValue), "%d", iMhz19Co2SensTemp_g);
        strValue = szValue;
    }
    else
    {
        strValue = "???";
    }

    TRACE1("- AppGetMhz19Value: strState=%s\n", strValue.c_str());

    return (strValue);

}





//-------------------------------------------------------------------------//
//                                                                         //
//          Handlers for Web Page '/board_input_keys.html'                 //
//                                                                         //
//-------------------------------------------------------------------------//

//---------------------------------------------------------------------------
//  Application Callback Handler: Resolve Input Key Symbol in HTML Document
//---------------------------------------------------------------------------

String  AppCbHdlrHtmlResolveSymbolInputKeys (const String& strSymbol_p)
{

String  strSymbol;
String  strKeyNum;
int     iKeyNum;
String  strState;


    TRACE1("+ CallBack Handler 'AppCbHdlrHtmlResolveSymbolInputKeys()': strSymbol_p=%s\n", strSymbol_p.c_str());

    strState = "";

    strSymbol = strSymbol_p;
    strSymbol.toUpperCase();
    if ( strSymbol.startsWith("STATE_KEY_") )
    {
        strKeyNum = strSymbol.substring(sizeof("STATE_KEY_")-1);
        TRACE1("  KeyNum: %s\n", strKeyNum.c_str());

        if (strKeyNum == String("FEATURE_ENABLE"))
        {
            strState = ( CFG_ENABLE_DI_DO ) ? "1" : "0";
        }
        else
        {
            iKeyNum = strKeyNum.toInt();
            TRACE1("  KeyNum: %d\n", iKeyNum);

            strState = AppGetInputKeyState(iKeyNum);
        }
    }

    TRACE1("- CallBack Handler 'AppCbHdlrHtmlResolveSymbolInputKeys()': strState=%s\n", strState.c_str());

    return (strState);

}



//---------------------------------------------------------------------------
//  Application Callback Handler: Handle Request for ('/get_input', HTTP_GET)
//---------------------------------------------------------------------------

void  AppCbHdlrHtmlOnRequestGetInputKeys (AsyncWebServerRequest* pServerRequest_p)
{

#define PARAM_NAME_KEY      "key"

String  strKeyName;
String  strKeyNum;
int     iKeyNum;
String  strState;


    TRACE0("+ AppCbHdlrHtmlOnRequestGetInputKeys('/get_input', HTTP_GET)\n");
    TRACE1("  URL:  %s\n", pServerRequest_p->url().c_str());
    TRACE1("  Host: %s\n", pServerRequest_p->host().c_str());

    if (pServerRequest_p != NULL)
    {
        // get values on <ESP_IP>/get_input?key=<KeyNum>
        if (pServerRequest_p->hasParam(PARAM_NAME_KEY))
        {
            strKeyName = pServerRequest_p->getParam(PARAM_NAME_KEY)->value();
            strKeyName.toUpperCase();
            TRACE1("  KeyName: %s\n", strKeyName.c_str());
            if ( strKeyName.startsWith("KEY_") )
            {
                strKeyNum = strKeyName.substring(sizeof("KEY_")-1);
                TRACE1("  KeyNum: %s\n", strKeyNum.c_str());

                iKeyNum = strKeyNum.toInt();
                TRACE1("  KeyNum: %d\n", iKeyNum);

                strState = AppGetInputKeyState(iKeyNum);
            }
            else
            {
                strState = "{unknown}";
            }
        }
        else
        {
            strState = "{unknown}";
        }
    }

    pServerRequest_p->send(200, "text/plain", strState);

    TRACE2("  KEY: %s - State: %s\n", strKeyNum.c_str(), strState.c_str());
    TRACE0("- AppCbHdlrHtmlOnRequestGetInputKeys('/get_input', HTTP_GET)\n");

    return;

}



//---------------------------------------------------------------------------
//  Get current State of Input Keys
//---------------------------------------------------------------------------

String  AppGetInputKeyState (int iKeyNum_p)
{

int     iState;
String  strState;


    TRACE1("+ AppGetInputKeyState: iKeyNum_p=%d\n", iKeyNum_p);

    if ( CFG_ENABLE_DI_DO )
    {
        switch (iKeyNum_p)
        {
            case 0:
            {
                iState = !digitalRead(PIN_KEY0);                    // Keys are inverted (1=off, 0=on)
                break;
            }
            case 1:
            {
                iState = !digitalRead(PIN_KEY1);                    // Keys are inverted (1=off, 0=on)
                break;
            }
            default:
            {
                iState = -1;
                break;
            }
        }

        switch (iState)
        {
            case 0:
            {
                strState = "Off";
                break;
            }
            case 1:
            {
                strState = "On";
                break;
            }
            default:
            {
                strState = "(unknown)";
                break;
            }
        }
    }
    else
    {
        strState = "???";
    }

    TRACE1("- AppGetInputKeyState: strState=%s\n", strState.c_str());

    return (strState);

}





//-------------------------------------------------------------------------//
//                                                                         //
//          Handlers for Web Page '/board_output_leds.html'                //
//                                                                         //
//-------------------------------------------------------------------------//

//---------------------------------------------------------------------------
//  Application Callback Handler: Resolve Output LEDs Symbol in HTML Document
//---------------------------------------------------------------------------

String  AppCbHdlrHtmlResolveSymbolOutputLeds (const String& strSymbol_p)
{

String  strSymbol;
String  strLedClass;
String  strValue;


    TRACE1("+ CallBack Handler 'AppCbHdlrHtmlResolveSymbolOutputLeds()': strSymbol_p=%s\n", strSymbol_p.c_str());

    strValue = "";

    strSymbol = strSymbol_p;
    strSymbol.toUpperCase();
    if ( strSymbol.startsWith("STATE_LEDS_") )
    {
        strLedClass = strSymbol.substring(sizeof("STATE_LEDS_")-1);
        TRACE1("  LedClass: %s\n", strLedClass.c_str());

        if (strLedClass == String("FEATURE_ENABLE"))
        {
            strValue = ( CFG_ENABLE_DI_DO ) ? "1" : "0";
        }
        else
        {
            strValue = AppGetOutputLedsState(strLedClass);
        }
    }

    TRACE1("- CallBack Handler 'AppCbHdlrHtmlResolveSymbolOutputLeds()': strValue=%s\n", strValue.c_str());

    return (strValue);

}



//---------------------------------------------------------------------------
//  Application Callback Handler: Handle Set for ('/set_output', HTTP_POST)
//---------------------------------------------------------------------------

void  AppCbHdlrHtmlOnTellOutputLeds (AsyncWebServerRequest* pServerRequest_p)
{

#define PARAM_NAME_LED      "led"
#define PARAM_NAME_STATE    "state"

String  strLedClass;
String  strState;
int     iState;


    TRACE0("+ AppCbHdlrHtmlOnTellOutputLeds('/set_output', HTTP_POST)\n");
    TRACE1("  URL:  %s\n", pServerRequest_p->url().c_str());
    TRACE1("  Host: %s\n", pServerRequest_p->host().c_str());

    if (pServerRequest_p != NULL)
    {
        if ( CFG_ENABLE_DI_DO )
        {
            // get values on <ESP_IP>/set_output?led=<LedType>&state=<State>
            if (pServerRequest_p->hasParam(PARAM_NAME_LED) && pServerRequest_p->hasParam(PARAM_NAME_STATE))
            {
                strLedClass = pServerRequest_p->getParam(PARAM_NAME_LED)->value();
                strLedClass.toUpperCase();
                TRACE1("  LedClass: %s\n", strLedClass.c_str());

                strState = pServerRequest_p->getParam(PARAM_NAME_STATE)->value();
                iState = strState.toInt();
                TRACE1("  State: %d\n", iState);

                LedBarIndicator_g = kLedBarNone;                // prevent overwriting of LED Bar by other sources
                AppSetOutputLedsState(strLedClass, iState);
            }
            else
            {
                strLedClass = "{not sent}";
                strState    = "{not sent}";
            }
        }
        else
        {
            strLedClass = "{not enabled}";
            strState   = "{not enabled}";
        }
    }

    pServerRequest_p->send(200);

    TRACE2("  LED: %s - Set to: %s\n", strLedClass.c_str(), strState.c_str());
    TRACE0("- AppCbHdlrHtmlOnTellOutputLeds('/set_output', HTTP_POST)\n");

    return;

}



//---------------------------------------------------------------------------
//  Get current State of Output LEDs
//---------------------------------------------------------------------------

String  AppGetOutputLedsState (String strLedClass_p)
{

int     iState;
String  strState;


    TRACE1("+ AppGetOutputLedsState: strLedClass_p=%s\n", strLedClass_p.c_str());

    if ( CFG_ENABLE_DI_DO )
    {
        strLedClass_p.toUpperCase();
        iState = 0;

        if (strLedClass_p == "GREEN")
        {
            iState = digitalRead(PIN_LED0);
        }
        else if (strLedClass_p == "YELLOW")
        {
            iState = digitalRead(PIN_LED3);
        }
        else if (strLedClass_p == "RED")
        {
            iState = digitalRead(PIN_LED6);
        }

        if ( iState )
        {
            strState = "checked";
        }
        else
        {
            strState = "";
        }
    }

    TRACE1("- AppGetOutputLedsState: strState=%s\n", strState.c_str());

    return (strState);

}



//---------------------------------------------------------------------------
//  Set State of Output LEDs
//---------------------------------------------------------------------------

void  AppSetOutputLedsState (String strLedClass_p, int iState_p)
{

int  iState;


    TRACE2("+ AppSetOutputLedsState: strLedClass_p=%s, iState_p=%d\n", strLedClass_p.c_str(), iState_p);

    if ( CFG_ENABLE_DI_DO )
    {
        strLedClass_p.toUpperCase();
        iState = (iState_p == 0) ? LOW : HIGH;

        if (strLedClass_p == "LED_GREEN")
        {
            digitalWrite(PIN_LED0, iState);
            digitalWrite(PIN_LED1, iState);
            digitalWrite(PIN_LED2, iState);
        }
        else if (strLedClass_p == "LED_YELLOW")
        {
            digitalWrite(PIN_LED3, iState);
            digitalWrite(PIN_LED4, iState);
            digitalWrite(PIN_LED5, iState);
        }
        else if (strLedClass_p == "LED_RED")
        {
            digitalWrite(PIN_LED6, iState);
            digitalWrite(PIN_LED7, iState);
            digitalWrite(PIN_LED8, iState);
        }
    }

    TRACE0("- AppSetOutputLedsState\n");

    return;

}





//=========================================================================//
//                                                                         //
//          S M A R T B O A R D   A P P   F U N C T I O N S                //
//                                                                         //
//=========================================================================//

//---------------------------------------------------------------------------
//  Process LED Bar Indicator depending on its Data Source
//---------------------------------------------------------------------------

void  AppProcessLedBarIndicator()
{

    switch (LedBarIndicator_g)
    {
        case kLedBarNone:
        {
            break;
        }

        case kLedBarDht22Temperature:
        {
            AppSetDht22TemperatureLedBarIndicator();
            break;
        }

        case kLedBarDht22Humidity:
        {
            AppSetDht22HumidityLedBarIndicator();
            break;
        }

        case kLedBarMhz19Co2Level:
        {
            AppSetMhz19Co2LedBarIndicator();
            break;
        }

        default:
        {
            AppPresentLedBar(0);
            break;
        }
    }

    return;

}



//---------------------------------------------------------------------------
//  Set LED Bar Indicator for DHT22 Temperature
//---------------------------------------------------------------------------

void  AppSetDht22TemperatureLedBarIndicator()
{

int iBarValue;


    iBarValue = 0;

    //------------------------------------------
    if (flDhtTemperature_g < 0)
    {
        iBarValue = 0;
    }
    //------------------------------------------
    else if (flDhtTemperature_g <= 15)              // -+-
    {                                               //  |
        iBarValue = 1;                              //  |
    }                                               //  |
    else if (flDhtTemperature_g <= 18)              //  |
    {                                               //  |
        iBarValue = 2;                              //  | Green
    }                                               //  |
    else if (flDhtTemperature_g <= 20)              //  |
    {                                               //  |
        iBarValue = 3;                              //  |
    }                                               // -+-
    //------------------------------------------
    else if (flDhtTemperature_g <= 22)              // -+-
    {                                               //  |
        iBarValue = 4;                              //  |
    }                                               //  |
    else if (flDhtTemperature_g <= 24)              //  |
    {                                               //  |
        iBarValue = 5;                              //  | Yellow
    }                                               //  |
    else if (flDhtTemperature_g <= 26)              //  |
    {                                               //  |
        iBarValue = 6;                              //  |
    }                                               // -+-
    //------------------------------------------
    else if (flDhtTemperature_g <= 30)              // -+-
    {                                               //  |
        iBarValue = 7;                              //  |
    }                                               //  |
    else if (flDhtTemperature_g <= 35)              //  |
    {                                               //  |
        iBarValue = 8;                              //  | Red
    }                                               //  |
    else                                            //  |
    {                                               //  |
        iBarValue = 9;                              //  |
    }                                               // -+-


    AppPresentLedBar(iBarValue);


    return;

}



//---------------------------------------------------------------------------
//  Set LED Bar Indicator for DHT22 Humidity
//---------------------------------------------------------------------------

void  AppSetDht22HumidityLedBarIndicator()
{

int iBarValue;


    iBarValue = 0;

    //------------------------------------------
    if (flDhtHumidity_g < 5)
    {
        iBarValue = 0;
    }
    //------------------------------------------
    else if (flDhtHumidity_g <= 20)                 // -+-
    {                                               //  |
        iBarValue = 1;                              //  |
    }                                               //  |
    else if (flDhtHumidity_g <= 30)                 //  |
    {                                               //  |
        iBarValue = 2;                              //  | Green
    }                                               //  |
    else if (flDhtHumidity_g <= 40)                 //  |
    {                                               //  |
        iBarValue = 3;                              //  |
    }                                               // -+-
    //------------------------------------------
    else if (flDhtHumidity_g <= 50)                 // -+-
    {                                               //  |
        iBarValue = 4;                              //  |
    }                                               //  |
    else if (flDhtHumidity_g <= 60)                 //  |
    {                                               //  |
        iBarValue = 5;                              //  | Yellow
    }                                               //  |
    else if (flDhtHumidity_g <= 70)                 //  |
    {                                               //  |
        iBarValue = 6;                              //  |
    }                                               // -+-
    //------------------------------------------
    else if (flDhtHumidity_g <= 80)                 // -+-
    {                                               //  |
        iBarValue = 7;                              //  |
    }                                               //  |
    else if (flDhtHumidity_g <= 90)                 //  |
    {                                               //  |
        iBarValue = 8;                              //  | Red
    }                                               //  |
    else                                            //  |
    {                                               //  |
        iBarValue = 9;                              //  |
    }                                               // -+-


    AppPresentLedBar(iBarValue);


    return;

}



//---------------------------------------------------------------------------
//  Set LED Bar Indicator for MH-Z19 CO2 Level
//---------------------------------------------------------------------------

void  AppSetMhz19Co2LedBarIndicator()
{

int iBarValue;


    iBarValue = 0;

    //------------------------------------------
    if (iMhz19Co2Value_g < 10)
    {
        iBarValue = 0;
    }
    //------------------------------------------
    else if (iMhz19Co2Value_g <= 500)               // -+-
    {                                               //  |
        iBarValue = 1;                              //  |
    }                                               //  |
    else if (iMhz19Co2Value_g <= 750)               //  |
    {                                               //  |
        iBarValue = 2;                              //  | Green
    }                                               //  |
    else if (iMhz19Co2Value_g <= 1000)              //  |
    {                                               //  |
        iBarValue = 3;                              //  |
    }                                               // -+-
    //------------------------------------------
    else if (iMhz19Co2Value_g <= 1333)              // -+-
    {                                               //  |
        iBarValue = 4;                              //  |
    }                                               //  |
    else if (iMhz19Co2Value_g <= 1666)              //  |
    {                                               //  |
        iBarValue = 5;                              //  | Yellow
    }                                               //  |
    else if (iMhz19Co2Value_g <= 2000)              //  |
    {                                               //  |
        iBarValue = 6;                              //  |
    }                                               // -+-
    //------------------------------------------
    else if (iMhz19Co2Value_g <= 3000)              // -+-
    {                                               //  |
        iBarValue = 7;                              //  |
    }                                               //  |
    else if (iMhz19Co2Value_g <= 4000)              //  |
    {                                               //  |
        iBarValue = 8;                              //  | Red
    }                                               //  |
    else                                            //  |
    {                                               //  |
        iBarValue = 9;                              //  |
    }                                               // -+-


    AppPresentLedBar(iBarValue);


    return;

}





//=========================================================================//
//                                                                         //
//          S M A R T B O A R D   H A R D W A R E   F U N C T I O N S      //
//                                                                         //
//=========================================================================//

//---------------------------------------------------------------------------
//  Process DHT Sensor (Temperature/Humidity)
//---------------------------------------------------------------------------

void  AppProcessDhtSensor (uint32_t ui32DhtReadInterval_p, bool fPrintSensorValues_p)
{

char      szSensorValues[64];
uint32_t  ui32CurrTick;
float     flTemperature;
float     flHumidity;


    ui32CurrTick = millis();
    if ((ui32CurrTick - ui32LastTickDhtRead_g) >= ui32DhtReadInterval_p)
    {
        flTemperature = DhtSensor_g.readTemperature(false);                 // false = Temp in Celsius degrees, true = Temp in Fahrenheit degrees
        flHumidity    = DhtSensor_g.readHumidity();

        // check if values read from DHT22 sensor are valid
        if ( !isnan(flTemperature) && !isnan(flHumidity) )
        {
            // chache DHT22 sensor values for processing in HTML document and for displaying in the LED Bar indicator
            flDhtTemperature_g = flTemperature;
            flDhtHumidity_g    = flHumidity;

            // print DHT22 sensor values in Serial Console (Serial Monitor)
            if ( fPrintSensorValues_p )
            {
                snprintf(szSensorValues, sizeof(szSensorValues), "DHT22: Temperature=%.1f, Humidity=%.1f", flTemperature, flHumidity);
                Serial.println(szSensorValues);
            }
        }
        else
        {
            // reading sensor failed
            Serial.println("ERROR: Failed to read from DHT sensor!");
        }

        ui32LastTickDhtRead_g = ui32CurrTick;
    }

    return;

}



//---------------------------------------------------------------------------
//  Process MH-Z19 Sensor (CO2Value/SensorTemperature)
//---------------------------------------------------------------------------

void  AppProcessMhz19Sensor (uint32_t ui32Mhz19ReadInterval_p, bool fPrintSensorValues_p)
{

char      szSensorValues[64];
uint32_t  ui32CurrTick;
int       iMhz19Co2Value;
int       iMhz19Co2SensTemp;


    ui32CurrTick = millis();
    if ((ui32CurrTick - ui32LastTickMhz19Read_g) >= ui32Mhz19ReadInterval_p)
    {
        iMhz19Co2Value    = Mhz19Sensor_g.getCO2();                     // MH-Z19: Request CO2 (as ppm)
        iMhz19Co2SensTemp = Mhz19Sensor_g.getTemperature();             // MH-Z19: Request Sensor Temperature (as Celsius)

        // chache MH-Z19 sensor value for processing in HTML document and for displaying in the LED Bar indicator
        iMhz19Co2Value_g    = iMhz19Co2Value;
        iMhz19Co2SensTemp_g = iMhz19Co2SensTemp;

        // print MH-Z19 sensor values in Serial Console (Serial Monitor)
        if ( fPrintSensorValues_p )
        {
            snprintf(szSensorValues, sizeof(szSensorValues), "MH-Z19: CO2(ppm)=%d, SensTemp=%d", iMhz19Co2Value_g, iMhz19Co2SensTemp_g);
            Serial.println(szSensorValues);
        }

        ui32LastTickMhz19Read_g = ui32CurrTick;
    }

    return;

}



//---------------------------------------------------------------------------
//  Calibrate MH-Z19 Sensor Manually
//---------------------------------------------------------------------------

bool  AppMhz19CalibrateManually()
{

int   iCountDownCycles;
bool  fCalibrateSensor;


    // turn off auto calibration
    Mhz19Sensor_g.autoCalibration(false);

    Serial.println();
    Serial.println();
    Serial.println("************************************************");
    Serial.println("---- Manually MH-Z19 Sensor Calibration ----");
    Serial.print("Countdown: ");

    // run calibration start countdown
    // (KEY0=pressed -> continue countdown, KEY0=released -> abort countdown)
    iCountDownCycles = 9;                                               // LED Bar Length = 9 LEDs
    fCalibrateSensor = true;
    do
    {
        Serial.print(iCountDownCycles); Serial.print(" ");
        if ( digitalRead(PIN_KEY0) )                                    // Keys are inverted (1=off, 0=on)
        {
            fCalibrateSensor = false;
            break;
        }
        AppPresentLedBar(iCountDownCycles);
        delay(500);

        if ( digitalRead(PIN_KEY0) )                                    // Keys are inverted (1=off, 0=on)
        {
            fCalibrateSensor = false;
            break;
        }
        AppPresentLedBar(0);
        delay(500);
    }
    while (iCountDownCycles-- > 0);

    // run calibration or abort?
    if ( fCalibrateSensor )
    {
        // calibrate sensor
        Serial.println();
        Serial.println("MH-Z19 Sensor Calibration...");
        Mhz19Sensor_g.calibrate();
        Serial.println("  -> done.");

        // signal calibration finished
        iCountDownCycles = 3;
        do
        {
            AppPresentLedBar(9);
            delay(125);
            AppPresentLedBar(0);
            delay(125);
        }
        while (iCountDownCycles-- > 1);
    }
    else
    {
        Serial.println("ABORT");
    }

    Serial.println("************************************************");
    Serial.println();
    Serial.println();

    return (fCalibrateSensor);

}



//---------------------------------------------------------------------------
//  Calibrate MH-Z19 Sensor Unattended
//---------------------------------------------------------------------------

bool  AppMhz19CalibrateUnattended()
{

// According to DataSheet the MH-Z19 Sensor must be in stable 400ppm ambient
// environment for more than 20 minutes.
uint32_t  ui32LeadTime = (25 * 60 * 1000);                       // 25 Minutes in [ms]

char      szTextBuff[64];
uint32_t  ui32StartTime;
int32_t   i32RemainTime;
int       iCountDownCycles;
int       iLedBarValue;


    // turn off auto calibration
    Mhz19Sensor_g.autoCalibration(false);

    Serial.println();
    Serial.println();
    Serial.println("************************************************");
    Serial.println("---- Unattended MH-Z19 Sensor Calibration ----");
    Serial.println("Countdown:");

    // run countdown to stabilize sensor in a 400ppm ambient environment for more than 20 minutes
    ui32StartTime = millis();
    do
    {
        i32RemainTime = (int) (ui32LeadTime - (millis() - ui32StartTime));

        // divide the remaining time over the 9 LEDs of LED Bar
        iLedBarValue = (int)((i32RemainTime / (ui32LeadTime / 9)) + 1);
        if (iLedBarValue > 9)
        {
            iLedBarValue = 9;
        }

        snprintf(szTextBuff, sizeof(szTextBuff), "  Remaining Time: %d [sec] -> LedBarValue=%d", i32RemainTime/1000, iLedBarValue);
        Serial.println(szTextBuff);

        AppPresentLedBar(iLedBarValue);
        delay(500);
        AppPresentLedBar(0);
        delay(500);
    }
    while (i32RemainTime > 0);

    // calibrate sensor
    Serial.println();
    Serial.println("MH-Z19 Sensor Calibration...");
    Mhz19Sensor_g.calibrate();
    Serial.println("  -> done.");

    // signal calibration finished
    iCountDownCycles = 3;
    do
    {
        AppPresentLedBar(9);
        delay(125);
        AppPresentLedBar(0);
        delay(125);
    }
    while (iCountDownCycles-- > 1);

    Serial.println("************************************************");
    Serial.println();
    Serial.println();

    return (true);

}



//---------------------------------------------------------------------------
//  Present LED Bar (0 <= iBarValue_p <= 9)
//---------------------------------------------------------------------------

void  AppPresentLedBar (int iBarValue_p)
{
    AppPresentLedBar(iBarValue_p, false);
}
//---------------------------------------------------------------------------
void  AppPresentLedBar (int iBarValue_p, bool fInvertBar_p)
{

uint32_t  ui32BarBitMap;


    if (iBarValue_p < 0)
    {
        ui32BarBitMap = 0x0000;             // set LED0..LED8 = OFF
    }
    else if (iBarValue_p > 9)
    {
        ui32BarBitMap = 0x01FF;             // set LED0..LED8 = ON
    }
    else
    {
        ui32BarBitMap = 0x0000;
        while (iBarValue_p > 0)
        {
            ui32BarBitMap <<= 1;
            ui32BarBitMap  |= 1;
            iBarValue_p--;
        }
    }

    if ( fInvertBar_p )
    {
        ui32BarBitMap ^= 0x01FF;
    }

    digitalWrite(PIN_LED0, (ui32BarBitMap & 0b000000001) ? HIGH : LOW);
    digitalWrite(PIN_LED1, (ui32BarBitMap & 0b000000010) ? HIGH : LOW);
    digitalWrite(PIN_LED2, (ui32BarBitMap & 0b000000100) ? HIGH : LOW);
    digitalWrite(PIN_LED3, (ui32BarBitMap & 0b000001000) ? HIGH : LOW);
    digitalWrite(PIN_LED4, (ui32BarBitMap & 0b000010000) ? HIGH : LOW);
    digitalWrite(PIN_LED5, (ui32BarBitMap & 0b000100000) ? HIGH : LOW);
    digitalWrite(PIN_LED6, (ui32BarBitMap & 0b001000000) ? HIGH : LOW);
    digitalWrite(PIN_LED7, (ui32BarBitMap & 0b010000000) ? HIGH : LOW);
    digitalWrite(PIN_LED8, (ui32BarBitMap & 0b100000000) ? HIGH : LOW);

    return;

}





//=========================================================================//
//                                                                         //
//          E S P 3 2   H A R D W A R E   T I M E R   F U N C T I O N S    //
//                                                                         //
//=========================================================================//

//---------------------------------------------------------------------------
//  ESP32 Hardware Timer ISR
//---------------------------------------------------------------------------

void  IRAM_ATTR  OnTimerISR()
{

    bStatusLedState_g = !bStatusLedState_g;
    digitalWrite(PIN_STATUS_LED, bStatusLedState_g);

    return;

}



//---------------------------------------------------------------------------
//  ESP32 Hardware Timer Start
//---------------------------------------------------------------------------

void  Esp32TimerStart (uint32_t ui32TimerPeriod_p)
{

uint32_t  ui32TimerPeriod;


    // stop a timer that may still be running
    Esp32TimerStop();

    // use 1st timer of 4
    // 1 tick take 1/(80MHZ/80) = 1us -> set divider 80 and count up
    pfnOnTimerISR_g = timerBegin(0, 80, true);

    // attach OnTimerISR function to timer
    timerAttachInterrupt(pfnOnTimerISR_g, &OnTimerISR, true);

    // set alarm to call OnTimerISR function, repeat alarm automatically (third parameter)
    ui32TimerPeriod = (unsigned long)ui32TimerPeriod_p * 1000L;         // ms -> us
    timerAlarmWrite(pfnOnTimerISR_g, ui32TimerPeriod, true);

    // start periodically alarm
    timerAlarmEnable(pfnOnTimerISR_g);

    return;

}



//---------------------------------------------------------------------------
//  ESP32 Hardware Timer Stop
//---------------------------------------------------------------------------

void  Esp32TimerStop()
{

    if (pfnOnTimerISR_g != NULL)
    {
        // stop periodically alarm
        timerAlarmDisable(pfnOnTimerISR_g);

        // dettach OnTimerISR function from timer
        timerDetachInterrupt(pfnOnTimerISR_g);

        // stop timer
        timerEnd(pfnOnTimerISR_g);
    }

    bStatusLedState_g = LOW;
    digitalWrite(PIN_STATUS_LED, bStatusLedState_g);

    return;

}





//=========================================================================//
//                                                                         //
//          P R I V A T E   G E N E R I C   F U N C T I O N S              //
//                                                                         //
//=========================================================================//

//---------------------------------------------------------------------------
//  Get Unique Client Name
//---------------------------------------------------------------------------

String  GetUniqueClientName (const char* pszClientPrefix_p)
{

String  strChipID;
String  strClientName;


    // Create a unique client name, based on ChipID (the ChipID is essentially its 6byte MAC address)
    strChipID = GetChipID();
    strClientName  = pszClientPrefix_p;
    strClientName += strChipID;

    return (strClientName);

}



//---------------------------------------------------------------------------
//  Get ChipID as String
//---------------------------------------------------------------------------

String  GetChipID()
{

String  strChipID;


    strChipID = GetEsp32MacId(false);

    return (strChipID);

}



//---------------------------------------------------------------------------
//  Get ChipMAC as String
//---------------------------------------------------------------------------

String  GetChipMAC()
{

String  strChipMAC;


    strChipMAC = GetEsp32MacId(true);

    return (strChipMAC);

}



//---------------------------------------------------------------------------
//  Get GetEsp32MacId as String
//---------------------------------------------------------------------------

String  GetEsp32MacId (bool fUseMacFormat_p)
{

uint64_t  ui64MacID;
String    strMacID;
byte      bDigit;
char      acDigit[2];
int       iIdx;


    ui64MacID = ESP.getEfuseMac();
    strMacID = "";
    for (iIdx=0; iIdx<6; iIdx++)
    {
        bDigit = (byte) (ui64MacID >> (iIdx * 8));
        sprintf(acDigit, "%02X", bDigit);
        strMacID += String(acDigit);

        if (fUseMacFormat_p && (iIdx<5))
        {
            strMacID += ":";
        }
    }

    strMacID.toUpperCase();

    return (strMacID);

}





//=========================================================================//
//                                                                         //
//          D E B U G   F U N C T I O N S                                  //
//                                                                         //
//=========================================================================//

//---------------------------------------------------------------------------
//  DEBUG: Dump Buffer
//---------------------------------------------------------------------------

#ifdef DEBUG_DUMP_BUFFER

void  DebugDumpBuffer (String strBuffer_p)
{

int            iBufferLen = strBuffer_p.length();
unsigned char  abDataBuff[iBufferLen];

    strBuffer_p.getBytes(abDataBuff, iBufferLen);
    DebugDumpBuffer(abDataBuff, strBuffer_p.length());

    return;

}

//---------------------------------------------------------------------------

void  DebugDumpBuffer (const void* pabDataBuff_p, unsigned int uiDataBuffLen_p)
{

#define COLUMNS_PER_LINE    16

const unsigned char*  pabBuffData;
unsigned int          uiBuffSize;
char                  szLineBuff[128];
unsigned char         bData;
int                   nRow;
int                   nCol;

    // get pointer to buffer and length of buffer
    pabBuffData = (const unsigned char*)pabDataBuff_p;
    uiBuffSize  = (unsigned int)uiDataBuffLen_p;


    // dump buffer contents
    for (nRow=0; ; nRow++)
    {
        sprintf(szLineBuff, "\n%04lX:   ", (unsigned long)(nRow*COLUMNS_PER_LINE));
        Serial.print(szLineBuff);

        for (nCol=0; nCol<COLUMNS_PER_LINE; nCol++)
        {
            if ((unsigned int)nCol < uiBuffSize)
            {
                sprintf(szLineBuff, "%02X ", (unsigned int)*(pabBuffData+nCol));
                Serial.print(szLineBuff);
            }
            else
            {
                Serial.print("   ");
            }
        }

        Serial.print(" ");

        for (nCol=0; nCol<COLUMNS_PER_LINE; nCol++)
        {
            bData = *pabBuffData++;
            if ((unsigned int)nCol < uiBuffSize)
            {
                if ((bData >= 0x20) && (bData < 0x7F))
                {
                    sprintf(szLineBuff, "%c", bData);
                    Serial.print(szLineBuff);
                }
                else
                {
                    Serial.print(".");
                }
            }
            else
            {
                Serial.print(" ");
            }
        }

        if (uiBuffSize > COLUMNS_PER_LINE)
        {
            uiBuffSize -= COLUMNS_PER_LINE;
        }
        else
        {
            break;
        }

        Serial.flush();     // give serial interface time to flush data
    }

    Serial.print("\n");

    return;

}

#endif




// EOF
