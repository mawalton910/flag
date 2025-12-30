# Guru Games - Flag Capture System
**Current Version**: Flag v25.1.0

A territory control and flag capture system built on the ESP32 Dev Module (WROOM) for dynamic faction-based gameplay in live-action role-playing events.

## 🎮 What It Does

The Flag device creates dynamic territory control points for LARP players:

### For Players
- **Flag Capture**: Scan your badge to claim territory for your faction
- **Visual Feedback**: NeoPixel LED strip displays current faction ownership with colors
- **Mode Switching**: Device supports Flag, Relay, and Access modes
- **Real-time Updates**: Background tasks keep game state synchronized
- **Shimmer Effects**: Visual animations indicate ownership changes

### For Game Masters
- **Remote Updates**: Push firmware updates to all devices via OTA
- **WiFi Integration**: Automatic network selection based on signal strength
- **Multi-Mode Operation**: Switch between Flag capture, Relay, and Access control modes
- **Device Monitoring**: Track device status and ownership through server integration

## 🔧 Hardware

- **ESP32 Dev Module (WROOM)** with WiFi connectivity
- **MFRC522 RFID Reader** for badge scanning
- **NeoPixel LED Strip** for visual faction indicators
- Dual-core task management for responsive gameplay

## 📦 Operational Modes

### 1. Flag Mode (Default)
Territory control system where players can:
- Scan badge to capture flag for their faction
- Visual confirmation with faction-colored LEDs
- Server-side validation and ownership tracking
- Persistent faction control until captured by another team

### 2. Relay Mode
Quick badge scanning mode for game integration and badge status updates.

### 3. Access Mode
Access control system for location-based permissions and entry validation.

## 🎯 How Players Use It

1. **Approach Flag Station** → NeoPixel strip shows current faction ownership (or white if unclaimed)
2. **Scan Your Badge** → RFID reader detects your badge
3. **Capture Flag** → System validates and assigns ownership to your faction
4. **Visual Confirmation** → LED strip changes to your faction's color
5. **Hold Territory** → Flag remains your faction's until another team captures it

## 🎨 Visual Indicators

- **White LEDs**: Flag is unclaimed or neutral
- **Faction Colors**: LED strip displays the color of the controlling faction
- **Shimmer Effect**: Animated color transitions during ownership changes
- **Random Colors**: Indicates system activity or mode changes

## 📡 Over-the-Air Updates

Devices support remote firmware updates for seamless version management:
- Scan a designated trigger card to start update
- Device automatically downloads and installs new firmware
- Progress displayed via Serial output with percentage and download statistics
- Automatic reboot after successful installation
- All devices can be updated simultaneously

## 🎮 Game Integration

The system integrates with backend servers to:
- Track flag ownership in real-time
- Validate faction membership for captures
- Record capture history with timestamps
- Synchronize game state across all devices
- Provide analytics for territory control

## 🔐 Security

- Physical RFID badge required for all captures
- Faction-based access control
- Debounce protection to prevent duplicate scans
- Encrypted communication with backend servers
- Server-side validation of all capture attempts

## 🛠️ Technical Features

### Dual-Core Architecture
- **Core 0**: Background tasks for game state synchronization
- **Core 1**: Main loop for RFID scanning and LED control

### WiFi Management
- Automatic network selection based on signal strength
- Fallback to strongest available network
- Connection retry logic for reliability

### LED Control
- Smooth color transitions and animations
- Faction-specific color mapping
- Shimmer effects for visual appeal
- Low-level brightness control for battery efficiency

## 🙏 Acknowledgments

Built with ESP32 Dev Module (WROOM), MFRC522 RFID module, NeoPixel LEDs, and the Arduino framework.
