# Hardware Verification — BSidesKC Badge

## PSRAM Detection
- [ ] Run `ESP.getPsramSize()` — record result
- [ ] If PSRAM present, verify speed/mode (QIO vs QSPI)
- [ ] Document actual chip markings on PCB

## Flash Verification
- [ ] Confirm flash size: expected 16MB (board JSON says 8MB N8)
- [ ] Verify QIO mode at 80MHz

## Display (ILI9341)
- [ ] SPI pins: MOSI(11) SCLK(12) CS(10) DC(5) RST(4) BL(6)
- [ ] Backlight control works
- [ ] Color test pattern renders correctly

## Touch (FT6336U)
- [ ] I2C address detected (SDA:8, SCL:9)
- [ ] Touch coordinates map correctly to display
- [ ] INT pin (7) fires on touch events

## SD Card
- [ ] SPI pins: MOSI(35) SCK(36) MISO(37) CS(47)
- [ ] Card detect and mount
- [ ] Read/write test

## Buttons & Encoder
- [ ] Boot(0), Enter(38), Back(39) — debounce verified
- [ ] Rotary encoder A(45) B(48) Button(20) — direction correct

## NeoPixels & Buzzer
- [ ] 6× WS2812B on GPIO 18
- [ ] Status LED on GPIO 21
- [ ] Buzzer on GPIO 19

## Battery (MAX17048)
- [ ] I2C detection on shared bus
- [ ] Voltage and SOC readings valid
