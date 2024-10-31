#include <SPI.h>
#include <MFRC522.h>

#define SS_PIN 10
#define RST_PIN 9

MFRC522 mfrc522(SS_PIN, RST_PIN);   // 创建 MFRC522 实例

void setup() {
  Serial.begin(9600);
  SPI.begin();       // 初始化 SPI 总线
  mfrc522.PCD_Init();   // 初始化 MFRC522
  mfrc522.PCD_DumpVersionToSerial();  // 输出 MFRC522 版本信息到串口
}

void loop() {
  // 检测是否有新的卡片出现
  if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
    // 进行卡片认证
    MFRC522::MIFARE_Key key;
    for (byte i = 0; i < 6; i++) {
      key.keyByte[i] = 0xFF;   // 默认认证密钥为 0xFF 0xFF 0xFF 0xFF 0xFF 0xFF
    }
    byte startSector = 1;   // 起始扇区
    byte totalSectors = 3;   // 总共要写入的扇区数

    bool authenticationSuccess = true;
    for (byte sector = startSector; sector < startSector + totalSectors; sector++) {
      if (mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, mfrc522.PICC_GetSector(sector), &key, &(mfrc522.uid)) != MFRC522::STATUS_OK) {
        authenticationSuccess = false;
        break;
      }
    }

    if (authenticationSuccess) {
      // 认证成功，准备要写入的长字符串数据
      String longData = "This is a long string data to be written into MIFARE card. It exceeds the capacity of a single sector.";   // 要写入的长字符串数据

      // 将长字符串分块写入卡片的多个扇区
      int remainingBytes = longData.length();
      int sector = startSector;
      int dataOffset = 0;
      while (remainingBytes > 0 && sector < startSector + totalSectors) {
        byte data[48];
        int bytesToWrite = min(48, remainingBytes);

        longData.getBytes(data, bytesToWrite, dataOffset);
        dataOffset += bytesToWrite;
        remainingBytes -= bytesToWrite;

        // 写入数据
        for (int block = 0; block < 3; block++) {
          byte blockAddr = mfrc522.PICC_GetBlock(sector, block);
          if (mfrc522.MIFARE_Write(blockAddr, data + (block * 16), 16)) {
            Serial.print("Sector ");
            Serial.print(sector);
            Serial.print(" Block ");
            Serial.print(block);
            Serial.println(" 数据写入成功！");
          } else {
            Serial.print("Sector ");
            Serial.print(sector);
            Serial.print(" Block ");
            Serial.print(block);
            Serial.println(" 数据写入失败！");
            break;
          }
        }

        sector++;
      }

      mfrc522.PICC_HaltA();   // 停止卡片操作
      mfrc522.PCD_StopCrypto1();   // 停止加密

    } else {
      Serial.println("卡片认证失败！");
    }
  }
}
