#include <WiFi.h>                  // Include WiFi library for network connectivity.
#include "esp_http_server.h"       // Include the HTTP server library.
#include "esp_timer.h"             // Include the timer library (for timing operations).
#include "esp_camera.h"            // Include the camera library (for ESP32-CAM).
 
// Define the camera model being used.
// The AI Thinker model is one of the most popular ESP32-CAM modules.
#define CAMERA_MODEL_AI_THINKER
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM     0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27

#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM       5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22 
 
// Replace these with your WiFi network credentials.
const char* ssid = "iPhoneAJ2025";      // Your Wi-Fi network name.
const char* password = "aedanjaeson";  // Your Wi-Fi password.
 
// ------------------------------
// HTTP Streaming Definitions
// ------------------------------
// Define a boundary string used in multipart HTTP responses.
// This boundary separates individual JPEG frames in the stream.
#define PART_BOUNDARY "123456789000000000000987654321"
 
// Define the MIME type for the HTTP response as multipart MJPEG.
// It tells the browser that the content is a continuous stream of JPEG images.
static const char* _STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=" PART_BOUNDARY;
 
// Define the boundary string with newline characters as required by HTTP.
static const char* _STREAM_BOUNDARY = "\r\n--" PART_BOUNDARY "\r\n";
 
// Define a template for each JPEG part header. %u will be replaced with the JPEG size.
static const char* _STREAM_PART = "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";
 
// ------------------------------
// Global HTTP Server Variables
// ------------------------------
// This handle is used to manage the HTTP server for streaming.
httpd_handle_t stream_httpd = NULL;
 
// ------------------------------
// HTTP Stream Handler Function
// ------------------------------
// This function is the core of the streaming process. It runs each time a client
// connects to the streaming URI and then repeatedly sends JPEG frames as part of a 
// multipart HTTP response.
static esp_err_t stream_handler(httpd_req_t *req) {
  camera_fb_t * fb = NULL;         // Pointer to the frame buffer received from the camera.
  esp_err_t res = ESP_OK;           // Variable to store the result of HTTP functions.
  size_t _jpg_buf_len = 0;          // This will hold the length of the JPEG image buffer.
  uint8_t * _jpg_buf = NULL;        // Pointer to the buffer that holds JPEG data.
  char * part_buf[64];              // Temporary buffer for constructing HTTP header strings.
 
  // Set the content type of the HTTP response to our defined multipart MJPEG type.
  res = httpd_resp_set_type(req, _STREAM_CONTENT_TYPE);
  if(res != ESP_OK){
    return res; // If setting the header fails, exit the handler.
  }
 
  // Set HTTP header to allow cross-origin requests. This is important if your UI is hosted on a different domain.
  httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
 
  // Begin an infinite loop that will continuously capture frames from the camera
  // and send them as part of the MJPEG stream.
  while(true){
    // Capture a frame from the camera.
    fb = esp_camera_fb_get();
    if (!fb) {
      Serial.println("Camera capture failed"); // Print error if frame capture fails.
      res = ESP_FAIL;
    } else {
      // Check if the frame is already in JPEG format.
      if(fb->format != PIXFORMAT_JPEG){
        // If not, convert the frame to JPEG using the frame2jpg() function.
        // The quality parameter is set to 80.

        // uint16_t *img565 = (uint16_t *)fb->buf;
        // convertToGrayscale(img565, fb->width, fb->height);


        bool jpeg_converted = frame2jpg(fb, 80, &_jpg_buf, &_jpg_buf_len);
        // After conversion, return the frame buffer to free up memory.
        esp_camera_fb_return(fb);
        fb = NULL;
        // If JPEG conversion failed, log the error.
        if(!jpeg_converted){
          Serial.println("JPEG compression failed");
          res = ESP_FAIL;
        }
      } else {
        // If the frame is already JPEG, simply set the buffer pointer and length.

        _jpg_buf_len = fb->len; // JPEG length equals the captured frame length.
        _jpg_buf = fb->buf;     // Buffer pointer is provided by the camera driver.
      }
    }
 
    // If no error has occurred so far, prepare to send the captured JPEG frame.
    if(res == ESP_OK){
      // Format the part header string with the JPEG buffer length.
      // snprintf writes the formatted string into part_buf.
      size_t hlen = snprintf((char *)part_buf, 64, _STREAM_PART, _jpg_buf_len);
      // Send the part header as a chunk.
      res = httpd_resp_send_chunk(req, (const char *)part_buf, hlen);
    }
    if(res == ESP_OK){
      // Send the JPEG image data as the next chunk.
      res = httpd_resp_send_chunk(req, (const char *)_jpg_buf, _jpg_buf_len);
    }
    if(res == ESP_OK){
      // Send the boundary string that marks the end of this JPEG part.
      res = httpd_resp_send_chunk(req, _STREAM_BOUNDARY, strlen(_STREAM_BOUNDARY));
    }
 
    // Memory management:
    // If fb is still valid (in some error cases), return it.
    if(fb){
      esp_camera_fb_return(fb); // Free the frame buffer.
      fb = NULL;
      _jpg_buf = NULL;
    } else if(_jpg_buf){
      // If we allocated a JPEG buffer (via frame2jpg), free it.
      free(_jpg_buf);
      _jpg_buf = NULL;
    }
 
    // If any of the HTTP send operations failed, break out of the loop.
    if(res != ESP_OK){
      break;
    }
  }
  return res; // Return the final result.
}
 
void convertToGrayscale(uint16_t* image, int width, int height) {
  int totalPixels = width * height;
  for (int i = 0; i < totalPixels; i++) {
    // Each pixel in RGB565 is 16 bits:
    // Bits [15:11] = Red (5 bits), [10:5] = Green (6 bits), [4:0] = Blue (5 bits)
    uint16_t pixel = image[i];
    // Extract red, green, and blue components.
    uint8_t r = (pixel >> 11) & 0x1F;   // 5 bits for red.
    uint8_t g = (pixel >> 5) & 0x3F;    // 6 bits for green.
    uint8_t b = pixel & 0x1F;           // 5 bits for blue.
    
    // Scale the components to 8-bit values.
    r = (r * 255) / 31;
    g = (g * 255) / 63;
    b = (b * 255) / 31;
    
    // Calculate the grayscale value using a weighted average.
    // Common weights are 0.299 for red, 0.587 for green, 0.114 for blue.
    uint8_t gray = (uint8_t)((r * 30 + g * 59 + b * 11) / 100);
    
    // Convert the 8-bit grayscale back to RGB565 format.
    // Scale the 8-bit gray value to 5 bits for red and blue, 6 bits for green.
    uint8_t r5 = (gray * 31) / 255;
    uint8_t g6 = (gray * 63) / 255;
    uint8_t b5 = (gray * 31) / 255;
    
    // Combine the scaled components into a single RGB565 pixel.
    image[i] = (r5 << 11) | (g6 << 5) | b5;
  }
}


// ------------------------------
// Start Camera Server Function
// ------------------------------
// This function initializes and starts the HTTP server that will serve the stream.
void startCameraServer() {
  // Get the default HTTP server configuration.
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
 
  // Create a URI structure for the stream handler.
  // When a client accesses the root path "/", the stream_handler function is called.
  httpd_uri_t stream_uri = {
      .uri       = "/",              // The URL path.
      .method    = HTTP_GET,         // HTTP GET request.
      .handler   = stream_handler,   // Function to handle requests.
      .user_ctx  = NULL              // No user context needed.
  };
 
  // Start the HTTP server with the given configuration.
  if (httpd_start(&stream_httpd, &config) == ESP_OK) {
      // Register the URI handler so that requests to "/" are handled by stream_handler.
      httpd_register_uri_handler(stream_httpd, &stream_uri);
  }
}
 
// ------------------------------
// Setup Function
// ------------------------------
// The setup() function runs once at startup. It initializes Serial, WiFi, the camera,
// and finally starts the camera server.
void setup() {
  Serial.begin(115200);         // Initialize Serial communication for debugging.
 
  // Begin WiFi connection using the provided credentials.
  WiFi.begin(ssid, password);
  // Wait in a loop until the WiFi connection is established.
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
 
  // ------------------------------
  // Camera Configuration
  // ------------------------------
  // Create and configure a camera_config_t structure that holds camera settings.
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_RGB565;
 
  // Reduce frame size to VGA to avoid memory issues
  config.frame_size = FRAMESIZE_VGA;
  config.jpeg_quality = 12;
  config.fb_count = 1;
 
  // Initialize the camera with the specified configuration.
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    // If initialization fails, print the error code.
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }
 
  // Start the camera server to begin streaming.
  startCameraServer();
 
  // Print the local IP address so you know where to connect.
  Serial.print("Camera Ready! Use 'http://");
  Serial.print(WiFi.localIP());
  Serial.println("' to connect");
}
 
// ------------------------------
// Loop Function
// ------------------------------
// The loop() function runs repeatedly after setup().
// Here we only add a minimal delay to let the HTTP server handle incoming requests.
void loop() {
  delay(1); // Minimal delay to free up CPU cycles.
}
