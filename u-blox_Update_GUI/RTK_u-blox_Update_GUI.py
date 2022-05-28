"""
This is a simple Python3 PyQt5 u-blox firmware update GUI for the SparkFun RTK products.
It is a wrapper for u-blox's ubxfwupdate.exe. The actual updating is done by ubxfwupdate.exe.
This GUI checks which device you are trying to update before calling ubxfwupdate.exe.
This is to avoid you accidentally updating a NEO-D9S with ZED-F9P firmware and vice versa.
(Don't worry if that happens. You can recover. But you need access to the SafeBoot pin to
 force the module into a safe state for re-updating.)
If you _really_ know what you are doing, you can disable this check with the "Override" option.

ubxfwupdate.exe is a Windows executable. This tool will only work on Windows. Sorry about that.

Please make sure you are using Python3. You will see a bunch of errors with Python2.
To install PyQt5:
  pip install PyQt5
or
  pip3 install PyQt5
or
  sudo apt-get install python3-pyqt5
You may also need:
  sudo apt-get install python3-pyqt5.qtserialport

Pyinstaller:
Windows:
pyinstaller --onefile --clean --noconsole --distpath=./Windows_exe --icon=RTK.ico --add-binary="ubxfwupdate.exe;." --add-binary="libMPSSE.dll;." --add-binary="flash.xml;." --add-binary="RTK.png;." RTK_u-blox_Update_GUI.py

Pyinstaller needs:
RTK_u-blox_Update_GUI.py (this file!)
RTK.ico (icon file for the .exe)
RTK.png (icon for the GUI widget)
ubxfwupdate.exe (copied from u-center v22.02)
libMPSSE.dll (copied from u-center v22.02)
flash.xml (copied from u-center v22.02)

MIT license

Please see the LICENSE.md for more details

"""

from typing import Iterator, Tuple

from PyQt5.QtCore import QSettings, QProcess, QTimer, Qt, QIODevice, pyqtSlot
from PyQt5.QtWidgets import QWidget, QLabel, QComboBox, QGridLayout, \
    QPushButton, QApplication, QLineEdit, QFileDialog, QPlainTextEdit, \
    QAction, QActionGroup, QMenu, QMenuBar, QMainWindow, QMessageBox, \
    QCheckBox
from PyQt5.QtGui import QCloseEvent, QTextCursor, QIcon, QFont
from PyQt5.QtSerialPort import QSerialPortInfo, QSerialPortInfo

import sys
import os

import serial
import time

# Setting constants
SETTING_PORT_NAME = 'port_name'
SETTING_FIS_LOCATION = 'fis_location'
SETTING_FIRMWARE_LOCATION = 'firmware_location'
SETTING_BAUD_RATE = 'baud'

guiVersion = 'v1.0'

def gen_serial_ports() -> Iterator[Tuple[str, str, str]]:
    """Return all available serial ports."""
    ports = QSerialPortInfo.availablePorts()
    return ((p.description(), p.portName(), p.systemLocation()) for p in ports)

#https://stackoverflow.com/a/50914550
def resource_path(relative_path):
    """ Get absolute path to resource, works for dev and for PyInstaller """
    base_path = getattr(sys, '_MEIPASS', os.path.dirname(os.path.abspath(__file__)))
    return os.path.join(base_path, relative_path)

# noinspection PyArgumentList

class MainWidget(QWidget):
    """Main Widget."""

    def __init__(self, parent: QWidget = None) -> None:
        super().__init__(parent)

        self.p = None # This will be the updater QProcess

        # Firmware file location line edit
        self.firmware_label = QLabel(self.tr('Firmware File:'))
        self.firmwareLocation_lineedit = QLineEdit()
        self.firmware_label.setBuddy(self.firmwareLocation_lineedit)
        self.firmwareLocation_lineedit.setEnabled(False)
        self.firmwareLocation_lineedit.returnPressed.connect(
            self.on_browse_firmware_btn_pressed)

        # Browse for new file button
        self.firmware_browse_btn = QPushButton(self.tr('Browse'))
        self.firmware_browse_btn.setEnabled(True)
        self.firmware_browse_btn.pressed.connect(self.on_browse_firmware_btn_pressed)

        # FIS file location line edit
        self.fis_label = QLabel(self.tr('FIS File:'))
        self.fisLocation_lineedit = QLineEdit()
        self.fis_label.setBuddy(self.fisLocation_lineedit)
        self.fisLocation_lineedit.setEnabled(False)
        self.fisLocation_lineedit.returnPressed.connect(
            self.on_browse_fis_btn_pressed)

        # Browse for new file button
        self.fis_browse_btn = QPushButton(self.tr('Browse'))
        self.fis_browse_btn.setEnabled(True)
        self.fis_browse_btn.pressed.connect(self.on_browse_fis_btn_pressed)

        # Clear Button
        self.clear_fis_btn = QPushButton(self.tr('Clear'))
        self.clear_fis_btn.clicked.connect(self.on_clear_fis_btn_pressed)

        # Port Combobox
        self.port_label = QLabel(self.tr('COM Port:'))
        self.port_combobox = QComboBox()
        self.port_label.setBuddy(self.port_combobox)
        self.update_com_ports()

        # Refresh Button
        self.refresh_btn = QPushButton(self.tr('Refresh'))
        self.refresh_btn.clicked.connect(self.on_refresh_btn_pressed)

        # Baudrate Combobox
        self.baud_label = QLabel(self.tr('Baud Rate:'))
        self.baud_combobox = QComboBox()
        self.baud_label.setBuddy(self.baud_combobox)
        self.update_baud_rates()

        # Upload Button
        myFont=QFont()
        myFont.setBold(True)
        self.upload_btn = QPushButton(self.tr('  Update Firmware  '))
        self.upload_btn.setFont(myFont)
        self.upload_btn.clicked.connect(self.on_upload_btn_pressed)

        # Packet Dump Button
        self.packet_dump_btn = QCheckBox(self.tr('Packet Dump'))
        self.packet_dump_btn.setChecked(False)

        # Safeboot Button
        self.safeboot_btn = QCheckBox(self.tr('Enter Safeboot'))
        self.safeboot_btn.setChecked(True)
        self.safeboot_btn.toggled.connect(lambda:self.button_state(self.safeboot_btn))

        # Training Button
        self.training_btn = QCheckBox(self.tr('Training Sequence'))
        self.training_btn.setChecked(True)

        # Reset After Button
        self.reset_btn = QCheckBox(self.tr('Reset After'))
        self.reset_btn.setChecked(True)

        # Chip Erase Button
        self.chip_erase_btn = QCheckBox(self.tr('Chip Erase'))
        self.chip_erase_btn.setChecked(False)

        # Override Button
        self.override_btn = QCheckBox(self.tr('Override'))
        self.override_btn.setChecked(False)
        self.override_btn.toggled.connect(lambda:self.button_state(self.override_btn))

        # Messages Bar
        self.messages_label = QLabel(self.tr('Status / Warnings:'))

        # Messages Window
        messageFont=QFont("Courier")
        self.messageBox = QPlainTextEdit()
        self.messageBox.setFont(messageFont)

        # Arrange Layout
        layout = QGridLayout()
        
        layout.addWidget(self.firmware_label, 1, 0)
        layout.addWidget(self.firmwareLocation_lineedit, 1, 1)
        layout.addWidget(self.firmware_browse_btn, 1, 2)

        layout.addWidget(self.fis_label, 2, 0)
        layout.addWidget(self.fisLocation_lineedit, 2, 1)
        layout.addWidget(self.fis_browse_btn, 2, 2)
        layout.addWidget(self.clear_fis_btn, 2, 3)

        layout.addWidget(self.port_label, 3, 0)
        layout.addWidget(self.port_combobox, 3, 1)
        layout.addWidget(self.refresh_btn, 3, 2)

        layout.addWidget(self.baud_label, 4, 0)
        layout.addWidget(self.baud_combobox, 4, 1)
        layout.addWidget(self.upload_btn, 4, 2)

        layout.addWidget(self.packet_dump_btn, 4, 3)
        layout.addWidget(self.safeboot_btn, 5, 3)
        layout.addWidget(self.training_btn, 6, 3)
        layout.addWidget(self.reset_btn, 7, 3)
        layout.addWidget(self.chip_erase_btn, 8, 3)
        layout.addWidget(self.override_btn, 9, 3)

        layout.addWidget(self.messages_label, 9, 0)
        layout.addWidget(self.messageBox, 10, 0, 5, 4)

        self.setLayout(layout)

        self.settings = QSettings()
        #self._clean_settings() # This will delete all existing settings! Use with caution!
        self._load_settings()

    def button_state(self, b) -> None:
        if b.text() == "Enter Safeboot":
            if b.isChecked() == True:
                self.training_btn.setChecked(True)
                self.training_btn.setEnabled(True)
            else:
                self.training_btn.setChecked(False)
                self.training_btn.setEnabled(False)
        elif b.text() == "Override":
            if b.isChecked() == True:
                self.show_error_message(">>>>> Override enabled <<<<<\nFirmware version check is disabled")

    def writeMessage(self, msg) -> None:
        self.messageBox.moveCursor(QTextCursor.End)
        self.messageBox.ensureCursorVisible()
        self.messageBox.appendPlainText(msg)
        self.messageBox.ensureCursorVisible()
        self.messageBox.repaint()

    def insertPlainText(self, msg) -> None:
        if msg.startswith("\r"):
            self.messageBox.moveCursor(QTextCursor.StartOfLine, QTextCursor.KeepAnchor)
            self.messageBox.cut()
            self.messageBox.insertPlainText(msg[1:])
        else:
            self.messageBox.insertPlainText(msg)
        self.messageBox.ensureCursorVisible()
        self.messageBox.repaint()


    def _load_settings(self) -> None:
        """Load settings on startup."""
        port_name = self.settings.value(SETTING_PORT_NAME)
        if port_name is not None:
            index = self.port_combobox.findData(port_name)
            if index > -1:
                self.port_combobox.setCurrentIndex(index)

        baud = self.settings.value(SETTING_BAUD_RATE)
        if baud is not None:
            index = self.baud_combobox.findData(baud)
            if index > -1:
                self.baud_combobox.setCurrentIndex(index)

        lastFile = self.settings.value(SETTING_FIRMWARE_LOCATION)
        if lastFile is not None:
            self.firmwareLocation_lineedit.setText(lastFile)

        lastFile = self.settings.value(SETTING_FIS_LOCATION)
        if lastFile is not None:
            self.fisLocation_lineedit.setText(lastFile)
        else:
            self.fisLocation_lineedit.clear()

    def _save_settings(self) -> None:
        """Save settings on shutdown."""
        self.settings.setValue(SETTING_PORT_NAME, self.port)
        self.settings.setValue(SETTING_BAUD_RATE, self.baudRate)
        self.settings.setValue(SETTING_FIRMWARE_LOCATION, self.theFirmwareName)
        self.settings.setValue(SETTING_FIS_LOCATION, self.theFisName)

    def _clean_settings(self) -> None:
        """Clean (remove) all existing settings."""
        self.settings.clear()

    def show_error_message(self, msg: str) -> None:
        """Show a Message Box with the error message."""
        QMessageBox.critical(self, QApplication.applicationName(), str(msg))

    def update_com_ports(self) -> None:
        """Update COM Port list in GUI."""
        previousPort = self.port # Record the previous port before we clear the combobox
        
        self.port_combobox.clear()

        index = 0
        indexOfPrevious = -1
        for desc, name, sys in gen_serial_ports():
            longname = desc + " (" + name + ")"
            self.port_combobox.addItem(longname, sys)
            if(sys == previousPort): # Previous port still exists so record it
                indexOfPrevious = index
            index = index + 1

        if indexOfPrevious > -1: # Restore the previous port if it still exists
            self.port_combobox.setCurrentIndex(indexOfPrevious)

    def update_baud_rates(self) -> None:
        """Update baud rate list in GUI."""
        # Lowest speed first so code defaults to that
        # if settings.value(SETTING_BAUD_RATE) is None
        self.baud_combobox.clear()
        self.baud_combobox.addItem("9600", 9600)
        self.baud_combobox.addItem("38400", 38400)
        self.baud_combobox.addItem("115200", 115200)
        self.baud_combobox.addItem("230400", 230400)
        self.baud_combobox.addItem("460800", 460800)

    @property
    def port(self) -> str:
        """Return the current serial port."""
        return str(self.port_combobox.currentData())

    @property
    def baudRate(self) -> str:
        """Return the current baud rate."""
        return str(self.baud_combobox.currentData())

    @property
    def theFirmwareName(self) -> str:
        """Return the firmware file name."""
        return self.firmwareLocation_lineedit.text()

    @property
    def theFisName(self) -> str:
        """Return the FIS file name."""
        return self.fisLocation_lineedit.text()

    def closeEvent(self, event: QCloseEvent) -> None:
        """Handle Close event of the Widget."""
        try:
            self._save_settings()
        except:
            pass

        event.accept()

    def on_refresh_btn_pressed(self) -> None:
        self.update_com_ports()
        self.writeMessage("Ports Refreshed\n")

    def on_browse_firmware_btn_pressed(self) -> None:
        """Open dialog to select firmware file."""
        options = QFileDialog.Options()
        fileName, _ = QFileDialog.getOpenFileName(
            None,
            "Select Firmware to Upload",
            "",
            "Firmware Files (*.bin);;All Files (*)",
            options=options)
        if fileName:
            self.firmwareLocation_lineedit.setText(fileName)

    def on_browse_fis_btn_pressed(self) -> None:
        """Open dialog to select FIS file."""
        options = QFileDialog.Options()
        fileName, _ = QFileDialog.getOpenFileName(
            None,
            "Select FIS (Flash Information Structure) file",
            "",
            "FIS Files (*.xml);;All Files (*)",
            options=options)
        if fileName:
            self.fisLocation_lineedit.setText(fileName)

    def on_clear_fis_btn_pressed(self) -> None:
        """Clear the FIS filename."""
        self.fisLocation_lineedit.clear()

    def update_finished(self) -> None:
        """The update QProcess has finished."""
        self.writeMessage("Update has finished")
        self.p = None

    def handle_stderr(self):
        data = self.p.readAllStandardError()
        stderr = bytes(data).decode("utf8")
        self.insertPlainText(stderr)

    def handle_stdout(self):
        data = self.p.readAllStandardOutput()
        stdout = bytes(data).decode("utf8")
        self.insertPlainText(stdout)

    def on_upload_btn_pressed(self) -> None:
        """Update the firmware"""

        self.writeMessage("\n")

        portAvailable = False
        for desc, name, sys in gen_serial_ports():
            if (sys == self.port):
                portAvailable = True
        if (portAvailable == False):
            self.writeMessage("Port No Longer Available")
            return

        firmwareExists = False
        try:
            f = open(self.theFirmwareName)
            firmwareExists = True
        except IOError:
            firmwareExists = False
        finally:
            if (firmwareExists == False):
                self.writeMessage("Firmware file Not Found")
                return
            f.close()

        if self.theFisName != '':
            fisExists = False
            try:
                f = open(self.theFisName)
                fisExists = True
            except IOError:
                fisExists = False
            finally:
                if (fisExists == False):
                    self.writeMessage("FIS file Not Found")
                    return
                f.close()

        if self.p is not None:
            self.writeMessage("Update is already running")
            return

        # Open the port. Ask the module for UBX-MON-VER
        try:
            port = serial.Serial(self.port, baudrate=int(self.baudRate), timeout=1)
        except (ValueError, IOError) as err:
            self.writeMessage(str(err))
            return
        try:
            ubx_mon_ver = bytes([0xb5, 0x62, 0x0a, 0x04, 0x00, 0x00, 0x0e, (0x0a + 0x0e + 0x0e + 0x0e)])
            port.write(ubx_mon_ver)
        except (ValueError, IOError) as err:
            self.writeMessage(str(err))
            port.close()
            return

        # Collect 2 seconds worth of data
        self.writeMessage("Requesting the receiver and software version (UBX-MON-VER)\n")
        startTime = time.time()
        response = []
        while time.time() < (startTime + 2.0):
            response.append(port.read(100))
        port.close()

        response = b''.join(response)

        printable = ''
        for c in response:
            if (c >= ord(' ')) and (c <= ord('~')):
                printable += chr(c)
            if c == 0: # The UBX-MON-VER extensions are null-terminated. Convert the nulls to spaces
                printable += ' '

        if 0: # Change this to 1 to enable the diagnostic writeMessage
            self.writeMessage(printable)

        # Look for the MOD and FWVER
        mod = ''
        startPos = printable.find("MOD=")
        if startPos >= 0:
            endPos = printable.find(" ",startPos)
            if endPos >= 0:
                mod = printable[startPos : endPos]
                self.writeMessage(mod)
        fwver = ''
        startPos = printable.find("FWVER=")
        if startPos >= 0:
            endPos = printable.find(" ",startPos)
            if endPos >= 0:
                fwver = printable[startPos : endPos] # Only store the three letter product version
                endPos = printable.find(" ",endPos + 1)
                if endPos >= 0:
                    self.writeMessage(printable[startPos : endPos]) # Print the full product version

        if self.override_btn.isChecked() == False:

            if fwver == '':
                self.writeMessage("Unable to read the receiver and software version. Aborting...")
                return

            # Check that the three letter product version appears in the firmware filename
            self.writeMessage("\nChecking that " + fwver[6:] + " appears in the firmware filename")

            if self.theFirmwareName.find(fwver[6:]) < 0:
                self.writeMessage("\nThe firmware appears invalid for this module. Aborting...")
                return

        self.override_btn.setChecked(False)

        self.writeMessage("\nUpdating firmware\n")

        command = []
        command.extend(["-p",self.port])
        command.extend(["-b",self.baudRate + ":" + self.baudRate + ":115200"])
        if self.fisLocation_lineedit.text() != '':
            command.extend(["-F", self.theFisName])
        else:
            command.extend(["-F", resource_path("flash.xml")])
        if self.packet_dump_btn.isChecked() == True:
            command.extend(["-v","2"])
        else:
            command.extend(["-v","1"])
        if self.safeboot_btn.isChecked() == True:
            command.extend(["-s","1"])
            if self.training_btn.isChecked() == True:
                command.extend(["-t","1"])
            else:
                command.extend(["-t","0"])
        else:
            command.extend(["-s","0"])
        if self.reset_btn.isChecked() == True:
            command.extend(["-R","1"])
        else:
            command.extend(["-R","0"])
        if self.chip_erase_btn.isChecked() == True:
            command.extend(["-C","1"])
        else:
            command.extend(["-C","0"])
        command.append(self.theFirmwareName)

        self.writeMessage("ubxfwupdate.exe %s\n\n" % " ".join(command))

        if 1: # Change this to 0 to skip the actual update
            self.p = QProcess()
            self.p.readyReadStandardOutput.connect(self.handle_stdout)
            self.p.readyReadStandardError.connect(self.handle_stderr)
            self.p.finished.connect(self.update_finished)
            self.p.start(resource_path("ubxfwupdate.exe"), command)

if __name__ == '__main__':
    from sys import exit as sysExit
    app = QApplication([])
    app.setOrganizationName('SparkFun')
    app.setApplicationName('SparkFun RTK u-blox Firmware Update Tool ' + guiVersion)
    app.setWindowIcon(QIcon(resource_path("RTK.png")))
    w = MainWidget()
    w.show()
    sys.exit(app.exec_())
