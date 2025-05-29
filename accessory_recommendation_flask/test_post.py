import requests

"""
cd accessory_recommendation_flask
python test_post.py

"""
response = requests.post(
    "http://127.0.0.1:8000/receive",
    json={"style": "正式", "hairstyle": "長髮"},
)

print(response.status_code)
print(response.text)
