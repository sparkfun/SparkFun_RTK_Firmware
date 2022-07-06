"""
This is a simple Python3 PyQt5 firmware upload GUI for the SparkFun RTK products - based on ESP32 esptool.exe

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
pyinstaller --onefile --clean --noconsole --distpath=./Windows_exe --icon=RTK.ico --add-binary="RTK_Surveyor.ino.partitions.bin;." --add-binary="RTK_Surveyor.ino.bootloader.bin;." --add-binary="boot_app0.bin;." --add-binary="RTK.png;." RTK_Firmware_Uploader_GUI.py
Linux:
pyinstaller --onefile --clean --noconsole --distpath=./Linux_exe --icon=RTK.ico --add-binary="RTK_Surveyor.ino.partitions.bin:." --add-binary="RTK_Surveyor.ino.bootloader.bin:." --add-binary="boot_app0.bin:." --add-binary="RTK.png:." RTK_Firmware_Uploader_GUI.py

Pyinstaller needs:
RTK_Firmware_Uploader_GUI.py (this file!)
RTK.ico (icon file for the .exe)
RTK.png (icon for the GUI widget)
esptool.py (v3.3, copied from https://github.com/espressif/esptool/releases/tag/v3.3)
RTK_Surveyor.ino.partitions.bin
RTK_Surveyor.ino.bootloader.bin
boot_app0.bin


MIT license

Please see the LICENSE.md for more details

"""

from typing import Iterator, Tuple

from PyQt5.QtCore import QSettings, QProcess, QTimer, Qt, QIODevice, pyqtSlot
from PyQt5.QtWidgets import QWidget, QLabel, QComboBox, QGridLayout, \
    QPushButton, QApplication, QLineEdit, QFileDialog, QPlainTextEdit, \
    QAction, QActionGroup, QMenu, QMenuBar, QMainWindow, QMessageBox
from PyQt5.QtGui import QCloseEvent, QTextCursor, QIcon, QFont
from PyQt5.QtSerialPort import QSerialPortInfo

import sys
import os

import esptool
from esptool import ESPLoader
from esptool import UnsupportedCommandError
from esptool import NotSupportedError
from esptool import NotImplementedInROMError
from esptool import FatalError

# Setting constants
SETTING_PORT_NAME = 'port_name'
SETTING_FILE_LOCATION = 'file_location'
SETTING_PARTITION_LOCATION = 'partition_location'
SETTING_BAUD_RATE = 'baud'

guiVersion = 'v1.3'

def gen_serial_ports() -> Iterator[Tuple[str, str, str]]:
    """Return all available serial ports."""
    ports = QSerialPortInfo.availablePorts()
    return ((p.description(), p.portName(), p.systemLocation()) for p in ports)

#https://stackoverflow.com/a/50914550
def resource_path(relative_path):
    """ Get absolute path to resource, works for dev and for PyInstaller """
    base_path = getattr(sys, '_MEIPASS', os.path.dirname(os.path.abspath(__file__)))
    return os.path.join(base_path, relative_path)

class messageRedirect:
    """Wrap a class around a QPlainTextEdit so we can redirect stdout and stderr to it"""

    def __init__(self, edit, out=None) -> None:
        self.edit = edit
        self.out = out

    def write(self, msg) -> None:
        if msg.startswith("\r"):
            self.edit.moveCursor(QTextCursor.StartOfLine, QTextCursor.KeepAnchor)
            self.edit.cut()
            self.edit.insertPlainText(msg[1:])
        else:
            self.edit.insertPlainText(msg)
        self.edit.ensureCursorVisible()
        self.edit.repaint()
        if self.out: # Echo to out (stdout) too if desired
            self.out.write(msg)

    def flush(self) -> None:
        None

    def isatty(self):
        return True

# noinspection PyArgumentList

class MainWidget(QWidget):
    """Main Widget."""

    def __init__(self, parent: QWidget = None) -> None:
        super().__init__(parent)

        # File location line edit
        self.file_label = QLabel(self.tr('Firmware File:'))
        self.fileLocation_lineedit = QLineEdit()
        self.file_label.setBuddy(self.fileLocation_lineedit)
        self.fileLocation_lineedit.setEnabled(False)
        self.fileLocation_lineedit.returnPressed.connect(self.on_browse_btn_pressed)

        # Browse for new file button
        self.browse_btn = QPushButton(self.tr('Browse'))
        self.browse_btn.setEnabled(True)
        self.browse_btn.pressed.connect(self.on_browse_btn_pressed)

        # Partition file location line edit
        self.partition_label = QLabel(self.tr('Partition File:'))
        self.partitionFileLocation_lineedit = QLineEdit()
        self.partition_label.setBuddy(self.partitionFileLocation_lineedit)
        self.partitionFileLocation_lineedit.setEnabled(False)
        self.partitionFileLocation_lineedit.returnPressed.connect(self.on_partition_browse_btn_pressed)

        # Browse for new file button
        self.partition_browse_btn = QPushButton(self.tr('Browse'))
        self.partition_browse_btn.setEnabled(True)
        self.partition_browse_btn.pressed.connect(self.on_partition_browse_btn_pressed)

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
        self.upload_btn = QPushButton(self.tr('  Upload Firmware  '))
        self.upload_btn.setFont(myFont)
        self.upload_btn.clicked.connect(self.on_upload_btn_pressed)

        # Messages Bar
        self.messages_label = QLabel(self.tr('Status / Warnings:'))

        # Messages Window
        self.messageBox = QPlainTextEdit()
        self.messageBox.setReadOnly(True)
        self.messageBox.clear()

        # Arrange Layout
        layout = QGridLayout()
        
        layout.addWidget(self.file_label, 1, 0)
        layout.addWidget(self.fileLocation_lineedit, 1, 1)
        layout.addWidget(self.browse_btn, 1, 2)

        layout.addWidget(self.partition_label, 2, 0)
        layout.addWidget(self.partitionFileLocation_lineedit, 2, 1)
        layout.addWidget(self.partition_browse_btn, 2, 2)

        layout.addWidget(self.port_label, 3, 0)
        layout.addWidget(self.port_combobox, 3, 1)
        layout.addWidget(self.refresh_btn, 3, 2)

        layout.addWidget(self.baud_label, 4, 0)
        layout.addWidget(self.baud_combobox, 4, 1)
        layout.addWidget(self.upload_btn, 4, 2)

        layout.addWidget(self.messages_label, 5, 0)
        layout.addWidget(self.messageBox, 6, 0, 5, 3)

        self.setLayout(layout)

        self.settings = QSettings()
        #self._clean_settings() # This will delete all existing settings! Use with caution!        
        self._load_settings()

    def writeMessage(self, msg) -> None:
        self.messageBox.moveCursor(QTextCursor.End)
        self.messageBox.ensureCursorVisible()
        self.messageBox.appendPlainText(msg)
        self.messageBox.ensureCursorVisible()
        self.messageBox.repaint()

    def _load_settings(self) -> None:
        """Load settings on startup."""
        port_name = self.settings.value(SETTING_PORT_NAME)
        if port_name is not None:
            index = self.port_combobox.findData(port_name)
            if index > -1:
                self.port_combobox.setCurrentIndex(index)

        lastFile = self.settings.value(SETTING_FILE_LOCATION)
        if lastFile is not None:
            self.fileLocation_lineedit.setText(lastFile)

        lastFile = self.settings.value(SETTING_PARTITION_LOCATION)
        if lastFile is not None:
            self.partitionFileLocation_lineedit.setText(lastFile)
        else:
            self.partitionFileLocation_lineedit.setText(resource_path("RTK_Surveyor.ino.partitions.bin"))

        baud = self.settings.value(SETTING_BAUD_RATE)
        if baud is not None:
            index = self.baud_combobox.findData(baud)
            if index > -1:
                self.baud_combobox.setCurrentIndex(index)

    def _save_settings(self) -> None:
        """Save settings on shutdown."""
        self.settings.setValue(SETTING_PORT_NAME, self.port)
        self.settings.setValue(SETTING_FILE_LOCATION, self.theFileName)
        self.settings.setValue(SETTING_PARTITION_LOCATION, self.thePartitionFileName)
        self.settings.setValue(SETTING_BAUD_RATE, self.baudRate)

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
        # Highest speed first so code defaults to that
        # if settings.value(SETTING_BAUD_RATE) is None
        self.baud_combobox.clear()
        self.baud_combobox.addItem("921600", 921600)
        self.baud_combobox.addItem("460800", 460800)
        self.baud_combobox.addItem("115200", 115200)

    @property
    def port(self) -> str:
        """Return the current serial port."""
        return str(self.port_combobox.currentData())

    @property
    def baudRate(self) -> str:
        """Return the current baud rate."""
        return str(self.baud_combobox.currentData())

    @property
    def theFileName(self) -> str:
        """Return the file name."""
        return self.fileLocation_lineedit.text()

    @property
    def thePartitionFileName(self) -> str:
        """Return the partition file name."""
        return self.partitionFileLocation_lineedit.text()

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

    def on_browse_btn_pressed(self) -> None:
        """Open dialog to select bin file."""
        options = QFileDialog.Options()
        fileName, _ = QFileDialog.getOpenFileName(
            None,
            "Select Firmware to Upload",
            "",
            "Firmware Files (*.bin);;All Files (*)",
            options=options)
        if fileName:
            self.fileLocation_lineedit.setText(fileName)

    def on_partition_browse_btn_pressed(self) -> None:
        """Open dialog to select partition bin file."""
        options = QFileDialog.Options()
        fileName, _ = QFileDialog.getOpenFileName(
            None,
            "Select Partition File",
            "",
            "Parition Files (*.bin);;All Files (*)",
            options=options)
        if fileName:
            self.partitionFileLocation_lineedit.setText(fileName)

    def on_upload_btn_pressed(self) -> None:
        """Upload the firmware"""
        portAvailable = False
        for desc, name, sys in gen_serial_ports():
            if (sys == self.port):
                portAvailable = True
        if (portAvailable == False):
            self.writeMessage("Port No Longer Available")
            return

        fileExists = False
        try:
            f = open(self.fileLocation_lineedit.text())
            fileExists = True
        except IOError:
            fileExists = False
        finally:
            if (fileExists == False):
                self.writeMessage("File Not Found")
                return
            f.close()

        try:
            self._save_settings() # Save the settings in case the upload crashes
        except:
            pass

        self.writeMessage("Uploading firmware\n")

        command = []
        #command.extend(["--trace"]) # Useful for debugging
        command.extend(["--chip","esp32"])
        command.extend(["--port",self.port])
        command.extend(["--baud",self.baudRate])
        command.extend(["--before","default_reset","--after","hard_reset","write_flash","-z","--flash_mode","dio","--flash_freq","80m","--flash_size","detect"])
        command.extend(["0x1000",resource_path("RTK_Surveyor.ino.bootloader.bin")])
        command.extend(["0x8000",self.thePartitionFileName])
        command.extend(["0xe000",resource_path("boot_app0.bin")])
        command.extend(["0x10000",self.theFileName])

        self.writeMessage("Command: esptool.main(%s)\n\n" % " ".join(command))

        #print("python esptool.py %s\n\n" % " ".join(command)) # Useful for debugging - cut and paste into a command prompt

        try:
            esptool.main(command)
        except (ValueError, IOError, FatalError, ImportError, NotImplementedInROMError, UnsupportedCommandError, NotSupportedError, RuntimeError) as err:
            self.writeMessage(str(err))
        except:
            pass


if __name__ == '__main__':
    from sys import exit as sysExit
    app = QApplication([])
    app.setOrganizationName('SparkFun')
    app.setApplicationName('SparkFun RTK Firmware Uploader ' + guiVersion)
    app.setWindowIcon(QIcon(resource_path("RTK.png")))
    w = MainWidget()
    if 1: # Change to 0 to have the messages echoed on stdout
        sys.stdout = messageRedirect(w.messageBox) # Divert stdout to messageBox
        sys.stderr = messageRedirect(w.messageBox) # Divert stderr to messageBox
    else:
        sys.stdout = messageRedirect(w.messageBox, sys.stdout) # Echo to stdout too
        sys.stderr = messageRedirect(w.messageBox, sys.stderr) # Echo to stderr too
    w.show()
    sys.exit(app.exec_())
