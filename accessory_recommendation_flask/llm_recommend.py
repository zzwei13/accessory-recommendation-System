import requests
from dotenv import load_dotenv
import os
from pathlib import Path

# 載入 .env
# 指定專案根目錄下的 .env
env_path = Path(__file__).resolve().parents[1] / ".env"
load_dotenv(dotenv_path=env_path)
TOGETHER_API_KEY = os.getenv("TOGETHER_API_KEY")
TOGETHER_MODEL = "mistralai/Mixtral-8x7B-Instruct-v0.1"  # 或 "mistral-7b-instruct"
TOGETHER_API_URL = (
    "https://api.together.xyz/v1/chat/completions"  # API URL 是固定的，不需放進 .env
)

# 假設產品資料
PRODUCTS = [
    {"id": "A01", "name": "金色愛心耳環", "tags": ["耳環", "金色", "甜美"]},
    {"id": "A02", "name": "銀色幾何手環", "tags": ["手環", "銀色", "簡約"]},
    {"id": "A03", "name": "金色珍珠項鍊", "tags": ["項鍊", "金色", "優雅"]},
    {"id": "A04", "name": "玫瑰金花朵耳環", "tags": ["耳環", "金色", "浪漫"]},
    {"id": "A05", "name": "銀色簡約戒指", "tags": ["戒指", "銀色", "簡約"]},
]


def get_llm_recommendation(color: str, style: str, hairstyle: str) -> str:
    system_prompt = (
        "你是一位時尚飾品顧問，根據使用者的穿搭顏色、風格與髮型，"
        "請推薦適合的飾品種類（如項鍊、耳環、手環、戒指等）、顏色（金色、銀色等）與風格（甜美、個性、優雅、簡約、華麗），"
        "並簡要說明搭配理由，語氣自然優雅，不需列點。"
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
        "max_tokens": 300,
    }

    response = requests.post(TOGETHER_API_URL, headers=headers, json=payload)

    if response.status_code == 200:
        return response.json()["choices"][0]["message"]["content"].strip()
    else:
        return f"[錯誤] 無法取得回應：{response.status_code}\n{response.text}"


def extract_keywords(text: str) -> list:
    """從 LLM 推薦語句中抽出關鍵字（簡易版本）"""
    keywords = []
    for kw in [
        "耳環",
        "項鍊",
        "手環",
        "戒指",
        "金色",
        "銀色",
        "黑色",
        "玫瑰金",
        "甜美",
        "優雅",
        "個性",
        "簡約",
        "浪漫",
        "華麗",
    ]:
        if kw in text:
            keywords.append(kw)
    return keywords


def find_matching_products(keywords: list) -> list:
    """依據關鍵詞找出相符產品"""
    matches = []
    for p in PRODUCTS:
        score = len(set(p["tags"]) & set(keywords))
        if score > 0:
            matches.append((p, score))
    # 按相似度排序
    matches.sort(key=lambda x: -x[1])
    return [p[0] for p in matches[:3]]  # 回傳前3個相符產品


# 測試整合
if __name__ == "__main__":
    color = "藍色"
    style = "浪漫"
    hairstyle = "短髮"

    recommendation = get_llm_recommendation(color, style, hairstyle)
    print("飾品推薦語句：\n", recommendation)

    keywords = extract_keywords(recommendation)
    print("\n抽取關鍵詞：", keywords)

    matches = find_matching_products(keywords)
    print("\n推薦產品清單：")
    for m in matches:
        print(f"產品編號：{m['id']}｜名稱：{m['name']}｜標籤：{m['tags']}")
