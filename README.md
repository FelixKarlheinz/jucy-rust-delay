<img width="581" height="347" alt="JucyRustDelay" src="https://github.com/user-attachments/assets/ed0f533b-3600-4bfc-8837-0e8c24496938" />

# JucyRustDelay



A small experimental audio plugin exploring **Rust-based DSP integrated into a JUCE plugin**.

This repository is combining JUCE for the plugin layer with Rust for the DSP core via CXX bindings.

It's build on top of the great example for Rust integration into JUCE from [steckes](https://github.com/steckes). 
The implementation of the delay is mostly drawn from the [rust_community](https://github.com/RustAudio) with small edits.  

---

## Features

- JUCE-based VST3 / AU plugin
- DSP core written in **Rust**
- C++ ↔ Rust integration using **cxx**
- Delay effect with subtle saturation, drift and noise
- Cross-platform build via **CMake + Cargo**

---

## Building

### Prerequisites

- **CMake ≥ 3.22**
- **Rust (stable toolchain)**
- **JUCE dependencies** (fetched automatically)
- **C++17 compiler**
- **Ninja** (recommended)

### Build steps

```bash
git clone https://github.com/FelixKarlheinz/jucy-rust-delay.git
cd jucy-rust-delay

cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
