// main.cpp – RP2350A programmiert ICE40UP5K per Slave-SPI

#include <Arduino.h>
#include <SPI.h>

#include "fpga_bitstream.h"
const uint8_t* bitfile     = (const uint8_t*)firmware_bin;
const uint32_t  bitfile_len = 12345;
// ── GPIO Pins (Exakt abgestimmt auf RP2350 Hardware-SPI1) ─────────────────
constexpr uint8_t PIN_FPGA_MISO   = 12;  // GPIO12 ← SPI1 RX (FPGA SO)
constexpr uint8_t PIN_FPGA_CS     = 13;  // GPIO13 → SPI1 CSn (FPGA SS)
constexpr uint8_t PIN_FPGA_SCK    = 14;  // GPIO14 → SPI1 SCK (FPGA CLK)
constexpr uint8_t PIN_FPGA_MOSI   = 15;  // GPIO15 → SPI1 TX (FPGA SI)
constexpr uint8_t PIN_FPGA_CRESET = 16;  // GPIO16 → Creset (Hardware-Reset)
constexpr uint8_t PIN_FPGA_CDONE  = 17;  // GPIO17 ← CDone (Status-Pin)

// ── SPI1 Pins setzen ──────────────────────────────────────────────────────
void spi_init_pins() {
    SPI1.setSCK(PIN_FPGA_SCK);
    SPI1.setTX(PIN_FPGA_MOSI);
    SPI1.setRX(PIN_FPGA_MISO);
    SPI1.setCS(PIN_FPGA_CS);
}

// ── Hilfsfunktionen ───────────────────────────────────────────────────────
void spi_send(const uint8_t* data, size_t len) {
    for (size_t i = 0; i < len; i++) {
        SPI1.transfer(data[i]);
    }
}

// ── Debug Info ────────────────────────────────────────────────────────────
void debug_info() {
    Serial.println("\n[DEBUG] System Info:");
    Serial.printf("  Freier Heap    : %d Bytes\n", rp2040.getFreeHeap());
    Serial.printf("  Bitfile Groesse: %lu Bytes\n", bitfile_len);
    Serial.printf("  Erste 4 Bytes  : 0x%02X 0x%02X 0x%02X 0x%02X\n",
                  bitfile[0], bitfile[1], bitfile[2], bitfile[3]);

    // Korrekter iCE40 Header-Check (iCE40 Bitstreams starten oft mit 0xFF)
    if (bitfile[0] == 0xFF) {
        Serial.println("  Bitfile Header: OK (Startet mit 0xFF Sync)");
    } else {
        Serial.println("  WARNUNG: Bitfile Header ungewöhnlich! Prüfe das raw-Format.");
    }

    Serial.printf("  CRESET : %s\n", digitalRead(PIN_FPGA_CRESET) ? "HIGH" : "LOW");
    Serial.printf("  CDONE  : %s\n", digitalRead(PIN_FPGA_CDONE)  ? "HIGH" : "LOW");
    Serial.println();
}

// ── FPGA Konfiguration ────────────────────────────────────────────────────
bool fpga_configure() {
    Serial.println("[FPGA] Starte Konfiguration...");

    // Schritt 1: Beide Pins definiert hochsetzen
    digitalWrite(PIN_FPGA_CS,     HIGH);
    digitalWrite(PIN_FPGA_CRESET, HIGH);
    delay(10);

    // Schritt 2: CS LOW, dann CRESET LOW (Zwingt iCE40 in den Slave-SPI Modus)
    digitalWrite(PIN_FPGA_CS,     LOW);
    delay(1);
    digitalWrite(PIN_FPGA_CRESET, LOW);
    delay(2);  // min. 200ns laut Datenblatt

    // Schritt 3: CRESET wieder HIGH (CS MUSS dabei zwingend LOW bleiben!)
    digitalWrite(PIN_FPGA_CRESET, HIGH);
    delay(5);  // iCE40 braucht bis zu 1.2ms zum Löschen des internen RAMs

    // Schritt 4: CDONE sollte jetzt LOW sein (Bereit für Daten)
    Serial.printf("  CDONE vor Konfig: %s (erwartet: LOW)\n",
                  digitalRead(PIN_FPGA_CDONE) ? "HIGH" : "LOW");

    // Schritt 5: 8 Dummy-Clocks mit CS HIGH
    digitalWrite(PIN_FPGA_CS, HIGH);
    delayMicroseconds(10);
    SPI1.transfer(0x00);  // 8 Clocks erzeugen
    delayMicroseconds(10);

    // Schritt 6: CS wieder LOW für die eigentliche Übertragung
    digitalWrite(PIN_FPGA_CS, LOW);
    delayMicroseconds(10);

    Serial.print("[FPGA] Sende Bitfile... ");
    Serial.flush();
    uint32_t t = millis();
    
    // Bitstream via SPI jagen
    spi_send(bitfile, bitfile_len);
    
    Serial.printf("fertig in %lums\n", millis() - t);

    // Schritt 7: CS wieder HIGH setzen
    digitalWrite(PIN_FPGA_CS, HIGH);

    // Schritt 8: Mindestens 49 zusätzliche Takte (Wake-up Clocks)
    // Damit taktet sich das FPGA aus dem Konfigurationsmodus in den User-Modus
    for (int i = 0; i < 7; i++) {
        SPI1.transfer(0x00);  // 7 * 8 = 56 Clocks
    }

    // Schritt 9: Kurze Pause und Erfolgskontrolle
    delay(100);
    bool success = (digitalRead(PIN_FPGA_CDONE) == HIGH);
    
    Serial.printf("  CDONE nach Konfig: %s\n", success ? "HIGH (ERFOLG!)" : "LOW (FEHLER)");
    Serial.println(success ? "[FPGA] Bootvorgang abgeschlossen!" : "[FPGA] Konfiguration fehlgeschlagen!");
    
    return success;
}

// ── Setup ─────────────────────────────────────────────────────────────────
void setup() {
    Serial.begin(115200);
    
    // Wartezeit beim Booten, damit du Zeit hast, den Serial Monitor zu öffnen
    delay(3000); 
    Serial.println("\n=== RP2350A TO iCE40 BOOTLOADER ===");
    Serial.flush();

    // GPIO Richtungen festlegen
    pinMode(PIN_FPGA_CS,     OUTPUT);
    pinMode(PIN_FPGA_CRESET, OUTPUT);
    pinMode(PIN_FPGA_CDONE,  INPUT);
    pinMode(PIN_FPGA_MISO, INPUT_PULLUP);

    // Grundzustand herstellen
    digitalWrite(PIN_FPGA_CS,     HIGH);
    digitalWrite(PIN_FPGA_CRESET, HIGH);

    // Zeige Debug-Werte im Terminal
    debug_info();

    // SPI1 initialisieren
    spi_init_pins();
    SPI1.begin();
    
    // Auf 5 MHz reduziert (5000000UL) für saubere Signale ohne Reflexionen
    SPI1.beginTransaction(SPISettings(5000000UL, MSBFIRST, SPI_MODE0));

    // FPGA programmieren
    if (!fpga_configure()) {
        SPI1.endTransaction();
        SPI1.end();
        Serial.println("[ERROR] Programmierung abgebrochen.");
        while (true) {
            Serial.println("[ERROR] Halted – Bitte Verkabelung und Pull-Ups prüfen.");
            delay(5000);
        }
    }

    // SPI nach erfolgreicher Programmierung freigeben
    SPI1.endTransaction();
    SPI1.end();

    Serial.println("[INFO] FPGA ist aktiv und läuft im User-Mode.");
}

// ── Loop ──────────────────────────────────────────────────────────────────
void loop() {
    static uint32_t last = 0;
    if (millis() - last >= 5000) {
        last = millis();
        bool cdone_status = digitalRead(PIN_FPGA_CDONE);
        Serial.printf("[STATUS] CDONE=%s  Uptime=%lus\n",
                      cdone_status ? "OK (HIGH)" : "FAIL (LOW)",
                      millis() / 1000UL);
    }
}