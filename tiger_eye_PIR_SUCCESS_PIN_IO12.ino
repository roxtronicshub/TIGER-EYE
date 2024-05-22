/* ======================================== Including the libraries */
#include "esp_camera.h"
#include "SPI.h"
#include "driver/rtc_io.h"
#include "ESP32_MailClient.h"
#include <FS.h>
#include <SPIFFS.h>
#include <WiFi.h>
/* ======================================== */

/* ======================================== Defining variables for Email */
/*
 * Specifically for Gmail users :
 * - To send Email using Gmail use port 465 (SSL) and SMTP Server smtp.gmail.com
 * - Especially for the Sender's Gmail account, so that ESP32 CAM can log into the sender's Gmail account,
 *   the Sender's Gmail account must activate 2-Step Verification then get "App Passwords". The method is in the video. Watch carefully.
 */
#define emailSenderAccount      "roxtronicshub@@gmail.com"
#define emailSenderAppPassword  "epuh zkjl bqmp qigd"
#define smtpServer              "smtp.gmail.com"
#define smtpServerPort          465
#define emailSubject            "TIGEREYE-CAM Photo Captured"
#define emailRecipient          "roxtronicshub@gmail.com"
/* ======================================== */

/* ======================================== Defining the Camera Model and GPIO */
#define CAMERA_MODEL_AI_THINKER

#if defined(CAMERA_MODEL_AI_THINKER)
  #define PWDN_GPIO_NUM     32
  #define RESET_GPIO_NUM    -1
  #define XCLK_GPIO_NUM      0
  #define SIOD_GPIO_NUM     26
  #define SIOC_GPIO_NUM     27
  
  #define Y9_GPIO_NUM       35
  #define Y8_GPIO_NUM       34
  #define Y7_GPIO_NUM       39
  #define Y6_GPIO_NUM       36
  #define Y5_GPIO_NUM       21
  #define Y4_GPIO_NUM       19
  #define Y3_GPIO_NUM       18
  #define Y2_GPIO_NUM        5
  #define VSYNC_GPIO_NUM    25
  #define HREF_GPIO_NUM     23
  #define PCLK_GPIO_NUM     22
#else
  #error "Camera model not selected"
#endif
/* ======================================== */
 
#define FILE_PHOTO "/photo.jpg" //--> Photo File Name to save in SPIFFS

#define pin_Pir 12 //--> PIR Motion Detector PIN
#define pin_Led 4  //--> On-Board LED FLash

/* ======================================== Variables for network */
// REPLACE WITH YOUR NETWORK CREDENTIALS
const char* ssid = "roxlabs";
const char* password = "Roxylab@1996";
/* ======================================== */

SMTPData smtpData; //--> The Email Sending data object contains config and data to send

/* ________________________________________________________________________________ Subroutine to turn on or off the LED Flash */
void LEDFlashState(bool state) {
  digitalWrite(pin_Led, state);
}
/* ________________________________________________________________________________ */

/* ________________________________________________________________________________ Subroutine for LED Flash blinking */
// Example :
// LEDFlashBlink(2, 250); --> blinks 2 times with a delay of 250 milliseconds. 
void LEDFlashBlink(int blink_count, int time_delay) {
  digitalWrite(pin_Led, LOW);
  for(int i = 1; i <= blink_count*2; i++) {
    digitalWrite(pin_Led, !digitalRead(pin_Led));
    delay(time_delay);
  }
}
/* ________________________________________________________________________________ */

/* ________________________________________________________________________________ Function to read PIR sensor value (HIGH/1 OR LOW/0) */
bool PIR_State() {
  bool PRS = digitalRead(pin_Pir);
  return PRS;
}
/* ________________________________________________________________________________ */

/* ________________________________________________________________________________ Function to check if photos are saved correctly in SPIFFSl */
bool checkPhoto(fs::FS &fs) {
  File f_pic = fs.open(FILE_PHOTO);
  unsigned int pic_sz = f_pic.size();
  Serial.printf("File name: %s | size: %d\n", FILE_PHOTO, pic_sz);
  return (pic_sz > 100);
  f_pic.close();
}
/* ________________________________________________________________________________ */

/* ________________________________________________________________________________ Subroutine for formatting SPIFFS */
// This subroutine is used in case of failure to write or save the image file to SPIFFS.
void SPIFFS_format() {
  bool formatted = SPIFFS.format();
  Serial.println();
  Serial.println("Format SPIFFS...");
  if(formatted){
    Serial.println("\n\nSuccess formatting");
  }else{
    Serial.println("\n\nError formatting");
  }
  Serial.println();
}
/* ________________________________________________________________________________ */

/* ________________________________________________________________________________ Subroutine for Capture Photo and Save it to SPIFFS */
void capturePhotoSaveSpiffs( void ) {
  camera_fb_t * fb = NULL; //--> pointer
  bool Status_save_photo = 0; //--> Boolean indicating if the picture has been taken correctly

  /* ---------------------------------------- Take a photo with the camera */
  Serial.println();
  Serial.println("Taking a photo...");
  do {
    LEDFlashState(true);
    delay(2000);
    fb = esp_camera_fb_get();
    if (!fb) {
      Serial.println("Camera capture failed.");
      Serial.println("Carry out the re-capture process...");
    }
    LEDFlashState(false);
  } while ( !fb );
  Serial.println("Take photo successfully.");
  /* ---------------------------------------- */

  /* ---------------------------------------- Save photos to SPIFFS */
  do {
    LEDFlashBlink(2,250); //--> The LED Flash blinks 2 times with a duration of 250 milliseconds, which means it starts the process of saving photos to SPIFFS.
  
    /* :::::::::::::::::::::::::::::::::::::::::::::::: Photo file name */
    Serial.printf("Picture file name: %s\n", FILE_PHOTO);
    File file = SPIFFS.open(FILE_PHOTO, FILE_WRITE);
    /* :::::::::::::::::::::::::::::::::::::::::::::::: */

    /* :::::::::::::::::::::::::::::::::::::::::::::::: Insert the data in the photo file */
    if (!file) {
      Serial.println("Failed to open file in writing mode.");
      SPIFFS_format();
      capturePhotoSaveSpiffs();
      return;
    }
    else {
      file.write(fb->buf, fb->len); // payload (image), payload length
      Serial.print("The picture has been saved in ");
      Serial.print(FILE_PHOTO);
      Serial.print(" - Size: ");
      Serial.print(file.size());
      Serial.println(" bytes.");
    }
    /* :::::::::::::::::::::::::::::::::::::::::::::::: */
    
    file.close(); //--> Close the file
    
    /* :::::::::::::::::::::::::::::::::::::::::::::::: check if file has been correctly saved in SPIFFS */
    Serial.println("Checking if the picture file has been saved correctly in SPIFFS...");
    Status_save_photo = checkPhoto(SPIFFS);
    if (Status_save_photo == 1) {
      Serial.println("The picture file has been saved correctly in SPIFFS.");
    } else {
      Serial.println("The picture file is not saved correctly in SPIFFS.");
      Serial.println("Carry out the re-save process...");
      Serial.println();
    }
    /* :::::::::::::::::::::::::::::::::::::::::::::::: */
  } while (!Status_save_photo);
  /* ---------------------------------------- */

  esp_camera_fb_return(fb); //--> return the frame buffer back to the driver for reuse.
  LEDFlashBlink(1,1000); //--> The LED Flash flashes once with a duration of 1 second, which means that the process of capturing photos and saving photos to SPIFFS has been completed.
}
/* ________________________________________________________________________________ */

/* ________________________________________________________________________________ Subroutine to get the Email sending status */
// Callback function to get the Email sending status
void sendCallback(SendStatus msg) {
  Serial.println(msg.info()); //--> Print the current status
}
/* ________________________________________________________________________________ */

/* ________________________________________________________________________________ Subroutine for send photos via Email */
void sendPhoto( void ) {
  LEDFlashBlink(3,250); //--> The LED Flash blinks 3 times with a duration per 250 milliseconds, which means it starts the process of sending photos via email.
  Serial.println("Sending email...");
  
  // Set the SMTP Server Email host, port, account and password
  smtpData.setLogin(smtpServer, smtpServerPort, emailSenderAccount, emailSenderAppPassword); 
  
  // Set the sender name and Email
  smtpData.setSender("ESP32-CAM UTEH STR PIR Sensor", emailSenderAccount); 
  
  // Set Email priority or importance High, Normal, Low or 1 to 5 (1 is highest)
  smtpData.setPriority("High"); 

  // Set the subject
  smtpData.setSubject(emailSubject); 

  // Set the email message in HTML format
  smtpData.setMessage("<h2>Photo captured with ESP32-CAM and attached in this email.</h2>", true); 
  // Set the email message in text format
  //smtpData.setMessage("Photo captured with ESP32-CAM and attached in this email.", false); 

  // Add recipients, can add more than one recipient
  smtpData.addRecipient(emailRecipient); 
  //smtpData.addRecipient(emailRecipient2);

  // Add attach files from SPIFFS
  smtpData.addAttachFile(FILE_PHOTO, "image/jpg"); 

  // Set the storage type to attach files in your email (SPIFFS)
  smtpData.setFileStorageType(MailClientStorageType::SPIFFS); 

  // sendCallback
  smtpData.setSendCallback(sendCallback); 
  
  // Start sending Email, can be set callback function to track the status
  if (!MailClient.sendMail(smtpData))
  Serial.println("Error sending Email, " + MailClient.smtpErrorReason());

  // Clear all data from Email object to free memory
  smtpData.empty();

  // The LED Flash flashes 1 time with a duration per 1 second, 
  // which means that the process of sending photos via email has been completed (regardless of whether the photo was successfully sent or not).
  LEDFlashBlink(1,1000); 
  delay(2000);
}
/* ________________________________________________________________________________ */

/* ________________________________________________________________________________ VOID SETUP() */
void setup() {
  // put your setup code here, to run once:
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); //--> disable brownout detector

  Serial.begin(115200);
  Serial.println();
  
  pinMode(pin_Pir, INPUT);
  pinMode(pin_Led, OUTPUT);

  /* ---------------------------------------- Loop to stabilize the PIR sensor at first power on. */
  /*
   * I created this loop because from the tests I did that when the PIR sensor starts to turn on,
   * the PIR sensor takes at least 30 seconds to be able to detect movement or objects stably or with little noise.
   * I don't know if it's because of the quality factor of the PIR sensor I have.
   * From this source: https://lastminuteengineers.com/pir-sensor-arduino-tutorial/,
   * indeed the PIR sensor takes 30-60 seconds from the time it is turned on to be able to detect objects or movements properly.
   */
  LEDFlashBlink(2, 250); //--> The LED Flash blinks 2 times with a duration per 250 milliseconds, which means the process to stabilize the PIR sensor is started.
  Serial.println("Wait 60 seconds for the PIR sensor to stabilize.");
  Serial.println("Count down :");
  for(int i = 59; i > -1; i--) {
    Serial.print(i);
    Serial.println(" second");
    delay(1000);
  }
  Serial.println("The time to stabilize the PIR sensor is complete.");
  Serial.println();
  /* ---------------------------------------- */
  
  /* ---------------------------------------- Connect to Wi-Fi */
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    LEDFlashBlink(1,250);
    Serial.print(".");
  }
  LEDFlashState(false);
  Serial.println();
  Serial.print("Successfully connected to ");
  Serial.println(ssid);
  /* ---------------------------------------- */

  /* ---------------------------------------- Print ESP32 Local IP Address */
  Serial.print("IP Address: http://");
  Serial.println(WiFi.localIP());
  Serial.println();
  /* ---------------------------------------- */

  /* ---------------------------------------- Starting to mount SPIFFS */
  Serial.println("Starting to mount SPIFFS...");
  if (!SPIFFS.begin(true)) {
    Serial.println("An Error has occurred while mounting SPIFFS");
    Serial.println("ESP32 Cam Restart...");
    ESP.restart();
  }
  else {
    Serial.println("SPIFFS mounted successfully");
  }
  /* ---------------------------------------- */

  /* ---------------------------------------- Camera configuration. */
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
  config.pixel_format = PIXFORMAT_JPEG;
  
  if(psramFound()){
    config.frame_size = FRAMESIZE_UXGA; //--> FRAMESIZE_ + QVGA|CIF|VGA|SVGA|XGA|SXGA|UXGA
    /*
     * From source https://randomnerdtutorials.com/esp32-cam-ov2640-camera-settings/ :
     * - The image quality (jpeg_quality) can be a number between 0 and 63.
     * - Higher numbers mean lower quality.
     * - Lower numbers mean higher quality.
     * - Very low numbers for image quality, specially at higher resolution can make the ESP32-CAM to crash or it may not be able to take the photos properly.
     */
    config.jpeg_quality = 20;
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }
  /* ---------------------------------------- */

  /* ---------------------------------------- Initialize camera */
  Serial.println();
  Serial.println("Camera initialization...");
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    Serial.println("ESP32 Cam Restart...");
    ESP.restart();
  }
  Serial.print("Camera initialization was successful.");
  Serial.println();
  /* ---------------------------------------- */

  LEDFlashBlink(2, 1000); //--> The LED Flash blinks 2 times with a duration per 1 second, which means the Setup process is complete.
}
/* ________________________________________________________________________________ */

/* ________________________________________________________________________________ VOID LOOP() */
void loop() {
  // put your main code here, to run repeatedly:

  // If the PIR sensor data = 1 means that objects and movements are detected, it will be an indicator to start the image capture process and the process of sending images via email.
  if(PIR_State() == 1) { 
    capturePhotoSaveSpiffs(); //--> Calling the capturePhotoSaveSpiffs() subroutine.
    sendPhoto(); //--> Calling the sendPhoto() subroutine.
  }
  delay(1);
}
/* ________________________________________________________________________________ */
//<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
