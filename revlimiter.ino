

/*Projeto da UC de SEEV:

João Carlos da Silva Cunha
 Duarte Pereira Pires
IPLEIRIA - Instituto Politécnico de Leiria
ESTG - Escola Superior de Tecnologia e Gestão
EAU- Licenciatura em Engenharia Automóvel
SEEV - Sistemas Elétricos e Eletrónicos de Veículos

TP1: Pretende-se  neste  trabalho  prático  a  implementação  de um limitador de rotação para motores de combustão interna. O limitador apenas atua na ignição do veículo deixando assim, o controlo injeção sem alterações.
O limitador de RPM pode ser atuado de duas formas distintas:
- via manual através de um botão implementado no veículo ou via Bluetooth onde se pode inserir o valor desejado para o limite de RPM.

Há outras funcionalidades onde o utilizador pode requisitar dados como:
-Ultimo limite de RPM implementado
-Temperatura de gases de escape
-RPM exato a que o motor se encontra
-Erros no sensor de temperatura
e até proceder á desativação do limitador de rotação.

Temos outras funcionalidades neste sistema como leds de indicação se o Limitador de rotação está ativo, led de simulação de corrente que chega à bobine e outro caso a temperatura exceda o nível permitido. */



#include <Wire.h>                    // Biblioteca para comunicação I2C
#include <math.h>                    // Biblioteca para funções matemáticas
#include "Arduino.h"                 // Biblioteca principal do Arduino
#include <stdio.h>                   // Biblioteca para funções de entrada e saída padrão
#include "EEPROM.h"                  // Biblioteca para usar a memória EEPROM
#include "BluetoothSerial.h"         // Biblioteca para comunicação via Bluetooth Serial
#include <OneWire.h>                 // Biblioteca para comunicação OneWire
#include <DallasTemperature.h>       // Biblioteca para usar sensores de temperatura Dallas
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
//#include <Adafruit_I2CDevice.h>

// Definir os terminais do LCD
#define TFT_CS   05
#define TFT_DC   26
#define TFT_MOSI 23
#define TFT_MISO 19
#define TFT_SCLK 18
#define TFT_RST -1 // ligar ao 3V3

// Distância entre caracteres em pixels
#define DISTC 12

// Define os pinos dos LEDs
#define TEMP_LED_PIN 17               // Pino do LED de temperatura
#define REV_LED_PIN 33               // Pino do LED do limitador de RPM
#define TranPin 16                   // Pino do transistor de ignição
//#define IgnCoilPin 2               // Pino da bobina de ignição (comentado)
#define BOTAO 32                     // Pino do botão (GPIO32)

#define EEPROM_SIZE 1                // Define o tamanho da EEPROM (16kb)

// Define o pino de dados do DS18B20
#define ONE_WIRE_BUS 4               // Pino para comunicação OneWire com o sensor DS18B20

#define I2C_ADDRESS 0x08             // Endereço I2C do receptor

//Definir se o revlimiter inicia ligado ou desligado
#define REVLIMITER_STARTUP true

// debounce
#define DEBOUNCE 500                 // Tempo de debounce para o botão (em milissegundos)
long tempoatual = 0;                 // Armazena o tempo atual
long intervalo = 0;     // Armazena o intervalo de tempo entre pressões do botão
bool estadoRevLimiter = true; // Estado do limitador de RPM (ativado ou desativado)

// Configura o OneWire e o sensor de temperatura
OneWire oneWire(ONE_WIRE_BUS); // Configura a comunicação OneWire no pino definido
DallasTemperature sensors(&oneWire); // Configura o sensor de temperatura Dallas

// Bluetooth
BluetoothSerial SerialBT;         // Instância para comunicação Bluetooth Serial

// tasks
void vBrain(void *pvParameters);     // Declaração da função da tarefa vBrain
void vDisplay(void *pvParameters);
void vBluetooth(void *pvParameters);
void vBotao(void *pvParameters);
void vTemperatura(void *pvParameters);

// Função chamada quando receber dados I2C
void receiveEvent(int); // Declaração da função de callback para receber dados I2C

// interrupcao
void IRAM_ATTR botao_itnterrupt(void); // Declaração da função de interrupção do botão

// queues
QueueHandle_t xAtualRPM;             // Fila para armazenar a RPM atual
QueueHandle_t xTemperatura;             // Fila para armazenar a temp atual
QueueHandle_t xBluetooth;             // Fila para armazenar a temp atual
QueueHandle_t xEstadoRevLimiter;             // Fila para armazenar a temp atual

// Semáforo
SemaphoreHandle_t xMutexBluetooth;  // Mutex para proteger o acesso ao bluetooth
SemaphoreHandle_t xSemaforoBotao; //Tirar carga da interrupção e passa-la para uma interrupção sincronizada com o evento

void setup() {
	vTaskPrioritySet(NULL, configMAX_PRIORITIES - 1); // Define a prioridade máxima para a tarefa atual

	Serial.begin(115200);             // Inicializa a comunicação serial

	// queues
	xAtualRPM = xQueueCreate(1, sizeof(int)); // Cria uma fila para armazenar um inteiro (RPM atual)
	xTemperatura = xQueueCreate(1, sizeof(float)); // Cria uma fila para armazenar um float (temp atual)
	xBluetooth = xQueueCreate(1, sizeof(String));
	xEstadoRevLimiter = xQueueCreate(1, sizeof(bool)); //Queue com o estado do revlimiter, dois estados, ligado ou desligado, true ou false

	xMutexBluetooth = xSemaphoreCreateMutex(); // Cria um mutex para proteger o acesso ao bluetooth
	xSemaforoBotao = xSemaphoreCreateBinary();


	// Configuração dos pinos dos LEDs como saída
	pinMode(TEMP_LED_PIN, OUTPUT); // Define o pino do LED de temperatura como saída
	pinMode(REV_LED_PIN, OUTPUT); // Define o pino do LED do limitador de RPM como saída
	pinMode(TranPin, OUTPUT); // Define o pino do transistor de ignição como saída
	pinMode(BOTAO, INPUT);            // Define o pino do botão como entrada

	// Attach interrupt to GPIO32
	attachInterrupt(digitalPinToInterrupt(BOTAO), botao_itnterrupt, RISING); // Define a interrupção para o botão

	xTaskCreatePinnedToCore(vBrain, "Brain", 4096, NULL, 2, NULL, 1); // Cria a tarefa vBrain e a associa a um núcleo específico
	xTaskCreatePinnedToCore(vDisplay, "Display", 4096, NULL, 3, NULL, 1);
	xTaskCreatePinnedToCore(vBluetooth, "Bluetooth", 4096, NULL, 2, NULL, 1);
	xTaskCreatePinnedToCore(vBotao, "Botao", 4096, NULL, 1, NULL, 1);
	//xTaskCreatePinnedToCore(vTemperatura, "Temperatura", 4096, NULL, 2, NULL, 1);

}



void vBluetooth(void *pvParameters) {


	EEPROM.begin(EEPROM_SIZE);     // Inicializa a EEPROM com o tamanho definido

	SerialBT.begin("gs500"); // Inicializa a comunicação Bluetooth com o nome "gs500"

	while (true) {
		xSemaphoreTake(xMutexBluetooth, portMAX_DELAY);
		if (SerialBT.available()) {
			String receivedMessage = ""; //Assim, apaga sempre a string qnd receber uma nova mensagem
			while (SerialBT.available()) {
				char caracter = SerialBT.read();
				if ((caracter != '\n') || (caracter != '\r'))
					receivedMessage += caracter;
				vTaskDelay(5 / portTICK_PERIOD_MS); // Small delay for stability
			}
			//char msg = (char) receivedMessage;
			xQueueSend(xBluetooth, &receivedMessage, portMAX_DELAY);
		}
		xSemaphoreGive(xMutexBluetooth); // Libera o mutex
		vTaskDelay(25 / portTICK_PERIOD_MS);  // Allow FreeRTOS to switch tasks
	}
}

//Tarefa que emite à queue que o botao foi pressionado
//Tem prioridade máxima para garantir sincronismo com a interrupção


void vBotao(void *pvParameters) {
//Variáveis locais
	long intervalo = 0, ultimoTempo = 0;
	bool estadoRevLimiter = REVLIMITER_STARTUP;
//Garantir que a Queue inicia com o valor desejável
	xQueueSend(xEstadoRevLimiter, &estadoRevLimiter, portMAX_DELAY);

	while (1) {
		//Se receber o smafororo da interrupção
		if (xSemaphoreTake(xSemaforoBotao, portMAX_DELAY) == pdTRUE) {
			intervalo = millis() - ultimoTempo; //calcula o tempo desde a ultima interrupção

			if (intervalo > DEBOUNCE) { //Self explanatory....
				//Espera indefinidamente até conseguir espreitar o valor
				if (xQueuePeek(xEstadoRevLimiter, &estadoRevLimiter,
						portMAX_DELAY) == pdTRUE) {

					estadoRevLimiter = !estadoRevLimiter;  //alterna o estado

					Serial.println("mudou estado rev limter \n");

					xQueueOverwrite(xEstadoRevLimiter, &estadoRevLimiter);
				}

				ultimoTempo = millis(); // Atualiza para o tempo atual
			}
		}
	}
}

void vBrain(void *pvParameters) {
	float Time = 0;
	bool State = false;
	bool Memory = false;
	String msg = "";
	char incomingChar;
	int Message = 0; // Recebido por Bluetooth
	int i = 0;
	float temp_Millisec = 0;
	int RevLimit = 2500;        // Define o limite inicial de RPM
	int atualRPM = 0;           // Armazena a RPM atual
	float temperatureC = 0;     // Armazena a temperatura em Celsius

	//variavel local, nao há probelam pq a Task vBotao tem mais prioridade que esta e é criada primeiro
	bool estadoRevLimiter = true;

	// Inicializa o sensor de temperatura
	sensors.begin();     // Inicializa a comunicação com o sensor de temperatura

	Wire.begin(I2C_ADDRESS); // Inicializa a comunicação I2C como escravo com o endereço 0x08
	Wire.onReceive(receiveEvent); // Define a função de callback para receber dados via I2C


	while (true) {
		xQueuePeek(xAtualRPM, &atualRPM, portMAX_DELAY); // Obtém uma cópia do valor de RPM da fila
		xQueuePeek(xEstadoRevLimiter, &estadoRevLimiter, portMAX_DELAY);

		// Verifica se há dados disponíveis no Bluetooth
		//Seo valor de espera fosse 0 na queue, rebentava o esp
		if (xQueueReceive(xBluetooth, &msg, (50 / portTICK_PERIOD_MS)) && xSemaphoreTake(xMutexBluetooth, (50 / portTICK_PERIOD_MS))) {
			Serial.print("DADOS \n");
			Serial.print("Received: ");
			Serial.println(msg);
			//char incomingChar = SerialBT.read(); // Lê um caractere recebido via Bluetooth */
			/*
			 if (SerialBT.available()) {
			 char incomingChar = SerialBT.read(); // Lê um caractere recebido via Bluetooth
			 if (incomingChar != '\n')
			 msg += String(incomingChar); // Acumula os caracteres recebidos até encontrar um '\n'*/

			if (msg == "LIMIT\r\n") { // Solicitação do limite de RPM atual
				Serial.print("LIMIT \n");
				SerialBT.print("RevLimit: ");
				SerialBT.println(RevLimit); // Envia o limite de RPM via Bluetooth
				Message = 0;
				msg = "";
			}

			else if (msg == "RPM\r\n") { // Solicitação da RPM atual
				Serial.print("RPM \n");
				SerialBT.print("RPM: ");
				SerialBT.println(atualRPM); // Envia a RPM atual via Bluetooth
				Message = 0;
				msg = "";
			}

			else if (msg == "OFF\r\n") { // Desativa o limite de
				Serial.print(" OFF \n");
				Message = 0;
				msg = "";
				estadoRevLimiter = false;
				digitalWrite(REV_LED_PIN, LOW); // Desliga o LED do limite de RPM
			}

			else if (msg == "TEMP\r\n") { // Solicitação da temperatura atual
				Serial.print("TEMP  \n");
				SerialBT.print("Temperatura: ");
				SerialBT.print(temperatureC); // Envia a temperatura via Bluetooth
				SerialBT.println(" ºC");
				Message = 0;
				msg = "";
			} else if (msg != "") {
				Serial.print("CONVERSAO\n");
				Message = (msg.toInt()); // Converte a string em int
				msg = "";
			}
			// Verifica se a mensagem de RPM é válida
			if (Message > 1 && Message <= 25500 && msg != "OFF\r\n") {
				RevLimit = Message; // Atualiza o limite de RPM
				Message = 0;
				estadoRevLimiter = true;
			}

		}

		xQueueOverwrite(xEstadoRevLimiter, &estadoRevLimiter);

		// Se o limitador de RPM está ativado
		if (estadoRevLimiter == true) {
			// Verifica se a RPM atual excede o limite
			if (atualRPM > RevLimit) {
				digitalWrite(TranPin, LOW); // Desliga a ignição
				digitalWrite(REV_LED_PIN, HIGH); // Acende o LED do limite de RPM
				SerialBT.println("🏍💨🔥💨🔥🔥"); // Envia mensagem via Bluetooth
			} else {
				digitalWrite(TranPin, HIGH); // Ativa o transistor, ligando a ignição
				digitalWrite(REV_LED_PIN, HIGH); // Desliga o LED do limite de RPM
			}
		} else {
			digitalWrite(TranPin, HIGH); // Liga a ignição
			digitalWrite(REV_LED_PIN, LOW); // Desliga o LED do limite de RPM
		}

		xSemaphoreGive(xMutexBluetooth); // Libera o mutex

		// Solicita a leitura da temperatura do sensor
		sensors.requestTemperatures();
		temperatureC = sensors.getTempCByIndex(0); // Obtém a temperatura em Celsius
		if (temperatureC != DEVICE_DISCONNECTED_C) {

			//Serial.println(temperatureC);
			xQueueOverwrite(xTemperatura, &temperatureC);

			// Verifica se a temperatura é superior a 25ºC
			if (temperatureC > 25) {
				digitalWrite(TEMP_LED_PIN, HIGH); // Acende o LED de temperatura
			} else {
				digitalWrite(TEMP_LED_PIN, LOW); // Desliga o LED de temperatura
			}
		} else {
			Serial.println("Erro ao ler o sensor de temperatura!");

		}

		vTaskDelay(200 / portTICK_PERIOD_MS); // Aguarda 100 milissegundos antes de repetir o loop
	}
}

void vDisplay(void *pvParameters) {
	int bar_h, color;
	float temperatura_atual = 0, temperatura_ant = 0;   // Temperatura atual e
	float temperatura = 0;
	int atualRPM = 0;           // Armazena a RPM atual
	int RPM_ant = 0;
	int AtualRPM = 0;

// Criar um objeto tft com indicação dos terminais CS e DC
	Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK,
	TFT_RST, TFT_MISO);

// Inicializar o tft
	tft.begin();
// Colocar fundo preto
	tft.fillScreen(ILI9341_BLACK);
// Definir orientação da escrita
	tft.setRotation(0);

// Desenhar/escrever conteúdos fixos:
// Escrever header com nome da unidade curricular e ano
	tft.setCursor(0, 10);
	tft.setTextColor(ILI9341_WHITE);
	tft.setTextSize(2);
	tft.println("   Eng.Automovel ");
	tft.println("        SEEV        ");
	tft.println("     2024/2025      ");
// Desenhar retangulo em torno do texto escrito
	tft.drawRect(5, 5, 230, 56, ILI9341_RED);
	tft.drawRect(6, 6, 228, 56, ILI9341_RED); //Aumentar espessura
// Desenhar retangulo azul com cantos redondos para o título
	tft.fillRoundRect(1, 130, 130, 28, 10, ILI9341_BLUE);
// Escrever título
	tft.setCursor(5, 140);
	tft.setTextSize(2);
	tft.println("Temp.Motor");

	// Desenhar retangulo azul com cantos redondos para o título
		tft.fillRoundRect(137, 90, 70, 28, 10, ILI9341_BLUE);
	// Escrever título
		tft.setCursor(155, 97 );
		tft.setTextSize(2);
		tft.println("RPM");

// Desenhar retangulo branco para representar barra
	tft.drawRect(149, 129, 42, 152, ILI9341_WHITE);

	while (1) {
		xQueuePeek(xTemperatura, &temperatura_atual, portMAX_DELAY);
		xQueuePeek(xAtualRPM, &atualRPM, (TickType_t) 250);
		 Serial.print("TEMPERATURA: ");
		 Serial.println(temperatura_atual);

		// Se temperatura mudar, então atualizar visor LCD
		if ((int) temperatura_atual != (int) temperatura_ant) {

			if (temperatura_atual < 25) {
				color = ILI9341_CYAN;
			} else if (temperatura_atual < 30) {
				color = ILI9341_GREEN;
			} else {
				color = ILI9341_RED;
			}

			// Desenhar círculo (com espessura)
			tft.fillCircle(60, 240, 45, color);
			tft.fillCircle(60, 240, 42, ILI9341_BLACK);
			// Escrever valor da temperatura dentro do círculo
			tft.setTextSize(2);
			tft.setCursor(25, 235);
			tft.setTextColor(color, ILI9341_BLACK);
			tft.print("   "); // Limpa área de escrita

			// Converter temperatura para string
			//String RPMStr = String((int) atualRPM);
			String tempStr = String((int) temperatura_atual); // Calcular número de caracteres para achar posição de escrita
			int len = tempStr.length();
			int posicao = 3 - len;
			// Definir posição de escrita e escrever a temperatura
			tft.setCursor(25 + (posicao * DISTC), 235);
			tft.print((int) temperatura_atual); //int para tirar o decimal
			tft.print((char) 167); // Símbolo do grau (extended ASCII)
			tft.print("C");

			temperatura_ant = temperatura_atual;
		}

		if (RPM_ant != atualRPM) {

			// Derivar cor da barra
			if (atualRPM < 7000) {
				color = ILI9341_GREEN;
			} else {
				if (atualRPM < 11000) {
					color = ILI9341_YELLOW;
				} else {
					color = ILI9341_RED;
				}
			}

			bar_h = (atualRPM / 100);
			// Desenhar barra:
			// Parte inferior da barra (tamanho fixo = 5)
			// Desenhar região da barra
			tft.fillRect(150, 131, 40, 150 - bar_h, ILI9341_BLACK);

			tft.fillRect(150, 280 - bar_h, 40, bar_h, color);
			// Actualizar temperatura anterior
			RPM_ant = atualRPM;

		}
		vTaskDelay(50 / portTICK_PERIOD_MS); //"Refresh Rate" do display
	}
}

// Callback para lidar com dados recebidos via I2C
void receiveEvent(int bytes) {
	int receivedRpm;
	if (bytes >= sizeof(receivedRpm)) {
		// Lê o valor enviado pelo transmissor
		Wire.readBytes((char*) &receivedRpm, sizeof(receivedRpm));

		xQueueOverwrite(xAtualRPM, &receivedRpm); // Sobrescreve o valor de RPM na fila

	}
}

void IRAM_ATTR botao_itnterrupt(void) {
// Give the semaphore from ISR
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
//Como aconteceu a interrupçõa, vamos dar o semaforo para ser recebido pela task do botao
	xSemaphoreGiveFromISR(xSemaforoBotao, &xHigherPriorityTaskWoken);

// Yield to the task if a higher priority task was woken
//portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

void loop() {
	vTaskDelete(NULL); // Deleta a tarefa loop, pois não é utilizada no FreeRTOS
}
