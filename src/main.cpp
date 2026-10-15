#include <Arduino.h>
#include "AdafruitIO_WiFi.h"
#include <NewPing.h>
#include "secrets.h"

const int LED_ALARME = 13;
const int BUZZER_PIN = 27;
const int BOTAO_FISICO = 26;

// --------------------------------

// Pinos do sensor ultrassônico
const int TRIG_PIN = 12;
const int ECHO_PIN = 14;

// distância máxima de range
const int MAX_DISTANCE = 50;
const int LIMITE_DISTANCIA = 15;

NewPing sonar(
    TRIG_PIN, ECHO_PIN, MAX_DISTANCE);

AdafruitIO_WiFi io(
    IO_USERNAME,
    IO_KEY,
    WIFI_SSID,
    WIFI_PASS);

AdafruitIO_Feed *feedbotao = io.feed("botao");

// ----------------------------------
// Estado do Sistema
bool alarmeAtivo = false;
unsigned long ultimaLeituraSensor = 0;
const unsigned long INTERVALO_SENSOR = 500;
// ----------------------------------

// Controle do botão
byte ultimaLeituraBotao = HIGH;
byte estadobotaoestavel = HIGH;
unsigned long ultimamudancabotao = 0;
const unsigned long TEMPO_DEBOUNCE = 150;
// ------------------------------------------------------

void handleAlarme(AdafruitIO_Data *data);

void verificarBotao();

void verificarSensor();

void ativarAlerta();

void desligarAlerta();

void testeGeral();

void setup()
{
  Serial.begin(115200);

  pinMode(LED_ALARME, OUTPUT);
  digitalWrite(LED_ALARME, LOW);

  pinMode(BOTAO_FISICO, INPUT_PULLUP);

  pinMode(BUZZER_PIN, OUTPUT);

  desligarAlerta();

  feedbotao->onMessage(handleAlarme);

  Serial.println();
  Serial.println("Iniciando a ESP...");

  io.connect();

  while (io.status() < AIO_CONNECTED)
  {
    Serial.print(".");
    delay(500);
  }

  Serial.println();

  Serial.println(io.statusText());

  Serial.println("Adafruit IO Conectado!");

  feedbotao->get();

  Serial.println("Sistema de alarme iniciado...");
  delay(2000);
}

void loop()
{
  io.run();

  verificarBotao();
  verificarSensor();

  // testeGeral();
}

void handleAlarme(AdafruitIO_Data *data)
{
  String valor = data->toString();

  valor.trim();
  valor.toLowerCase();

  if (valor == "true")
  {
    alarmeAtivo = true;
    Serial.println("Alarme ARMADO remotamente");
  }
  else if (valor == "false")
  {
    alarmeAtivo = false;
    Serial.println("Alarme DESARMADO remotamente");
  }
  else
  {
    Serial.println("Informação inválida do FEED...");
  }
}

void verificarBotao()
{
  int leituraAtual = digitalRead(BOTAO_FISICO);

  if (leituraAtual != ultimaLeituraBotao)
  {
    ultimamudancabotao = millis();
    ultimaLeituraBotao = leituraAtual;
  }

  if (millis() - ultimamudancabotao > TEMPO_DEBOUNCE)
  {
    estadobotaoestavel = leituraAtual;
    if (estadobotaoestavel == LOW)
    {
      alarmeAtivo = !alarmeAtivo;
      feedbotao->save(
          String(alarmeAtivo ? "true" : "false"));
      Serial.println(
          alarmeAtivo
              ? "Alarme ATIVADO pelo botão físico."
              : "Alarme DESATIVADO pelo botão físico.");

      if (!alarmeAtivo)
      {
        desligarAlerta();
      }
    }
  }
}

void verificarSensor()
{
  if (millis() - ultimaLeituraSensor < INTERVALO_SENSOR)
  {
    return;
  }

  ultimaLeituraSensor = millis();
  unsigned int distancia = sonar.ping_cm();
  Serial.print("Distância: ");
  Serial.print(distancia);
  Serial.println(" cm");

  bool intrusao = alarmeAtivo && distancia > 0 && distancia <= LIMITE_DISTANCIA;
  if (intrusao)
  {
    ativarAlerta();
  }
  else
  {
    desligarAlerta();
  }
}

void ativarAlerta()
{
  digitalWrite(LED_ALARME, 1);
  digitalWrite(BUZZER_PIN, 1);
}

void desligarAlerta()
{
  digitalWrite(LED_ALARME, 0);
  digitalWrite(BUZZER_PIN, 0);
}

void testeGeral()
{
  // Teste do ultrassônico
  unsigned int distancia = sonar.ping_cm();
  Serial.print("Distância: ");
  Serial.print(distancia);
  Serial.println(" cm");

  // Teste do botão
  Serial.print("Estado do Botão: ");
  Serial.println(digitalRead(BOTAO_FISICO));

  // Teste do LED / buzzer
  digitalWrite(LED_ALARME, 1);
  digitalWrite(BUZZER_PIN, 1);
  delay(500);
  digitalWrite(LED_ALARME, 0);
  digitalWrite(BUZZER_PIN, 0);
  delay(500);
}