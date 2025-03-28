#include <WiFi.h>
#include "esp_http_server.h"
#include "esp_timer.h"
#include "esp_camera.h"

// Define the camera model being used.
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
const char* ssid = "wifi";
const char* password = "password";

// HTTP Streaming Definitions
#define PART_BOUNDARY "123456789000000000000987654321"
static const char* _STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=" PART_BOUNDARY;
static const char* _STREAM_BOUNDARY = "\r\n--" PART_BOUNDARY "\r\n";
static const char* _STREAM_PART = "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";

httpd_handle_t stream_httpd = NULL;

// Function to convert RGB565 image to grayscale
void convertToGrayscale(uint8_t* image, int width, int height) {
  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
      uint16_t pixel = ((uint16_t*)image)[y * width + x];
      uint8_t r = (pixel >> 11) & 0x1F;
      uint8_t g = (pixel >> 5) & 0x3F;
      uint8_t b = pixel & 0x1F;

      // Convert to 8-bit values
      r = (r << 3) | (r >> 2);
      g = (g << 2) | (g >> 4);
      b = (b << 3) | (b >> 2);

      // Calculate grayscale value
      uint8_t gray = (r * 30 + g * 59 + b * 11) / 100;

      // Convert back to RGB565
      uint16_t gray_pixel = ((gray >> 3) << 11) | ((gray >> 2) << 5) | (gray >> 3);

      ((uint16_t*)image)[y * width + x] = gray_pixel;
    }
  }
}

//Function to apply Sobel filter on 8-bit grayscale image
//Computes the edge strength at each pixel by looking at the brightness changes
// in a 3x3 neighborhood using two small matrices (kernels) for horizontal and vertical gradients.

uint8_t* applySobel(uint8_t* gray, int width, int height) {
  int numPixels = width * height;
  // Allocate memory for the output edge image (8 bits per pixel).
  
  uint8_t* edges = (uint8_t*) ps_malloc(numPixels);
  if (!edges) return NULL;
  
  // Initialize the output buffer with zeros.
  memset(edges, 0, numPixels);
  
  // Define Sobel kernels.
  int Gx[3][3] = {
    { -1,  0,  1 },
    { -2,  0,  2 },
    { -1,  0,  1 }
  };
  int Gy[3][3] = {
    { -1, -2, -1 },
    {  0,  0,  0 },
    {  1,  2,  1 }
  };
  
  // Process each pixel, skipping the border (since a 3x3 neighborhood is required).
  for (int y = 1; y < height - 1; y++) {
    for (int x = 1; x < width - 1; x++) {
      int sumX = 0, sumY = 0;
      // For each pixel, apply the 3x3 kernel.
      for (int j = -1; j <= 1; j++) {
        for (int i = -1; i <= 1; i++) {
          // Get the brightness value from the grayscale image.
          int pixel = gray[(y + j) * width + (x + i)];
          sumX += Gx[j + 1][i + 1] * pixel;
          sumY += Gy[j + 1][i + 1] * pixel;
        }
      }
      // Combine horizontal and vertical gradients.
      // Using the sum of absolute values for speed.
      int magnitude = abs(sumX) + abs(sumY);
      // Clamp the result to the 0-255 range.
      if (magnitude > 255) magnitude = 255;
      edges[y * width + x] = (uint8_t) magnitude;
    }
  }
  
  return edges;
}

// HTTP Stream Handler Function
static esp_err_t stream_handler(httpd_req_t *req) {
  camera_fb_t * fb = NULL;
  esp_err_t res = ESP_OK;
  size_t _jpg_buf_len = 0;
  uint8_t * _jpg_buf = NULL;
  char * part_buf[64];

  res = httpd_resp_set_type(req, _STREAM_CONTENT_TYPE);
  if(res != ESP_OK){
    return res;
  }

  httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");

  while(true){
    fb = esp_camera_fb_get();
    if (!fb) {
      Serial.println("Camera capture failed");
      res = ESP_FAIL;
    } else {
      if(fb->format == PIXFORMAT_GRAYSCALE){
        // Convert the frame to grayscale
        //convertToGrayscale(fb->buf, fb->width, fb->height);
        // Convert the grayscale frame to JPEG
        uint8_t* edgeImage = applySobel(fb->buf, fb->width, fb->height);
        if(edgeImage == NULL){
          Serial.println("Failed to apply Sobel filter");
          res = ESP_FAIL;
        } else{
        bool jpeg_converted = frame2jpg(fb, 80, &_jpg_buf, &_jpg_buf_len);
          free(edgeImage);
          if(!jpeg_converted){
            Serial.println("JPEG compression failed");
            res = ESP_FAIL;
          }
        }
      } else {
        _jpg_buf_len = fb->len;
        _jpg_buf = fb->buf;
      }
    }

    if(res == ESP_OK){
      size_t hlen = snprintf((char *)part_buf, 64, _STREAM_PART, _jpg_buf_len);
      res = httpd_resp_send_chunk(req, (const char *)part_buf, hlen);
    }
    if(res == ESP_OK){
      res = httpd_resp_send_chunk(req, (const char *)_jpg_buf, _jpg_buf_len);
    }
    if(res == ESP_OK){
      res = httpd_resp_send_chunk(req, _STREAM_BOUNDARY, strlen(_STREAM_BOUNDARY));
    }

    if(fb){
      esp_camera_fb_return(fb);
      fb = NULL;
      _jpg_buf = NULL;
    } else if(_jpg_buf){
      free(_jpg_buf);
      _jpg_buf = NULL;
    }

    if(res != ESP_OK){
      break;
    }
  }
  return res;
}

// Start Camera Server Function
void startCameraServer() {
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  httpd_uri_t stream_uri = {
      .uri       = "/",
      .method    = HTTP_GET,
      .handler   = stream_handler,
      .user_ctx  = NULL
  };

  if (httpd_start(&stream_httpd, &config) == ESP_OK) {
      httpd_register_uri_handler(stream_httpd, &stream_uri);
  }
}

// Setup Function
void setup() {
  Serial.begin(115200);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");

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
  config.pixel_format = PIXFORMAT_GRAYSCALE;

  // Reduce frame size to VGA to avoid memory issues
  config.frame_size = FRAMESIZE_QVGA;
  config.jpeg_quality = 12;
  config.fb_count = 1;

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }

  // sensor_t * s = esp_camera_sensor_get();
  // s->set_exposure_ctrl(s, 1);
  // s->set_aec2(s, 1);
  // s->set_gain_ctrl(s, 1);
  startCameraServer();

  Serial.print("Camera Ready! Use 'http://");
  Serial.print(WiFi.localIP());
  Serial.println("' to connect");
}

// Loop Function
void loop() {
  delay(1);
}
