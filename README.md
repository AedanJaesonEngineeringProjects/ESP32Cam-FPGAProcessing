# ESP32Cam-FPGAProcessing

---

## **📘 System Design Documentation (docs/system_design.md)**

### **1. System Overview**
The **ESP32-CAM** is used as the main controller and image capture device, while an **FPGA** performs high-speed image processing. Processed images are either:
- **Stored** on an SD card
- **Streamed** via an IoT backend (Azure)
- **Displayed** on a web interface

### **2. Hardware Design**
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

### **3. Image Processing Pipeline**
1. **Image Capture** (ESP32-CAM)
2. **Image Transmission** (UART/SPI to FPGA)
3. **Processing on FPGA** (Edge detection)
4. **Return Processed Image** to ESP32
5. **Store to SD card or upload to Azure**

### **4. FPGA Processing Algorithm**
#### **Edge Detection Algorithm (Sobel Operator)**
```verilog
module edge_detection (
    input wire clk,
    input wire [7:0] pixel_in,
    output wire [7:0] pixel_out
);
  // Implement Sobel operator here
endmodule