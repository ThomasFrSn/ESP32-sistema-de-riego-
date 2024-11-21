#include <WiFi.h>
#include "ThingSpeak.h" // Librería oficial de ThingSpeak

// Configuración WiFi
const char* ssid = "Thomy_2.4G";     // Nombre de tu red WiFi
const char* password = "Thomy2005";  // Contraseña de tu red WiFi

// Configuración ThingSpeak
WiFiClient client;
unsigned long myChannelNumber = 2749703;  // Número de canal de ThingSpeak
const char* myWriteAPIKey = "K8G9X36B1CYU6HO8";  // API Key de escritura de ThingSpeak

// Pines del sensor ultrasónico
const int trigPin = 12; // Pin TRIG del HC-SR04
const int echoPin = 14; // Pin ECHO del HC-SR04

// Pines del sensor de humedad de suelo
const int soilPin = 34; // Pin analógico del sensor de humedad

// Pines del sensor de flujo de agua
const int flowPin = 27; // Pin digital del sensor de flujo de agua
volatile int flowPulseCount = 0; // Contador de pulsos del sensor
float flowRate = 0.0;

// Variables globales
long duration;
float distance = 0.0;
int humedadCruda = 0;
int porcentajeHumedad = 0;
unsigned long lastTime = 0; 
const unsigned long interval = 20000; // Enviar datos cada 20 segundos

// Interrupción para el sensor de flujo de agua
void IRAM_ATTR pulseCounter() {
  flowPulseCount++;
}

// Conexión a la red WiFi
void setup_wifi() {
  Serial.println("Conectando a WiFi...");
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConectado a WiFi");
}

void setup() {
  Serial.begin(115200);

  // Configuración de pines
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(soilPin, INPUT);
  pinMode(flowPin, INPUT_PULLUP);

  // Configurar interrupciones del sensor de flujo
  attachInterrupt(digitalPinToInterrupt(flowPin), pulseCounter, RISING);

  // Conexión a WiFi
  setup_wifi();

  // Inicializar ThingSpeak
  ThingSpeak.begin(client);
}

void loop() {
  // --- Sensor ultrasónico ---
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  duration = pulseIn(echoPin, HIGH);
  distance = (duration * 0.0343) / 2;

  // --- Sensor de humedad de suelo ---
  humedadCruda = analogRead(soilPin); 
  porcentajeHumedad = map(humedadCruda, 2800, 4095, 100, 0);
  porcentajeHumedad = constrain(porcentajeHumedad, 0, 100);

  // --- Sensor de flujo de agua ---
  unsigned long currentTime = millis();
  if (currentTime - lastTime >= interval) {
    flowRate = (flowPulseCount / 7.5) / ((currentTime - lastTime) / 60000.0);
    flowPulseCount = 0;
    lastTime = currentTime;
  }

  // Mostrar datos en el monitor serial
  Serial.print("Distancia: ");
  Serial.print(distance);
  Serial.println(" cm");

  Serial.print("Humedad del suelo (cruda): ");
  Serial.print(humedadCruda);
  Serial.print(" | Porcentaje: ");
  Serial.print(porcentajeHumedad);
  Serial.println("%");

  Serial.print("Caudal de agua: ");
  Serial.print(flowRate);
  Serial.println(" L/min");

  // --- Enviar datos a ThingSpeak ---
  ThingSpeak.setField(1, distance);          // Enviar distancia al campo 1
  ThingSpeak.setField(2, porcentajeHumedad); // Enviar humedad al campo 2
  ThingSpeak.setField(3, flowRate);          // Enviar caudal al campo 3

  int responseCode = ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);
  if (responseCode == 200) {
    Serial.println("Datos enviados correctamente a ThingSpeak");
  } else {
    Serial.print("Error al enviar datos. Código de respuesta: ");
    Serial.println(responseCode);
  }

  delay(20000); // Esperar 20 segundos antes de la próxima lectura
}
