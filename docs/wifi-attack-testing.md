# WiFi Attack Field Testing — Issues #38–41

> **Only test on networks you own or have written authorization to test.**

## Prerequisites

- Dedicated test AP (WPA2, no internet uplink)
- Test client device connected to test AP
- SD card inserted (for PCAP saves)
- Serial monitor at 115200 baud

---

## Deauth Testing (#38)

**Menu:** WiFi → Scan APs → Select APs → Deauth

**Procedure:**
1. Scan APs, select your test AP
2. Start deauth attack
3. Verify serial shows `esp_wifi_80211_tx()` calls
4. Verify test client disconnects (check client WiFi status)
5. Press BACK to stop

**Expected:** Target clients disconnect within 5s. Serial shows frame injection logs. Clean stop on BACK.

**Failure signs:** No disconnection, TX error codes in serial, crash on start/stop.

**Notes:** ESP32-S3 uses identical `esp_wifi_80211_tx` API. If deauth fails, check raw TX return code in serial — non-zero means injection blocked.

---

## Beacon Spam Testing (#39)

**Menu:** WiFi → Beacon Spam → Random/List/Target

**Procedure:**
1. Select Random beacon spam mode
2. Open WiFi scanner on phone/laptop
3. Verify 10+ fake SSIDs appear within 3 seconds
4. Press BACK to stop
5. Verify fake SSIDs disappear from scanner

**Expected:** Fake APs visible on nearby devices. Frame construction logged to serial. Clean stop.

**Failure signs:** No fake SSIDs visible, serial shows frame errors, menu hang on stop.

---

## PMKID/EAPOL Capture Testing (#40)

**Menu:** WiFi → Scan APs → Select APs → PMKID Scan

**Procedure:**
1. Ensure `SavePCAP` is `true` in Settings
2. Scan and select your WPA2 test AP
3. Start PMKID scan
4. Force a client reconnection to the test AP (toggle WiFi on client)
5. Watch serial for `eapolSnifferCallback` events
6. Press BACK to stop after capture
7. Remove SD card, open `.pcap` in Wireshark

**Expected:** Serial shows EAPOL frame detection. PCAP file created in SD root. Wireshark parses valid 802.11 frames. Handshake messages (msg 1-4) tracked.

**Failure signs:** No callback events, empty PCAP, Wireshark parse errors.

**Validation:** In Wireshark, filter `eapol` — should see handshake frames. PCAP header: magic `0xa1b2c3d4`, link type 105.

---

## Evil Portal Testing (#41)

**Menu:** WiFi → Evil Portal

**Setup:** Place `index.html` on SD card root (see field-testing-guide.md for template).

**Procedure:**
1. Start Evil Portal from menu
2. Verify badge creates AP (check with phone WiFi scan)
3. Connect client to portal AP
4. Verify captive portal page auto-opens in browser
5. Submit test credentials in form
6. Verify credentials appear in serial output
7. Press BACK to stop portal

**Expected:** AP visible, DNS redirect works, HTML renders, form submissions captured on serial. Clean shutdown.

**Failure signs:** AP not visible, blank page, DNS redirect fails, crash on stop.

**Fallback:** If SD HTML fails, use serial command `sethtml=<html>...</html>` to set portal content directly.

---

## Hardware Test Checklist

| # | Test | Pass? | Notes |
|---|------|-------|-------|
| 1 | Deauth: target disconnects | | |
| 2 | Deauth: clean start/stop cycle (3×) | | |
| 3 | Beacon: fake SSIDs visible on phone | | |
| 4 | Beacon: clean stop, SSIDs disappear | | |
| 5 | PMKID: EAPOL callback fires on reconnect | | |
| 6 | PMKID: PCAP valid in Wireshark | | |
| 7 | Evil Portal: AP visible to clients | | |
| 8 | Evil Portal: captive portal renders HTML | | |
| 9 | Evil Portal: form submission captured | | |
| 10 | No crash on repeated attack start/stop | | |
| 11 | Memory stable (check heap after each test) | | |
| 12 | NeoPixels show red during attacks | | |
