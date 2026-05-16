// ============================================================
// CompagnonV2 — net/http_client.cpp
// Groq API : STT (Whisper), Chat (LLaMA), TTS (PlayAI)
// WiFiClientSecure + bundle CA Mozilla
// ============================================================
#include "http_client.h"
#include "../storage/nvs_store.h"
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Arduino.h>

// Bundle CA intégré par ESP-IDF (activé via sdkconfig)
extern const uint8_t x509_crt_bundle_start[] asm("_binary_x509_crt_bundle_start");
extern const uint8_t x509_crt_bundle_end[]   asm("_binary_x509_crt_bundle_end");

#define GROQ_HOST         "api.groq.com"
#define GROQ_STT_URL      "https://api.groq.com/openai/v1/audio/transcriptions"
#define GROQ_CHAT_URL     "https://api.groq.com/openai/v1/chat/completions"
#define GROQ_TTS_URL      "https://api.groq.com/openai/v1/audio/speech"
#define GROQ_STT_MODEL    "whisper-large-v3-turbo"
#define GROQ_CHAT_MODEL   "llama-3.3-70b-versatile"
#define GROQ_TTS_MODEL    "playai-tts"
#define GROQ_TTS_VOICE    "Fritz-PlayAI"
#define HTTP_TIMEOUT_MS   10000

namespace HttpClient {

// ── Helper : client sécurisé avec bundle CA ───────────────────
static WiFiClientSecure _mkClient() {
    WiFiClientSecure c;
    c.setCACertBundle(x509_crt_bundle_start,
        x509_crt_bundle_end - x509_crt_bundle_start);
    return c;
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

    WiFiClientSecure client = _mkClient();
    HTTPClient http;
    http.begin(client, GROQ_STT_URL);
    http.setTimeout(HTTP_TIMEOUT_MS);
    http.addHeader("Authorization", "Bearer " + key);

    // Boundary multipart
    String boundary = "----CompagnonBoundary";
    String contentType = "multipart/form-data; boundary=" + boundary;
    http.addHeader("Content-Type", contentType);

    // Body
    String head = "--" + boundary + "\r\n";
    head += "Content-Disposition: form-data; name=\"file\"; filename=\"audio.wav\"\r\n";
    head += "Content-Type: audio/wav\r\n\r\n";
    String tail = "\r\n--" + boundary + "--\r\n";
    // model part
    String modelPart = "--" + boundary + "\r\n";
    modelPart += "Content-Disposition: form-data; name=\"model\"\r\n\r\n";
    modelPart += String(GROQ_STT_MODEL) + "\r\n";

    // Assembler dans un buffer
    size_t totalLen = modelPart.length() + head.length() + len + tail.length();
    uint8_t* body = (uint8_t*)malloc(totalLen);
    if (!body) { http.end(); return ""; }

    size_t pos = 0;
    memcpy(body + pos, modelPart.c_str(), modelPart.length()); pos += modelPart.length();
    memcpy(body + pos, head.c_str(), head.length());           pos += head.length();
    memcpy(body + pos, wav, len);                               pos += len;
    memcpy(body + pos, tail.c_str(), tail.length());

    int code = http.POST(body, totalLen);
    free(body);

    if (code != 200) {
        Serial.printf("[HTTP] STT error %d\n", code);
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

    WiFiClientSecure client = _mkClient();
    HTTPClient http;
    http.begin(client, GROQ_CHAT_URL);
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
        Serial.printf("[HTTP] Chat error %d\n", code);
        http.end();
        return "";
    }

    String resp = http.getString();
    http.end();

    JsonDocument doc;
    if (deserializeJson(doc, resp) != DeserializationError::Ok) return "";
    return doc["choices"][0]["message"]["content"] | String("");
}

// ── TTS — POST JSON → PCM binaire ────────────────────────────
std::vector<uint8_t> textToSpeech(const String& text) {
    String key = _apiKey();
    if (key.isEmpty()) return {};

    WiFiClientSecure client = _mkClient();
    HTTPClient http;
    http.begin(client, GROQ_TTS_URL);
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
        Serial.printf("[HTTP] TTS error %d\n", code);
        http.end();
        return {};
    }

    // Lire le corps binaire
    int len = http.getSize();
    std::vector<uint8_t> pcm;
    if (len > 0) pcm.reserve(len);

    WiFiClient* stream = http.getStreamPtr();
    uint8_t buf[512];
    while (http.connected() && (len > 0 || len == -1)) {
        size_t avail = stream->available();
        if (avail) {
            size_t toRead = min(avail, sizeof(buf));
            size_t rd = stream->readBytes(buf, toRead);
            pcm.insert(pcm.end(), buf, buf + rd);
            if (len != -1) len -= rd;
        } else {
            delay(1);
        }
    }
    http.end();
    Serial.printf("[HTTP] TTS received %d bytes\n", (int)pcm.size());
    return pcm;
}

} // namespace HttpClient
