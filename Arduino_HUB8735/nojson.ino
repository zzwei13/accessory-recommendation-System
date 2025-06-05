//Generated Date: Thu, 31 Oct 2024 15:19:42 GMT

int curly = 0;
int dress = 0;
int long_hair = 0;
int short_hair = 0;
int suit = 0;
int tshirt = 0;
unsigned long lastPrintTime = 0;

#include <WiFi.h>
WiFiSSLClient client;
#include "StreamIO.h"
#include "VideoStream.h"
#include "RTSP.h"
#include "NNObjectDetection.h"
#include "VideoStreamOverlay.h"
#define amb82_CHANNEL 0
#define CHANNELNN 3
#define NNWIDTH  576
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

// char _lwifi_ssid[] = "ERTLab";
// char _lwifi_pass[] = "csieert201";
char _lwifi_ssid[] = "Cheng";
char _lwifi_pass[] = "19960116";

void initWiFi() {

  for (int i=0;i<2;i++) {
    WiFi.begin(_lwifi_ssid, _lwifi_pass);

    delay(1000);
    Serial.println("");
    Serial.print("Connecting to ");
    Serial.println(_lwifi_ssid);

    long int StartTime=millis();
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        if ((StartTime+5000) < millis()) break;
    }

    if (WiFi.status() == WL_CONNECTED) {
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
struct ObjectDetectionItem {
    uint8_t index;
    const char* objectName;
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

void ODPostProcess(std::vector<ObjectDetectionResult> results) {
    uint16_t im_h = config.height();
    uint16_t im_w = config.width();
    OSD.createBitmap(amb82_CHANNEL);
    if (ObjDet.getResultCount() > 0) {
        for (int i = 0; i < ObjDet.getResultCount(); i++) {
            int obj_type = results[i].type();
            if (itemList[obj_type].filter) {
                ObjectDetectionResult item = results[i];
                int xmin = (int)(item.xMin() * im_w);
                int xmax = (int)(item.xMax() * im_w);
                int ymin = (int)(item.yMin() * im_h);
                int ymax = (int)(item.yMax() * im_h);
                //printf("Item %d %s:\t%d %d %d %d\n\r", i, itemList[obj_type].objectName, xmin, xmax, ymin, ymax);
                OSD.drawRect(amb82_CHANNEL, xmin, ymin, xmax, ymax, 3, OSD_COLOR_WHITE);
                char text_str[20];
                snprintf(text_str, sizeof(text_str), "%s %d", itemList[obj_type].objectName, item.score());
                OSD.drawText(amb82_CHANNEL, xmin, ymin - OSD.getTextHeight(amb82_CHANNEL), text_str, OSD_COLOR_CYAN);
  Serial.println((String(String(itemList[obj_type].objectName))+String(", ")+String(item.score())+String(", ")+String(xmin)+String(", ")+String(ymin)+String(", ")+String(xmax)+String(", ")+String(ymax)+String(", ")+String((xmax-xmin))+String(", ")+String((ymax-ymin))));
  if ((String(itemList[obj_type].objectName)=="curly")) {
    curly++;
  }
  if ((String(itemList[obj_type].objectName)=="dress")) {
    dress++;
  }
  if ((String(itemList[obj_type].objectName)=="long")) {
    long_hair++;
  }
  if ((String(itemList[obj_type].objectName)=="short")) {
    short_hair++;
  }
  if ((String(itemList[obj_type].objectName)=="suit")) {
    suit++;
  }
  if ((String(itemList[obj_type].objectName)=="tshirt")) {
    tshirt++;
  }

            }
        }
    }
  // Serial.println((String("物件數：")+String(ObjDet.getResultCount())+
  //               String(", 捲髮：")+String(curly)+
  //               String(", 洋裝：")+String(dress)+
  //               String(", 長髮：")+String(long_hair)+
  //               String(", 短髮：")+String(short_hair)+
  //               String(", 西裝：")+String(suit)+
  //               String(", T恤：")+String(tshirt)));

  // curly = 0;
  // dress = 0;
  // long_hair = 0;
  // short_hair = 0;
  // suit = 0;
  // tshirt = 0;

    OSD.update(amb82_CHANNEL);
}

void setup()
{
  Serial.begin(115200);
  initWiFi();
  Serial.println(WiFi.localIP());

  config.setBitrate(1 * 1024 * 1024);
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
  if (videoStreamer.begin() != 0) {
      Serial.println("StreamIO link start failed");
  }
  Camera.channelBegin(amb82_CHANNEL);
  videoStreamerNN.registerInput(Camera.getStream(CHANNELNN));
  videoStreamerNN.setStackSize();
  videoStreamerNN.setTaskPriority();
  videoStreamerNN.registerOutput(ObjDet);
  if (videoStreamerNN.begin() != 0) {
      Serial.println("StreamIO link start failed");
  }
  Camera.channelBegin(CHANNELNN);
  OSD.configVideo(amb82_CHANNEL, config);
  OSD.begin();

  //使用自訂yolo模型於SD卡載入，須開啟內建Arduino IDE選擇模型來源為SD卡後燒錄。
}

void loop()
{
  unsigned long currentMillis = millis();

  if (currentMillis - lastPrintTime >= 5000) { // 每 5 秒顯示一次結果
    lastPrintTime = currentMillis;

    Serial.println((String("物件數：")+String(ObjDet.getResultCount())+
                  String(", 捲髮：")+String(curly)+
                  String(", 長髮：")+String(long_hair)+
                  String(", 短髮：")+String(short_hair)+
                  String(", 洋裝：")+String(dress)+
                  String(", 西裝：")+String(suit)+
                  String(", T恤：")+String(tshirt)));

    // 歸零統計
    curly = 0;
    dress = 0;
    long_hair = 0;
    short_hair = 0;
    suit = 0;
    tshirt = 0;
  }
}

