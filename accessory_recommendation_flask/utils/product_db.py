# utils/product_db.py

import json
import os

# 假設 data/products.json 放在同一層資料夾中
PRODUCT_JSON_PATH = os.path.join(os.path.dirname(__file__), "../data/products.json")


def load_products():
    """載入產品資料"""
    with open(PRODUCT_JSON_PATH, "r", encoding="utf-8") as f:
        products = json.load(f)
        print(f"[INFO] 成功載入 {len(products)} 筆產品資料。")
        return products


def find_matching_products(items: list, products: list, top_k: int = 3) -> list:
    """
    根據 LLM 回傳的推薦項目，找出最相符的產品。
    將推薦項目的 type、style、color 與產品的 tags 比對，計算交集數量作為匹配分數。
    - type: 2分
    - style: 1分
    - color: 1分
    """
    # 權重設定
    weights = {
        "type": 2,
        "style": 1,
        "color": 1,
    }

    scores = []

    for product in products:
        product_tags = set(tag.lower() for tag in product.get("tags", []))
        score = 0
        """
        for item in items:
            #  收集推薦項目的 type, style, color，轉成小寫集合
            item_tags = set(
                [
                    item.get("type", "").lower(),
                    item.get("style", "").lower(),
                    item.get("color", "").lower(),
                ]
            )
            # 對應的交集數量即為分數
            score += len(product_tags & item_tags)

        scores.append((score, product))
        """

        for item in items:
            # 分別比對每個欄位是否出現在 tags 中，若有則加相對權重
            for key in ["type", "style", "color"]:
                value = item.get(key, "").lower()
                if value and value in product_tags:
                    score += weights.get(key, 0)

        scores.append((score, product))

    # 過濾掉沒有匹配的
    scores = [item for item in scores if item[0] > 0]
    scores.sort(key=lambda x: x[0], reverse=True)

    return [item[1] for item in scores[:top_k]]
