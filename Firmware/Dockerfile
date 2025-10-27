FROM ubuntu:latest AS upstream

ARG DEBIAN_FRONTEND=noninteractive

ENV FILENAME_PREFIX=RTK_Firmware
ENV FIRMWARE_VERSION_MAJOR=99
ENV FIRMWARE_VERSION_MINOR=99
ENV CORE_VERSION=2.0.2

# ESP32 Core Debug Level
# We use "none" for releases and "error" for release_candidate
# You may find "verbose" useful while you are debugging your changes
ENV DEBUG_LEVEL=error

# Developer Mode
# You may find "true" useful while you are making changes
# Set to false for releases
ENV ENABLE_DEVELOPER=true

# If you have your own u-blox PointPerfect token, define it here
# or pass it in as an arg when building the Dockerfile
ARG POINTPERFECT_TOKEN=0xAA,0xBB,0xCC,0xDD,0x00,0x11,0x22,0x33,0x0A,0x0B,0x0C,0x0D,0x00,0x01,0x02,0x03

# Get curl and python3
RUN apt-get update \
    && apt-get install -y curl python3 python3-pip python3-venv python-is-python3 \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*

# Avoid the externally managed environment constraint
RUN PYTHON_VER=$(ls /usr/lib | grep python3.) \
    && echo "Python version: ${PYTHON_VER}" \
    && rm /usr/lib/${PYTHON_VER}/EXTERNALLY-MANAGED

# Install Python dependencies - esptool needs pyserial
#RUN python3 -m pip install --upgrade pip && \
RUN pip install pyserial

# Setup Arduino CLI
#RUN curl -fsSL https://raw.githubusercontent.com/arduino/arduino-cli/master/install.sh | BINDIR=/usr/local/bin sh
RUN curl -fsSL https://raw.githubusercontent.com/arduino/arduino-cli/master/install.sh | sh

# Start config file
RUN arduino-cli config init --additional-urls https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json,https://espressif.github.io/arduino-esp32/package_esp32_dev_index.json

# Update core index
RUN arduino-cli core update-index

# Update library index
RUN arduino-cli lib update-index

# Install platform
RUN arduino-cli core install "esp32:esp32@${CORE_VERSION}"

# Get Known Libraries
RUN arduino-cli lib install ArduinoJson@6.19.4
RUN arduino-cli lib install ESP32Time@2.0.0
RUN arduino-cli lib install ESP32_BleSerial@1.0.5
RUN arduino-cli lib install "ESP32-OTA-Pull"@1.0.0
RUN arduino-cli lib install Ethernet@2.0.2
RUN arduino-cli lib install JC_Button@2.1.2
RUN arduino-cli lib install PubSubClient@2.8.0
RUN arduino-cli lib install "SdFat"@2.1.1
RUN arduino-cli lib install "SparkFun LIS2DH12 Arduino Library"@1.0.3
RUN arduino-cli lib install "SparkFun MAX1704x Fuel Gauge Arduino Library"@1.0.4
RUN arduino-cli lib install "SparkFun u-blox GNSS v3"@3.0.14
RUN arduino-cli lib install "SparkFun_WebServer_ESP32_W5500"@1.5.5
RUN arduino-cli lib install "SparkFun Qwiic OLED Arduino Library"@1.0.13
RUN arduino-cli lib install SSLClientESP32@2.0.0

# Enable external libs
RUN arduino-cli config set library.enable_unsafe_install true

# Get external libs
RUN arduino-cli lib install --git-url https://github.com/me-no-dev/ESPAsyncWebServer.git
RUN arduino-cli lib install --git-url https://github.com/me-no-dev/AsyncTCP.git

# Copy RTK_Everywhere into /work and build deployment image
FROM upstream AS deployment

# Create work directory
WORKDIR /work
ADD . .

# Get current date
#RUN echo "$(date +'%b_%d_%Y')" > date_scores.txt
#RUN DATE_SCORES=$(cat date_scores.txt) && echo "Date: ${DATE_SCORES}"
#RUN echo "$(date +'%b %d %Y')" > date_no_scores.txt
#RUN DATE_NO_SCORES=$(cat date_no_scores.txt) && echo "Date: ${DATE_NO_SCORES}"

# Patch Server.h to avoid https://github.com/arduino-libraries/Ethernet/issues/88#issuecomment-455498941
WORKDIR /work/RTK_Surveyor/Patch
RUN cp Server.h "/root/.arduino15/packages/esp32/hardware/esp32/${CORE_VERSION}/cores/esp32/Server.h"

# Update form.h with index_html and main_js
WORKDIR /work/Tools
RUN python index_html_zipper.py ../RTK_Surveyor/AP-Config/index.html ../RTK_Surveyor/form.h
RUN python main_js_zipper.py ../RTK_Surveyor/AP-Config/src/main.js ../RTK_Surveyor/form.h

# Copy custom app3M_fat9M_16MB.csv
WORKDIR /work
RUN cp app3M_fat9M_16MB.csv "/root/.arduino15/packages/esp32/hardware/esp32/${CORE_VERSION}/tools/partitions/app3M_fat9M_16MB.csv"

# Compile Sketch
WORKDIR /work/RTK_Surveyor
RUN arduino-cli compile --fqbn "esp32:esp32:esp32":DebugLevel=${DEBUG_LEVEL} \
    ./RTK_Surveyor.ino \
    --build-property build.partitions=app3M_fat9M_16MB \
    --build-property upload.maximum_size=3145728 \
    --build-property "compiler.cpp.extra_flags=\"-DPOINTPERFECTTOKEN=${POINTPERFECT_TOKEN}\" \
    \"-DFIRMWARE_VERSION_MAJOR=${FIRMWARE_VERSION_MAJOR}\" \
    \"-DFIRMWARE_VERSION_MINOR=${FIRMWARE_VERSION_MINOR}\" \
    \"-DENABLE_DEVELOPER=${ENABLE_DEVELOPER}\"" \
    --export-binaries

# Copy the compile output. List the files
FROM deployment AS output
COPY --from=deployment /work/RTK_Surveyor/build/esp32.esp32.esp32 /
CMD echo $(ls /*.*)
