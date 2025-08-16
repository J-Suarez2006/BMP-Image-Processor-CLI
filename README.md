# BMP-Image-Processor-CLI

A lightweight **command-line interface (CLI) tool** for reading, describing, and processing **24-bit uncompressed BMP files**.  
Supports extracting image metadata, reading pixel data, and performing basic manipulations while adhering to the BMP file structure specification.

---

## 📌 Features

- **Read 24-bit BMP files** (uncompressed, BI_RGB format)
- **Display image metadata**:
  - File signature
  - Dimensions (width × height)
  - File size
  - Pixel data offset
  - Image orientation (top-down / bottom-up)
- **Basic image manipulation**:
  - Invert
  - Grayscale
  - Horizontal flip
  - Vertical flip
- **BMP file creation** using C++ fstream

