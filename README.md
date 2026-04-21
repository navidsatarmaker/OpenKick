<div align="center">
  <h1>🎛️ OpenKick</h1>
  <p><strong>A hyper-lightweight, 100% open-source Volume Ducking and Transient Sidechain VST3 utility.</strong></p>
</div>

<br>

**OpenKick** gives you perfect sidechaining control with an absolutely pristine visual interface. Whether you need rapid EDM 4-on-the-floor ducking synced perfectly to your DAW, or surgical volume-shaping that reacts directly to external drum hits, OpenKick delivers flawlessly without bloating your project.

### Why OpenKick?
Most modern volume shaping plugins consume gigabytes of memory or hundreds of megabytes of RAM just for their complicated vector GUIs. **OpenKick uses approximately 37MB of RAM per instance.** You can comfortably drop 50+ instances of OpenKick across a dense mixing project without your computer breaking a sweat. It is purely non-destructive, zero-latency mathematics.

---

## 🔥 Key Features

*   **16 Precision Ducking Algorithms**: Features meticulously designed mathematical curves, including S-Curves, Block Ramps, Sine Pumps, Convex sweeps, and an interactive **Custom User Node** envelope.
*   **Dual Trigger Engines**:
    *   **Host Sync Mode**: Locks directly into your DAW’s internal clock (with `std::fmod` free-running decoupling) giving you flawless ducking synced at 1/8, 1/4, 1/4T (Triplets), 1/2T, and 1/1 Bars. 
    *   **Audio Trigger Mode**: Routes external audio (e.g., your separate drum bus) directly into the plugin. A high-speed transient envelope follower detects the hits and ducks your instrument on the fly—automatically resting back to full volume when the drums stop.
*   **Ghost Visualizer Arrays**: Features a dual-oscilloscope design. See your primary instrument waveform in bright white, while the ducking sidechain input audio renders translucently in the background to visually align your mix perfectly.
*   **Phase Shift Slider**: Instantly slide and offset the envelope’s phase relationship left or right to fix early or late kick transients without needing to manually nudge MIDI clips.
*   **Hardware GUI Architecture**: Hard-coded natively in JUCE C++ with direct vector graphics. Includes a restricted 2:1 geometric UI layout lock, dynamically scalable layout arrays, 60 FPS gravitational decay rendering, and 3D gradient radial components without requiring heavyweight graphics libraries.

---

## 💻 Build Instructions (CMake + JUCE)

OpenKick is built upon the JUCE framework using `FetchContent`. Compiling it from source is incredibly easy.

### Prerequisites:
*   [CMake](https://cmake.org/download/) (v3.21+)
*   A functional C++ Compiler (MSVC for Windows, Xcode for macOS, GCC/Clang for Linux)

### Compilation Steps:
1. Clone this repository to your local machine:
   ```bash
   git clone https://github.com/your-username/OpenKick.git
   cd OpenKick
   ```
2. Configure the CMake project. JUCE will automatically fetch down into your environment during this step:
   ```bash
   cmake -B build
   ```
3. Compile the VST3 and Standalone formats natively:
   ```bash
   cmake --build build --config Release
   ```
4. **Done!** The output `.vst3` file will be generated inside the `build/OpenKick_artefacts/Release/VST3` folder. Drop it into your DAW plugin directory and start producing.

---

## 🎨 Website & Pre-Compiled Releases
Want to see OpenKick in action or download the pre-compiled VST3 versions directly? 
*   **[Visit the OpenKick Website](https://openkick-plugin.netlify.app/)** (Replace with your actual Netlify domain!)

---

## ⚖️ License & Open Source
OpenKick is built purely as an educational, non-profit resource for the music production community. It is fundamentally aimed at reducing the barrier to entry for high-quality audio DSP mixing utilities.

Distributed under the **MIT License**. See `LICENSE` for more information.
