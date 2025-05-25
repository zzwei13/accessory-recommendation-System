專案利用 Ambarella HUB-8735 (AmebaPro2) 平台進行即時物件偵測與串流輸出，搭配 YOLOv4-Tiny 自訂模型，可辨識衣著顏色、風格、髮型等資訊，並透過 RTSP 進行影像串流。此系統適用於穿搭推薦等場景。

### 軟體準備
1. 安裝 AmebaPro2 Arduino 開發套件
在 Arduino IDE 的「偏好設定」>「附加開發板管理員網址」加入以下網址：

```bash
https://github.com/ideashatch/HUB-8735/raw/main/amebapro2_arduino/Arduino_package/id
```
### 功能說明
- 支援 YOLOv4-Tiny 自訂模型推論
- 可辨識 16 種類別：
-- 衣著：Tshirt, dress, suit
-- 髮型：long, longcurly, short, shortcurly

- 影像串流輸出：透過 RTSP 可在 VLC 等播放器觀看即時影像
- OSD 疊加框線與辨識結果分數
- 串流與偵測分兩路平行運行


### 執行流程
1. 將程式碼燒錄至 AmebaPro2
2. 打開 Serial Monitor（115200 baud）
3. 成功連線後，終端機會顯示 IP 位址
4. 開啟 VLC 或其他 RTSP 播放器
- 輸入串流網址，例如：

```arduino
rtsp://<your_device_ip>:554/test
```