"""
cd accessory_recommendation_flask
python app.py

"""

from flask import Flask, render_template, request, jsonify, Response
from dotenv import load_dotenv
import os
import requests
import json
from utils.product_db import load_products, find_matching_products
from utils.llm_helper import get_llm_recommendation

load_dotenv()
app = Flask(__name__)
# 一次性讀入產品資料
products_data = load_products()

# 暫存最新推薦結果與商品
latest_data = {"recommendation": "", "products": []}


# 關鍵字詞庫
KEYWORDS = [
    "可愛",
    "個性潮流",
    "氣質",
    "幾何",
    "俏皮",
    "華麗",
    "優雅氣質",
    "個性",
    "甜美",
    "簡約",
]


def extract_keywords(text):
    return [kw for kw in KEYWORDS if kw in text]


@app.route("/", methods=["GET", "POST"])
def index():
    print("index route reached")
    recommendation = ""
    matched_products = []

    if request.method == "POST":
        color = request.form.get("color")
        style = request.form.get("style")
        hairstyle = request.form.get("hairstyle")

        # 呼叫 LLM 模型取得推薦
        recommendation = get_llm_recommendation(color, style, hairstyle)

        if "error_detail" in recommendation:
            return jsonify({"error": recommendation["error_detail"]}), 400

        # 將推薦內容比對產品資料，篩選出匹配商品
        description = recommendation.get("description", "沒有推薦的描述")
        matched_products = find_matching_products(
            recommendation.get("items", []), products_data
        )
    # 確保這裡有 return（避免 None）
    return render_template(
        "index.html", recommendation=recommendation, products=matched_products
    )


@app.route("/receive", methods=["POST"])
def receive_from_camera():
    data = request.get_json()
    color = data.get("color")
    style = data.get("style")
    hairstyle = data.get("hairstyle")

    if not all([color, style, hairstyle]):
        return jsonify({"error": "缺少參數"}), 400

    recommendation = get_llm_recommendation(color, style, hairstyle)
    if "error_detail" in recommendation:
        return jsonify({"error": recommendation["error_detail"]}), 400

    description = recommendation.get("description", "沒有推薦的描述")
    matched_products = find_matching_products(
        recommendation.get("items", []), products_data
    )

    latest_data.update(
        {
            "recommendation": description,
            "products": matched_products,
            "color": color,
            "style": style,
            "hairstyle": hairstyle,
        }
    )

    return jsonify({"status": "success"})


# 此 API 提供前端輪詢取得最新推薦結果與對應商品，避免重複查詢。
@app.route("/latest", methods=["GET"])
def get_latest():
    return Response(
        json.dumps(latest_data, ensure_ascii=False),
        content_type="application/json; charset=utf-8",
    )


@app.errorhandler(500)
def internal_error(error):
    import traceback

    return (
        jsonify(
            {
                "error": "Internal Server Error",
                "message": str(error),
                "trace": traceback.format_exc(),
            }
        ),
        500,
    )


if __name__ == "__main__":
    app.run(debug=True, port=8000)
