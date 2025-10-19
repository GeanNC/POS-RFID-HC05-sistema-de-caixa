//IDENTAÇÃO ESTILO ALLMAN
#include <U8g2lib.h> // Biblioteca para utilizaçao do I2C
#include <Wire.h>    // Biblioteca para utilizaçao do I2C
#include <MFRC522.h> // Biblioteca para utilização do RFID
#include <SPI.h>     // Biblioteca para utilização do SPI
#include <SoftwareSerial.h> // Biblioteca para utilização do HC-05

#define BOTTOM_RETIRAR 5 // Botão para alternar modo retirar/adicionar
#define BOTTOM_FINAL 4   // Botão para finalizar pedido

#define LED_TEST 6 // LED indicador de modo retirar

#define C_SELECT 10 // Pinos RFID
#define RESET 9 

MFRC522 rfid(C_SELECT, RESET); // Instancia o módulo RFID
U8X8_SSD1306_128X32_UNIVISION_HW_I2C displayLED(U8X8_PIN_NONE); // Display OLED 128x32 I2C
SoftwareSerial BT(2, 3); //Bluetooth

String serial = ""; // Armazena entrada vinda do serial
String dados = ""; // Armazena endereço da tag RFID

unsigned long tempo_anterior = 0; // Controle de tempo RFID
unsigned long tempo_botao_anterior = 0; // Controle de tempo do botão retirar
unsigned long tempo_botao2_anterior = 0; // Controle de tempo do botão finalizar

float total; // Valor total calculado
int id_pedido = 0; // Contador de pedidos realizados

bool fim = false; // Indica se pedido foi finalizado
bool retirar = false; // Indica modo retirar (true) ou adicionar (false)
bool estado_anterior = false; // Guarda último estado do botão retirar
bool estado_anterior2 = false; // Guarda último estado do botão finalizar

struct Produtos // Estrutura de produtos
{
  String ID_produto; // ID RFID vinculado ao produto
  String nome;       // Nome do produto
  float preco;       // Preço unitário
  int quant;         // Quantidade
};
Produtos produto[] = // Cadastro dos produtos
{
  {" 39 9D F5 03", "Queijo", 17.9, 0}, 
  {" BD 6A 02 04", "Picanha", 77.5, 0}, 
  {" FC 6A 00 04", "Morango", 13.9, 0}, 
  {" F9 63 7E 05", "Cafe", 34.0, 0}, 
  {" 09 14 0F 02", "Tang Stit liru", 1.29, 0}
};

void print_display()// Função responsável por otimizar o uso do display no código
{
  int i;
  if (dados == " 39 9D F5 03") i = 0; // identifica produto pelo UID
  if (dados == " BD 6A 02 04") i = 1; 
  if (dados == " FC 6A 00 04") i = 2; 
  if (dados == " F9 63 7E 05") i = 3; 
  if (dados == " 09 14 0F 02") i = 4; 

  if (retirar == true) // modo remover quantidade ativo
  {
    if (produto[i].quant > 0) // evita quantidade negativa
    {
      produto[i].quant--;
    }
  } 
  else // modo adicionar quantidade ativo
  {
    produto[i].quant++;
  }
  total = produto[i].preco * produto[i].quant;
  for (int L = 1; L < 4; L++) // limpa linhas inferiores do display
  {
    displayLED.clearLine(L);
  }
  displayLED.setCursor(0, 1);// Printa Produto, Quantidade e Valor total no display
  displayLED.print(produto[i].nome);
  displayLED.setCursor(0, 2);
  displayLED.print("Qtd. x");
  displayLED.print(produto[i].quant);
  displayLED.setCursor(0, 3);
  displayLED.print("Tot. R$");
  displayLED.print(total);
}

void displayRESET()// reseta o display no inicio da execução e a cada novo pedido
{
  retirar = false;

  for (int j = 0; j < 5; j++) // reseta quantidades de todos os produtos
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
  SPI.begin(); // inicializa SPI

  displayLED.begin(); // inicializa display
  displayLED.setPowerSave(0); // desativa economia de energia
  displayLED.setFont(u8x8_font_chroma48medium8_r); // define fonte padrão

  rfid.PCD_Init(); // inicializa o RFID
  Serial.println("RFID: Operacional");

  pinMode(BOTTOM_RETIRAR, INPUT);
  pinMode(BOTTOM_FINAL, INPUT);
  pinMode(LED_TEST, OUTPUT);

  displayRESET(); // zera interface do display
}

void loop()
{
  if(Serial.available()) // verifica dados recebidos via Serial
  {
    serial = Serial.readString();//leitura
    BT.print(serial); //print
  }

  if(retirar == false && digitalRead(BOTTOM_RETIRAR) == 1 && (millis() - tempo_botao_anterior >= 500)) // ativa modo retirar
  {
    retirar = true; 
    tempo_botao_anterior = millis(); 
  } 
  if(retirar == true && digitalRead(BOTTOM_RETIRAR) == 1 && (millis() - tempo_botao_anterior >= 500)) // desativa modo retirar
  {
    retirar = false; 
    tempo_botao_anterior = millis(); 
  }
  if (retirar != estado_anterior) // atualiza display (apenas a primeira linha) ao trocar de modo
  {
    estado_anterior = retirar; // salva estado atual

    displayLED.clearLine(0);
    displayLED.setCursor(0, 0);

    if (retirar == true) // mostra modo remover
    {
      digitalWrite(LED_TEST, 1);
      displayLED.print("Item(Remover):");
    }
    else // mostra modo adicionar
    {
      digitalWrite(LED_TEST, 0);
      displayLED.print("Item(Adicionar):");
    }
  }
  
  if(fim == false && digitalRead(BOTTOM_FINAL) == 1 && (millis() - tempo_botao2_anterior >= 500)) // ativa modo final
  {
    fim = true; 
    tempo_botao2_anterior = millis(); 
  }
  if(fim == true && digitalRead(BOTTOM_FINAL) == 1 && (millis() - tempo_botao2_anterior >= 500)) // desativa modo final
  {
    fim = false; 
    tempo_botao2_anterior = millis(); 
  }
  if (fim != estado_anterior2) // mudança no estado de finalização
  {
    estado_anterior2 = fim;

    if(fim == true) // imprime resumo da compra
    {
      id_pedido++;
      int quant_tot = 0;
      total = 0;

      BT.println("---Resumo da Compra---");// Bloco do resumo da compra a ser enviado via Bluetooth
      BT.println();
      if (id_pedido < 10) {BT.print("ID Ped. 00"); } 
      else if (id_pedido >= 10 && id_pedido < 100) {BT.print("ID Ped. 0"); } 
      else {BT.print("ID Ped. "); }
      BT.println(id_pedido);
      BT.println();
      for (int j = 0; j < 5; j++) // percorre produtos e imprime itens válidos
      {
        if (produto[j].quant > 0) // apenas produtos comprados
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
      for (int j = 0; j < 5; j++) // soma total
      {
        total = (produto[j].quant * produto[j].preco) + total;
      }
      BT.println(total);
      BT.print("Qtd Total de itens: ");
      for (int j = 0; j < 5; j++) // soma quantidades totais
      {
        quant_tot = produto[j].quant + quant_tot;
      }
      BT.println(quant_tot);
      BT.println();
      BT.println("-------------------------------------");
      
      total = 0;
      quant_tot = 0;

      displayLED.clear(); // Bloco do resumo da compra no display
      displayLED.setCursor(0, 0);
      displayLED.print("Resumo da Compra");
      displayLED.setCursor(0, 1);
      displayLED.print("Tot. R$");
      for (int j = 0; j < 5; j++) // recalcula total para display
      {
        total = (produto[j].quant * produto[j].preco) + total;
      }
      displayLED.print(total);
      displayLED.setCursor(0, 2);
      displayLED.print("Qtd Tot. x");
      for (int j = 0; j < 5; j++) // recalcula qtd total para display
      {
        quant_tot = produto[j].quant + quant_tot;
      }
      displayLED.print(quant_tot);
    }
    else if (fim == false) // retorna para modo inicial
    {
      displayRESET(); 
    }
  }

  if (millis() - tempo_anterior >= 2000) // temporiza leitura RFID
  { 
    tempo_anterior = millis(); // reinicia contador

    if (!rfid.PICC_IsNewCardPresent()) // sem novo cartão
    { 
      return;
    }

    if (!rfid.PICC_ReadCardSerial()) // erro de leitura
    { 
      return;
    }
  }

  if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) // nova tag detectada
  {
    if (fim == true) // reinicia após finalização
    {
      fim = false;
      estado_anterior2 = false;
      displayRESET();
    }

    Serial.print("Endereco da TAG (HEX): ");

    for (byte i = 0; i < rfid.uid.size; i++)
    {
      if (rfid.uid.uidByte[i] < 0x10)
      {
        Serial.print(" 0");
      }
      else // imprime espaço
      {
        Serial.print(" ");
      }

      Serial.print(rfid.uid.uidByte[i], HEX); 

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
    dados.toUpperCase(); // converte UID para maiúsculo
    Serial.println(); // quebra de linha
    rfid.PICC_HaltA(); // pausa leitura da tag

    print_display(); // atualiza display com item
  }
  dados = ""; // limpa buffer de leitura
}