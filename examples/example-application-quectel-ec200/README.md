[![Anedya Documentation](https://img.shields.io/badge/Anedya-Documentation-blue?style=for-the-badge)](https://docs.anedya.io?utm_source=github&utm_medium=link&utm_campaign=github-examples&utm_content=esp32-idf)

# anedya-dev-esp-id-example


Anedya-Dev-ESP-IDF examples for the Quectel EC200 U/G.

--- 

## Getting Started with Anedya

Follow these steps to set up your project with Anedya:

1. **Create an Anedya Account**:
   - Sign up for an account at [Anedya](https://anedya.io) and log in.

2. **Create a New Project**:
   - In the Anedya Dashboard, create a new project to organize your devices and data.

3. **Define Variables**:
   - Define variables (e.g., `temperature`, `humidity`) for your project to track device data.

4. **Add a Node**:
   - Create a node to represent a physical device (e.g., "Room1" or "Study Room") to interact with the hardware.

> **Note**: Ensure you fill in the variable identifiers correctly, as they are crucial for mapping the data between your physical device and the Anedya platform.

---

## Menu-Config Setup

To configure your project with the correct settings in ESP-IDF, follow these steps:

1. **Set SDK Network Interface**:
   - AnedyaSDK->Network Options
   - Select "Network Interface: Quectel EC200"

2. **Disable certain NVS settings**:
   - Uncheck the following options:
     - Store PHY calibration data in NVS
     - WiFi NVS flash

3. **Select Flash Size**:
   - Set the flash size of your device.

> **Note**: This example has been tested with the ESP32 board with 4MB flash.

4. **Partition Table**:
   - Choose the `Custom partition table CSV` option.

5. **Configure Anedya Credentials**:
   - Provide the following credentials:
     - **Connection Key**
     - **Binding Secret** (Use binding secret to bind the device with the Anedya node) 
     - **Physical Device ID**

> **Note**: Ensure you have pre-authorized or bound the device with the node, otherwise Anedya will take the device as unauthorized.

6. **Adjust Stack Size**:
   - Set the "Main stack size" to `10240`.

---

## Example Usage

Depending on your use case, you can enable or disable tasks in the `main.c` file.

---

## Additional Resources

- **Anedya Python SDK**:
   - [PyPI](https://pypi.org/project/anedya-dev-sdk/)
   - [GitHub Repository](https://github.com/anedyaio/anedya-dev-sdk-python)

- **Official Anedya Documentation**:  
   - Visit the [Anedya Documentation](https://docs.anedya.io) for detailed guides and API references.

- **Anedya Website**:
   - Explore more at [Anedya.io](https://anedya.io/?utm_source=github&utm_medium=link&utm_campaign=github-examples&utm_content=esp32_quectel).
