# utils/product_db.py

import json
import os

# 假設 data/products.json 放在同一層資料夾中
PRODUCT_JSON_PATH = os.path.join(os.path.dirname(__file__), "../data/products.json")


def load_products():
    """載入產品資料"""
    with open(PRODUCT_JSON_PATH, "r", encoding="utf-8") as f:
        return json.load(f)


def find_matching_products(items: list, products: list, top_k: int = 3) -> list:
    """
    根據 LLM 回傳的結構化推薦項目，找出最相符的產品。
    比對依據：類型（type）、顏色（color）、風格（style）與產品 tags 的重合度。
    """
    scores = []

    for product in products:
        product_tags = [tag.lower() for tag in product["tags"]]
        score = 0

        for item in items:
            type_match = item["type"].lower() in product_tags
            color_match = item["color"].lower() in product_tags
            style_match = item["style"].lower() in product_tags
            score += type_match + color_match + style_match

        scores.append((score, product))

    scores.sort(key=lambda x: x[0], reverse=True)

    return [item[1] for item in scores[:top_k]]
