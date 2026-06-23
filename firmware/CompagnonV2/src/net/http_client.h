// ============================================================
// CompagnonV2 — net/http_client.h
// Clients HTTPS vers Groq API
//   transcribeAudio  → Whisper STT
//   chatCompletion   → LLaMA chat
//   textToSpeech     → Groq TTS → PCM/WAV brut
// Clé API lue depuis NVS (namespace "app", clé "groq_api_key")
// ============================================================
#pragma once
#include <Arduino.h>
#include <vector>

namespace HttpClient {

// POST WAV multipart → transcription texte ("" si erreur)
String transcribeAudio(const uint8_t* wav, size_t len);

// POST JSON chat → réponse texte ("" si erreur)
String chatCompletion(const String& prompt);

// POST JSON TTS → vecteur PCM 16-bit 24kHz (vide si erreur)
std::vector<uint8_t> textToSpeech(const String& text);

} // namespace HttpClient
