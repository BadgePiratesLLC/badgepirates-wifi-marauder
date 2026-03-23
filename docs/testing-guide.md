# Testing Guide — BSidesKC Marauder Port

## Simulation Testing (No Hardware)
1. Compile check: `pio run -e bsideskc` must succeed with zero errors
2. Static analysis: review all `#ifdef BSIDESKC_BADGE` paths for correctness
3. Pin conflict audit: verify no GPIO used by two peripherals

## Hardware Smoke Test
1. Flash firmware via USB
2. Display shows Marauder boot screen
3. Touch input registers (check serial monitor for coordinates)
4. SD card mounts and lists files
5. WiFi scan returns nearby APs

## Feature Regression Matrix

| Feature | Phase | Status |
|---------|-------|--------|
| Display init | 2 | |
| Touch input | 2 | |
| Button navigation | 2 | |
| Rotary encoder | 2 | |
| WiFi scan | 3 | |
| Packet monitor | 3 | |
| Deauth | 3 | |
| Beacon spam | 3 | |
| PMKID capture | 3 | |
| Evil Portal | 3 | |
| BLE scan | 4 | |
| BLE skimmer detect | 4 | |
| SD PCAP save | 5 | |
| Settings persist | 5 | |
| NeoPixel feedback | 6 | |
| Battery display | 6 | |

## Known Issues
_(Track issues here as they arise)_
