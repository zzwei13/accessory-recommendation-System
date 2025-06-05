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
            style = name == "tshirt" ? "��" : (name == "dress" ? "�v��" : "���");

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
            hairstyle = "�����v";
        else if (hasShort)
            hairstyle = "�u���v";
        else
            hairstyle = "���v";
    }
    else if (hasLong)
    {
        hairstyle = "���v";
    }
    else if (hasShort)
    {
        hairstyle = "�u�v";
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
    const char *host = "172.20.10.3"; // �q�� IP 172.20.10.3
    int port = 8000;

    if (client.connect(host, port))
    {
        Serial.println("Connected to Flask server");

        String request = String("POST /receive HTTP/1.1\r\n") +
                         "Host: " + host + "\r\n" +
                         "Content-Type: application/json\r\n" +
                         "Content-Length: " + jsonData.length() + "\r\n\r\n" +
                         jsonData;

        client.print(request); // �o�e POST �ШD
        Serial.println("Request sent:");
        Serial.println(request);

        // Ū�����A���^��
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

// �����ܼ�
String lastSentJson = "";
int sameResultCount = 0;
const int thresholdSameResult = 3;
bool isDetectionPaused = true; // �w�]���u�Ȱ����v

// ----------- Object Detection Callback -----------
void ODPostProcess(std::vector<ObjectDetectionResult> results)
{

    if (results.size() == 0)
    {
        return;
    }

    if (isDetectionPaused)
    {
        OSD.createBitmap(amb82_CHANNEL); // ���صe���A�۷��M��
        OSD.update(amb82_CHANNEL);       // ��s�e��
        return;
    }

    // ���]�p�ƾ�
    tshirt = suit = dress = 0;
    long_hair = short_hair = curly = 0;

    // �i�@�B�z��G�O�_�����Ī���]�p�C��B�v���B��ۡ^
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
        // �S���ݭn������A���L�������G
        return;
    }

    String currentJson = mapToJSON(results);
    Serial.println("Current JSON: " + currentJson);

    // �P�_�O�_��W���ۦP
    if (currentJson == lastSentJson)
    {
        sameResultCount++;
    }
    else
    {
        sameResultCount = 1; // �s���G�A���]�p�ƾ�
        lastSentJson = currentJson;
    }
    Serial.print("sameResultCount = ");
    Serial.println(sameResultCount);

    if (sameResultCount >= thresholdSameResult && !isDetectionPaused)
    {
        // �s�򵲪G�ۦP�F���e�A�e�X�üȰ�����
        Serial.println("Threshold reached, sending JSON and pausing detection...");
        sendJSON(currentJson);
        isDetectionPaused = true;
        sameResultCount = 0; // ���m�p�ơA���ݥ~���_��
    }

    // �N�������Gø�s��e��
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

                // �[�J��ɭ���A�T�O���W�X�e���d��
                xmin = std::max(0, std::min(xmin, im_w - 1));
                xmax = std::max(0, std::min(xmax, im_w - 1));
                ymin = std::max(0, std::min(ymin, im_h - 1));
                ymax = std::max(0, std::min(ymax, im_h - 1));

                OSD.drawRect(amb82_CHANNEL, xmin, ymin, xmax, ymax, 3, OSD_COLOR_WHITE);

                char text_str[20]; // ø�s�ؽu
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
        sprintf(buf, "����ơG%d,T��G%d, �v�ˡG%d, ��ˡG%d, ���v�G%d, �u�v�G%d, ���v�G%d",
                count, tshirt, suit, dress, long_hair, short_hair, curly);
        Serial.println(buf);
        Serial.println("");
    }
    OSD.update(amb82_CHANNEL);
}

// �ǰe HTTP �^�������U���
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
    client.println("Access-Control-Allow-Origin: *"); // �i��G�䴩�e�ݸ��
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
    ObjDet.begin(); // YOLOv4-Tiny ����

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
    videoStreamerNN.registerOutput(ObjDet); // �����ҰʤF���׬y�{
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
                // HTTP header ������ \r\n\r\n
                if (request.endsWith("\r\n\r\n"))
                {
                    break;
                }
            }
        }

        Serial.println("Request:");
        Serial.println(request);

        // ���]�Ʊq�u�Ȱ������v���A�������u�Ұʰ����v���A�A
        // �}�l����i�檫�󰻴��P���G��X�C
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
          isDetectionPaused = false; // ��_����
          Serial.println("Detection resumed!");

          // �ǰe HTTP �^��
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

        delay(3); // �T�O�ǿ駹��
        incomingClient.stop();
        Serial.println("Client Disconnected");
    }
}
