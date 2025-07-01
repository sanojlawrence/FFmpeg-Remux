extern "C" {
#include <jni.h>
#include <libavformat/avformat.h>
#include <libavutil/log.h>
#include <libavutil/error.h>
}

#include <mutex>

static JavaVM* g_vm = nullptr;
static jobject g_callback = nullptr;
static jmethodID onLog = nullptr;
static jmethodID onError = nullptr;
static jmethodID onComplete = nullptr;
static jmethodID onProgress = nullptr;

std::mutex callback_mutex;

void log_callback(void* ptr, int level, const char* fmt, va_list vl) {
    if (!g_vm || !onLog || !g_callback) return;

    JNIEnv* env = nullptr;
    bool needDetach = false;

    if (g_vm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6) == JNI_EDETACHED) {
        g_vm->AttachCurrentThread(&env, nullptr);
        needDetach = true;
    }

    char buffer[1024];
    vsnprintf(buffer, sizeof(buffer), fmt, vl);
    jstring msg = env->NewStringUTF(buffer);

    callback_mutex.lock();
    env->CallVoidMethod(g_callback, onLog, msg);
    callback_mutex.unlock();

    env->DeleteLocalRef(msg);
    if (needDetach) g_vm->DetachCurrentThread();
}

// ✅ Called from ffmpeg_main (you must wire it)
extern "C" void update_progress(float progress) {
    if (!g_vm || !onProgress || !g_callback) return;

    JNIEnv* env = nullptr;
    bool needDetach = false;

    if (g_vm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6) == JNI_EDETACHED) {
        g_vm->AttachCurrentThread(&env, nullptr);
        needDetach = true;
    }

    callback_mutex.lock();
    env->CallVoidMethod(g_callback, onProgress, progress);
    callback_mutex.unlock();

    if (needDetach) g_vm->DetachCurrentThread();
}

extern "C"
JNIEXPORT jint JNICALL
Java_com_jldevelopers_ffmpegremux_FFmpegLoader_nativeExecuteFFmpeg(JNIEnv *env, jclass clazz,
                                                                     jobjectArray args,
                                                                     jobject callback) {
    int argc = env->GetArrayLength(args);
    char** argv = new char*[argc + 1];
    for (int i = 0; i < argc; i++) {
        auto jstr = (jstring) env->GetObjectArrayElement(args, i);
        const char* raw = env->GetStringUTFChars(jstr, nullptr);
        argv[i] = strdup(raw);
        env->ReleaseStringUTFChars(jstr, raw);
        env->DeleteLocalRef(jstr);
    }
    argv[argc] = nullptr;

    env->GetJavaVM(&g_vm);
    g_callback = env->NewGlobalRef(callback);

    jclass cbClass = env->GetObjectClass(callback);
    onLog = env->GetMethodID(cbClass, "onLog", "(Ljava/lang/String;)V");
    onError = env->GetMethodID(cbClass, "onError", "(Ljava/lang/String;)V");
    onComplete = env->GetMethodID(cbClass, "onComplete", "(I)V");
    onProgress = env->GetMethodID(cbClass, "onProgress", "(F)V");

    av_log_set_callback(log_callback);

    int result = 0;
    extern int ffmpeg_main(int argc, char** argv);
    try {
        result = ffmpeg_main(argc, argv);
    } catch (...) {
        if (onError) {
            jstring errMsg = env->NewStringUTF("Native crash in ffmpeg_main");
            env->CallVoidMethod(g_callback, onError, errMsg);
            env->DeleteLocalRef(errMsg);
        }
        result = -1;
    }

    if (onComplete)
        env->CallVoidMethod(g_callback, onComplete, result);

    env->DeleteGlobalRef(g_callback);
    g_callback = nullptr;

    for (int i = 0; i < argc; ++i) free(argv[i]);
    delete[] argv;

    return result;
}