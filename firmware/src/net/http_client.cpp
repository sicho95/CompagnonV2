// ============================================================
// CompagnonV2 — net/http_client.cpp
// fix: setCACertBundle(esp_crt_bundle_attach) → setInsecure()
// fix: WiFiClient* → NetworkClient* (esp32 3.3.x)
// fix #7  — WiFiClientSecure static, pas de copie par valeur
// fix #8  — timeout global sur lecture stream TTS
// R1      — vTaskDelay(1) au lieu de delay(1) dans boucle TTS
// ============================================================
#include "http_client.h"
#include "../storage/nvs_store.h"
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Arduino.h>
#include <NetworkClient.h>

#define GROQ_STT_URL    "https://api.groq.com/openai/v1/audio/transcriptions"
#define GROQ_CHAT_URL   "https://api.groq.com/openai/v1/chat/completions"
#define GROQ_TTS_URL    "https://api.groq.com/openai/v1/audio/speech"
#define GROQ_STT_MODEL  "whisper-large-v3-turbo"
#define GROQ_CHAT_MODEL "llama-3.3-70b-versatile"
#define GROQ_TTS_MODEL  "playai-tts"
#define GROQ_TTS_VOICE  "Fritz-PlayAI"
#define HTTP_TIMEOUT_MS       10000
#define TTS_STREAM_TIMEOUT_MS 15000

namespace HttpClient {

// fix #7 — client static, jamais copié
static WiFiClientSecure _client;

static void _configureClient() {
    // fix: setCACertBundle(esp_crt_bundle_attach) ne compile plus en esp32 3.3.x
    _client.setInsecure();
}

static String _apiKey() {
    return NvsStore::getString("app", "groq_api_key", "");
}

// ── STT — POST multipart/form-data WAV ───────────────────────
String transcribeAudio(const uint8_t* wav, size_t len) {
    String key = _apiKey();
    if (key.isEmpty()) {
        Serial.println("[HTTP] No Groq API key");
        return "";
    }
    _configureClient();
    HTTPClient http;
    http.begin(_client, GROQ_STT_URL);
    http.setTimeout(HTTP_TIMEOUT_MS);
    http.addHeader("Authorization", "Bearer " + key);

    String boundary = "----CompagnonBoundary";
    http.addHeader("Content-Type", "multipart/form-data; boundary=" + boundary);

    String modelPart = "--" + boundary + "\r\n";
    modelPart += "Content-Disposition: form-data; name=\"model\"\r\n\r\n";
    modelPart += String(GROQ_STT_MODEL) + "\r\n";

    String head = "--" + boundary + "\r\n";
    head += "Content-Disposition: form-data; name=\"file\"; filename=\"audio.wav\"\r\n";
    head += "Content-Type: audio/wav\r\n\r\n";

    String tail = "\r\n--" + boundary + "--\r\n";

    size_t totalLen = modelPart.length() + head.length() + len + tail.length();
    uint8_t* body = (uint8_t*)malloc(totalLen);
    if (!body) { http.end(); return ""; }

    size_t pos = 0;
    memcpy(body + pos, modelPart.c_str(), modelPart.length()); pos += modelPart.length();
    memcpy(body + pos, head.c_str(),      head.length());      pos += head.length();
    memcpy(body + pos, wav,               len);                pos += len;
    memcpy(body + pos, tail.c_str(),      tail.length());

    int code = http.POST(body, totalLen);
    free(body);

    if (code != 200) {
        Serial.printf("[HTTP] STT error %d: %s\n", code, http.getString().c_str());
        http.end();
        return "";
    }
    String resp = http.getString();
    http.end();

    JsonDocument doc;
    if (deserializeJson(doc, resp) != DeserializationError::Ok) return "";
    return doc["text"] | String("");
}

// ── Chat — POST JSON ──────────────────────────────────────────
String chatCompletion(const String& prompt) {
    String key = _apiKey();
    if (key.isEmpty()) return "";

    _configureClient();
    HTTPClient http;
    http.begin(_client, GROQ_CHAT_URL);
    http.setTimeout(HTTP_TIMEOUT_MS);
    http.addHeader("Authorization", "Bearer " + key);
    http.addHeader("Content-Type",  "application/json");

    JsonDocument req;
    req["model"] = GROQ_CHAT_MODEL;
    JsonArray msgs = req["messages"].to<JsonArray>();
    JsonObject msg = msgs.add<JsonObject>();
    msg["role"]    = "user";
    msg["content"] = prompt;

    String body;
    serializeJson(req, body);

    int code = http.POST(body);
    if (code != 200) {
        Serial.printf("[HTTP] Chat error %d: %s\n", code, http.getString().c_str());
        http.end();
        return "";
    }
    String resp = http.getString();
    http.end();

    JsonDocument doc;
    if (deserializeJson(doc, resp) != DeserializationError::Ok) return "";
    return doc["choices"][0]["message"]["content"] | String("");
}

// ── TTS — POST JSON → PCM/WAV binaire ────────────────────────
std::vector<uint8_t> textToSpeech(const String& text) {
    String key = _apiKey();
    if (key.isEmpty()) return {};

    _configureClient();
    HTTPClient http;
    http.begin(_client, GROQ_TTS_URL);
    http.setTimeout(HTTP_TIMEOUT_MS);
    http.addHeader("Authorization", "Bearer " + key);
    http.addHeader("Content-Type",  "application/json");
    http.addHeader("Accept",        "audio/wav");

    JsonDocument req;
    req["model"] = GROQ_TTS_MODEL;
    req["input"] = text;
    req["voice"] = GROQ_TTS_VOICE;

    String body;
    serializeJson(req, body);

    int code = http.POST(body);
    if (code != 200) {
        Serial.printf("[HTTP] TTS error %d: %s\n", code, http.getString().c_str());
        http.end();
        return {};
    }

    // fix: NetworkClient* au lieu de WiFiClient* (esp32 3.3.x)
    int contentLen = http.getSize();
    std::vector<uint8_t> pcm;
    if (contentLen > 0) pcm.reserve((size_t)contentLen);

    NetworkClient* stream = http.getStreamPtr();
    uint8_t buf[512];
    unsigned long t0 = millis();
    int remaining = contentLen;

    while (http.connected() && (remaining > 0 || contentLen == -1)) {
        if (millis() - t0 > TTS_STREAM_TIMEOUT_MS) {
            Serial.println("[HTTP] TTS stream timeout");
            break;
        }
        size_t avail = stream->available();
        if (avail) {
            size_t toRead = min(avail, sizeof(buf));
            size_t rd = stream->readBytes(buf, toRead);
            pcm.insert(pcm.end(), buf, buf + rd);
            if (contentLen != -1) remaining -= (int)rd;
        } else {
            vTaskDelay(pdMS_TO_TICKS(1));
        }
    }
    http.end();
    Serial.printf("[HTTP] TTS received %d bytes\n", (int)pcm.size());
    return pcm;
}

} // namespace HttpClient
