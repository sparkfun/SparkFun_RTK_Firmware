var ws = new WebSocket("ws://192.168.1.105/ws"); //WiFi mode
//var ws = new WebSocket("ws://192.168.4.1/ws"); //AP Mode

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
                hide("sdMounted");
            }
            else if (val == "true") {
                show("sdMounted");
            }
        }
        else if (id == "platformPrefix") {
            platformPrefix = val;
            document.title = "RTK " + platformPrefix + " Setup";

            if (platformPrefix == "Surveyor") hide("dataPortChannelDropdown");
            if (platformPrefix == "Express Plus") {
                ge("muxChannel2").innerHTML = "Wheel/Dir Encoder";

                hide("baseConfig"); //Hide Base Config section

                hide("msgUBX_NAV_SVIN"); //Hide unsupported messages
                hide("msgUBX_RTCM_1005");
                hide("msgUBX_RTCM_1074");
                hide("msgUBX_RTCM_1077");
                hide("msgUBX_RTCM_1084");
                hide("msgUBX_RTCM_1087");

                hide("msgUBX_RTCM_1094");
                hide("msgUBX_RTCM_1097");
                hide("msgUBX_RTCM_1124");
                hide("msgUBX_RTCM_1127");
                hide("msgUBX_RTCM_1230");

                hide("msgUBX_RTCM_4072_0");
                hide("msgUBX_RTCM_4072_1");
            }
        }
        else if (id.includes("sdFreeSpace")
            || id.includes("sdUsedSpace")
            || id.includes("rtkFirmwareVersion")
            || id.includes("zedFirmwareVersion")
        ) {
            ge(id).innerHTML = val;
        }
        else if (id.includes("firmwareFileName")) {
            show("firmwareAvailable"); //Turn on firmware area

            ge(id).innerHTML = val;
            if (id.includes("0")) show("firmwareFile0");
            if (id.includes("1")) show("firmwareFile1");
            if (id.includes("2")) show("firmwareFile2");
            if (id.includes("3")) show("firmwareFile3");
            if (id.includes("4")) show("firmwareFile4");
            if (id.includes("5")) show("firmwareFile5");
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
            try {
                ge(id).value = val;
            } catch (error) {
                console.log("Issue with ID: " + id)
            }
        }
    }
    //console.log("Settings loaded");

    //Force element updates
    ge("measurementRateHz").dispatchEvent(new CustomEvent('change'));
    ge("baseTypeSurveyIn").dispatchEvent(new CustomEvent('change'));
    ge("baseTypeFixed").dispatchEvent(new CustomEvent('change'));
    ge("fixedBaseCoordinateTypeECEF").dispatchEvent(new CustomEvent('change'));
    ge("fixedBaseCoordinateTypeGeo").dispatchEvent(new CustomEvent('change'));
    ge("enableLogging").dispatchEvent(new CustomEvent('change'));
    ge("enableNtripServer").dispatchEvent(new CustomEvent('change'));
    ge("dataPortChannel").dispatchEvent(new CustomEvent('change'));
}

function hide(id) {
    ge(id).style.display = "none";
}

function show(id) {
    ge(id).style.display = "block";
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
    ge(id + 'Error').innerHTML = '<br>Error: ' + errorText;
}

function clearError(id) {
    ge(id + 'Error').innerHTML = '';
}

function showSuccess(id, msg) {
    ge(id + 'Success').innerHTML = '<br>Success: ' + msg;
}

function clearSuccess(id) {
    ge(id + 'Success').innerHTML = '';
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
    checkConstellations();

    checkElementValue("UBX_NMEA_DTM", 0, 20, "Must be between 0 and 20", "collapseGNSSConfigMsg");
    checkElementValue("UBX_NMEA_GBS", 0, 20, "Must be between 0 and 20", "collapseGNSSConfigMsg");
    checkElementValue("UBX_NMEA_GGA", 0, 20, "Must be between 0 and 20", "collapseGNSSConfigMsg");
    checkElementValue("UBX_NMEA_GLL", 0, 20, "Must be between 0 and 20", "collapseGNSSConfigMsg");
    checkElementValue("UBX_NMEA_GNS", 0, 20, "Must be between 0 and 20", "collapseGNSSConfigMsg");

    checkElementValue("UBX_NMEA_GRS", 0, 20, "Must be between 0 and 20", "collapseGNSSConfigMsg");
    checkElementValue("UBX_NMEA_GSA", 0, 20, "Must be between 0 and 20", "collapseGNSSConfigMsg");
    checkElementValue("UBX_NMEA_GST", 0, 20, "Must be between 0 and 20", "collapseGNSSConfigMsg");
    checkElementValue("UBX_NMEA_GSV", 0, 20, "Must be between 0 and 20", "collapseGNSSConfigMsg");
    checkElementValue("UBX_NMEA_RMC", 0, 20, "Must be between 0 and 20", "collapseGNSSConfigMsg");

    checkElementValue("UBX_NMEA_VLW", 0, 20, "Must be between 0 and 20", "collapseGNSSConfigMsg");
    checkElementValue("UBX_NMEA_VTG", 0, 20, "Must be between 0 and 20", "collapseGNSSConfigMsg");
    checkElementValue("UBX_NMEA_ZDA", 0, 20, "Must be between 0 and 20", "collapseGNSSConfigMsg");

    checkElementValue("UBX_NAV_ATT", 0, 20, "Must be between 0 and 20", "collapseGNSSConfigMsg");
    checkElementValue("UBX_NAV_CLOCK", 0, 20, "Must be between 0 and 20", "collapseGNSSConfigMsg");
    checkElementValue("UBX_NAV_DOP", 0, 20, "Must be between 0 and 20", "collapseGNSSConfigMsg");
    checkElementValue("UBX_NAV_EOE", 0, 20, "Must be between 0 and 20", "collapseGNSSConfigMsg");
    checkElementValue("UBX_NAV_GEOFENCE", 0, 20, "Must be between 0 and 20", "collapseGNSSConfigMsg");

    checkElementValue("UBX_NAV_HPPOSECEF", 0, 20, "Must be between 0 and 20", "collapseGNSSConfigMsg");
    checkElementValue("UBX_NAV_HPPOSLLH", 0, 20, "Must be between 0 and 20", "collapseGNSSConfigMsg");
    checkElementValue("UBX_NAV_ODO", 0, 20, "Must be between 0 and 20", "collapseGNSSConfigMsg");
    checkElementValue("UBX_NAV_ORB", 0, 20, "Must be between 0 and 20", "collapseGNSSConfigMsg");
    checkElementValue("UBX_NAV_POSECEF", 0, 20, "Must be between 0 and 20", "collapseGNSSConfigMsg");

    checkElementValue("UBX_NAV_POSLLH", 0, 20, "Must be between 0 and 20", "collapseGNSSConfigMsg");
    checkElementValue("UBX_NAV_PVT", 0, 20, "Must be between 0 and 20", "collapseGNSSConfigMsg");
    checkElementValue("UBX_NAV_RELPOSNED", 0, 20, "Must be between 0 and 20", "collapseGNSSConfigMsg");
    checkElementValue("UBX_NAV_SAT", 0, 20, "Must be between 0 and 20", "collapseGNSSConfigMsg");
    checkElementValue("UBX_NAV_SIG", 0, 20, "Must be between 0 and 20", "collapseGNSSConfigMsg");

    checkElementValue("UBX_NAV_STATUS", 0, 20, "Must be between 0 and 20", "collapseGNSSConfigMsg");
    checkElementValue("UBX_NAV_SVIN", 0, 20, "Must be between 0 and 20", "collapseGNSSConfigMsg");
    checkElementValue("UBX_NAV_TIMEBDS", 0, 20, "Must be between 0 and 20", "collapseGNSSConfigMsg");
    checkElementValue("UBX_NAV_TIMEGAL", 0, 20, "Must be between 0 and 20", "collapseGNSSConfigMsg");
    checkElementValue("UBX_NAV_TIMEGLO", 0, 20, "Must be between 0 and 20", "collapseGNSSConfigMsg");

    checkElementValue("UBX_NAV_TIMEGPS", 0, 20, "Must be between 0 and 20", "collapseGNSSConfigMsg");
    checkElementValue("UBX_NAV_TIMELS", 0, 20, "Must be between 0 and 20", "collapseGNSSConfigMsg");
    checkElementValue("UBX_NAV_TIMEUTC", 0, 20, "Must be between 0 and 20", "collapseGNSSConfigMsg");
    checkElementValue("UBX_NAV_VELECEF", 0, 20, "Must be between 0 and 20", "collapseGNSSConfigMsg");
    checkElementValue("UBX_NAV_VELNED", 0, 20, "Must be between 0 and 20", "collapseGNSSConfigMsg");

    checkElementValue("UBX_RXM_MEASX", 0, 20, "Must be between 0 and 20", "collapseGNSSConfigMsg");
    checkElementValue("UBX_RXM_RAWX", 0, 20, "Must be between 0 and 20", "collapseGNSSConfigMsg");
    checkElementValue("UBX_RXM_RLM", 0, 20, "Must be between 0 and 20", "collapseGNSSConfigMsg");
    checkElementValue("UBX_RXM_RTCM", 0, 20, "Must be between 0 and 20", "collapseGNSSConfigMsg");
    checkElementValue("UBX_RXM_SFRBX", 0, 20, "Must be between 0 and 20", "collapseGNSSConfigMsg");

    checkElementValue("UBX_MON_COMMS", 0, 20, "Must be between 0 and 20", "collapseGNSSConfigMsg");
    checkElementValue("UBX_MON_HW2", 0, 20, "Must be between 0 and 20", "collapseGNSSConfigMsg");
    checkElementValue("UBX_MON_HW3", 0, 20, "Must be between 0 and 20", "collapseGNSSConfigMsg");
    checkElementValue("UBX_MON_HW", 0, 20, "Must be between 0 and 20", "collapseGNSSConfigMsg");
    checkElementValue("UBX_MON_IO", 0, 20, "Must be between 0 and 20", "collapseGNSSConfigMsg");

    checkElementValue("UBX_MON_MSGPP", 0, 20, "Must be between 0 and 20", "collapseGNSSConfigMsg");
    checkElementValue("UBX_MON_RF", 0, 20, "Must be between 0 and 20", "collapseGNSSConfigMsg");
    checkElementValue("UBX_MON_RXBUF", 0, 20, "Must be between 0 and 20", "collapseGNSSConfigMsg");
    checkElementValue("UBX_MON_RXR", 0, 20, "Must be between 0 and 20", "collapseGNSSConfigMsg");
    checkElementValue("UBX_MON_TXBUF", 0, 20, "Must be between 0 and 20", "collapseGNSSConfigMsg");

    checkElementValue("UBX_TIM_TM2", 0, 20, "Must be between 0 and 20", "collapseGNSSConfigMsg");
    checkElementValue("UBX_TIM_TP", 0, 20, "Must be between 0 and 20", "collapseGNSSConfigMsg");
    checkElementValue("UBX_TIM_VRFY", 0, 20, "Must be between 0 and 20", "collapseGNSSConfigMsg");

    checkElementValue("UBX_RTCM_1005", 0, 20, "Must be between 0 and 20", "collapseGNSSConfigMsg");
    checkElementValue("UBX_RTCM_1074", 0, 20, "Must be between 0 and 20", "collapseGNSSConfigMsg");
    checkElementValue("UBX_RTCM_1077", 0, 20, "Must be between 0 and 20", "collapseGNSSConfigMsg");
    checkElementValue("UBX_RTCM_1084", 0, 20, "Must be between 0 and 20", "collapseGNSSConfigMsg");
    checkElementValue("UBX_RTCM_1087", 0, 20, "Must be between 0 and 20", "collapseGNSSConfigMsg");

    checkElementValue("UBX_RTCM_1094", 0, 20, "Must be between 0 and 20", "collapseGNSSConfigMsg");
    checkElementValue("UBX_RTCM_1097", 0, 20, "Must be between 0 and 20", "collapseGNSSConfigMsg");
    checkElementValue("UBX_RTCM_1124", 0, 20, "Must be between 0 and 20", "collapseGNSSConfigMsg");
    checkElementValue("UBX_RTCM_1127", 0, 20, "Must be between 0 and 20", "collapseGNSSConfigMsg");
    checkElementValue("UBX_RTCM_1230", 0, 20, "Must be between 0 and 20", "collapseGNSSConfigMsg");

    checkElementValue("UBX_RTCM_4072_0", 0, 20, "Must be between 0 and 20", "collapseGNSSConfigMsg");
    checkElementValue("UBX_RTCM_4072_1", 0, 20, "Must be between 0 and 20", "collapseGNSSConfigMsg");

    checkElementValue("UBX_ESF_MEAS", 0, 20, "Must be between 0 and 20", "collapseGNSSConfigMsg");
    checkElementValue("UBX_ESF_RAW", 0, 20, "Must be between 0 and 20", "collapseGNSSConfigMsg");
    checkElementValue("UBX_ESF_STATUS", 0, 20, "Must be between 0 and 20", "collapseGNSSConfigMsg");
    checkElementValue("UBX_ESF_ALG", 0, 20, "Must be between 0 and 20", "collapseGNSSConfigMsg");
    checkElementValue("UBX_ESF_INS", 0, 20, "Must be between 0 and 20", "collapseGNSSConfigMsg");

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
        clearSuccess('saveBtn');
    }
    else if (errorCount > 1) {
        showError('saveBtn', "Please clear " + errorCount + " errors");
        clearSuccess('saveBtn');
    }
    else {
        //Tell Arduino we're ready to save
        sendData();
        clearError('saveBtn');
        showSuccess('saveBtn', "All saved!");
    }
}

function checkConstellations() {
    if (ge("ubxConstellationsGPS").checked == false
        && ge("ubxConstellationsGalileo").checked == false
        && ge("ubxConstellationsBeiDou").checked == false
        && ge("ubxConstellationsGLONASS").checked == false
    ) {
        ge("collapseGNSSConfig").classList.add('show');
        showError('ubxConstellations', "Please choose one constellation");
        errorCount++;
    }
    else
        clearError("ubxConstellations");
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
        clearError(id);
}
function checkElementString(id, min, max, errorText, collapseID) {
    value = ge(id).value;
    if (value.length < min || value.length > max) {
        ge(id + 'Error').innerHTML = 'Error: ' + errorText;
        ge(collapseID).classList.add('show');
        errorCount++;
    }
    else
        clearError(id);
}

function resetToFactoryDefaults() {
    ge("factoryDefaultsMsg").innerHTML = "Defaults Applied. Please wait for device reset..."
    ws.send("factoryDefaultReset,1,");
}

function exitConfig() {
    show("exitPage");
    hide("mainPage");
    ws.send("exitToRoverMode,1,");
}

function firmwareUploadWait() {
    ge("firmwareUploadMsg").innerHTML = "<br>Uploading, please wait....";
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
            show("dataPortBaudDropdown");
        }
        else {
            hide("dataPortBaudDropdown");
        }
    });
})