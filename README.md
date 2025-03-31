# ESP32Cam-FPGAProcessing

---

##  System Design Documentation

### **1. Project Overview**
The **ESP32Cam-FPGAProcessing** project aims to explore the implementation of FPGA image processing using ESP32 microcontrollers. The primary objective is to capture images using the ESP32-CAM, process these images on an FPGA, and then either store the processed images on an SD card, stream them via an IoT backend (Azure), or display them on a web interface.

### **2. Goals and Plans**
1. **Create Schematic and Barebones Design**: Develop the initial design and set up a web server to stream images.
   - Status: **DONE**
2. **Implement Real-Time Image Processing**: Develop and implement grayscale and Sobel filter functions.
   - Status: **Attempted, not fully implemented**
3. **Develop Image Processing Functions on PC**: Create image processing functions, web server, and HTML on a PC using C++ for easier testing and engineering.
   - Status: **Starting now**
4. **Port to MCU**: Transfer the developed functions to the microcontroller.
5. **Experiment and Implement on FPGA**: Test and implement the design on an FPGA using tools like Verilator.
6. **Finalize Design and Functionality**: Complete the project with all desired functionalities.

### **3. Hardware Design**
#### **ESP32-CAM Pinout**
| ESP32-CAM Pin | Function |
|--------------|------------|
| GPIO0 | Flash mode select |
| GPIO1, GPIO3 | UART for FPGA |
| GPIO4 | Built-in LED |
| GPIO13, GPIO15 | SPI to SD card |
| GPIO2 | External LED control |

#### **FPGA Pin Connections**
| FPGA Pin | Function |
|--------------|------------|
| UART_RX | Receive image data from ESP32 |
| UART_TX | Send processed image to ESP32 |
| SPI_MISO, MOSI | High-speed data transfer |

### **4. Image Processing Pipeline**
1. **Image Capture** (ESP32-CAM)
2. **Image Transmission** (UART/SPI to FPGA)
3. **Processing on FPGA** (Edge detection)
4. **Return Processed Image** to ESP32
5. **Store to SD card or upload to Azure**

### **5. FPGA Processing Algorithm**
#### **Edge Detection Algorithm (Sobel Operator)**
The Sobel operator is implemented on the FPGA for edge detection. The algorithm processes the image pixels to highlight the edges by computing the gradient magnitude.

### **6. Main Code (camera.ino)**
The `camera.ino` file is the main Arduino sketch that initializes the camera, captures images, and processes them. Below is a summary of the key functions and their purposes:

- **WiFi and HTTP Server Setup**: Connects to WiFi and sets up an HTTP server to stream images.
- **Grayscale Conversion**: Converts captured RGB images to grayscale.
- **Sobel Filter Application**: Applies the Sobel filter to detect edges in the grayscale image.
- **HTTP Stream Handler**: Handles HTTP requests to stream processed images.

#### **Key Functions Explained**
1. **setup()**: Initializes the camera, connects to WiFi, and starts the HTTP server.
2. **loop()**: Keeps the program running.
3. **convertToGrayscale()**: Converts RGB565 images to grayscale.
4. **applySobel()**: Applies the Sobel filter to a grayscale image to detect edges.
5. **stream_handler()**: Handles HTTP requests to stream the processed image frames.
6. **startCameraServer()**: Starts the HTTP server for streaming images.

### **Conclusion**
This project is a work in progress with the ultimate goal of integrating FPGA-based image processing with ESP32 microcontrollers. The current progress includes setting up the initial design, attempting real-time image processing, and starting the development of image processing functions on a PC. The next steps involve porting these functions to the MCU, experimenting with FPGA implementation, and finalizing the design.

For detailed system design documentation, please refer to [docs/system_design.md](docs/system_design.md).
