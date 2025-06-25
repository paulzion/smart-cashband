# SMART CASHBAND: RFID AND BIOMETRIC SECURITY WITH BLOCKCHAIN INTEGRATION

A secure physical unlocking and blockchain-logged authentication system that combines **RFID, fingerprint biometrics**, and **ESP32-controlled hardware** with **blockchain** for tamper-proof tracking. This project is ideal for any **government or organizational use** where secure asset handling is required not limited to salary distribution.

---

## 🙏 Acknowledgement First

All glory and thanks to **my Lord Jesus Christ**, by whose grace this project was made possible.

---

## 📦 Project Structure

```
smart-cashband/
├── firmware/        # PlatformIO ESP32 code (RFID, Fingerprint, Relay, Wi-Fi)
│   ├── blockchain_interface.cpp
│   ├── main.cpp
│   └── platformio.env
├── blockchain/      # Hardhat smart contract, scripts, ContractABI.js
│   ├── ContractABI.js
│   ├── Lock.js
│   ├── RFIDAccess.sol
│   ├── hardhat.config.js
│   ├── index.js
│   ├── package.json
│   ├── package-lock.json
│   └── test.js
├── media/           # Demo video and images
├── LICENSE          # MIT License
├── README.md        # This file
└── .gitignore       # Ignore build & system files
```

---

## ⚙️ Hardware Requirements

| Component               | Quantity |
| ----------------------- | -------- |
| ESP32 Dev Board         | 1        |
| R307 Fingerprint Sensor | 1        |
| MFRC522 RFID Module     | 1        |
| JF-0630B Solenoid Lock  | 1        |
| 5V Relay Module         | 1        |
| SW-420 Tilt Sensor      | 1        |
| 12V Adapter (for lock)  | 1        |
| Jumper Wires (Custom)   | Many     |

> ⚠️ **Make yourself 1-to-3 pin female-to-female jumper wires** using wire cutters and soldering. These help in managing limited GPIO pins and multiple ground/power connections.

---

## 🔌 Hardware Wiring

### 🔹 Fingerprint Sensor (R307)

* Red wire  → **3.3V** (ESP32)
* Black wire → **GND**
* Green wire → **TX** (ESP32 GPIO3 / RX0)
* White wire → **RX** (ESP32 GPIO1 / TX0)

### 🔹 RFID (MFRC522)

* VCC       → **3.3V**
* GND       → **GND**
* RST       → **GPIO 4**
* SDA       → **GPIO 5**
* MOSI      → **GPIO 23**
* MISO      → **GPIO 19**
* SCK       → **GPIO 18**

### 🔹 SW-420 Tilt Sensor

* VCC       → **3.3V**
* GND       → **GND**
* DO        → **GPIO 15**

### 🔹 Relay + Solenoid Lock Wiring

* Relay IN       → **GPIO 2**
* Relay VCC      → **3.3V**
* Relay GND      → **GND**

> 🔐 Solenoid Lock Powered by 12V Adapter:

* Red wire (from adapter) → **Relay COM**
* Blue wire 1 (lock) → **Relay NO**
* Blue wire 2 (lock) → **GND**
* Black wire (from adapter) → **GND splitter**

---

## 💻 Software Requirements

### 🔹 Firmware (PlatformIO)

* PlatformIO Core (VS Code)
* Board: `esp32dev`
* Framework: `arduino`
* Baud Rate: `115200`

**lib\_deps in ********`platformio.ini`********:**

```ini
lib_deps =
  https://github.com/miguelbalboa/rfid.git
  https://github.com/adafruit/Adafruit-Fingerprint-Sensor-Library.git
  https://github.com/tzapu/WiFiManager.git
```

### 🔹 Blockchain (Hardhat)

* Node.js (v16 or higher recommended)
* Hardhat: `npx hardhat`
* Local blockchain node: `npx hardhat node`

Project files include:

* `RFIDAccess.sol` smart contract
* `Lock.js`, `index.js`, and `deploy.js` scripts
* `ContractABI.js` used to interface ESP32 with blockchain events

---

## 🔗 Blockchain Integration Flow

1. ESP32 authenticates user (fingerprint + RFID)
2. Unlock event triggers relay and solenoid
3. RFID UID + unlock success is sent to a smart contract
4. Smart contract logs the event (`unlocked`, `transaction done`) on local chain
5. `ContractABI.js` is used in the firmware to interact with the contract

---

## 📘 Example Use Case Scenarios

### 🧑‍🏭 Scenario 1: Contractor Pays Temporary Worker

1. Temporary worker places fingerprint
2. RFID tag on cash bundle is read
3. If both match registered credentials, the solenoid unlocks the bundle
4. Blockchain logs: `worker_id`, `bundle_id`, `unlocked`, `payment_done`
5. Event is tamper-proof and auditable by authorities

### 🏛️ Scenario 2: Registrar Office Land Document Fee Submission

1. Citizen receives a cashband for submitting registration fee
2. Fingerprint + RFID authentication performed before unlocking bundle
3. Solenoid opens, money taken, event logged on-chain
4. Prevents bribery, ensures secure one-time access

---

## 🎥 Demo

Find the working video and screenshots inside the `media/` folder.

* `demo.mp4`: Live working demo of the SMART CASHBAND
* System wiring images
* RFID + fingerprint + blockchain unlocking in action

---

## 🧾 License

This project is licensed under the MIT License — free to use, modify, and distribute with credit.

---

## 🙌 Credits

* 🙏 My Lord and Savior **Jesus Christ** for wisdom, grace, and guidance
* 🧠 Developed by **Paul Zion D**
* 🤝 Special thanks to my friends **Pratheep M** and **Rokeshkumar B** for their help and support
* 👩‍🏫 **Nirmala B**, Assistant Professor, Computer Science and Engineering, DMI College of Engineering for her guidance and mentorship
* 🏫 DMI College of Engineering, Chennai – under 2021 regulation
* ❤️ Open-source community contributors

---

## 🏁 Status

✅ Completed (Firmware + Blockchain tested locally)
🔗 Future plan: deploy smart contract to public blockchain, integrate with dashboard

---

## 🚀 Future Enhancements

1. 🤖 **AI-based Estimation System**
   Integrate AI to estimate infrastructure project costs (e.g., flyovers) to detect and prevent fake or inflated budget estimations by corrupt officials.

2. 🌐 **Web3 Transparency Portal**
   Develop a decentralized website where all **Business-to-Government (B2G)** transactions are transparently logged on the blockchain and viewable by the public for maximum accountability.

3. 🎥 **Smart Contract Video Recording**
   Mandate that every physical contract or transaction is videographed, uploaded, and linked to the corresponding blockchain entry. The video can be viewed on the portal by the general public.

4. 🕵️‍♂️ **Anonymous Grievance Blockchain System**
   Create a secure grievance reporting mechanism for citizens to anonymously report bribery or harassment by officials. The blockchain ensures tamper-proof records of all complaints.

5. 🧑‍💼 **Privileged Access Web3 Interface**
   The Web3 site will support multi-role access levels:

   | Role                 | Description                                  |
   | -------------------- | -------------------------------------------- |
   | Public               | Can view transactions and contract videos    |
   | Admin                | Oversees all registered modules              |
   | Worker (under Admin) | Performs on-ground contract duties           |
   | Government Executive | Authorizes and validates major actions       |
   | Contractor           | Registered bidder and executor of contracts  |
   | Government Official  | Provides citizen services and utility access |

6. 💰 **Denomination-wise Rupee Counting at Exit Points**
   Mandate denomination-wise counting and logging of Rupee notes when public users or officials exit government offices. Ensures strict accountability and makes unrecorded cash movement traceable.

These enhancements push the SMART CASHBAND project from hardware-based unlocking to full-scale **anti-corruption infrastructure with AI and blockchain**, serving citizens and officials alike.

---

## 📫 Contact

For inquiries, improvements, or collaborations, contact:
📧 paulzion0234@gmail.com

---

## 📄 Publication

This project has been published in the **International Research Journal of Modernization in Engineering Technology and Science (IRJMETS)**.

---

> “Designed for transparency. Built for security. Ready for any government asset distribution mission.”
