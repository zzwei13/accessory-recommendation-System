# utils/llm_helper.py

import os
import requests
import json
from dotenv import load_dotenv
from pathlib import Path


# 找到上層資料夾（accessory_recommendation_flask）裡的 .env 檔案
env_path = Path(__file__).resolve().parent.parent / ".env"
load_dotenv(dotenv_path=env_path)

TOGETHER_API_KEY = os.getenv("TOGETHER_API_KEY")
TOGETHER_MODEL = "meta-llama/Llama-3.3-70B-Instruct-Turbo-Free"  # "mistralai/Mixtral-8x7B-Instruct-v0.1"
TOGETHER_API_URL = "https://api.together.xyz/v1/chat/completions"


def get_llm_recommendation(style: str, hairstyle: str) -> str:
    system_prompt = (
        "你是一位時尚飾品顧問，請根據使用者的穿搭風格與髮型，推薦適合的飾品。\n\n"
        "請依照以下規則操作：\n"
        "1. 飾品建議須包含：\n"
        "   - 種類（例如：項鍊、耳環、戒指、手鍊）\n"
        "   - 顏色（金色、銀色）\n"
        "   - 風格（可愛、俏皮、甜美、個性、幾何、優雅、簡約、華麗）\n"
        "2. 請簡要說明搭配理由（用繁體中文）\n"
        "3. 若 style 或 hairstyle 缺失，請給出通用搭配建議，以簡約、百搭為主\n"
        "4. 僅使用以下 JSON 格式回覆，不要有多餘文字：\n\n"
        "{\n"
        '  "description": "自然語言推薦說明，請用繁體中文",\n'
        '  "items": [\n'
        '    {"type": "飾品種類", "style": "風格", "color": "顏色"},\n'
        '    {"type": "飾品種類", "style": "風格", "color": "顏色"}\n'
        "  ]\n"
        "}"
    )

    user_prompt = f"風格是「{style}」，髮型是「{hairstyle}」。"

    headers = {
        "Authorization": f"Bearer {TOGETHER_API_KEY}",
        "Content-Type": "application/json",
    }

    payload = {
        "model": TOGETHER_MODEL,
        "messages": [
            {"role": "system", "content": system_prompt},
            {"role": "user", "content": user_prompt},
        ],
        "temperature": 0.7,
        "max_tokens": 500,
    }

    try:
        response = requests.post(TOGETHER_API_URL, headers=headers, json=payload)
        response.raise_for_status()  # 確保 HTTP 請求成功

        content = response.json()["choices"][0]["message"]["content"].strip()

        try:
            result = json.loads(content)  # 嘗試解析返回的 JSON
            return {"description": result["description"], "items": result["items"]}
        except json.JSONDecodeError as e:
            return {
                "description": "[錯誤] 回傳格式解析失敗",
                "items": [],
                "error_detail": f"JSON解析錯誤: {e}",
                "raw_content": content,
            }

    except requests.exceptions.RequestException as e:
        return {
            "description": f"[錯誤] 無法取得回應：{e}",
            "items": [],
            "error_detail": str(e),
        }


""" 回傳格式範例
{
  "description": "以甜美風格搭配白色穿搭及長髮，可選擇金色小巧耳環與珍珠項鍊，展現優雅與柔和氣質。",
  "items": [
    {"type": "耳環", "style": "簡約", "color": "銀色"},
    {"type": "項鍊", "style": "優雅", "color": "玫瑰金"}
  ]
}

"""
