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


def get_llm_recommendation(color: str, style: str, hairstyle: str) -> str:
    system_prompt = (
        "你是一位時尚飾品顧問，根據使用者的穿搭顏色、風格與髮型，"
        "請推薦適合的飾品種類（如項鍊、耳環等）、顏色（金色、銀色等）與風格（甜美、個性、優雅、簡約、華麗），"
        "並簡要說明搭配理由。"
        "請以以下 JSON 格式回覆，不要有多餘文字：\n\n"
        "{\n"
        '  "description": "自然語言推薦說明，請用繁體中文",\n'
        '  "items": [\n'
        '    {"type": , "color": , "style": },\n'
        '    {"type": , "color": , "style": }\n'
        "  ]\n"
        "}"
    )

    user_prompt = f"穿搭顏色是「{color}」，風格是「{style}」，髮型是「{hairstyle}」。"

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
    {"type": "耳環", "color": "金色", "style": "甜美"},
    {"type": "項鍊", "color": "珍珠白", "style": "優雅"}
  ]
}

"""
