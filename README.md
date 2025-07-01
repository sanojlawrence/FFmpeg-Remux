# FFmpeg-Remux

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

```bash
git clone https://github.com/sanojlawrence/FFmpeg-Remux.git
cd FFmpeg-Remux
⚠️ Note: FFmpeg source files are not included in this repository to reduce size. The project is set up to use prebuilt .so files, and the ffmpeg source folder is excluded via .gitignore.

FFmpeg Libraries
The project uses prebuilt FFmpeg .so libraries placed under:

css
Copy
Edit
app/src/main/jniLibs/<ABI>/
You can build these yourself from FFmpeg sources (configured with only the needed components, e.g., demuxing/muxing support and GPL-free options) or use precompiled libraries.

Build
Open the project in Android Studio.

Let Gradle sync and finish indexing.

Select a connected device or emulator.

Click Run.

⚙️ Usage
Open the app on your Android device.

Select an input video file.

Choose which video and audio tracks to keep.

Tap Start Remux.

After completion, find the output file in the specified output folder.

💬 FFmpeg Commands
The app uses FFmpeg commands similar to:

bash
Copy
Edit
ffmpeg -i input.mkv -map 0:v:0 -map 0:a:0 -c copy output.mp4
-map options to select specific tracks

-c copy to copy streams without re-encoding

🏷️ Project Structure
bash
Copy
Edit
app/
├── java/com/jldevelopers/ffmpegremux/
│   ├── FFmpegLoader.java       # JNI bridge to call FFmpeg
│   ├── MainActivity.java       # Main UI logic
│   └── utils/                  # Utility classes
├── jniLibs/                    # Prebuilt FFmpeg native libraries
├── res/
│   └── layout, drawable, etc.
└── AndroidManifest.xml
🛡️ License
This project is licensed under the MIT License. See the LICENSE file for details.

💙 Contributing
Contributions, issues, and feature requests are welcome!
Feel free to open an issue or submit a pull request.

🙏 Acknowledgements
FFmpeg — the powerful multimedia framework

Android NDK and JNI documentation

📧 Contact
If you'd like to collaborate or have questions, open an issue or contact me via GitHub profile.

yaml
Copy
Edit
