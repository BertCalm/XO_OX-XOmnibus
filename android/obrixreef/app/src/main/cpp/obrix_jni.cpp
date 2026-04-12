// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
//
// JNI bridge — exposes ObrixAudioEngine to Kotlin via native methods.
// The engine is a process-wide singleton, created on first startAudio().

#include "obrix_audio_engine.h"

#include <jni.h>
#include <android/log.h>
#include <memory>
#include <string>

#define LOG_TAG "ObrixJNI"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

static std::unique_ptr<obrix::ObrixAudioEngine> sEngine;

static obrix::ObrixAudioEngine& getEngine()
{
    if (!sEngine)
        sEngine = std::make_unique<obrix::ObrixAudioEngine>();
    return *sEngine;
}

extern "C"
{

// --- Lifecycle ---

JNIEXPORT jboolean JNICALL
Java_com_xoox_obrixreef_audio_ObrixNative_startAudio(JNIEnv*, jobject)
{
    return getEngine().start() ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT void JNICALL
Java_com_xoox_obrixreef_audio_ObrixNative_stopAudio(JNIEnv*, jobject)
{
    if (sEngine)
        sEngine->stop();
}

JNIEXPORT jboolean JNICALL
Java_com_xoox_obrixreef_audio_ObrixNative_isRunning(JNIEnv*, jobject)
{
    return (sEngine && sEngine->isRunning()) ? JNI_TRUE : JNI_FALSE;
}

// --- MIDI Input ---

JNIEXPORT void JNICALL
Java_com_xoox_obrixreef_audio_ObrixNative_noteOn(JNIEnv*, jobject, jint note, jfloat velocity)
{
    if (sEngine)
        sEngine->noteOn(note, velocity);
}

JNIEXPORT void JNICALL
Java_com_xoox_obrixreef_audio_ObrixNative_noteOff(JNIEnv*, jobject, jint note)
{
    if (sEngine)
        sEngine->noteOff(note);
}

JNIEXPORT void JNICALL
Java_com_xoox_obrixreef_audio_ObrixNative_allNotesOff(JNIEnv*, jobject)
{
    if (sEngine)
        sEngine->allNotesOff();
}

// --- Parameters ---

JNIEXPORT void JNICALL
Java_com_xoox_obrixreef_audio_ObrixNative_setParameter(JNIEnv*, jobject, jint index, jfloat value)
{
    if (sEngine)
        sEngine->setParameter(index, value);
}

JNIEXPORT jfloat JNICALL
Java_com_xoox_obrixreef_audio_ObrixNative_getParameter(JNIEnv*, jobject, jint index)
{
    return sEngine ? sEngine->getParameter(index) : 0.0f;
}

JNIEXPORT jint JNICALL
Java_com_xoox_obrixreef_audio_ObrixNative_getParameterCount(JNIEnv*, jobject)
{
    return getEngine().getParameterCount();
}

JNIEXPORT jstring JNICALL
Java_com_xoox_obrixreef_audio_ObrixNative_getParameterIdAt(JNIEnv* env, jobject, jint index)
{
    auto id = getEngine().getParameterIdAt(index);
    return env->NewStringUTF(id.c_str());
}

// --- Preset Loading ---

JNIEXPORT void JNICALL
Java_com_xoox_obrixreef_audio_ObrixNative_loadPresetJson(JNIEnv* env, jobject, jstring json)
{
    // For now, accept a simple "id=value\n" text format
    // Full .xometa JSON parsing will be added in Phase 3
    (void)env;
    (void)json;
    LOGI("loadPresetJson: not yet implemented (Phase 3)");
}

// --- Queries ---

JNIEXPORT jint JNICALL
Java_com_xoox_obrixreef_audio_ObrixNative_getActiveVoiceCount(JNIEnv*, jobject)
{
    return sEngine ? sEngine->getActiveVoiceCount() : 0;
}

JNIEXPORT jdouble JNICALL
Java_com_xoox_obrixreef_audio_ObrixNative_getSampleRate(JNIEnv*, jobject)
{
    return sEngine ? sEngine->getSampleRate() : 48000.0;
}

// --- Raw MIDI (for MidiInputHandler) ---

JNIEXPORT void JNICALL
Java_com_xoox_obrixreef_audio_ObrixNative_sendMidiBytes(
    JNIEnv* env, jobject, jbyteArray data, jint offset, jint count)
{
    if (!sEngine || count < 2) return;

    auto* bytes = env->GetByteArrayElements(data, nullptr);
    if (!bytes) return;

    uint8_t status = static_cast<uint8_t>(bytes[offset]);
    uint8_t d1 = count > 1 ? static_cast<uint8_t>(bytes[offset + 1]) : 0;
    uint8_t d2 = count > 2 ? static_cast<uint8_t>(bytes[offset + 2]) : 0;

    uint8_t type = status & 0xF0;

    if (type == 0x90 && d2 > 0)
        sEngine->noteOn(d1, static_cast<float>(d2) / 127.0f);
    else if (type == 0x80 || (type == 0x90 && d2 == 0))
        sEngine->noteOff(d1);
    else if (type == 0xB0 && d1 == 123)
        sEngine->allNotesOff();

    env->ReleaseByteArrayElements(data, bytes, JNI_ABORT);
}

} // extern "C"
