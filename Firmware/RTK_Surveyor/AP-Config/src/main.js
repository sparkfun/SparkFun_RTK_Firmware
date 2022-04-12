//var ws = new WebSocket("ws://192.168.1.105/ws"); //WiFi mode
var ws = new WebSocket("ws://192.168.4.1/ws"); //AP Mode

ws.onmessage = function (msg) {
    parseIncoming(msg.data);
};

function ge(e) {
    return document.getElementById(e);
}

var platformPrefix = "Surveyor";

function parseIncoming(msg) {
    //console.log("incoming message: " + msg);

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

            if (platformPrefix == "Surveyor") {
                hide("dataPortChannelDropdown");

                hide("sensorConfig"); //Hide Sensor Config section

                hide("msgUBX_ESF_MEAS"); //Hide unsupported messages
                hide("msgUBX_ESF_RAW");
                hide("msgUBX_ESF_STATUS");
                hide("msgUBX_ESF_ALG");
                hide("msgUBX_ESF_INS");
            }
            else if (platformPrefix == "Express" || platformPrefix == "Facet") {
                hide("sensorConfig"); //Hide Sensor Config section

                hide("msgUBX_ESF_MEAS"); //Hide unsupported messages
                hide("msgUBX_ESF_RAW");
                hide("msgUBX_ESF_STATUS");
                hide("msgUBX_ESF_ALG");
                hide("msgUBX_ESF_INS");
            }
            else if (platformPrefix == "Express Plus") {
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
        else if (id.includes("firmwareUploadComplete")) {
            firmwareUploadComplete();
        }
        else if (id.includes("firmwareUploadStatus")) {
            firmwareUploadStatus(val);
        }

        //Check boxes / radio buttons
        else if (val == "true") {
            try {
                ge(id).checked = true;
            } catch (error) {
                console.log("Issue with ID: " + id)
            }
        }
        else if (val == "false") {
            try {
                ge(id).checked = false;
            } catch (error) {
                console.log("Issue with ID: " + id)
            }
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

    console.log("Sending: " + settingCSV);
    ws.send(settingCSV);
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

function checkMessageValue(id) {
    checkElementValue(id, 0, 20, "Must be between 0 and 20", "collapseGNSSConfigMsg");
}

function validateFields() {
    //Collapse all sections
    ge("collapseGNSSConfig").classList.remove('show');
    ge("collapseGNSSConfigMsg").classList.remove('show');
    ge("collapseBaseConfig").classList.remove('show');
    ge("collapsePortsConfig").classList.remove('show');
    ge("collapseSensorConfig").classList.remove('show');
    ge("collapseSystemConfig").classList.remove('show');

    errorCount = 0;

    //GNSS Config
    checkElementValue("measurementRateHz", 0.1, 10, "Must be between 0 and 10Hz", "collapseGNSSConfig");
    checkConstellations();

    checkMessageValue("UBX_NMEA_DTM");
    checkMessageValue("UBX_NMEA_GBS");
    checkMessageValue("UBX_NMEA_GGA");
    checkMessageValue("UBX_NMEA_GLL");
    checkMessageValue("UBX_NMEA_GNS");

    checkMessageValue("UBX_NMEA_GRS");
    checkMessageValue("UBX_NMEA_GSA");
    checkMessageValue("UBX_NMEA_GST");
    checkMessageValue("UBX_NMEA_GSV");
    checkMessageValue("UBX_NMEA_RMC");

    checkMessageValue("UBX_NMEA_VLW");
    checkMessageValue("UBX_NMEA_VTG");
    checkMessageValue("UBX_NMEA_ZDA");

    checkMessageValue("UBX_NAV_ATT");
    checkMessageValue("UBX_NAV_CLOCK");
    checkMessageValue("UBX_NAV_DOP");
    checkMessageValue("UBX_NAV_EOE");
    checkMessageValue("UBX_NAV_GEOFENCE");

    checkMessageValue("UBX_NAV_HPPOSECEF");
    checkMessageValue("UBX_NAV_HPPOSLLH");
    checkMessageValue("UBX_NAV_ODO");
    checkMessageValue("UBX_NAV_ORB");
    checkMessageValue("UBX_NAV_POSECEF");

    checkMessageValue("UBX_NAV_POSLLH");
    checkMessageValue("UBX_NAV_PVT");
    checkMessageValue("UBX_NAV_RELPOSNED");
    checkMessageValue("UBX_NAV_SAT");
    checkMessageValue("UBX_NAV_SIG");

    checkMessageValue("UBX_NAV_STATUS");
    checkMessageValue("UBX_NAV_SVIN");
    checkMessageValue("UBX_NAV_TIMEBDS");
    checkMessageValue("UBX_NAV_TIMEGAL");
    checkMessageValue("UBX_NAV_TIMEGLO");

    checkMessageValue("UBX_NAV_TIMEGPS");
    checkMessageValue("UBX_NAV_TIMELS");
    checkMessageValue("UBX_NAV_TIMEUTC");
    checkMessageValue("UBX_NAV_VELECEF");
    checkMessageValue("UBX_NAV_VELNED");

    checkMessageValue("UBX_RXM_MEASX");
    checkMessageValue("UBX_RXM_RAWX");
    checkMessageValue("UBX_RXM_RLM");
    checkMessageValue("UBX_RXM_RTCM");
    checkMessageValue("UBX_RXM_SFRBX");

    checkMessageValue("UBX_MON_COMMS");
    checkMessageValue("UBX_MON_HW2");
    checkMessageValue("UBX_MON_HW3");
    checkMessageValue("UBX_MON_HW");
    checkMessageValue("UBX_MON_IO");

    checkMessageValue("UBX_MON_MSGPP");
    checkMessageValue("UBX_MON_RF");
    checkMessageValue("UBX_MON_RXBUF");
    checkMessageValue("UBX_MON_RXR");
    checkMessageValue("UBX_MON_TXBUF");

    checkMessageValue("UBX_TIM_TM2");
    checkMessageValue("UBX_TIM_TP");
    checkMessageValue("UBX_TIM_VRFY");

    checkMessageValue("UBX_RTCM_1005");
    checkMessageValue("UBX_RTCM_1074");
    checkMessageValue("UBX_RTCM_1077");
    checkMessageValue("UBX_RTCM_1084");
    checkMessageValue("UBX_RTCM_1087");

    checkMessageValue("UBX_RTCM_1094");
    checkMessageValue("UBX_RTCM_1097");
    checkMessageValue("UBX_RTCM_1124");
    checkMessageValue("UBX_RTCM_1127");
    checkMessageValue("UBX_RTCM_1230");

    checkMessageValue("UBX_RTCM_4072_0");
    checkMessageValue("UBX_RTCM_4072_1");

    checkMessageValue("UBX_ESF_MEAS");
    checkMessageValue("UBX_ESF_RAW");
    checkMessageValue("UBX_ESF_STATUS");
    checkMessageValue("UBX_ESF_ALG");
    checkMessageValue("UBX_ESF_INS");

    //Base Config
    checkElementValue("observationSeconds", 60, 600, "Must be between 60 to 600", "collapseBaseConfig");
    checkElementValue("observationPositionAccuracy", 1, 5.1, "Must be between 1.0 to 5.0", "collapseBaseConfig");
    checkElementValue("fixedEcefX", -5000000, 5000000, "Must be -5000000 to 5000000", "collapseBaseConfig");
    checkElementValue("fixedEcefY", -5000000, 5000000, "Must be -5000000 to 5000000", "collapseBaseConfig");
    if (ge("fixedEcefZ").value == 0.0) ge("fixedEcefZ").value = 4084500;
    checkElementValue("fixedEcefZ", 3300000, 5000000, "Must be 3300000 to 5000000", "collapseBaseConfig");
    checkElementValue("fixedLat", -180, 180, "Must be -180 to 180", "collapseBaseConfig");
    checkElementValue("fixedLong", -180, 180, "Must be -180 to 180", "collapseBaseConfig");
    checkElementValue("fixedAltitude", -11034, 8849, "Must be -11034 to 8849", "collapseBaseConfig");

    checkElementString("ntripServer_wifiSSID", 1, 30, "Must be 1 to 30 characters", "collapseBaseConfig");
    checkElementString("ntripServer_wifiPW", 0, 30, "Must be 0 to 30 characters", "collapseBaseConfig");
    checkElementString("ntripServer_CasterHost", 1, 30, "Must be 1 to 30 characters", "collapseBaseConfig");
    checkElementValue("ntripServer_CasterPort", 1, 99999, "Must be 1 to 99999", "collapseBaseConfig");
    checkElementString("ntripServer_MountPoint", 1, 30, "Must be 1 to 30 characters", "collapseBaseConfig");
    checkElementString("ntripServer_MountPointPW", 1, 30, "Must be 1 to 30 characters", "collapseBaseConfig");

    //System Config
    checkElementValue("maxLogTime_minutes", 1, 2880, "Must be 1 to 2880", "collapseSystemConfig");
    checkElementValue("maxLogLength_minutes", 1, 2880, "Must be 1 to 2880", "collapseSystemConfig");

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

function zeroElement(id) {
    ge(id).value = 0;
}

function zeroMessages() {
    zeroElement("UBX_NMEA_DTM");
    zeroElement("UBX_NMEA_GBS");
    zeroElement("UBX_NMEA_GGA");
    zeroElement("UBX_NMEA_GLL");
    zeroElement("UBX_NMEA_GNS");

    zeroElement("UBX_NMEA_GRS");
    zeroElement("UBX_NMEA_GSA");
    zeroElement("UBX_NMEA_GST");
    zeroElement("UBX_NMEA_GSV");
    zeroElement("UBX_NMEA_RMC");

    zeroElement("UBX_NMEA_VLW");
    zeroElement("UBX_NMEA_VTG");
    zeroElement("UBX_NMEA_ZDA");

    zeroElement("UBX_NAV_ATT");
    zeroElement("UBX_NAV_CLOCK");
    zeroElement("UBX_NAV_DOP");
    zeroElement("UBX_NAV_EOE");
    zeroElement("UBX_NAV_GEOFENCE");

    zeroElement("UBX_NAV_HPPOSECEF");
    zeroElement("UBX_NAV_HPPOSLLH");
    zeroElement("UBX_NAV_ODO");
    zeroElement("UBX_NAV_ORB");
    zeroElement("UBX_NAV_POSECEF");

    zeroElement("UBX_NAV_POSLLH");
    zeroElement("UBX_NAV_PVT");
    zeroElement("UBX_NAV_RELPOSNED");
    zeroElement("UBX_NAV_SAT");
    zeroElement("UBX_NAV_SIG");

    zeroElement("UBX_NAV_STATUS");
    zeroElement("UBX_NAV_SVIN");
    zeroElement("UBX_NAV_TIMEBDS");
    zeroElement("UBX_NAV_TIMEGAL");
    zeroElement("UBX_NAV_TIMEGLO");

    zeroElement("UBX_NAV_TIMEGPS");
    zeroElement("UBX_NAV_TIMELS");
    zeroElement("UBX_NAV_TIMEUTC");
    zeroElement("UBX_NAV_VELECEF");
    zeroElement("UBX_NAV_VELNED");

    zeroElement("UBX_RXM_MEASX");
    zeroElement("UBX_RXM_RAWX");
    zeroElement("UBX_RXM_RLM");
    zeroElement("UBX_RXM_RTCM");
    zeroElement("UBX_RXM_SFRBX");

    zeroElement("UBX_MON_COMMS");
    zeroElement("UBX_MON_HW2");
    zeroElement("UBX_MON_HW3");
    zeroElement("UBX_MON_HW");
    zeroElement("UBX_MON_IO");

    zeroElement("UBX_MON_MSGPP");
    zeroElement("UBX_MON_RF");
    zeroElement("UBX_MON_RXBUF");
    zeroElement("UBX_MON_RXR");
    zeroElement("UBX_MON_TXBUF");

    zeroElement("UBX_TIM_TM2");
    zeroElement("UBX_TIM_TP");
    zeroElement("UBX_TIM_VRFY");

    zeroElement("UBX_RTCM_1005");
    zeroElement("UBX_RTCM_1074");
    zeroElement("UBX_RTCM_1077");
    zeroElement("UBX_RTCM_1084");
    zeroElement("UBX_RTCM_1087");

    zeroElement("UBX_RTCM_1094");
    zeroElement("UBX_RTCM_1097");
    zeroElement("UBX_RTCM_1124");
    zeroElement("UBX_RTCM_1127");
    zeroElement("UBX_RTCM_1230");

    zeroElement("UBX_RTCM_4072_0");
    zeroElement("UBX_RTCM_4072_1");

    zeroElement("UBX_ESF_MEAS");
    zeroElement("UBX_ESF_RAW");
    zeroElement("UBX_ESF_STATUS");
    zeroElement("UBX_ESF_ALG");
    zeroElement("UBX_ESF_INS");
}
function resetToNmeaDefaults() {
    zeroMessages();
    ge("UBX_NMEA_GGA").value = 1;
    ge("UBX_NMEA_GSA").value = 1;
    ge("UBX_NMEA_GST").value = 1;
    ge("UBX_NMEA_GSV").value = 4;
    ge("UBX_NMEA_RMC").value = 1;
}
function resetToLoggingDefaults() {
    zeroMessages();
    ge("UBX_NMEA_GGA").value = 1;
    ge("UBX_NMEA_GSA").value = 1;
    ge("UBX_NMEA_GST").value = 1;
    ge("UBX_NMEA_GSV").value = 4;
    ge("UBX_NMEA_RMC").value = 1;
    ge("UBX_RXM_RAWX").value = 1;
    ge("UBX_RXM_SFRBX").value = 1;
}

function exitConfig() {
    show("exitPage");
    hide("mainPage");
    ws.send("exitAndReset,1,");
}

function firmwareUploadWait() {
    ge("firmwareUploadMsg").innerHTML = "<br>Uploading, please wait...";
}

function firmwareUploadStatus(val) {
    ge("firmwareUploadMsg").innerHTML = val;
}

function firmwareUploadComplete() {
    show("firmwareUploadComplete");
    hide("mainPage");
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
                //Disable Geodetic inputs
                ge("fixedLat").disabled = true;
                ge("fixedLong").disabled = true;
                ge("fixedAltitude").disabled = true;
            }
            else {
                //Disable ECEF inputs
                ge("fixedEcefX").disabled = true;
                ge("fixedEcefY").disabled = true;
                ge("fixedEcefZ").disabled = true;
                //Disable Geodetic inputs
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
            //Disable Geodetic inputs
            ge("fixedLat").disabled = true;
            ge("fixedLong").disabled = true;
            ge("fixedAltitude").disabled = true;
        }
        else {
            //Disable ECEF inputs
            ge("fixedEcefX").disabled = true;
            ge("fixedEcefY").disabled = true;
            ge("fixedEcefZ").disabled = true;
            //Enable Geodetic inputs
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
            //Enable Geodetic inputs
            ge("fixedLat").disabled = false;
            ge("fixedLong").disabled = false;
            ge("fixedAltitude").disabled = false;
        }
        else {
            //Enable ECEF inputs
            ge("fixedEcefX").disabled = false;
            ge("fixedEcefY").disabled = false;
            ge("fixedEcefZ").disabled = false;
            //Disable Geodetic inputs
            ge("fixedLat").disabled = true;
            ge("fixedLong").disabled = true;
            ge("fixedAltitude").disabled = true;
        }
    });

    ge("enableNtripServer").addEventListener("change", function () {
        if (ge("enableNtripServer").checked) {
            //Enable NTRIP inputs
            ge("ntripServer_wifiSSID").disabled = false;
            ge("ntripServer_wifiPW").disabled = false;
            ge("ntripServer_CasterHost").disabled = false;
            ge("ntripServer_CasterPort").disabled = false;
            ge("ntripServer_MountPoint").disabled = false;
            ge("ntripServer_MountPointPW").disabled = false;
        }
        else {
            //Disable NTRIP inputs
            ge("ntripServer_wifiSSID").disabled = true;
            ge("ntripServer_wifiPW").disabled = true;
            ge("ntripServer_CasterHost").disabled = true;
            ge("ntripServer_CasterPort").disabled = true;
            ge("ntripServer_MountPoint").disabled = true;
            ge("ntripServer_MountPointPW").disabled = true;
        }
    });

    ge("enableLogging").addEventListener("change", function () {
        if (ge("enableLogging").checked) {
            //Enable inputs
            ge("maxLogTime_minutes").disabled = false;
            ge("maxLogLength_minutes").disabled = false;
        }
        else {
            //Disable inputs
            ge("maxLogTime_minutes").disabled = true;
            ge("maxLogLength_minutes").disabled = true;
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

    ge("dataPortChannel").addEventListener("change", function () {
        if (ge("dataPortChannel").value == 0) {
            show("dataPortBaudDropdown");
        }
        else {
            hide("dataPortBaudDropdown");
        }
    });

    ge("dynamicModel").addEventListener("change", function () {
        if (ge("dynamicModel").value != 4 && ge("enableSensorFusion").checked) {
            ge("dynamicModelSensorFusionError").innerHTML = "<br>Warning: Dynamic Model not set to Automotive. Sensor Fusion is best used with the Automotive Dynamic Model.";
        }
        else {
            ge("dynamicModelSensorFusionError").innerHTML = "";
        }
    });

    ge("enableSensorFusion").addEventListener("change", function () {
        if (ge("dynamicModel").value != 4 && ge("enableSensorFusion").checked) {
            ge("dynamicModelSensorFusionError").innerHTML = "<br>Warning: Dynamic Model not set to Automotive. Sensor Fusion is best used with the Automotive Dynamic Model.";
        }
        else {
            ge("dynamicModelSensorFusionError").innerHTML = "";
        }
    });
})