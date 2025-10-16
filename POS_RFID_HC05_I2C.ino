#include <U8g2lib.h> // Biblioteca para utilizaçao do I2C
#include <Wire.h>    // Biblioteca para utilizaçao do I2C
#include <MFRC522.h> //Biblioteca para utilização do circuito RFID MFRC522
#include <SPI.h>     //Biblioteca para utilização do protocolo SPI
#include <SoftwareSerial.h>// Biblioteca para utilização do módulo Bluetooth HC-05

#define BOTTOM_RETIRAR 5
#define BOTTOM_FINAL 4

#define LED_TEST 6

#define C_SELECT 10 // Pino SDA do módulo;
#define RESET 9     // Pino RESET do módulo MFRC522;

MFRC522 rfid(C_SELECT, RESET); // Declaração do módulo com o nome "rfid"

U8X8_SSD1306_128X32_UNIVISION_HW_I2C displayLED(U8X8_PIN_NONE);

SoftwareSerial BT(2, 3); //13 = RX; 12 = TX; - TX DO MÓDULO VAI NA 13, RX DO MÓDULO VAI NA 12;

char info;
String serial = "";

String dados = ""; // String vazia para armazenar o endereço da tag/cartão RFID;

unsigned long tempo_anterior = 0;
unsigned long tempo_botao_anterior = 0;
unsigned long tempo_botao2_anterior = 0;

float total;

int id_pedido = 0;

bool fim = false;
bool retirar = false;
bool estado_anterior = false;
bool estado_anterior2 = false;

struct Produtos
{
  String ID_produto;
  String nome;
  float preco;
  int quant;
};
Produtos produto[] = 
{
  {" 39 9D F5 03", "Queijo", 17.9, 0}, 
  {" BD 6A 02 04", "Picanha", 77.5, 0}, 
  {" FC 6A 00 04", "Morango", 13.9, 0}, 
  {" F9 63 7E 05", "Cafe", 34.0, 0}, 
  {" 09 14 0F 02", "Tang Stit liru", 1.29, 0}
};

void print_display()
{
  int i;
  if (dados == " 39 9D F5 03") i = 0; 
  if (dados == " BD 6A 02 04") i = 1; 
  if (dados == " FC 6A 00 04") i = 2; 
  if (dados == " F9 63 7E 05") i = 3; 
  if (dados == " 09 14 0F 02") i = 4; 

  if (retirar == true) 
  {
    if (produto[i].quant > 0)
    {
      produto[i].quant--;
    }
  } 
  else 
  {
    produto[i].quant++;
  }

  total = produto[i].preco * produto[i].quant;
  for (int L = 1; L < 4; L++) displayLED.clearLine(L);  // limpa cada linha do display com excessao da primeira linha.
  displayLED.setCursor(0, 1);
  displayLED.print(produto[i].nome);
  displayLED.setCursor(0, 2);
  displayLED.print("Qtd. x");
  displayLED.print(produto[i].quant);
  displayLED.setCursor(0, 3);
  displayLED.print("Tot. R$");
  displayLED.print(total);
}

void displayRESET()
{
  retirar = false;
  for (int j = 0; j < 5; j++)
  {
    produto[j].quant = 0;
  }
  digitalWrite(LED_TEST, 0);
  displayLED.clear();
  displayLED.setCursor(0, 0);
  displayLED.print("Item(Adicionar):");
  displayLED.setCursor(0, 2);
  displayLED.print("EMPTY");
}

void setup()
{
  Serial.begin(9600);
  BT.begin(38400);
  SPI.begin(); // Inicialização do protocolo SPI;

  displayLED.begin(); // Inicializa o display
  displayLED.setPowerSave(0); // Liga o display (desliga o modo de economia de energia)
  displayLED.setFont(u8x8_font_chroma48medium8_r); // Escolhe uma fonte

  rfid.PCD_Init(); // Inicialização do módulo RFID;
  Serial.println("RFID: Operacional");

  pinMode(BOTTOM_RETIRAR, INPUT);
  pinMode(BOTTOM_FINAL, INPUT);

  pinMode(LED_TEST, OUTPUT);
  displayRESET();
}

void loop()
{
  if(BT.available())
  {
    info = BT.read();
    Serial.print(info);
  }
  if(Serial.available())
  {
    serial = Serial.readString(); // lê até Enter
    BT.print(serial); // envia de uma vez
  }

  if(retirar == false && digitalRead(BOTTOM_RETIRAR) == 1 && (millis() - tempo_botao_anterior >= 500)) {retirar = true; tempo_botao_anterior = millis(); } 
  if(retirar == true && digitalRead(BOTTOM_RETIRAR) == 1 && (millis() - tempo_botao_anterior >= 500)) {retirar = false; tempo_botao_anterior = millis(); }

  if (retirar != estado_anterior) // Só entra aqui quando o botão muda o modo
  {
    estado_anterior = retirar; // Atualiza o estado anterior

    displayLED.clearLine(0);  // Limpa apenas a primeira linha
    displayLED.setCursor(0, 0);
    if (retirar == true)
    {
      digitalWrite(LED_TEST, 1);
      displayLED.print("Item(Remover):");
    }
    else
    {
      digitalWrite(LED_TEST, 0);
      displayLED.print("Item(Adicionar):");
    }
  }
  
  if(fim == false && digitalRead(BOTTOM_FINAL) == 1 && (millis() - tempo_botao2_anterior >= 500)) {fim = true; tempo_botao2_anterior = millis(); }
  if(fim == true && digitalRead(BOTTOM_FINAL) == 1 && (millis() - tempo_botao2_anterior >= 500)) {fim = false; tempo_botao2_anterior = millis(); }

  if (fim != estado_anterior2)
  {
    estado_anterior2 = fim;

    if(fim == true)
    {
      id_pedido++;
      int quant_tot = 0;
      total = 0;

      BT.println("---Resumo da Compra---");
      BT.println();
      if (id_pedido < 10) {BT.print("ID Ped. 00"); } else if (id_pedido >= 10 && id_pedido < 100) {BT.print("ID Ped. 0"); } else {BT.print("ID Ped. "); }
      BT.println(id_pedido);
      BT.println();
      for (int j = 0; j < 5; j++)
      {
        if (produto[j].quant > 0)
        {
          BT.print(produto[j].quant);
          BT.print("x ");
          BT.print(produto[j].nome);
          BT.print("(s) -");
          BT.print(" R$");
          BT.print(produto[j].preco);
          BT.println(" un.");
        }
      }
      BT.println();
      BT.print("Total. R$");
      for (int j = 0; j < 5; j++)
      {
        total = (produto[j].quant * produto[j].preco) + total;
      }
      BT.println(total);
      BT.print("Qtd Total de itens: ");
      for (int j = 0; j < 5; j++)
      {
        quant_tot = produto[j].quant + quant_tot;
      }
      BT.println(quant_tot);
      BT.println();
      BT.println("-------------------------------------");
      
      total = 0;
      quant_tot = 0;

      displayLED.clear();
      displayLED.setCursor(0, 0);
      displayLED.print("Resumo da Compra");
      displayLED.setCursor(0, 1);
      displayLED.print("Tot. R$");
      for (int j = 0; j < 5; j++)
      {
        total = (produto[j].quant * produto[j].preco) + total;
      }
      displayLED.print(total);

      displayLED.setCursor(0, 2);
      displayLED.print("Qtd Tot. x");
      for (int j = 0; j < 5; j++)
      {
        quant_tot = produto[j].quant + quant_tot;
      }
      displayLED.print(quant_tot);
    }
    else if (fim == false) {displayRESET(); }
  }

  if (millis() - tempo_anterior >= 2000)// Temporização que faz com que o RFID realize uma leitura a cada 2 seg;
  { 
    tempo_anterior = millis(); // Variável tempo_anterior sendo "zerada";

    if (!rfid.PICC_IsNewCardPresent())// If para testar caso o módulo NÃO tenha lido nenhum cartão/tag;
    { 
      return;
    }

    if (!rfid.PICC_ReadCardSerial())// If para testar caso o módulo NÃO tenha conseguido ler o endereço do cartão/tag;
    { 
      return;
    }
  }

  if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial())
  {
    if (fim == true)
    {
      fim = false;
      estado_anterior2 = false;

      displayRESET();
    }
    Serial.print("Endereco da TAG (HEX): ");

    for (byte i = 0; i < rfid.uid.size; i++)
    { // Loop que percorre o endereço lido no RFID como um vetor;
      if (rfid.uid.uidByte[i] < 0x10)
      {
        Serial.print(" 0");
      }
      else
      {
        Serial.print(" ");
      }

      Serial.print(rfid.uid.uidByte[i], HEX); // Código para conversão dos dados lidos no módulo, de binário para HEX;

      if (rfid.uid.uidByte[i] < 0x10)
      {
        dados.concat(String(" 0"));
      }
      else
      {
        dados.concat(String(" "));
      }
      dados.concat(String(rfid.uid.uidByte[i], HEX));
    }
    dados.toUpperCase(); // Colocando todos os valores do endereço em caixa alta
    Serial.println(); // Printa os valores de endereço no Console Serial;

    rfid.PICC_HaltA(); // Faz com que o endereço de um cartão ou tag seja lido apenas uma vez

    print_display();
  }
  dados = "";
}