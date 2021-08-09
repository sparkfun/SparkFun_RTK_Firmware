var ws = new WebSocket("ws://192.168.1.228/ws"); //WiFi mode
//var ws = new WebSocket("ws://192.168.1.1/ws"); //AP Mode

ws.onmessage = function (msg) {
    parseIncoming(msg.data);
};

function ge(e) {
    return document.getElementById(e);
}

var platformPrefix = "Surveyor";

function parseIncoming(msg) {
    console.log("incoming message: " + msg);

    var data = msg.split(',');
    for (let x = 0; x < data.length - 1; x += 2) {
        var id = data[x];
        var val = data[x + 1];
        //console.log("id: " + id + ", val: " + val);

        //Special commands
        if (id.includes("sdMounted")) {
            //Turn on/off SD area
            if (val == "false") {
                ge("sdMounted").style.display = "none";
            }
            else if (val == "true") {
                ge("sdMounted").style.display = "block";
            }
        }
        else if (id == "platformPrefix") {
            platformPrefix = val;
            ge(id).innerHTML = "RTK " + val + " Setup";
            document.title = "RTK " + val + " Setup";

            if (platformPrefix == "Surveyor") ge("dataPortChannelDropdown").style.display = "none";
            if (platformPrefix == "Express Plus") ge("muxChannel2").innerHTML = "Wheel/Dir Encoder";
        }
        else if (id.includes("sdFreeSpace")
            || id.includes("sdUsedSpace")
            || id.includes("rtkFirmwareVersion")
            || id.includes("zedFirmwareVersion")
        ) {
            ge(id).innerHTML = val;
        }
        else if (id.includes("firmwareFileName")) {
            ge("firmwareAvailable").style.display = "block"; //Turn on firmware area

            ge(id).innerHTML = val;
            if (id.includes("0")) ge("firmwareFile0").style.display = "block";
            if (id.includes("1")) ge("firmwareFile1").style.display = "block";
            if (id.includes("2")) ge("firmwareFile2").style.display = "block";
            if (id.includes("3")) ge("firmwareFile3").style.display = "block";
            if (id.includes("4")) ge("firmwareFile4").style.display = "block";
            if (id.includes("5")) ge("firmwareFile5").style.display = "block";
        }

        //Check boxes / radio buttons
        else if (val == "true") {
            ge(id).checked = true;
        }
        else if (val == "false") {
            ge(id).checked = false;
        }

        //All regular input boxes and values
        else {
            ge(id).value = val;
        }
    }
    //console.log("Settings loaded");

    //Force element updates
    ge("measurementRateHz").dispatchEvent(new Event('change'));
    ge("baseTypeSurveyIn").dispatchEvent(new Event('change'));
    ge("baseTypeFixed").dispatchEvent(new Event('change'));
    ge("fixedBaseCoordinateTypeECEF").dispatchEvent(new Event('change'));
    ge("fixedBaseCoordinateTypeGeo").dispatchEvent(new Event('change'));
    ge("enableLogging").dispatchEvent(new Event('change'));
    ge("enableNtripServer").dispatchEvent(new Event('change'));
    ge("dataPortChannel").dispatchEvent(new Event('change'));
}

//Create CSV of all setting data
function sendData() {
    var settingCSV = "";

    //Input boxes
    var clsElements = document.querySelectorAll(".form-control, .form-dropdown");
    for (let x = 0; x < clsElements.length; x++) {
        settingCSV += clsElements[x].id + "," + clsElements[x].value + ",";
    }

    //Check boxes, radio buttons
    clsElements = document.querySelectorAll(".form-check-input, .form-radio");
    for (let x = 0; x < clsElements.length; x++) {
        settingCSV += clsElements[x].id + "," + clsElements[x].checked + ",";
    }

    //console.log(settingCSV);
    ws.send(settingCSV);
}

function sendFirmwareFile() {
    var firmwareFileName = "firmwareFileName,";

    //ID the firmware file radio
    if (ge("file0").checked) firmwareFileName += ge("firmwareFileName0").innerHTML;
    else if (ge("file1").checked) firmwareFileName += ge("firmwareFileName1").innerHTML;
    else if (ge("file2").checked) firmwareFileName += ge("firmwareFileName2").innerHTML;
    else if (ge("file3").checked) firmwareFileName += ge("firmwareFileName3").innerHTML;
    else if (ge("file4").checked) firmwareFileName += ge("firmwareFileName4").innerHTML;
    else if (ge("file5").checked) firmwareFileName += ge("firmwareFileName5").innerHTML;

    firmwareFileName += ","
    ws.send(firmwareFileName);
    ge("firmwareUpdateMsg").innerHTML = 'Updating, please wait for system reset...';
}

function showError(id, errorText) {
    ge(id + 'Error').innerHTML = 'Error: ' + errorText;
}

function clearError(id) {
    ge(id + 'Error').innerHTML = '';
}

function successMessage(id, msgText) {
    ge(id + 'Error').innerHTML = '<p style="color:green; display:inline;"><b>' + msgText + '</b></p>';
}

var errorCount = 0;

function validateFields() {
    //Collapse all sections
    ge("collapseGNSSConfig").classList.remove('show');
    ge("collapseGNSSConfigMsg").classList.remove('show');
    ge("collapseBaseConfig").classList.remove('show');
    ge("collapsePortsConfig").classList.remove('show');
    ge("collapseSystemConfig").classList.remove('show');

    errorCount = 0;
    //GNSS Config
    checkElementValue("measurementRateHz", 0.1, 10, "Must be between 0 and 10Hz", "collapseGNSSConfig");
    checkElementValue("UBX_NMEA_DTM", 0, 20, "Must be between 0 and 20", "collapseGNSSConfigMsg");

    //Base Config
    checkElementValue("observationSeconds", 60, 600, "Must be between 60 to 600", "collapseBaseConfig");
    checkElementValue("observationPositionAccuracy", 1, 5.1, "Must be between 1.0 to 5.0", "collapseBaseConfig");
    checkElementValue("fixedEcefX", -5000000, 5000000, "Must be -5000000 to 5000000", "collapseBaseConfig");
    checkElementValue("fixedEcefY", -5000000, 5000000, "Must be -5000000 to 5000000", "collapseBaseConfig");
    if (ge("fixedEcefZ").value == 0.0) ge("fixedEcefZ").value = 4084500;
    checkElementValue("fixedEcefZ", 4084500, 5000000, "Must be 4084500 to 5000000", "collapseBaseConfig");
    checkElementValue("fixedLat", -180, 180, "Must be -180 to 180", "collapseBaseConfig");
    checkElementValue("fixedLong", -180, 180, "Must be -180 to 180", "collapseBaseConfig");
    checkElementValue("fixedAltitude", 0, 8849, "Must be 0 to 8849", "collapseBaseConfig");

    checkElementString("wifiSSID", 1, 30, "Must be 1 to 30 characters", "collapseBaseConfig");
    checkElementString("wifiPW", 0, 30, "Must be 0 to 30 characters", "collapseBaseConfig");
    checkElementString("casterHost", 1, 30, "Must be 1 to 30 characters", "collapseBaseConfig");
    checkElementValue("casterPort", 1, 99999, "Must be 1 to 99999", "collapseBaseConfig");
    checkElementString("mountPoint", 1, 30, "Must be 1 to 30 characters", "collapseBaseConfig");
    checkElementString("mountPointPW", 1, 30, "Must be 1 to 30 characters", "collapseBaseConfig");

    //System Config
    checkElementValue("maxLogTime_minutes", 1, 2880, "Must be 1 to 2880", "collapseSystemConfig");

    //Port Config

    if (errorCount == 1) {
        showError('saveBtn', "Please clear " + errorCount + " error");
    }
    else if (errorCount > 1) {
        showError('saveBtn', "Please clear " + errorCount + " errors");
    }
    else {
        //Tell Arduino we're ready to save
        sendData();
        successMessage('saveBtn', "All saved");
    }
}

function checkElementValue(id, min, max, errorText, collapseID) {
    value = ge(id).value;
    if (value < min || value > max) {
        ge(id + 'Error').innerHTML = 'Error: ' + errorText;
        ge(collapseID).classList.add('show');
        if (collapseID == "collapseGNSSConfigMsg") ge("collapseGNSSConfig").classList.add('show');
        errorCount++;
    }
    else
        ge(id + 'Error').innerHTML = '';
}
function checkElementString(id, min, max, errorText, collapseID) {
    value = ge(id).value;
    if (value.length < min || value.length > max) {
        ge(id + 'Error').innerHTML = 'Error: ' + errorText;
        ge(collapseID).classList.add('show');
        errorCount++;
    }
    else
        ge(id + 'Error').innerHTML = '';
}

function resetToFactoryDefaults() {
    ge("factoryDefaultsMsg").innerHTML = "Defaults Applied. Please wait for device reset..."
    ws.send("factoryDefaultReset,1,");
}

function exitConfig() {
    ge("exitPage").style.display = "block"; //Show
    ge("mainPage").style.display = "none"; //Hide main page
    ws.send("exitToRoverMode,1,");
}

document.addEventListener("DOMContentLoaded", (event) => {

    ge("measurementRateHz").addEventListener("change", function () {
        ge("measurementRateSec").value = 1.0 / ge("measurementRateHz").value;
    });

    ge("measurementRateSec").addEventListener("change", function () {
        ge("measurementRateHz").value = 1.0 / ge("measurementRateSec").value;
    });

    ge("baseTypeSurveyIn").addEventListener("change", function () {
        if (ge("baseTypeSurveyIn").checked) {
            //Enable Survey based inputs
            ge("observationSeconds").disabled = false;
            ge("observationPositionAccuracy").disabled = false;

            //Disable Fixed base inputs
            ge("fixedEcefX").disabled = true;
            ge("fixedEcefY").disabled = true;
            ge("fixedEcefZ").disabled = true;
            ge("fixedLat").disabled = true;
            ge("fixedLong").disabled = true;
            ge("fixedAltitude").disabled = true;

            //Disable radio buttons
            ge("fixedBaseCoordinateTypeECEF").disabled = true;
            ge("fixedBaseCoordinateTypeGeo").disabled = true;
        }
    });

    ge("baseTypeFixed").addEventListener("change", function () {
        if (ge("baseTypeFixed").checked) {
            //Disable Survey based inputs
            ge("observationSeconds").disabled = true;
            ge("observationPositionAccuracy").disabled = true;

            //Enable radio buttons
            ge("fixedBaseCoordinateTypeECEF").disabled = false;
            ge("fixedBaseCoordinateTypeGeo").disabled = false;

            //Enable Fixed base inputs
            if (ge("fixedBaseCoordinateTypeECEF").checked) {
                //Enable ECEF inputs
                ge("fixedEcefX").disabled = false;
                ge("fixedEcefY").disabled = false;
                ge("fixedEcefZ").disabled = false;
                //Disable Geographic inputs
                ge("fixedLat").disabled = true;
                ge("fixedLong").disabled = true;
                ge("fixedAltitude").disabled = true;
            }
            else {
                //Disable ECEF inputs
                ge("fixedEcefX").disabled = true;
                ge("fixedEcefY").disabled = true;
                ge("fixedEcefZ").disabled = true;
                //Disable Geographic inputs
                ge("fixedLat").disabled = false;
                ge("fixedLong").disabled = false;
                ge("fixedAltitude").disabled = false;
            }
        }
    });

    ge("fixedBaseCoordinateTypeECEF").addEventListener("change", function () {
        //Enable Fixed base inputs
        if (ge("fixedBaseCoordinateTypeECEF").checked) {
            //Enable ECEF inputs
            ge("fixedEcefX").disabled = false;
            ge("fixedEcefY").disabled = false;
            ge("fixedEcefZ").disabled = false;
            //Disable Geographic inputs
            ge("fixedLat").disabled = true;
            ge("fixedLong").disabled = true;
            ge("fixedAltitude").disabled = true;
        }
        else {
            //Disable ECEF inputs
            ge("fixedEcefX").disabled = true;
            ge("fixedEcefY").disabled = true;
            ge("fixedEcefZ").disabled = true;
            //Disable Geographic inputs
            ge("fixedLat").disabled = false;
            ge("fixedLong").disabled = false;
            ge("fixedAltitude").disabled = false;
        }
    });

    ge("fixedBaseCoordinateTypeGeo").addEventListener("change", function () {
        //Enable Fixed base inputs
        if (ge("fixedBaseCoordinateTypeGeo").checked) {
            //Disable ECEF inputs
            ge("fixedEcefX").disabled = true;
            ge("fixedEcefY").disabled = true;
            ge("fixedEcefZ").disabled = true;
            //Disable Geographic inputs
            ge("fixedLat").disabled = false;
            ge("fixedLong").disabled = false;
            ge("fixedAltitude").disabled = false;
        }
        else {
            //Enable ECEF inputs
            ge("fixedEcefX").disabled = false;
            ge("fixedEcefY").disabled = false;
            ge("fixedEcefZ").disabled = false;
            //Disable Geographic inputs
            ge("fixedLat").disabled = true;
            ge("fixedLong").disabled = true;
            ge("fixedAltitude").disabled = true;
        }
    });

    ge("enableNtripServer").addEventListener("change", function () {
        if (ge("enableNtripServer").checked) {
            //Enable NTRIP inputs
            ge("wifiSSID").disabled = false;
            ge("wifiPW").disabled = false;
            ge("casterHost").disabled = false;
            ge("casterPort").disabled = false;
            ge("mountPoint").disabled = false;
            ge("mountPointPW").disabled = false;
        }
        else {
            //Disable NTRIP inputs
            ge("wifiSSID").disabled = true;
            ge("wifiPW").disabled = true;
            ge("casterHost").disabled = true;
            ge("casterPort").disabled = true;
            ge("mountPoint").disabled = true;
            ge("mountPointPW").disabled = true;
        }
    });

    ge("enableLogging").addEventListener("change", function () {
        if (ge("enableLogging").checked) {
            //Enable inputs
            ge("maxLogTime_minutes").disabled = false;
        }
        else {
            //Disable inputs
            ge("maxLogTime_minutes").disabled = true;
        }
    });

    ge("enableFactoryDefaults").addEventListener("change", function () {
        if (ge("enableFactoryDefaults").checked) {
            //Enable button
            ge("factoryDefaults").disabled = false;
        }
        else {
            //Disable button
            ge("factoryDefaults").disabled = true;
        }
    });

    //Enable the check box
    ge("firmwareFile0").addEventListener("change", function () { ge("enableFirmwareUpdate").disabled = false; });
    ge("firmwareFile1").addEventListener("change", function () { ge("enableFirmwareUpdate").disabled = false; });
    ge("firmwareFile2").addEventListener("change", function () { ge("enableFirmwareUpdate").disabled = false; });
    ge("firmwareFile3").addEventListener("change", function () { ge("enableFirmwareUpdate").disabled = false; });
    ge("firmwareFile4").addEventListener("change", function () { ge("enableFirmwareUpdate").disabled = false; });
    ge("firmwareFile5").addEventListener("change", function () { ge("enableFirmwareUpdate").disabled = false; });

    ge("enableFirmwareUpdate").addEventListener("change", function () {
        if (ge("enableFirmwareUpdate").checked) {
            //Enable button
            ge("firmwareUpdate").disabled = false;
        }
        else {
            //Disable button
            ge("firmwareUpdate").disabled = true;
        }
    });

    ge("dataPortChannel").addEventListener("change", function () {
        if (ge("dataPortChannel").value == 0) {
            //Show baud drop down
            ge("dataPortBaudDropdown").style.display = "block";
        }
        else {
            //Hide baud drop down
            ge("dataPortBaudDropdown").style.display = "none";
        }
    });
})