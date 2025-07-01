# FFmpeg-Remux

FFmpeg-Remux is an Android application that uses native FFmpeg libraries to remux (repackage) video and audio streams without re-encoding. It allows you to extract, copy, and repack video/audio tracks into a new container efficiently, preserving quality and saving time.

---

## ✨ Features

- Remux video files without re-encoding (`-c copy` support)
- Select video and audio tracks to keep
- Reduce file size while maintaining original quality
- Simple Android interface
- Uses custom-built FFmpeg native `.so` libraries

---

## 💡 What is remuxing?

Remuxing means changing the container format (e.g., from `.mkv` to `.mp4`) without altering the actual audio and video streams. It is faster and preserves original quality since it avoids re-encoding.

---

## 🚀 Getting Started

### Prerequisites

- Android Studio (Hedgehog or newer recommended)
- Android NDK (version 27 or compatible)
- Java 17+
- Minimum Android SDK: 21 (Lollipop)

---

### Clone the repository

git clone https://github.com/sanojlawrence/FFmpeg-Remux.git
cd FFmpeg-Remux
⚠️ Note: FFmpeg source files are not included in this repository to reduce size. The project is set up to use prebuilt .so files, and the ffmpeg source folder is excluded via .gitignore.

FFmpeg Libraries
The project uses prebuilt FFmpeg .so libraries placed under:

app/src/main/jniLibs/<ABI>/
You can build these yourself from FFmpeg sources (configured with only the needed components, e.g., demuxing/muxing support and GPL-free options) or use precompiled libraries.


⚡ Using the FFmpeg AAR dependency
If you'd like to include the prebuilt FFmpeg .aar as a dependency instead of including .so files manually, you can use:

```
implementation 'com.jldevelopers:ffmpegremux:1.0'
```
However, this library is currently only published to your local Maven repository (mavenLocal()), so you need to add this in your build.gradle:

```
repositories {
    mavenLocal()
    google()
    mavenCentral()
}
```

## ⚙️ Usage
🏃 Example Usage
Below is an example of how to remux a video file using FFmpegLoader:

```
String path = " ";        // Input file path
int audioTrack = 1;       // Audio track number to extract
String dest = "";         // Output file path

String[] ffmpegCommand = new String[] {
    "ffmpeg",
    "-i", path,
    "-map", "0:v:0",
    "-map", "0:a:" + audioTrack,
    "-c", "copy",
    dest
};

FFmpegLoader.executeAsync(ffmpegCommand, new FFmpegLoader.Callback() {
    @Override
    public void onComplete(int returnCode) {
        Log.d("FFmpegDebug", "Completed with code: " + returnCode);
    }

    @Override
    public void onProgress(float progress) {
        Log.d("FFmpegDebug", "Progress: " + progress);
    }

    @Override
    public void onLog(String logLine) {
        Log.d("FFmpegDebug", "Log: " + logLine);
    }

    @Override
    public void onError(String error) {
        Log.d("FFmpegDebug", "Error: " + error);
    }
});
```
## Explanation
-map 0:v:0 — Selects the first video track from the input.

-map 0:a:1 — Selects the second audio track (index starts from 0).

-c copy — Copies streams without re-encoding.

FFmpegLoader.executeAsync(...) — Runs FFmpeg asynchronously and returns callbacks for progress, logs, completion, and errors.

After completion, find the output file in the specified output folder.

## 💬 FFmpeg Commands
The app uses FFmpeg commands similar to:

```

ffmpeg -i input.mkv -map 0:v:0 -map 0:a:0 -c copy output.mp4
-map options to select specific tracks

-c copy to copy streams without re-encoding
```
##  🏷️ Project Structure
```
app/
├── java/com/jldevelopers/ffmpegremux/
│   ├── FFmpegLoader.java       # JNI bridge to call FFmpeg
│   ├── MainActivity.java       # Main UI logic
│   └── utils/                  # Utility classes
├── jniLibs/                    # Prebuilt FFmpeg native libraries
├── res/
│   └── layout, drawable, etc.
└── AndroidManifest.xml
```

## 💙 Contributing
Contributions, issues, and feature requests are welcome!
Feel free to open an issue or submit a pull request.

## 🙏 Acknowledgements
FFmpeg — the powerful multimedia framework

## Android NDK and JNI documentation

📧 Contact
If you'd like to collaborate or have questions, open an issue or contact me via GitHub profile.
