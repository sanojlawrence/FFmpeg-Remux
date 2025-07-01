package com.jldevelopers.ffmpegremux;

public class FFmpegLoader {
    static {
        System.loadLibrary("ffmpegwrapper");
    }

    public interface Callback {
        void onLog(String message);
        void onError(String message);
        void onProgress(float progress); // ✅ ADD THIS
        void onComplete(int result);
    }

    public static native int nativeExecuteFFmpeg(String[] args, Callback callback);

    public static void executeAsync(String[] args, Callback callback) {
        new Thread(() -> {
            try {
                int result = nativeExecuteFFmpeg(args, callback);
                callback.onComplete(result);
            } catch (Exception e) {
                callback.onError(e.getMessage());
            }
        }).start();
    }
}
