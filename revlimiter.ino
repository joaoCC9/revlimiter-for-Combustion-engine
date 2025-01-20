#include <Wire.h>
#include <math.h>
#include "Arduino.h"
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_task_wdt.h"
#include "EEPROM.h"
#include "BluetoothSerial.h"
#include <OneWire.h>
#include <DallasTemperature.h>

// Define os pinos dos LEDs
#define TEMP_LED_PIN 5
#define REV_LED_PIN 18
#define TranPin 19
#define IgnCoilPin 2
#define BOTAO 32  // Pin 32 (GPIO32)

#define EEPROM_SIZE 1

// Define o pino de dados do DS18B20
#define ONE_WIRE_BUS 4

#define I2C_ADDRESS 0x08 // Endereço I2C do receptor

//debounce
#define DEBOUNCE 500
long tempoatual = 0;
long intervalo = 0;
bool estadoRevLimiter = true;


//Bluetooth
BluetoothSerial SerialBT;

// Configura o OneWire e o sensor de temperatura
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

//tasks
void vBrain(void *pvParameters);

//handler
void receiveEvent(int);

//interrupcao
void IRAM_ATTR botao_itnterrupt(void);

//queues
QueueHandle_t xAtualRPM;

//Semafro
SemaphoreHandle_t xMutexQueueRPM;

void setup() {
	vTaskPrioritySet(NULL, configMAX_PRIORITIES - 1);

	Serial.begin(115200);

	//queues
	xAtualRPM = xQueueCreate(1, sizeof(int));

	xMutexQueueRPM = xSemaphoreCreateMutex();

	Wire.begin(I2C_ADDRESS); // Inicializa o I2C como escravo com endereço 0x08
	Wire.onReceive(receiveEvent);

	SerialBT.begin("gs500"); // Nome do sinal Bluetooth

	EEPROM.begin(EEPROM_SIZE);

	// Configuração dos pinos dos LEDs como saída
	pinMode(TEMP_LED_PIN, OUTPUT);
	pinMode(REV_LED_PIN, OUTPUT);
	pinMode(TranPin, OUTPUT);
	pinMode(BOTAO, INPUT);

	// Attach interrupt to GPIO32
	attachInterrupt(digitalPinToInterrupt(BOTAO), botao_itnterrupt, RISING);

	// Inicializa o sensor de temperatura
	sensors.begin();

	xTaskCreatePinnedToCore(vBrain, "vBrain", 4096, NULL, 1, NULL, 1);
}

void vBrain(void *pvParameters) {
	float Time = 0;
	bool State = false;
	bool Memory = false;
	String msg = "";
	char incomingChar;
	int Message = 0; //Recebido por bluethooth
	int i = 0;
	float temp_Millisec = 0;
	int RevLimit = 2500;
	int atualRPM = 0;
	float temperatureC = 0;

	while (true) {

		if (xSemaphoreTake(xMutexQueueRPM, portMAX_DELAY) == pdTRUE) {
			xQueuePeek(xAtualRPM, &atualRPM, (TickType_t) 250);
		}
		xSemaphoreGive(xMutexQueueRPM);

		if (SerialBT.available()) {
			char incomingChar = SerialBT.read();
			if (incomingChar != '\n')
				msg += String(incomingChar);

			if (msg == "LIMIT") { // Solicitação da RevLimit atual
				SerialBT.print("RevLimit: ");
				SerialBT.println(RevLimit);
				Message = 0;
				msg = "";
			}

			if (msg == "RPM") { // Solicitação da RPM atual
				SerialBT.print("RPM: ");
				SerialBT.println(atualRPM);
				Message = 0;
				msg = "";
			}

			if (msg == "OFF") { // Desativa o limite de RPM
				Message = 0;
				msg = "";
				estadoRevLimiter = false;
				digitalWrite(REV_LED_PIN, LOW); // Desliga o LED do limite de RPM
			}

			if (incomingChar == '\n') {
				Message = (msg.toInt()); //Converter a string em int
				msg = "";
				//Serial.println(Message);
			}

			if (msg == "TEMP") { // Solicitação da RevLimit atual
				SerialBT.print("Temperatura: ");
				SerialBT.print(temperatureC);
				SerialBT.println(" ºC");
				Message = 0;
				msg = "";
			}
		}

		if (Message > 1 && Message <= 25500 && msg != "OFF") { // Se a mensagem de RPM for válida
			RevLimit = Message;
			Message = 0;
			estadoRevLimiter = true;
		}

		if (estadoRevLimiter == true) {

			if (atualRPM > RevLimit) {
				digitalWrite(TranPin, LOW); // Ignição é ligada
				digitalWrite(REV_LED_PIN, HIGH); // acende o LED do limite de RPM
				SerialBT.println("🏍️💨🔥💨🔥🔥");
				//SerialBT.println('&#x1F525');
			} else {
				digitalWrite(TranPin, HIGH); // Transistor é ativado, o que fecha o circuit de alimentacao da  bobine (p.ex)
				digitalWrite(REV_LED_PIN, HIGH); // Desliga o LED do limite de RPM
			}
		} else {
			digitalWrite(TranPin, HIGH); // Ignição é ligada
			digitalWrite(REV_LED_PIN, LOW); // Transistor é ativado, o que fecha o circuit de alimentacao da  bobine
		}

		sensors.requestTemperatures(); // Envia comando para ler temperatura
		temperatureC = sensors.getTempCByIndex(0); // Obtém a temperatura em Celsius
		if (temperatureC != DEVICE_DISCONNECTED_C) {
			/*Serial.print("Temperatura: ");
			 Serial.print(temperatureC);
			 Serial.println(" ºC");
			 SerialBT.print("Temperatura: ");
			 SerialBT.print(temperatureC);
			 SerialBT.println(" ºC");*/

			// Verifica se a temperatura é superior a 105ºC
			if (temperatureC > 25) {
				digitalWrite(TEMP_LED_PIN, HIGH); // Acende o LED de temperatura
			} else {
				digitalWrite(TEMP_LED_PIN, LOW); // Desliga o LED de temperatura
			}
		} else {
			Serial.println("Erro ao ler o sensor de temperatura!");
		}

		vTaskDelay(100 / portTICK_PERIOD_MS);
	}
}

// Callback para lidar com dados recebidos via I2C
void receiveEvent(int bytes) {
	int receivedRpm;
	if (bytes >= sizeof(receivedRpm)) {
		// Lê o valor enviado pelo transmissor
		Wire.readBytes((char*) &receivedRpm, sizeof(receivedRpm));

		if (xSemaphoreTake(xMutexQueueRPM, portMAX_DELAY) == pdTRUE) {
			xQueueOverwrite(xAtualRPM, &receivedRpm);
		}
		xSemaphoreGive(xMutexQueueRPM);
	}
}

void IRAM_ATTR botao_itnterrupt(void) {
	intervalo = millis() - tempoatual;
	tempoatual = millis();

	if(intervalo > DEBOUNCE){
		estadoRevLimiter = !estadoRevLimiter;
	}
}

void loop() {
	vTaskDelete(NULL);
}
