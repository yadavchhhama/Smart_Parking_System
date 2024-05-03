#include <SPI.h>
#include <MFRC522.h>
#include <ESP32Servo.h>
#include <ESP_Mail_Client.h>

#define IR_Sensor 15 // D14 IR pin defined
#define RST_PIN   5  // Configurable, see typical pin layout above
#define SS_PIN    16 // Change this to the GPIO pin you want to use
#define SERVO_PIN 26

#define SMTP_SERVER "smtp.gmail.com"
#define SMTP_PORT 465
#define SENDER_EMAIL "anchalsachan945@gmail.com"
#define SENDER_PASSWORD "******"
#define RECIPIENT_EMAIL "30524csiot@gmail.com"
#define RECIPIENT_NAME "PARKING"

MFRC522 mfrc522(SS_PIN, RST_PIN);  // Create MFRC522 instance
Servo servoMotor;

const char* ssid = "narzo 50";
const char* password = "alpha@#123";
// Define allowed ID
byte allowedID[] = {0x07, 0xD7, 0x25, 0x52}; // Allowed UID

enum Transfer_Encoding { enc_7bit, enc_base64 };
SMTPSession smtp;
String emailBody; // Global variable to hold email content

bool isAllowedUID(byte* uid) {
  return memcmp(uid, allowedID, sizeof(allowedID)) == 0;
}

void setup() {
  Serial.begin(115200);          // Initialize serial communications 
  delay(500);

  WiFi.begin(ssid, password);
  Serial.println("");
  Serial.print("Connecting to WiFi");  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  SPI.begin();                   // Init SPI bus
  mfrc522.PCD_Init();            // Init MFRC522 card
  servoMotor.attach(SERVO_PIN);  // Attach servo to its control pin
  pinMode(IR_Sensor, INPUT);     // Initialize IR sensor as an input
  Serial.println("Scan PICC to see UID, type, and data blocks...");
}

void loop() {
  bool parkingOccupied = digitalRead(IR_Sensor) == LOW;  // Read IR sensor state continuously

  if (parkingOccupied) {
    Serial.println("Parking Occupied");
    sendEmail(4);
  } else {
    Serial.println("Parking Not Occupied");
  }

  rfidreader(); // Call the RFID card reading function
  delay(100);   // Add a small delay to avoid continuous reading
}

void rfidreader() {
  if (!mfrc522.PICC_IsNewCardPresent()) {
    return;
  }

  if (!mfrc522.PICC_ReadCardSerial()) {
    return;
  }

  bool parkingOccupied = digitalRead(IR_Sensor) == LOW;  // Read IR sensor state

  if (isAllowedUID(mfrc522.uid.uidByte) && !parkingOccupied) {
    Serial.print("Access Allowed - Card UID:");
    dump_byte_array(mfrc522.uid.uidByte, mfrc522.uid.size);
    Serial.println(" - Parking Available");
    sendEmail(1);
    moveServo();
  } else if (parkingOccupied) {
    sendEmail(2);
    Serial.println("Parking Occupied - Access Denied");
  } else {
    sendEmail(3);
    Serial.println("Access Denied!");
  }
}

void moveServo() {
  for (int pos = 0; pos <= 90; pos += 1) {
    servoMotor.write(pos);
    delay(15); 
  }
  delay(5000);
  for (int pos = 90; pos >= 0; pos -= 1) {
    servoMotor.write(pos);
    delay(15); 
  }
}

void dump_byte_array(byte *buffer, byte bufferSize) {
  for (byte i = 0; i < bufferSize; i++) {
    Serial.print(buffer[i] < 0x10 ? " 0" : " ");
    Serial.print(buffer[i], HEX);
  }
                                                                }

void sendEmail(int tempp) {
  ESP_Mail_Session session;
  session.server.host_name = SMTP_SERVER;
  session.server.port = SMTP_PORT;
  session.login.email = SENDER_EMAIL;
  session.login.password = SENDER_PASSWORD;

  SMTP_Message message;
  message.sender.name = "PARKING";
  message.sender.email = SENDER_EMAIL;
  message.subject = "PARKING";
  message.addRecipient(RECIPIENT_NAME, RECIPIENT_EMAIL);
  
  if (tempp == 1){
    emailBody = "Access Allowed Parking Available";
  }
  else if(tempp == 2){
    emailBody = "Parking Occupied - Access Denied";
  }
  else if(tempp == 3){
    emailBody = "Access Denied!";
  }
  else if(tempp == 4){
    emailBody = "Parking Occupied";
  }
  
  message.text.content = emailBody.c_str();
  message.text.charSet = "us-ascii";
  message.text.transfer_encoding = Transfer_Encoding::enc_7bit;

  if (!smtp.connect(&session))
    return;

  if (!MailClient.sendMail(&smtp, &message))
    Serial.println("Error sending Email, " + smtp.errorReason());
  else
    Serial.println("Email sent successfully!");
}