def recommend_accessories(color: str, style: str, hairstyle: str) -> str:
    accessories = []

    # 顏色對應推薦
    if color in ["藍", "灰", "綠", "冷色系"]:
        accessories.append("銀色飾品（如銀項鍊、銀耳環）")
    elif color in ["紅", "橘", "米", "暖色系"]:
        accessories.append("金色飾品（如鍍金手環、金戒指）")
    elif color in ["黑", "白", "中性色", "黑白灰"]:
        accessories.append("中性色飾品（依風格或髮型調整）")

    # 穿搭風格對應推薦
    if style == "正式":
        accessories.append("極簡金屬風飾品（圓弧設計、細鏈）或珍珠類飾品")
    elif style == "休閒":
        accessories.append("木質或手工繩編飾品")
    elif style == "浪漫":
        accessories.append("簡約幾何風項鍊或花朵造型耳環")
    elif style == "個性":
        accessories.append("粗鍊、幾何形狀的銀色飾品")

    # 髮型對應推薦
    if hairstyle == "長髮":
        accessories.append("項鍊 / 戒指（避免耳環被遮）")
    elif hairstyle == "捲髮":
        accessories.append("亮眼項鍊 + 手鍊")
    elif hairstyle == "短髮":
        accessories.append("耳環（幾何耳扣最適合）")

    # 組合所有建議
    recommendation = "推薦飾品組合：\n- " + "\n- ".join(accessories)
    return recommendation


if __name__ == "__main__":
    # 假設這些是 AI 辨識後的輸出
    wear_color = "藍"
    wear_style = "正式"
    hair_style = "短髮"

    result = recommend_accessories(wear_color, wear_style, hair_style)
    print(result)


""" 
延伸設計（可選功能）
你可以加入：

判斷優先順序（髮型 > 色彩 > 風格）

若重複推薦飾品，自動合併類型
"""
