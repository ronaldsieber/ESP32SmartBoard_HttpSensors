# ESP32SmartBoard_HttpSensors

This Arduino project enables the *ESP32SmartBoard* (see hardware project [ESP32SmartBoard_PCB](https://github.com/ronaldsieber/ESP32SmartBoard_PCB)) to present its sensor data via the Embedded WebServer. The values of the temperature and humidity sensor (DHT22), the CO2 sensor (MH-Z19) and the other peripherals of the board are displayed via HTTP pages. This means that these pages can be easily displayed in a browser on all common devices such as PCs, laptops, tablets or smartphones. The Embedded WebServer firmware is based on the *ESPAsyncWebServer* library, the pointer instruments are implemented using the open source library *gauge.min.js* (for download links see the end of this document).

In addition to this Arduino project, the [ESP32SmartBoard_MqttSensors](https://github.com/ronaldsieber/ESP32SmartBoard_MqttSensors) project is an alternative firmware for the *ESP32SmartBoard* that realizes a functionality that is basically similar. It differs from this  project here in that it sends the values of the sensors as MQTT messages. The two projects were designed for the following different use cases:

 - This project - **ESP32SmartBoard_HttpSensors**:
Easy to use, clear display of current sensor values | 
Stand-alone solution with Embedded WebServer, presentation of the current sensor values via HTTP pages in real time, direct access to the *ESP32SmartBoard* from devices such as PCs, laptop, tablet or smartphone
   
 - Alternative - **ESP32SmartBoard_MqttSensors**:
More complex setup, but supports evaluation and analysis of historical data | 
Part of an overall system made up of several *ESP32SmartBoards*, each board publishes its data via MQTT to a (shared) broker, who writes the data e.g. into a database (e.g. InfluxDB), from where it can be displayed and evaluated using graphical dashboards (e.g. Grafana)

## Project Overview

The *ESP32SmartBoard* establishes an Embedded WebServer and enables clients in the local network to access the values of the temperature and humidity sensor (DHT22), the CO2 sensor (MH-Z19) and the other peripherals of the board. The network settings required for this are described in the "WLAN configuration" section below.

![\[Project Overview\]](Documentation/ESP32SmartBoard_HttpSensors.png)

The Embedded WebServer of the *ESP32SmartBoard* interacts with the following HTTP pages:

| File | Meaning |
|--|--|
| index.html | Main menu |
| board_settings.html | Board configuration |
| dht22_as_gauge.html | Temperature and humidity sensors (DHT22) as graphical measuring instrument |
| dht22_as_text.html | Temperature and humidity sensor (DHT22) in textual form |
| mhz19_as_gauge.html | CO2 level and sensor temperature (MH-Z19) as graphic measuring instruments |
| mhz19_as_text.html | CO2 level and sensor temperature (MH-Z19) in text form |
| board_input_keys.html | Current state of the buttons KEY0 and KEY1 |
| board_output_leds.html | Setting the LEDs |


In order to call up the start page *index.html*, as usual, only the IP address of the *ESP32SmartBoard* has to be entered in the browser, e.g. *192.168.200.1* (the default address of the *ESP32SmartBoard*).

![\[index.html\]](Documentation/Screenshot_index.png)

All of the board functions listed above can be called up via the menu on the start page.

![\[mhz19_as_gauge.html\]](Documentation/Screenshot_mhz19_as_gauge.png)
The picture shows as an example the display of the CO2 level using a graphic pointer instrument.

![\[board_settings.html\]](Documentation/Screenshot_board_settings.png)

The "board_settings.html" page can be used to configure, among other things, which measured value is to be displayed on the board's LED bar. The following LED bar sources can be selected:

- temperature (DHT22)
- humidity (DHT22)
- CO2 level (MH-Z19)

The default source is defined with the constant: `DEFAULT_LED_BAR_INDICATOR = kLedBarMhz19Co2Level;`

## WLAN Configuration Section

The *ESP32SmartBoard* can set up its own WLAN as an Access Point (AP) as well as work in Client Mode (CM, Station) and log into an existing WLAN. The selection between AP and CM is made with following configuration setting:

    const int  CFG_WIFI_AP_MODE = 0;  // 0 = Device is Station/Client, 1 = Device is AP

The runtime configuration for the Client Mode (CM, Station) is carried out in the following configuration section:

    // WIFI Station/Client Configuration
    const char*  WIFI_STA_SSID     = "YourNetworkName";
    const char*  WIFI_STA_PASSWORD = "YourNetworkPassword";
    IPAddress    WIFI_STA_LOCAL_IP(192,168,30,100);   // for DHCP: IP(0,0,0,0)

If the board is operated as an Access Point (AP), the following section defines the runtime configuration:

    // WIFI AccessPoint Configuration
    const char*  WIFI_AP_SSID             = "ESP32SmartBoard";
    const char*  WIFI_AP_PASSWORD         = "HttpSensors";
    IPAddress    WIFI_AP_LOCAL_IP(192,168,200,1);     // IPAddr for Server if running in AP Mode
    const int    WIFI_AP_CHANNEL          = 1;        // WIFI Channel, from 1 to 13
    const bool   WIFI_AP_HIDDE_SSID       = false;    // true = Hide SSID
    const int    WIFI_AP_MAX_CONNECTIONS  = 4;        // max. simultaneous connected stations

If the *ESP32SmartBoard* sets up its own WLAN as an Access Point (AP), the device on which the HTML pages are to be displayed (laptop, tablet, smartphone) must be logged into this WLAN. To do this, select the WLAN defined under `WIFI_AP_SSID` on the device. The password specified under `WIFI_AP_PASSWORD` is to be used as the password.

## Application Configuration Section

At the beginning of the sketch there is the following configuration section:

    const int  CFG_ENABLE_NETWORK_SCAN  = 1;
    const int  CFG_ENABLE_DI_DO         = 1;
    const int  CFG_ENABLE_DHT_SENSOR    = 1;
    const int  CFG_ENABLE_MHZ_SENSOR    = 1;
    const int  CFG_ENABLE_STATUS_LED    = 1;

This enables the runtime execution of the associated code sections to be activated *(= 1)* or blocked *(= 0)*. This allows the sketch to be used for boards on which not all components are fitted, without the lack of components leading to runtime errors.

## Files for the ESPAsyncWebServer on the ESP32SmartBoard

The Embedded WebServer ESPAsyncWebServer requires access to various HTML files, images and JavaScript files at runtime. All of these files are located in the *'data'* folder of the current sketch. To transfer these files to the *ESP32SmartBoard*, the [Arduino ESP32 filesystem uploader plugin](https://github.com/me-no-dev/arduino-esp32fs-plugin/releases) for the Arduino IDE is required. The plugin copies all files from the *'data'* folder of the current sketch into the SPIFFS File System in the ESP32 Flash memory.

To install the plug-in, simply extract the contents of the ZIP file *'ESP32FS'* into the *'Arduino\Tools'* directory of the IDE.

The file transfer is started from the IDE via *"Tools -> ESP32 Sketch Data Upload"*.

![\[ESP32 Sketch Data Upload\]](Documentation/Screenshot_IDE_ESP32SketchDataUpload.png)
## Calibration of the CO2 sensor MH-Z19

**An incorrectly performed calibration can brick the sensor. It is therefore important to understand how calibration works.**

The sensor is designed for use in 24/7 continuous operation. It supports the modes AutoCalibration, calibration via Hardware Signal (triggered manually) and calibration via Software Command (also triggered manually). The *ESP32SmartBoard* uses the AutoCalibration and manual calibration modes via software commands. Regardless of the method used, the calibration sets the sensor-internal reference value of 400 ppm CO2. A concentration of 400 ppm is considered to be the normal CO2 value in the earth's atmosphere, i.e. a typical value for the outside air in rural areas.

**(1) AutoCalibration:**

With AutoCalibration, the sensor permanently monitors the measured CO2 values. The lowest value measured within 24 hours is interpreted as the reference value of 400 ppm CO2. This method is recommended by the sensor manufacturer for use of the sensor in normal living rooms or in offices that are regularly well ventilated. It is implicitly assumed that the air inside the roorm is completely exchanged during ventilation and thus the CO2 concentration in the room falls down to the normal value of the earth's atmosphere / outside air.

However, the sensor manufacturer explicitly states in the datasheet that the AutoCalibration method cannot be used for use in agricultural greenhouses, farms, refrigerators, etc. AutoCalibration should be disabled here.

In the *ESP32SmartBoard* Sketch the constant `DEFAULT_MHZ19_AUTO_CALIBRATION` is used to activate (true) or deactivate (false) the AutoCalibration method. The AutoCalibration method is set on system startup. If necessary, the AutoCalibration method can be inverted using a push button. The following steps are required for this:

1. Press KEY0 and keep it pressed until step 3
2. Press and release Reset on the ESP32DevKit
3. Hold down KEY0 for another 2 seconds
4. Release KEY0

**(2) Manually Calibration:**

Before a manual calibration, the sensor must be operated for at least 20 minutes in a stable reference environment with 400 ppm CO2. This requirement can only be approximated in the amateur and hobby area without a defined calibration environment. For this purpose, the *ESP32SmartBoard* can be operated outdoors in a shady place or inside a room near an open window, also in the shadow. In this environment, the *ESP32SmartBoard* must work for at least 20 minutes before the calibration can be triggered.

* (2a) Direct calibration:

If the *ESP32SmartBoard* has been working in the 400 ppm reference environment for at least 20 minutes (outdoors or inside at the open window), the sensor can be calibrated directly. The following steps are required for this:

1. Press KEY0 and keep it pressed until step 4
2. Press and release Reset on the ESP32DevKit
3. The *ESP32SmartBoard* starts a countdown of 9 seconds, the LED Bar blinkes every second and shows the remaining seconds
4. Keep KEY0 pressed the entire time until the countdown has elapsed and the LED Bar acknowledges the completed calibration with 3x quick flashes
5. Release KEY0

If KEY0 is released during the countdown, the *ESP32SmartBoard* aborts the procedure without calibrating the sensor. 

![\[Sequence Schema for direct manually Calibration\]](Documentation/SequenceSchema_ManuallyCalibration_Direct.png)
* (2b) Unattended, delayed Calibration:

The calibration can also be carried out unsupervised and with a time delay, when the *ESP32SmartBoard* has been placed in the 400 ppm reference environment (outdoors or inside at the open window). The following steps are required for this:

1. Press KEY1 and hold it down until step 2
2. Press and release Reset on the ESP32DevKit
3. Release KEY1

The *ESP32SmartBoard* starts a countdown of 25 minutes. During this countdown the LED Bar blinkes every second and shows the remaining time (25 minutes / 9 LEDs = 2:46 minutes / LED). After the countdown has elapsed, the calibration process is triggered and acknowledged with 3x quick flashing of the LED Bar. The *ESP32SmartBoard* is then restarted with a software reset.

![\[Sequence Schema for unattended, delayed Calibration\]](Documentation/SequenceSchema_ManuallyCalibration_Unattended.png)
## Run-time Output in the Serial Terminal Window

At runtime, all relevant information is output in the serial terminal window (115200Bd). In particular during the system start (sketch function `setup()`), error messages are also displayed here, which may be due to a faulty software configuration. These messages should be observed in any case, especially during commissioning.

In the main loop of the program (sketch function `main()`) the values of the DHT22 (temperature and humidity) and the MH-Z19 (CO2 level and sensor temperature) are displayed cyclically. In addition, messages here again provide information about any problems with accessing the sensors.

By activating the line `#define DEBUG` at the beginning of the sketch, further, very detailed runtime messages are displayed. These are particularly helpful during program development or for troubleshooting. By commenting out the line `#define DEBUG`, the output is suppressed again.

## ESPAsyncWebServer Basics

The *ESP32SmartBoard* Sketch uses the *ESPAsyncWebServer* library (https://github.com/me-no-dev/ESPAsyncWebServer). This library was specially designed for embedded devices. It supports a wide range of functionalities, different data sources and REST interfaces. The *ESP32SmartBoard* Sketch uses the following functionalities:

**(1) Preprocessing and delivery of HTML files with dynamic content**

To do this, the URL of the HTML files to be dynamically prepared is registered in the *"on"* method of the *AsyncWebServer* object for the *"GET"* operation and linked to a request handler:

    AsyncWebServer.on(<file_url>, HTTP_GET, <RequestHandler>(AsyncWebServerRequest* pServerRequest_p));

When a page is delivered via the *"GET"* operation, the symbolic placeholders contained in the page (*"%SENSOR_VALUE%"*) are dynamically replaced with the current values of the status and sensor data using the request handler. In this way, the data is inserted directly into the HTML code of the page. From the browser's point of view, the dynamically prepared page looks like a normal HTML page.


**(2) Delivery of static files**

The AsyncWebServer calls the general method *"onNotFound"* for all files whose URL has not been specifically registered via the *"on"* method of the *AsyncWebServer*. The *ESP32SmartBoard* Sketch uses this to deliver all other files such as static HTML pages (without dynamic content), images and similar files:

    WebServerSync.onNotFound([](AsyncWebServerRequest* pServerRequest_p)
    {
        String strPath = pServerRequest_p->url();
        pServerRequest_p->send(SPIFFS, strPath.c_str());
    });

**(3) Realization of REST Services**

Similar to the preparation and delivery of HTML files with dynamic content, the URLs for REST services are also registered using *"GET"* and *"POST"* operations in the *"on"* method of the *AsyncWebServer* object and linked to a request handler:

    AsyncWebServer.on(<rest_url>, HTTP_GET, <RequestHandler>(AsyncWebServerRequest* pServerRequest_p));

With the *"GET"* operation, data is requested from the *ESP32SmartBoard*. This is used in particular to query current status and sensor values.

    AsyncWebServer.on(<rest_url>, HTTP_POST, <RequestHandler>(AsyncWebServerRequest* pServerRequest_p));

The *"POST"* operation transfers data to the *ESP32SmartBoard*. This is used in particular to set configuration settings.

## AsyncWebServer on the ESP32SmartBoard

The main purpose of the HTML pages is to present dynamic status and sensor data and to send control or configuration data to the *ESP32SmartBoard*. The sketch uses the following mechanisms for this:

1. When a page is delivered using the *"GET"* operation, the symbolic placeholders contained in the page (*"%SENSOR_VALUE%"*) are replaced by the current values ​​of the status and sensor data. In this way, the data is inserted directly into the HTML code of the page. From the browser's point of view, they appear like static HTML values.

2. During the runtime, status and sensor data are cyclically queried again using the *"GET"* operation. The query of the values ​​from the *ESP32SmartBoard* as well as the updating of the display values ​​in the HTML elements is done by JavaScript code embedded in the page.

3. The sending of control or configuration data to the *ESP32SmartBoard* is done with the help of *"POST"* operations. The corresponding requests are also sent using JavaScript code embedded in the page.

In detail, the mechanisms listed above are implemented as follows:

**(1) Resolving symbolic placeholders ("%SENSOR_VALUE%")**

In order to dynamically embed status and sensor values in an HTML page, the relevant positions in the HTML code are marked with symbolic placeholders ("%SENSOR_VALUE%"). Using the example of the HTML file *"/dht22_as_text.html"*, the placeholder for the temperature value of the DHT22 sensor is embedded in the HTML code as follows:

    <span id="dht22_temperature">%DHT22_TEMPERATURE%</span>

The entry point for resolving symbolic placeholders on the *ESP32SmartBoard* is the *"on"* method of the *AsyncWebServer* object. The HTML file to be modified (*"/dht22_as_text.html"*) is registered for the *"GET"* operation and the request handler ("AsyncWebServerRequest*") is implemented as an anonymous function:

    pWebServer_g->on("/dht22_as_text.html", HTTP_GET, [](AsyncWebServerRequest* pServerRequest_p)
    {
        pServerRequest_p->send(SPIFFS, "/dht22_as_text.html", String(), false, AppCbHdlrHtmlResolveSymbolDht22Value);
    });

The *"send"* method reads the requested HTML file from the SPIFFS file system of the ESP32 flash memory and uses the specified callback function (here *"AppCbHdlrHtmlResolveSymbolDht22Value"*) to resolve the symbolic placeholders inserted in the HTML code (*"%SENSOR_VALUE%"*). The *AsyncWebServer* calls the callback function for each symbol to be resolved separately. The symbol is passed as a call parameter. The callback function provides the status and sensor value to be embedded in the HTML page as return value.

    // (simplified version, for real implementation see ESP32SmartBoard_HttpSensors.ino)
    String  AppCbHdlrHtmlResolveSymbolDht22Value (const String& strSymbol_p)
    {
    String  strValue;
    
        if (strSymbol_p == "DHT22_TEMPERATURE")
        {
            strValue = strDht22Temperature_g;
        }
        else if (strSymbol_p == "DHT22_HUMIDITY")
        {
            strValue = strDht22Humidity_g;
        }
        else
        {
            strValue = "?";
        }
    
        return (strValue);
    }

**(2) Cyclical query of the status and sensor data during runtime**

The cyclical query of the status and sensor data during runtime is done via REST service using the *"GET"* operation. For this purpose, JavaScript functions embedded in the HTML page are called cyclically using an interval timer. For example, the JavaScript function for querying the current temperature value from the DHT22 sensor is implemented as follows:

    function FunTimerHandlerGetDht22Temperature()
    {
        var XHttpReq = new XMLHttpRequest();
        XHttpReq.onreadystatechange = function()
        {
            if (this.readyState == 4 && this.status == 200)
            {
                document.getElementById("dht22_temperature").innerHTML = this.responseText;
            }
        };
        XHttpReq.open("GET", "/get_dht22?temperature", true);
        XHttpReq.send();
    }

The *"GET"* operation calls the URL *"/get_dht22? Temperature"* on the *ESP32SmartBoard*. The part before the *"?"* separator is the base URL (*"/get_dht22"*). The part after the separator is the parameter list (*"temperature"*). The entry point for resolving the URL is the *"on"* method of the *AsyncWebServer* object. The base URL (*"/get_dht22"*) is registered for the *"GET"* operation and assigned to the request handler *"AppCbHdlrHtmlOnRequestDht22"*:

    // Handle a GET request to <ESP_IP>/get_dht22?<ValueType>
    pWebServer_g->on("/get_dht22", HTTP_GET, AppCbHdlrHtmlOnRequestDht22);

The request handler *"AppCbHdlrHtmlOnRequestDht22"* is implemented in *ESP32SmartBoard_HttpSensors.ino* as follows:

    void  AppCbHdlrHtmlOnRequestDht22 (AsyncWebServerRequest* pServerRequest_p)
    {
    
    #define PARAM_NAME_TEMPERATURE      "temperature"
    #define PARAM_NAME_HUMIDITY         "humidity"
    
    String  strValue;
    
        if (pServerRequest_p != NULL)
        {
            // get values on <ESP_IP>/get_dht22?<ValueType>
            if (pServerRequest_p->hasParam(PARAM_NAME_TEMPERATURE))
            {
                strValue = AppGetDht22Value(PARAM_NAME_TEMPERATURE);
            }
            else if (pServerRequest_p->hasParam(PARAM_NAME_HUMIDITY))
            {
                strValue = AppGetDht22Value(PARAM_NAME_HUMIDITY);
            }
            else
            {
                strValue = "{unknown}";
            }
        }
    
        pServerRequest_p->send(200, "text/plain", strValue);
    }

**(3) Setting Outputs and writing Configuration Settings**

The setting of outputs and writing of configuration settings during runtime is done by REST service using the *"POST"* operation. This is implemented in the HTML page with the help of embedded JavaScript functions. For example, the JavaScript function for setting the heartbeat mode (blinking of the blue LED on the ESP32DevKit) is implemented as follows:

    function FunSliderSysLedHeartbeat(HtmlElement_p)
    {
        var Value;
    
        if ( HtmlElement_p.checked )
        {
            Value = "1";
        }
        else
        {
            Value = "0";
        }
    
        var XHttpReq = new XMLHttpRequest();
        XHttpReq.open("POST", "/settings?option=" + HtmlElement_p.id + "&value=" + Value, true);
        XHttpReq.send();
    }

The *"POST"* operation calls the URL *"/settings?Option=1"* on the *ESP32SmartBoard*. The part before the *"?"* separator is the base URL (*"/settings"*). The part after the separator is the parameter list ("option", value=1). The entry point for resolving the URL is the *"on"* method of the *AsyncWebServer* object. The base URL (*"/settings"*) for the *"POST"* operation is registered and assigned to the request handler *"AppCbHdlrHtmlOnRequestDht22"*:

    // Handle a POST request to <ESP_IP>/settings?option=<OptionType>&value=<SetValue>
    pWebServer_g-> on("/settings", HTTP_POST, AppCbHdlrHtmlOnTellBoardSettings);

The request handler *"AppCbHdlrHtmlOnTellBoardSettings"* is implemented in *ESP32SmartBoard_HttpSensors.ino* as follows:

    // (simplified version, for real implementation see ESP32SmartBoard_HttpSensors.ino)
    void  AppCbHdlrHtmlOnTellBoardSettings (AsyncWebServerRequest* pServerRequest_p)
    {
    
    #define PARAM_NAME_OPTION   "option"
    #define PARAM_NAME_VALUE    "value"
    
    String  strOptionType;
    String  strValue;
    int     iValue;
    
        if (pServerRequest_p != NULL)
        {
            // get values on <ESP_IP>/settings?option=<OptionType>&value=<SetValue>
            if (pServerRequest_p->hasParam(PARAM_NAME_OPTION) && pServerRequest_p->hasParam(PARAM_NAME_VALUE))
            {
                strOptionType = pServerRequest_p->getParam(PARAM_NAME_OPTION)->value();
                strOptionType.toUpperCase();
                strValue = pServerRequest_p->getParam(PARAM_NAME_VALUE)->value();
                iValue = strValue.toInt();
    
                if (strOptionType == String("SYSLED_HEARTBEAT"))
                {
                    fStatusLedHeartbeat_g = (iValue == 0) ? false : true;
                }
                else if (strOptionType == String("PRINT_SENSOR_VALUES"))
                {
                    fPrintSensorValues_g = (iValue == 0) ? false : true;
                }
            }
        }
    
        pServerRequest_p->send(200);
    }

**(4) Redirect to "index.html"**

In order to call up the start page *index.html*, as usual, only the IP address of the *ESP32SmartBoard* has to be entered in the browser, e.g. *192.168.200.1*. This call is forwarded internally by Sketch to the *index.html* start page.

The redirect is made in the *"onNotFound"* method of the *AsyncWebServer*. To do this, the URL *"/"* is evaluated and answered by an HTML redirect:

    <meta http-equiv="refresh" content="0; url='/index.html'" />

This HTML code is part of the `const char root_html[]` field, which is hard coded in the *ESP32SmartBoard_HttpSensors.ino* sketch. The functionality required for delivery in the *"onNotFound"* method is implemented as follows:

    // (simplified version, for real implementation see ESP32SmartBoard_HttpSensors.ino)
    pWebServer_g->onNotFound([](AsyncWebServerRequest* pServerRequest_p)
    {
        String strPath = pServerRequest_p->url();
        if (strPath == String("/"))
        {
            // Resolve <ESP32SmartBoard_IP> to 'index.html'
            pServerRequest_p->send_P(200, "text/html", root_html);
        }
        else
        {
            // ...
        }
    });

Redirecting the IP address to the start page *index.html* has the advantage that a dedicated *"on"* method of the *AsyncWebServer* object can also be defined for the start page in order to be able to resolve symbolic placeholders for this page as well. The start page *index.html*, for example, has symbolic placeholders for version number and build timestamp, which are resolved in this way.

## Used Third Party Components 

1. **Embedded WebServer**
The ESPAsyncWebServer library is used for the Embedded WebServer firmware: https://github.com/me-no-dev/ESPAsyncWebServer

2. **Graphic Pointer Instruments (Gauges)**
The gauge instruments used in the HTML pages for temperature, humidity and CO2 level are implemented by the open source library *"gauge.min.js"*. Detailed documentation, examples and the source code are available at https://canvas-gauges.com/. The *"gauge.min.js"* library is located together with the HTML files and images in the *'data'* folder of the sketch and is transferred from there to the SPIFFS file system in the ESP32 flash memory.

3. **MH-Z19 CO2 Sensor**
The following driver libraries are used for the MH-Z19 CO2 sensor:
https://www.arduino.cc/reference/en/libraries/mh-z19/ and https://www.arduino.cc/en/Reference/SoftwareSerial. The installation is done with the Library Manager of the Arduino IDE.

4. **DHT Sensor**
The driver library from Adafruit is used for the DHT sensor (temperature, humidity). The installation is done with the Library Manager of the Arduino IDE.


