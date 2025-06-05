// Generated Date: Thu, 31 Oct 2024 15:19:42 GMT
#include <WiFi.h>

// WiFiSSLClient client;
WiFiClient client;
WiFiServer server(80);

#include "StreamIO.h"
#include "VideoStream.h"
#include "RTSP.h"
#include "NNObjectDetection.h"
#include "VideoStreamOverlay.h"
#include <algorithm>

#define amb82_CHANNEL 0
#define CHANNELNN 3
#define NNWIDTH 576
#define NNHEIGHT 320

// VideoSetting config(VIDEO_VGA, 10, VIDEO_H264_JPEG, 1);
VideoSetting config(VIDEO_VGA, 15, VIDEO_H264_JPEG, 1);
VideoSetting configNN(NNWIDTH, NNHEIGHT, 10, VIDEO_RGB, 0);

NNObjectDetection ObjDet;
RTSP rtsp;
StreamIO videoStreamer(1, 1);
StreamIO videoStreamerNN(1, 1);

int rtsp_portnum;

uint32_t img_addr = 0;
uint32_t img_len = 0;

char _lwifi_ssid[] = "";
char _lwifi_pass[] = "";

void initWiFi()
{
    for (int i = 0; i < 2; i++)
    {
        WiFi.begin(_lwifi_ssid, _lwifi_pass);
        delay(1000);
        Serial.println("");
        Serial.print("Connecting to ");
        Serial.println(_lwifi_ssid);
        unsigned long StartTime = millis(); // long int StartTime=millis();
        while (WiFi.status() != WL_CONNECTED)
        {
            delay(500);
            if ((StartTime + 5000) < millis())
                break;
        }
        if (WiFi.status() == WL_CONNECTED)
        {
            Serial.println("");
            Serial.println("STAIP address: ");
            Serial.println(WiFi.localIP());
            Serial.println("");
            break;
        }
    }
}

#ifndef __OBJECTCLASSLIST_H__
#define __OBJECTCLASSLIST_H__
struct ObjectDetectionItem
{
    uint8_t index;
    const char *objectName;
    uint8_t filter;
};
ObjectDetectionItem itemList[6] = {
    {0, "curly", 1},
    {1, "dress", 1},
    {2, "long", 1},
    {3, "short", 1},
    {4, "suit", 1},
    {5, "tshirt", 1},
};
#endif

// ----------- Convert result to JSON -----------
String mapToJSON(std::vector<ObjectDetectionResult> results)
{
    String color = "", style = "", hairstyle = "";
    bool hasLong = false, hasShort = false, hasCurly = false;
    for (auto &item : results)
    {
        int type = item.type();
        if (type < 0 || type >= 6)
            continue;

        String name = String(itemList[type].objectName);

        if (style == "" && (name == "tshirt" || name == "dress" || name == "suit"))
            style = name == "tshirt" ? "休閒" : (name == "dress" ? "洋裝" : "西裝");

        if (name == "long")
            hasLong = true;
        else if (name == "short")
            hasShort = true;
        else if (name == "curly")
            hasCurly = true;
    }

    if (hasCurly)
    {
        if (hasLong)
            hairstyle = "長捲髮";
        else if (hasShort)
            hairstyle = "短捲髮";
        else
            hairstyle = "捲髮";
    }
    else if (hasLong)
    {
        hairstyle = "長髮";
    }
    else if (hasShort)
    {
        hairstyle = "短髮";
    }

    String json = "{";
    json += "\"style\":" + (style == "" ? "null" : "\"" + style + "\"") + ",";
    json += "\"hairstyle\":" + (hairstyle == "" ? "null" : "\"" + hairstyle + "\"");
    json += "}";

    return json;
}

// ----------- POST JSON to Flask Server -----------
void sendJSON(String jsonData)
{
    const char *host = "172.20.10.3"; // 電腦 IP 172.20.10.3
    int port = 8000;

    if (client.connect(host, port))
    {
        Serial.println("Connected to Flask server");

        String request = String("POST /receive HTTP/1.1\r\n") +
                         "Host: " + host + "\r\n" +
                         "Content-Type: application/json\r\n" +
                         "Content-Length: " + jsonData.length() + "\r\n\r\n" +
                         jsonData;

        client.print(request); // 發送 POST 請求
        Serial.println("Request sent:");
        Serial.println(request);

        // 讀取伺服器回應
        while (client.connected() || client.available())
        {
            if (client.available())
            {
                String line = client.readStringUntil('\n');
                Serial.println(line);
            }
        }

        client.stop();
        Serial.println("Connection closed.");
    }
    else
    {
        Serial.println("Connection to server failed.");
    }
}

int tshirt = 0;
int suit = 0;
int dress = 0;

int long_hair = 0;
int short_hair = 0;
int curly = 0;

// 控制變數
String lastSentJson = "";
int sameResultCount = 0;
const int thresholdSameResult = 3;
bool isDetectionPaused = true; // 預設為「暫停中」

// ----------- Object Detection Callback -----------
void ODPostProcess(std::vector<ObjectDetectionResult> results)
{

    if (results.size() == 0)
    {
        return;
    }

    if (isDetectionPaused)
    {
        OSD.createBitmap(amb82_CHANNEL); // 重建畫面，相當於清空
        OSD.update(amb82_CHANNEL);       // 更新畫面
        return;
    }

    // 重設計數器
    tshirt = suit = dress = 0;
    long_hair = short_hair = curly = 0;

    // 進一步篩選：是否有有效物件（如顏色、髮型、穿著）
    bool hasStyle = false;
    bool hasHairstyle = false;

    for (auto &item : results)
    {
        int type = item.type();
        if (type >= 0 && type < 6 && itemList[type].filter == 1)
        {
            if (type == 1 || type == 4 || type == 5)
                hasStyle = true;
            if (type == 0 || type == 2 || type == 3)
                hasHairstyle = true;
        }
        if (hasStyle && hasHairstyle)
            break;
    }

    if (!(hasStyle || hasHairstyle))
    {
        // 沒有需要的物件，跳過偵測結果
        return;
    }

    String currentJson = mapToJSON(results);
    Serial.println("Current JSON: " + currentJson);

    // 判斷是否跟上次相同
    if (currentJson == lastSentJson)
    {
        sameResultCount++;
    }
    else
    {
        sameResultCount = 1; // 新結果，重設計數器
        lastSentJson = currentJson;
    }
    Serial.print("sameResultCount = ");
    Serial.println(sameResultCount);

    if (sameResultCount >= thresholdSameResult && !isDetectionPaused)
    {
        // 連續結果相同達門檻，送出並暫停偵測
        Serial.println("Threshold reached, sending JSON and pausing detection...");
        sendJSON(currentJson);
        isDetectionPaused = true;
        sameResultCount = 0; // 重置計數，等待外部復原
    }

    // 將偵測結果繪製到畫面
    uint16_t im_h = config.height();
    uint16_t im_w = config.width();
    OSD.createBitmap(amb82_CHANNEL);

    if (ObjDet.getResultCount() > 0)
    {
        for (int i = 0; i < ObjDet.getResultCount(); i++)
        {
            int obj_type = results[i].type();
            if (itemList[obj_type].filter)
            {
                ObjectDetectionResult item = results[i];
                int xmin = (int)(item.xMin() * im_w);
                int xmax = (int)(item.xMax() * im_w);
                int ymin = (int)(item.yMin() * im_h);
                int ymax = (int)(item.yMax() * im_h);

                // 加入邊界限制，確保不超出畫面範圍
                xmin = std::max(0, std::min(xmin, im_w - 1));
                xmax = std::max(0, std::min(xmax, im_w - 1));
                ymin = std::max(0, std::min(ymin, im_h - 1));
                ymax = std::max(0, std::min(ymax, im_h - 1));

                OSD.drawRect(amb82_CHANNEL, xmin, ymin, xmax, ymax, 3, OSD_COLOR_WHITE);

                char text_str[20]; // 繪製框線
                snprintf(text_str, sizeof(text_str), "%s %d", itemList[obj_type].objectName, item.score());
                OSD.drawText(amb82_CHANNEL, xmin, ymin - OSD.getTextHeight(amb82_CHANNEL), text_str, OSD_COLOR_CYAN);
                Serial.println((String(itemList[obj_type].objectName) + ", " + String(item.score()) + ", " + String(xmin) + ", " + String(ymin) + ", " + String(xmax) + ", " + String(ymax) + ", " + String((xmax - xmin)) + ", " + String((ymax - ymin))));

                switch (obj_type)
                {
                case 0:
                    curly++;
                    break;
                case 1:
                    dress++;
                    break;
                case 2:
                    long_hair++;
                    break;
                case 3:
                    short_hair++;
                    break;
                case 4:
                    suit++;
                    break;
                case 5:
                    tshirt++;
                    break;
                }
            }
        }
    }
    int count = ObjDet.getResultCount();

    if (!isDetectionPaused)
    {
        char buf[256];
        sprintf(buf, "物件數：%d,T恤：%d, 洋裝：%d, 西裝：%d, 長髮：%d, 短髮：%d, 捲髮：%d",
                count, tshirt, suit, dress, long_hair, short_hair, curly);
        Serial.println(buf);
        Serial.println("");
    }
    OSD.update(amb82_CHANNEL);
}

// 傳送 HTTP 回應的輔助函數
void sendHttpResponse(WiFiClient &client, int statusCode, const String &contentType, const String &body)
{
    String statusLine;

    switch (statusCode)
    {
    case 200:
        statusLine = "HTTP/1.1 200 OK";
        break;
    case 404:
        statusLine = "HTTP/1.1 404 Not Found";
        break;
    default:
        statusLine = "HTTP/1.1 500 Internal Server Error";
        break;
    }

    client.println(statusLine);
    client.println("Content-Type: " + contentType);
    client.println("Connection: close");
    client.println("Access-Control-Allow-Origin: *"); // 可選：支援前端跨域
    client.println();
    client.println(body);
}

void setup()
{
    Serial.begin(115200);

    delay(3);
    initWiFi();
    server.begin();
    Serial.println(WiFi.localIP());
    config.setBitrate(1 * 1024 * 1024);

    Camera.configVideoChannel(amb82_CHANNEL, config);
    Camera.configVideoChannel(CHANNELNN, configNN);

    Camera.videoInit();
    Serial.println("Camera initialized.");

    rtsp.configVideo(config);
    rtsp.begin();
    rtsp_portnum = rtsp.getPort();
    // Serial.print("RTSP stream available at port: ");
    // Serial.println(rtsp_portnum);

    ObjDet.configVideo(configNN);
    ObjDet.setResultCallback(ODPostProcess);
    ObjDet.modelSelect(OBJECT_DETECTION, CUSTOMIZED_YOLOV4TINY, NA_MODEL, NA_MODEL);
    ObjDet.begin(); // YOLOv4-Tiny 偵測

    videoStreamer.registerInput(Camera.getStream(amb82_CHANNEL));
    videoStreamer.registerOutput(rtsp);
    if (videoStreamer.begin() != 0)
    {
        Serial.println("StreamIO link start failed");
    }
    Camera.channelBegin(amb82_CHANNEL);

    videoStreamerNN.registerInput(Camera.getStream(CHANNELNN));
    videoStreamerNN.setStackSize();
    videoStreamerNN.setTaskPriority();
    videoStreamerNN.registerOutput(ObjDet); // 直接啟動了推論流程
    if (videoStreamerNN.begin() != 0)
    {
        Serial.println("StreamIO link start failed");
    }
    Camera.channelBegin(CHANNELNN);

    OSD.configVideo(amb82_CHANNEL, config);
    OSD.begin();
}

unsigned long lastYoloTime = 0;

void loop()
{

    WiFiClient incomingClient = server.available();
    if (incomingClient)
    {
        Serial.println("New Client Connected");

        String request = "";
        while (incomingClient.connected())
        {
            if (incomingClient.available())
            {
                char c = incomingClient.read();
                request += c;
                // HTTP header 結尾為 \r\n\r\n
                if (request.endsWith("\r\n\r\n"))
                {
                    break;
                }
            }
        }

        Serial.println("Request:");
        Serial.println(request);

        // 讓設備從「暫停偵測」狀態切換成「啟動偵測」狀態，
        // 開始持續進行物件偵測與結果輸出。
        if (request.indexOf("GET /resume") >= 0)
        {
            isDetectionPaused = false;
            Serial.println("Detection resumed!");

            sendHttpResponse(incomingClient, 200, "text/plain", "Detection resumed!");
        }
        else
        {
            sendHttpResponse(incomingClient, 404, "text/plain", "Not Found");
        }

        /*if (request.indexOf("GET /resume") >= 0) {
          isDetectionPaused = false; // 恢復偵測
          Serial.println("Detection resumed!");

          // 傳送 HTTP 回應
          incomingClient.println("HTTP/1.1 200 OK");
          incomingClient.println("Content-Type: text/plain");
          incomingClient.println("Connection: close");
          incomingClient.println();
          incomingClient.println("Detection resumed!");
        } else {
          incomingClient.println("HTTP/1.1 404 Not Found");
          incomingClient.println("Content-Type: text/plain");
          incomingClient.println("Connection: close");
          incomingClient.println();
          incomingClient.println("Not Found");
        }*/

        delay(3); // 確保傳輸完成
        incomingClient.stop();
        Serial.println("Client Disconnected");
    }
}
