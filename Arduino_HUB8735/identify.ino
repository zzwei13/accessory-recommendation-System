// Generated Date: Thu, 31 Oct 2024 15:19:42 GMT

#include <WiFi.h>
WiFiSSLClient client;
#include "StreamIO.h"
#include "VideoStream.h"
#include "RTSP.h"
#include "NNObjectDetection.h"
#include "VideoStreamOverlay.h"
#define amb82_CHANNEL 0
#define CHANNELNN 3
#define NNWIDTH 576
#define NNHEIGHT 320
VideoSetting config(VIDEO_VGA, CAM_FPS, VIDEO_H264_JPEG, 1);
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
        long int StartTime = millis();
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
ObjectDetectionItem itemList[16] = {
    {0, "Black", 1},
    {1, "Blue", 1},
    {2, "Green", 1},
    {3, "Orange", 1},
    {4, "Pink", 1},
    {5, "Purple", 1},
    {6, "Red", 1},
    {7, "Tshirt", 1},
    {8, "While", 1},
    {9, "Yellow", 1},
    {10, "dress", 1},
    {11, "long", 1},
    {12, "longcurly", 1},
    {13, "short", 1},
    {14, "shortcurly", 1},
    {15, "suit", 1}};
#endif

int Black = 0;
int Blue = 0;
int Green = 0;
int Orange = 0;
int Pink = 0;
int Purple = 0;
int Red = 0;
int Tshirt = 0;
int While = 0;
int Yellow = 0;
int dress = 0;
int long_hair = 0;
int longcurly = 0;
int short_hair = 0;
int shortcurly = 0;
int suit = 0;

void ODPostProcess(std::vector<ObjectDetectionResult> results)
{
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
                OSD.drawRect(amb82_CHANNEL, xmin, ymin, xmax, ymax, 3, OSD_COLOR_WHITE);
                char text_str[20];
                snprintf(text_str, sizeof(text_str), "%s %d", itemList[obj_type].objectName, item.score());
                OSD.drawText(amb82_CHANNEL, xmin, ymin - OSD.getTextHeight(amb82_CHANNEL), text_str, OSD_COLOR_CYAN);
                Serial.println((String(itemList[obj_type].objectName) + ", " + String(item.score()) + ", " + String(xmin) + ", " + String(ymin) + ", " + String(xmax) + ", " + String(ymax) + ", " + String((xmax - xmin)) + ", " + String((ymax - ymin))));
                if (obj_type == 0)
                    Black++;
                else if (obj_type == 1)
                    Blue++;
                else if (obj_type == 2)
                    Green++;
                else if (obj_type == 3)
                    Orange++;
                else if (obj_type == 4)
                    Pink++;
                else if (obj_type == 5)
                    Purple++;
                else if (obj_type == 6)
                    Red++;
                else if (obj_type == 7)
                    Tshirt++;
                else if (obj_type == 8)
                    While++;
                else if (obj_type == 9)
                    Yellow++;
                else if (obj_type == 10)
                    dress++;
                else if (obj_type == 11)
                    long_hair++;
                else if (obj_type == 12)
                    longcurly++;
                else if (obj_type == 13)
                    short_hair++;
                else if (obj_type == 14)
                    shortcurly++;
                else if (obj_type == 15)
                    suit++;
            }
        }
    }
    Serial.print("??©ä»¶??¸ï??");
    Serial.print(ObjDet.getResultCount());
    Serial.print(", Blackï¼?");
    Serial.print(Black);
    Serial.print(", Blueï¼?");
    Serial.print(Blue);
    Serial.print(", Greenï¼?");
    Serial.print(Green);
    Serial.print(", Orangeï¼?");
    Serial.print(Orange);
    Serial.print(", Pinkï¼?");
    Serial.print(Pink);
    Serial.print(", Purpleï¼?");
    Serial.print(Purple);
    Serial.print(", Redï¼?");
    Serial.print(Red);
    Serial.print(", Tshirtï¼?");
    Serial.print(Tshirt);
    Serial.print(", Whileï¼?");
    Serial.print(While);
    Serial.print(", Yellowï¼?");
    Serial.print(Yellow);
    Serial.print(", dressï¼?");
    Serial.print(dress);
    Serial.print(", longï¼?");
    Serial.print(long_hair);
    Serial.print(", longcurlyï¼?");
    Serial.print(longcurly);
    Serial.print(", shortï¼?");
    Serial.print(short_hair);
    Serial.print(", shortcurlyï¼?");
    Serial.print(shortcurly);
    Serial.print(", suitï¼?");
    Serial.print(suit);
    Serial.println("");

    Black = Blue = Green = Orange = Pink = Purple = Red = Tshirt = While = Yellow = dress = long_hair = longcurly = short_hair = shortcurly = suit = 0;
    OSD.update(amb82_CHANNEL);
}

void setup()
{
    Serial.begin(115200);
    initWiFi();
    Serial.println(WiFi.localIP());

    config.setBitrate(2 * 1024 * 1024);
    Camera.configVideoChannel(amb82_CHANNEL, config);
    Camera.configVideoChannel(CHANNELNN, configNN);
    Camera.videoInit();
    rtsp.configVideo(config);
    rtsp.begin();
    rtsp_portnum = rtsp.getPort();
    ObjDet.configVideo(configNN);
    ObjDet.setResultCallback(ODPostProcess);
    ObjDet.modelSelect(OBJECT_DETECTION, CUSTOMIZED_YOLOV4TINY, NA_MODEL, NA_MODEL);
    ObjDet.begin();
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
    videoStreamerNN.registerOutput(ObjDet);
    if (videoStreamerNN.begin() != 0)
    {
        Serial.println("StreamIO link start failed");
    }
    Camera.channelBegin(CHANNELNN);
    OSD.configVideo(amb82_CHANNEL, config);
    OSD.begin();
}

void loop()
{
}
