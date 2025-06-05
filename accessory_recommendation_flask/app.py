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
from pathlib import Path


# 指定專案根目錄下的 .env
env_path = Path(__file__).resolve().parents[1] / ".env"
load_dotenv(dotenv_path=env_path)
app = Flask(__name__)
# 一次性讀入產品資料
products_data = load_products()

# 暫存最新推薦結果與商品
latest_data = {"recommendation": "", "products": []}


@app.route("/", methods=["GET", "POST"])
def index():
    print("index route reached")
    recommendation = ""
    matched_products = []

    if request.method == "POST":
        style = request.form.get("style")
        hairstyle = request.form.get("hairstyle")

        # 呼叫 LLM 模型取得推薦
        recommendation = get_llm_recommendation(style, hairstyle)

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
    try:
        # 嘗試以 JSON 格式接收資料
        data = request.get_json(force=True)
        print("收到 JSON:", data)
    except Exception as e:
        print("無法解析 JSON:", str(e))
        return jsonify({"error": f"無法解析 JSON: {str(e)}"}), 400

    if not data:
        return jsonify({"error": "沒有收到任何 JSON 資料"}), 400

    style = data.get("style")
    hairstyle = data.get("hairstyle")

    # 只要有一個值就繼續跑：
    if not any([style, hairstyle]):
        return jsonify({"error": "至少需要一個參數"}), 400

    # 呼叫 LLM 推薦函式（需已定義）
    recommendation = get_llm_recommendation(style, hairstyle)
    if "error_detail" in recommendation:
        return jsonify({"error": recommendation["error_detail"]}), 400

    description = recommendation.get("description", "沒有推薦的描述")
    # 比對推薦商品
    matched_products = find_matching_products(
        recommendation.get("items", []), products_data
    )
    # 更新最新資料（需已定義 latest_data）
    latest_data.update(
        {
            "recommendation": description,
            "products": matched_products,
            "style": style,
            "hairstyle": hairstyle,
        }
    )

    return jsonify({"status": "success", "received": data})


# 此 API 提供前端輪詢取得最新推薦結果與對應商品，避免重複查詢。
@app.route("/latest", methods=["GET"])
def get_latest():
    return Response(
        json.dumps(latest_data, ensure_ascii=False),
        content_type="application/json; charset=utf-8",
    )


# 清除資料 : 重置全域變數 latest_data 為空狀態。
@app.route("/clear-cache", methods=["POST"])
def clear_cache():
    global latest_data
    latest_data = {"recommendation": "", "products": []}
    return jsonify({"status": "success", "message": "快取已清除"})


@app.route("/resume-detection", methods=["POST"])
def resume_detection():
    """
    將前端請求轉發給 Hub8735 裝置的 /resume endpoint，
    用來喚醒設備進入偵測狀態。
    """
    hub8735_ip = os.getenv("HUB8735_ip")  # 建議寫入 .env，如172.20.10.7
    url = f"http://{hub8735_ip}/resume"
    try:
        print(f"Connecting to {url}")
        resp = requests.get(url, timeout=5)
        print(f"Response status: {resp.status_code}")
        if resp.status_code == 200:
            return jsonify({"status": "success", "message": "開始偵測"})
        else:
            return (
                jsonify(
                    {"status": "fail", "message": f"HUB8735 回應失敗: {resp.text}"}
                ),
                500,
            )
    except Exception as e:
        print(f"Exception occurred: {e}", flush=True)
        return jsonify({"status": "error", "message": str(e)}), 500


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
    app.run(
        host="0.0.0.0", port=8000, debug=True
    )  # Flask Server 監聽所有網卡 IP、自動列出錯誤堆疊，方便 debug。
