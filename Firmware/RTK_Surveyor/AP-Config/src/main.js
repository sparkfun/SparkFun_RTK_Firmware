var gateway = `ws://${window.location.hostname}/ws`;
var websocket;

window.addEventListener('load', onLoad);

function onLoad(event) {
    initWebSocket();
}

function initWebSocket() {
    websocket = new WebSocket(gateway);
    websocket.onmessage = function (msg) {
        parseIncoming(msg.data);
    };
}

function ge(e) {
    return document.getElementById(e);
}

var fixedLat = 0;
var fixedLong = 0;
var platformPrefix = "Surveyor";
var geodeticLat = 40.01;
var geodeticLon = -105.19;
var geodeticAlt = 1500.1;
var ecefX = -1280206.568;
var ecefY = -4716804.403;
var ecefZ = 4086665.484;
var lastFileName = "";
var fileNumber = 0;
var numberOfFilesSelected = 0;
var selectedFiles = "";
var showingFileList = false;
var obtainedMessageList = false;
var obtainedMessageListBase = false;
var showingMessageRTCMList = false;
var fileTableText = "";
var messageText = "";
var lastMessageType = "";
var lastMessageTypeBase = "";

var recordsECEF = [];
var recordsGeodetic = [];
var fullPageUpdate = false;

var resetTimeout;
var sendDataTimeout;
var checkNewFirmwareTimeout;
var getNewFirmwareTimeout;

const CoordinateTypes = {
    COORDINATE_INPUT_TYPE_DD: 0, //Default DD.ddddddddd
    COORDINATE_INPUT_TYPE_DDMM: 1, //DDMM.mmmmm
    COORDINATE_INPUT_TYPE_DD_MM: 2, //DD MM.mmmmm
    COORDINATE_INPUT_TYPE_DD_MM_DASH: 3, //DD-MM.mmmmm
    COORDINATE_INPUT_TYPE_DD_MM_SYMBOL: 4, //DD°MM.mmmmmmm'
    COORDINATE_INPUT_TYPE_DDMMSS: 5, //DD MM SS.ssssss
    COORDINATE_INPUT_TYPE_DD_MM_SS: 6, //DD MM SS.ssssss
    COORDINATE_INPUT_TYPE_DD_MM_SS_DASH: 7, //DD-MM-SS.ssssss
    COORDINATE_INPUT_TYPE_DD_MM_SS_SYMBOL: 8, //DD°MM'SS.ssssss"
    COORDINATE_INPUT_TYPE_DDMMSS_NO_DECIMAL: 9, //DDMMSS - No decimal
    COORDINATE_INPUT_TYPE_DD_MM_SS_NO_DECIMAL: 10, //DD MM SS - No decimal
    COORDINATE_INPUT_TYPE_DD_MM_SS_DASH_NO_DECIMAL: 11, //DD-MM-SS - No decimal
    COORDINATE_INPUT_TYPE_INVALID_UNKNOWN: 12,
}

var convertedCoordinate = 0.0;
var coordinateInputType = CoordinateTypes.COORDINATE_INPUT_TYPE_DD;

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
                hide("fileManager");
            }
            else if (val == "true") {
                show("fileManager");
            }
        }
        else if (id == "platformPrefix") {
            platformPrefix = val;
            document.title = "RTK " + platformPrefix + " Setup";
            fullPageUpdate = true;

            if (platformPrefix == "Surveyor") {
                show("baseConfig");
                hide("sensorConfig");
                hide("ppConfig");
                hide("ethernetConfig");
                hide("ntpConfig");
                //hide("allowWiFiOverEthernetClient"); //For future expansion
                //hide("allowWiFiOverEthernetServer"); //For future expansion

                hide("dataPortChannelDropdown");
            }
            else if (platformPrefix == "Express" || platformPrefix == "Facet") {
                show("baseConfig");
                hide("sensorConfig");
                hide("ppConfig");
                hide("ethernetConfig");
                hide("ntpConfig");
                //hide("allowWiFiOverEthernetClient"); //For future expansion
                //hide("allowWiFiOverEthernetServer"); //For future expansion
            }
            else if (platformPrefix == "Express Plus") {
                hide("baseConfig");
                show("sensorConfig");
                hide("ppConfig");
                hide("ethernetConfig");
                hide("ntpConfig");
                //hide("allowWiFiOverEthernetClient"); //For future expansion
                //hide("allowWiFiOverEthernetServer"); //For future expansion

                ge("muxChannel2").innerHTML = "Wheel/Dir Encoder";
            }
            else if (platformPrefix == "Facet L-Band" || platformPrefix == "Facet L-Band Direct") {
                show("baseConfig");
                hide("sensorConfig");
                show("ppConfig");
                hide("ethernetConfig");
                hide("ntpConfig");
                //hide("allowWiFiOverEthernetClient"); //For future expansion
                //hide("allowWiFiOverEthernetServer"); //For future expansion
            }
            else if (platformPrefix == "Reference Station") {
                show("baseConfig");
                hide("sensorConfig");
                hide("ppConfig");
                show("ethernetConfig");
                show("ntpConfig");
                //hide("allowWiFiOverEthernetClient"); //For future expansion
                //hide("allowWiFiOverEthernetServer"); //For future expansion
            }
        }
        else if (id.includes("zedFirmwareVersionInt")) {
            //Must come above zedFirmwareVersion test
            //Modify settings due to firmware limitations
            if (val >= 121) {
                select = ge("dynamicModel");
                let newOption = new Option('Mower', '11');
                select.add(newOption, undefined);
                newOption = new Option('E-Scooter', '12');
                select.add(newOption, undefined);
            }
        }
        //Strings generated by RTK unit
        else if (id.includes("sdFreeSpace")
            || id.includes("sdSize")
            || id.includes("hardwareID")
            || id.includes("zedFirmwareVersion")
            || id.includes("daysRemaining")
            || id.includes("profile0Name")
            || id.includes("profile1Name")
            || id.includes("profile2Name")
            || id.includes("profile3Name")
            || id.includes("profile4Name")
            || id.includes("profile5Name")
            || id.includes("profile6Name")
            || id.includes("profile7Name")
            || id.includes("radioMAC")
            || id.includes("deviceBTID")
            || id.includes("logFileName")
            || id.includes("batteryPercent")
        ) {
            ge(id).innerHTML = val;
        }
        else if (id.includes("rtkFirmwareVersion")) {
            ge("rtkFirmwareVersion").innerHTML = val;
            ge("rtkFirmwareVersionUpgrade").innerHTML = val;
        }
        else if (id.includes("confirmReset")) {
            resetComplete();
        }
        else if (id.includes("confirmDataReceipt")) {
            confirmDataReceipt();
        }

        else if (id.includes("profileNumber")) {
            currentProfileNumber = val;
            $("input[name=profileRadio][value=" + currentProfileNumber + "]").prop('checked', true);
        }
        else if (id.includes("firmwareUploadComplete")) {
            firmwareUploadComplete();
        }
        else if (id.includes("firmwareUploadStatus")) {
            firmwareUploadStatus(val);
        }
        else if (id.includes("geodeticLat")) {
            geodeticLat = val;
            ge(id).innerHTML = val;
        }
        else if (id.includes("geodeticLon")) {
            geodeticLon = val;
            ge(id).innerHTML = val;
        }
        else if (id.includes("geodeticAlt")) {
            geodeticAlt = val;
            ge(id).innerHTML = val;
        }
        else if (id.includes("ecefX")) {
            ecefX = val;
            ge(id).innerHTML = val;
        }
        else if (id.includes("ecefY")) {
            ecefY = val;
            ge(id).innerHTML = val;
        }
        else if (id.includes("ecefZ")) {
            ecefZ = val;
            ge(id).innerHTML = val;
        }
        else if (id.includes("espnowPeerCount")) {
            if (val > 0)
                ge("peerMACs").innerHTML = "";
        }
        else if (id.includes("peerMAC")) {
            ge("peerMACs").innerHTML += val + "<br>";
        }
        else if (id.includes("stationECEF")) {
            recordsECEF.push(val);
        }
        else if (id.includes("stationGeodetic")) {
            recordsGeodetic.push(val);
        }
        else if (id.includes("fmName")) {
            lastFileName = val;
        }
        else if (id.includes("fmSize")) {
            fileTableText += "<tr align='left'>";
            fileTableText += "<td>" + lastFileName + "</td>";
            fileTableText += "<td>" + val + "</td>";
            fileTableText += "<td><input type='checkbox' id='" + lastFileName + "' name='fileID' class='form-check-input fileManagerCheck'></td>";
            fileTableText += "</tr>";
        }
        else if (id.includes("fmNext")) {
            sendFile();
        }
        else if (id.includes("UBX_")) {
            var messageName = id;
            var messageRate = val;
            var messageNameLabel = "";

            var messageData = messageName.split('_');
            if (messageData.length >= 3) {
                var messageType = messageData[1]; //UBX_RTCM_1074 = RTCM
                if (lastMessageType != messageType) {
                    lastMessageType = messageType;
                    messageText += "<hr>";
                }

                messageNameLabel = messageData[1] + "_" + messageData[2]; //RTCM_1074
                if (messageData.length == 4) {
                    messageNameLabel = messageData[1] + "_" + messageData[2] + "_" + messageData[3]; //RTCM_4072_1
                }

                //Remove Base if seen
                messageNameLabel = messageNameLabel.split('Base').join(''); //UBX_RTCM_1074Base
            }

            messageText += "<div class='form-group row' id='msg" + messageName + "'>";
            messageText += "<label for='" + messageName + "' class='col-sm-4 col-6 col-form-label'>" + messageNameLabel + ":</label>";
            messageText += "<div class='col-sm-4 col-4'><input type='number' class='form-control'";
            messageText += "id='" + messageName + "' value='" + messageRate + "'>";
            messageText += "<p id='" + messageName + "Error' class='inlineError'></p>";
            messageText += "</div></div>";
        }
        else if (id.includes("checkingNewFirmware")) {
            checkingNewFirmware();
        }
        else if (id.includes("newFirmwareVersion")) {
            newFirmwareVersion(val);
        }
        else if (id.includes("gettingNewFirmware")) {
            gettingNewFirmware();
        }
        else if (id.includes("otaFirmwareStatus")) {
            otaFirmwareStatus(val);
        }
        else if (id.includes("batteryIconFileName")) {
            ge("batteryIconFileName").src = val;
        }
        else if (id.includes("coordinateInputType")) {
            coordinateInputType = val;
        }
        else if (id.includes("fixedLat")) {
            fixedLat = val;
        }
        else if (id.includes("fixedLong")) {
            fixedLong = val;
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

    ge("profileChangeMessage").innerHTML = '';
    ge("resetProfileMsg").innerHTML = '';

    //Don't update if all we received was coordinate info
    if (fullPageUpdate) {
        fullPageUpdate = false;

        //Force element updates
        ge("measurementRateHz").dispatchEvent(new CustomEvent('change'));
        ge("baseTypeSurveyIn").dispatchEvent(new CustomEvent('change'));
        ge("baseTypeFixed").dispatchEvent(new CustomEvent('change'));
        ge("fixedBaseCoordinateTypeECEF").dispatchEvent(new CustomEvent('change'));
        ge("fixedBaseCoordinateTypeGeo").dispatchEvent(new CustomEvent('change'));
        ge("enableLogging").dispatchEvent(new CustomEvent('change'));
        ge("enableNtripClient").dispatchEvent(new CustomEvent('change'));
        ge("enableNtripServer").dispatchEvent(new CustomEvent('change'));
        ge("dataPortChannel").dispatchEvent(new CustomEvent('change'));
        ge("enableExternalPulse").dispatchEvent(new CustomEvent('change'));
        ge("enablePointPerfectCorrections").dispatchEvent(new CustomEvent('change'));
        ge("radioType").dispatchEvent(new CustomEvent('change'));
        ge("antennaReferencePoint").dispatchEvent(new CustomEvent('change'));
        ge("autoIMUmountAlignment").dispatchEvent(new CustomEvent('change'));
        ge("enableARPLogging").dispatchEvent(new CustomEvent('change'));

        updateECEFList();
        updateGeodeticList();
        tcpClientBoxes();
        tcpServerBoxes();
        udpBoxes();
        dhcpEthernet();
        updateLatLong();
    }

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
    //Remove file manager files
    clsElements = document.querySelectorAll(".form-check-input:not(.fileManagerCheck), .form-radio");

    for (let x = 0; x < clsElements.length; x++) {
        settingCSV += clsElements[x].id + "," + clsElements[x].checked + ",";
    }

    for (let x = 0; x < recordsECEF.length; x++) {
        settingCSV += "stationECEF" + x + ',' + recordsECEF[x] + ",";
    }

    for (let x = 0; x < recordsGeodetic.length; x++) {
        settingCSV += "stationGeodetic" + x + ',' + recordsGeodetic[x] + ",";
    }

    console.log("Sending: " + settingCSV);
    websocket.send(settingCSV);

    sendDataTimeout = setTimeout(sendData, 2000);
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

function showMsg(id, msg, error = false) {
    if (error == true) {
        try {
            ge(id).classList.remove('inlineSuccess');
            ge(id).classList.add('inlineError');
        }
        catch { }
    }
    else {
        try {
            ge(id).classList.remove('inlineError');
            ge(id).classList.add('inlineSuccess');
        }
        catch { }
    }
    ge(id).innerHTML = '<br>' + msg;
}
function showMsgError(id, msg) {
    showMsg(id, "Error: " + msg, true);
}
function clearMsg(id, msg) {
    ge(id).innerHTML = '';
}

var errorCount = 0;

function checkMessageValue(id) {
    checkElementValue(id, 0, 255, "Must be between 0 and 255", "collapseGNSSConfigMsg");
}

function collapseSection(section, caret) {
    ge(section).classList.remove('show');
    ge(caret).classList.remove('icon-caret-down');
    ge(caret).classList.remove('icon-caret-up');
    ge(caret).classList.add('icon-caret-down');
}

function validateFields() {
    //Collapse all sections
    collapseSection("collapseProfileConfig", "profileCaret");
    collapseSection("collapseGNSSConfig", "gnssCaret");
    collapseSection("collapseGNSSConfigMsg", "gnssMsgCaret");
    collapseSection("collapseBaseConfig", "baseCaret");
    collapseSection("collapseSensorConfig", "sensorCaret");
    collapseSection("collapsePPConfig", "pointPerfectCaret");
    collapseSection("collapsePortsConfig", "portsCaret");
    collapseSection("collapseWiFiConfig", "wifiCaret");
    collapseSection("collapseTCPUDPConfig", "tcpUdpCaret");
    collapseSection("collapseRadioConfig", "radioCaret");
    collapseSection("collapseSystemConfig", "systemCaret");
    collapseSection("collapseEthernetConfig", "ethernetCaret");
    collapseSection("collapseNTPConfig", "ntpCaret");

    errorCount = 0;

    //Profile Config
    checkElementString("profileName", 1, 49, "Must be 1 to 49 characters", "collapseProfileConfig");

    //GNSS Config
    checkElementValue("measurementRateHz", 0.00012, 10, "Must be between 0.00012 and 10Hz", "collapseGNSSConfig");
    checkConstellations();

    checkElementValue("minElev", 0, 90, "Must be between 0 and 90", "collapseGNSSConfig");
    checkElementValue("minCNO", 0, 90, "Must be between 0 and 90", "collapseGNSSConfig");

    if (ge("enableNtripClient").checked) {
        checkElementString("ntripClient_CasterHost", 1, 45, "Must be 1 to 45 characters", "collapseGNSSConfig");
        checkElementValue("ntripClient_CasterPort", 1, 99999, "Must be 1 to 99999", "collapseGNSSConfig");
        checkElementString("ntripClient_MountPoint", 1, 30, "Must be 1 to 30 characters", "collapseGNSSConfig");
        checkElementCasterUser("ntripClient_CasterHost", "ntripClient_CasterUser", "rtk2go.com", "@", "Must be an email address", "collapseGNSSConfig");
    }
    // Don't overwrite with the defaults here. User may want to disable NTRIP but not lose the existing settings.
    // else {
    //     clearElement("ntripClient_CasterHost", "rtk2go.com");
    //     clearElement("ntripClient_CasterPort", 2101);
    //     clearElement("ntripClient_MountPoint", "bldr_SparkFun1");
    //     clearElement("ntripClient_MountPointPW");
    //     clearElement("ntripClient_CasterUser", "test@test.com");
    //     clearElement("ntripClient_CasterUserPW", "");
    //     ge("ntripClient_TransmitGGA").checked = true;
    // }

    //Check all UBX message boxes
    var ubxMessages = document.querySelectorAll('input[id^=UBX_]'); //match all ids starting with UBX_
    for (let x = 0; x < ubxMessages.length; x++) {
        var messageName = ubxMessages[x].id;
        checkMessageValue(messageName);
    }

    //Base Config
    if (platformPrefix != "Express Plus") {
        if (ge("baseTypeSurveyIn").checked) {
            checkElementValue("observationSeconds", 60, 600, "Must be between 60 to 600", "collapseBaseConfig");
            checkElementValue("observationPositionAccuracy", 1, 5.1, "Must be between 1.0 to 5.0", "collapseBaseConfig");

            clearElement("fixedEcefX", -1280206.568);
            clearElement("fixedEcefY", -4716804.403);
            clearElement("fixedEcefZ", 4086665.484);
            clearElement("fixedLatText", 40.09029479);
            clearElement("fixedLongText", -105.18505761);
            clearElement("fixedAltitude", 1560.089);
            clearElement("antennaHeight", 0);
            clearElement("antennaReferencePoint", 0);
        }
        else {
            clearElement("observationSeconds", 60);
            clearElement("observationPositionAccuracy", 5.0);

            if (ge("fixedBaseCoordinateTypeECEF").checked) {
                clearElement("fixedLatText", 40.09029479);
                clearElement("fixedLongText", -105.18505761);
                clearElement("fixedAltitude", 1560.089);
                clearElement("antennaHeight", 0);
                clearElement("antennaReferencePoint", 0);

                checkElementValue("fixedEcefX", -7000000, 7000000, "Must be -7000000 to 7000000", "collapseBaseConfig");
                checkElementValue("fixedEcefY", -7000000, 7000000, "Must be -7000000 to 7000000", "collapseBaseConfig");
                checkElementValue("fixedEcefZ", -7000000, 7000000, "Must be -7000000 to 7000000", "collapseBaseConfig");
            }
            else {
                clearElement("fixedEcefX", -1280206.568);
                clearElement("fixedEcefY", -4716804.403);
                clearElement("fixedEcefZ", 4086665.484);

                checkLatLong(); //Verify Lat/Long input type
                checkElementValue("fixedAltitude", -11034, 8849, "Must be -11034 to 8849", "collapseBaseConfig");

                checkElementValue("antennaHeight", -15000, 15000, "Must be -15000 to 15000", "collapseBaseConfig");
                checkElementValue("antennaReferencePoint", -200.0, 200.0, "Must be -200.0 to 200.0", "collapseBaseConfig");
            }
        }

        if (ge("enableNtripServer").checked == true) {
            checkElementString("ntripServer_CasterHost_0", 1, 49, "Must be 1 to 49 characters", "collapseBaseConfig");
            checkElementValue("ntripServer_CasterPort_0", 1, 99999, "Must be 1 to 99999", "collapseBaseConfig");
            checkElementString("ntripServer_CasterUser_0", 0, 49, "Must be 0 to 49 characters", "collapseBaseConfig");
            checkElementString("ntripServer_CasterUserPW_0", 0, 49, "Must be 0 to 49 characters", "collapseBaseConfig");
            checkElementString("ntripServer_MountPoint_0", 1, 49, "Must be 1 to 49 characters", "collapseBaseConfig");
            checkElementString("ntripServer_MountPointPW_0", 0, 49, "Must be 0 to 49 characters", "collapseBaseConfig");
            checkElementString("ntripServer_CasterHost_1", 0, 49, "Must be 0 to 49 characters", "collapseBaseConfig");
            checkElementValue("ntripServer_CasterPort_1", 0, 99999, "Must be 0 to 99999", "collapseBaseConfig");
            checkElementString("ntripServer_CasterUser_1", 0, 49, "Must be 0 to 49 characters", "collapseBaseConfig");
            checkElementString("ntripServer_CasterUserPW_1", 0, 49, "Must be 0 to 49 characters", "collapseBaseConfig");
            checkElementString("ntripServer_MountPoint_1", 0, 49, "Must be 0 to 49 characters", "collapseBaseConfig");
            checkElementString("ntripServer_MountPointPW_1", 0, 49, "Must be 0 to 49 characters", "collapseBaseConfig");
        }
        // Don't overwrite with the defaults here. User may want to disable NTRIP but not lose the existing settings.
        // else {
        //     clearElement("ntripServer_CasterHost_0", "rtk2go.com");
        //     clearElement("ntripServer_CasterPort_0", 2101);
        //     clearElement("ntripServer_CasterUser_0", "test@test.com");
        //     clearElement("ntripServer_CasterUserPW_0", "");
        //     clearElement("ntripServer_MountPoint_0", "bldr_dwntwn2");
        //     clearElement("ntripServer_MountPointPW_0", "WR5wRo4H");
        //     clearElement("ntripServer_CasterHost_1", "");
        //     clearElement("ntripServer_CasterPort_1", 0);
        //     clearElement("ntripServer_CasterUser_1", "");
        //     clearElement("ntripServer_CasterUserPW_1", "");
        //     clearElement("ntripServer_MountPoint_1", "");
        //     clearElement("ntripServer_MountPointPW_1", "");
        // }
    }

    //PointPerfect Config
    if (platformPrefix == "Facet L-Band" || platformPrefix == "Facet L-Band Direct") {
        if (ge("enablePointPerfectCorrections").checked == true) {
            value = ge("pointPerfectDeviceProfileToken").value;
            if (value.length > 0)
                checkElementString("pointPerfectDeviceProfileToken", 36, 36, "Must be 36 characters", "collapsePPConfig");
        }
        else {
            clearElement("pointPerfectDeviceProfileToken", "");
            ge("autoKeyRenewal").checked = true;
        }
    }

    //Sensor Config
    if (platformPrefix == "Express Plus") {
        if (ge("autoIMUmountAlignment").checked == false) {
            checkElementValue("imuYaw", 0, 360, "Must be between 0.0 to 360.0", "collapseSensorConfig");
            checkElementValue("imuPitch", -90, 90, "Must be between -90.0 to 90.0", "collapseSensorConfig");
            checkElementValue("imuRoll", -180, 180, "Must be between -180.0 to 180.0", "collapseSensorConfig");
        }
    }

    //WiFi Config
    checkElementString("wifiNetwork0SSID", 0, 50, "Must be 0 to 50 characters", "collapseWiFiConfig");
    checkElementString("wifiNetwork0Password", 0, 50, "Must be 0 to 50 characters", "collapseWiFiConfig");
    checkElementString("wifiNetwork1SSID", 0, 50, "Must be 0 to 50 characters", "collapseWiFiConfig");
    checkElementString("wifiNetwork1Password", 0, 50, "Must be 0 to 50 characters", "collapseWiFiConfig");
    checkElementString("wifiNetwork2SSID", 0, 50, "Must be 0 to 50 characters", "collapseWiFiConfig");
    checkElementString("wifiNetwork2Password", 0, 50, "Must be 0 to 50 characters", "collapseWiFiConfig");
    checkElementString("wifiNetwork3SSID", 0, 50, "Must be 0 to 50 characters", "collapseWiFiConfig");
    checkElementString("wifiNetwork3Password", 0, 50, "Must be 0 to 50 characters", "collapseWiFiConfig");

    //TCP/UDP Config
    if (ge("enablePvtClient").checked) {
        checkElementValue("pvtClientPort", 1, 65535, "Must be 1 to 65535", "collapseTCPUDPConfig");
        checkElementString("pvtClientHost", 1, 49, "Must be 1 to 49 characters", "collapseTCPUDPConfig");
    }
    if (ge("enablePvtServer").checked) {
        checkElementValue("pvtServerPort", 1, 65535, "Must be 1 to 65535", "collapseTCPUDPConfig");
    }
    if (ge("enablePvtUdpServer").checked) {
        checkElementValue("pvtUdpServerPort", 1, 65535, "Must be 1 to 65535", "collapseTCPUDPConfig");
    }
    checkCheckboxMutex("enablePvtClient", "enablePvtServer", "TCP Client and Server can not be enabled at the same time", "collapseTCPUDPConfig");

    //System Config
    if (ge("enableLogging").checked) {
        checkElementValue("maxLogTime_minutes", 1, 1051200, "Must be 1 to 1,051,200", "collapseSystemConfig");
        checkElementValue("maxLogLength_minutes", 1, 1051200, "Must be 1 to 1,051,200", "collapseSystemConfig");
    }
    else {
        clearElement("maxLogTime_minutes", 60 * 24);
        clearElement("maxLogLength_minutes", 60 * 24);
    }

    if (ge("enableARPLogging").checked) {
        checkElementValue("ARPLoggingInterval", 1, 600, "Must be 1 to 600", "collapseSystemConfig");
    }
    else {
        clearElement("ARPLoggingInterval", 10);
    }

    //Ethernet
    if (platformPrefix == "Reference Station") {
        if (ge("ethernetDHCP").checked == false) {
            checkElementIPAddress("ethernetIP", "Must be nnn.nnn.nnn.nnn", "collapseEthernetConfig");
            checkElementIPAddress("ethernetDNS", "Must be nnn.nnn.nnn.nnn", "collapseEthernetConfig");
            checkElementIPAddress("ethernetGateway", "Must be nnn.nnn.nnn.nnn", "collapseEthernetConfig");
            checkElementIPAddress("ethernetSubnet", "Must be nnn.nnn.nnn.nnn", "collapseEthernetConfig");
        }
        checkElementValue("ethernetNtpPort", 0, 65535, "Must be 0 to 65535", "collapseEthernetConfig");
    }

    //NTP
    if (platformPrefix == "Reference Station") {
        checkElementValue("ntpPollExponent", 3, 17, "Must be 3 to 17", "collapseNTPConfig");
        checkElementValue("ntpPrecision", -30, 0, "Must be -30 to 0", "collapseNTPConfig");
        checkElementValue("ntpRootDelay", 0, 10000000, "Must be 0 to 10,000,000", "collapseNTPConfig");
        checkElementValue("ntpRootDispersion", 0, 10000000, "Must be 0 to 10,000,000", "collapseNTPConfig");
        checkElementString("ntpReferenceId", 1, 4, "Must be 1 to 4 chars", "collapseNTPConfig");
    }

    //Port Config
    if (platformPrefix != "Surveyor") {
        if (ge("enableExternalPulse").checked) {
            checkElementValue("externalPulseTimeBetweenPulse_us", 1, 60000000, "Must be 1 to 60,000,000", "collapsePortsConfig");
            checkElementValue("externalPulseLength_us", 1, 60000000, "Must be 1 to 60,000,000", "collapsePortsConfig");
        }
        else {
            clearElement("externalPulseTimeBetweenPulse_us", 100000);
            clearElement("externalPulseLength_us", 1000000);
            ge("externalPulsePolarity").value = 0;
        }
    }
}

var currentProfileNumber = 0;

function changeProfile() {
    validateFields();

    if (errorCount == 1) {
        showError('saveBtn', "Please clear " + errorCount + " error");
        clearSuccess('saveBtn');
        $("input[name=profileRadio][value=" + currentProfileNumber + "]").prop('checked', true);
    }
    else if (errorCount > 1) {
        showError('saveBtn', "Please clear " + errorCount + " errors");
        clearSuccess('saveBtn');
        $("input[name=profileRadio][value=" + currentProfileNumber + "]").prop('checked', true);
    }
    else {
        ge("profileChangeMessage").innerHTML = 'Loading. Please wait...';

        currentProfileNumber = document.querySelector('input[name=profileRadio]:checked').value;

        sendData();
        clearError('saveBtn');
        showSuccess('saveBtn', "Saving...");

        websocket.send("setProfile," + currentProfileNumber + ",");

        ge("collapseProfileConfig").classList.add('show');
        collapseSection("collapseGNSSConfig", "gnssCaret");
        collapseSection("collapseGNSSConfigMsg", "gnssMsgCaret");
        collapseSection("collapseBaseConfig", "baseCaret");
        collapseSection("collapseSensorConfig", "sensorCaret");
        collapseSection("collapsePPConfig", "pointPerfectCaret");
        collapseSection("collapsePortsConfig", "portsCaret");
        collapseSection("collapseWiFiConfig", "wifiCaret");
        collapseSection("collapseTCPUDPConfig", "tcpUdpCaret");
        collapseSection("collapseRadioConfig", "radioCaret");
        collapseSection("collapseSystemConfig", "systemCaret");
        collapseSection("collapseEthernetConfig", "ethernetCaret");
        collapseSection("collapseNTPConfig", "ntpCaret");
    }
}

function saveConfig() {
    validateFields();

    if (errorCount == 1) {
        showError('saveBtn', "Please clear " + errorCount + " error");
        clearSuccess('saveBtn');
    }
    else if (errorCount > 1) {
        showError('saveBtn', "Please clear " + errorCount + " errors");
        clearSuccess('saveBtn');
    }
    else {
        sendData();
        clearError('saveBtn');
        showSuccess('saveBtn', "Saving...");
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

function checkBitMapValue(id, min, max, bitMap, errorText, collapseID) {
    value = ge(id).value;
    mask = ge(bitMap).value;
    if ((value < min) || (value > max) || ((mask & (1 << value)) == 0)) {
        ge(id + 'Error').innerHTML = 'Error: ' + errorText;
        ge(collapseID).classList.add('show');
        errorCount++;
    }
    else {
        clearError(id);
    }
}

//Check if Lat/Long input types are decipherable
function checkLatLong() {
    var id = "fixedLatText";
    var collapseID = "collapseBaseConfig";
    ge("detectedFormatText").value = "";

    var inputTypeLat = identifyInputType(ge(id).value)
    if (inputTypeLat == CoordinateTypes.COORDINATE_INPUT_TYPE_INVALID_UNKNOWN) {
        var errorText = "Coordinate format unknown";
        ge(id + 'Error').innerHTML = 'Error: ' + errorText;
        ge(collapseID).classList.add('show');
        errorCount++;
    }
    else if (convertedCoordinate < -180 || convertedCoordinate > 180) {
        var errorText = "Must be -180 to 180";
        ge(id + 'Error').innerHTML = 'Error: ' + errorText;
        ge(collapseID).classList.add('show');
        errorCount++;
    }
    else
        clearError(id);

    id = "fixedLongText";
    var inputTypeLong = identifyInputType(ge(id).value)
    if (inputTypeLong == CoordinateTypes.COORDINATE_INPUT_TYPE_INVALID_UNKNOWN) {
        var errorText = "Coordinate format unknown";
        ge(id + 'Error').innerHTML = 'Error: ' + errorText;
        ge(collapseID).classList.add('show');
        errorCount++;
    }
    else if (convertedCoordinate < -180 || convertedCoordinate > 180) {
        var errorText = "Must be -180 to 180";
        ge(id + 'Error').innerHTML = 'Error: ' + errorText;
        ge(collapseID).classList.add('show');
        errorCount++;
    }
    else
        clearError(id);

    if (inputTypeLong != inputTypeLat) {
        var errorText = "Formats must match";
        ge(id + 'Error').innerHTML = 'Error: ' + errorText;
        ge(collapseID).classList.add('show');
        errorCount++;
        ge("detectedFormatText").innerHTML = printableInputType(CoordinateTypes.COORDINATE_INPUT_TYPE_INVALID_UNKNOWN);
    }
    else
        ge("detectedFormatText").innerHTML = printableInputType(inputTypeLat);
}

//Based on the coordinateInputType, format the lat/long text boxes
function updateLatLong() {
    ge("fixedLatText").value = convertInput(fixedLat, coordinateInputType);
    ge("fixedLongText").value = convertInput(fixedLong, coordinateInputType);
    checkLatLong(); //Updates the detected format
}

function checkElementValue(id, min, max, errorText, collapseID) {
    value = ge(id).value;
    if (value < min || value > max || value == "") {
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

function checkElementIPAddress(id, errorText, collapseID) {
    value = ge(id).value;
    var data = value.split('.');
    if ((data.length != 4)
        || ((data[0] == "") || (isNaN(Number(data[0]))) || (data[0] < 0) || (data[0] > 255))
        || ((data[1] == "") || (isNaN(Number(data[1]))) || (data[1] < 0) || (data[1] > 255))
        || ((data[2] == "") || (isNaN(Number(data[2]))) || (data[2] < 0) || (data[2] > 255))
        || ((data[3] == "") || (isNaN(Number(data[3]))) || (data[3] < 0) || (data[3] > 255))) {
        ge(id + 'Error').innerHTML = 'Error: ' + errorText;
        ge(collapseID).classList.add('show');
        errorCount++;
    }
    else
        clearError(id);
}

function checkElementCasterUser(host, user, url, needs, errorText, collapseID) {
    if (ge(host).value.toLowerCase().includes(url)) {
        value = ge(user).value;
        if ((value.length < 1) || (value.length > 49) || (value.includes(needs) == false)) {
            ge(user + 'Error').innerHTML = 'Error: ' + errorText;
            ge(collapseID).classList.add('show');
            errorCount++;
        }
        else
            clearError(user);
    }
    else
        clearError(user);
}

function checkCheckboxMutex(id1, id2, errorText, collapseID) {
    if ((ge(id1).checked) && (ge(id2).checked)) {
        ge(id1 + 'Error').innerHTML = 'Error: ' + errorText;
        ge(id2 + 'Error').innerHTML = 'Error: ' + errorText;
        ge(collapseID).classList.add('show');
        errorCount++;
    }
    else {
        clearError(id1);
        clearError(id2);
    }
}

function clearElement(id, value) {
    ge(id).value = value;
    clearError(id);
}

function resetToFactoryDefaults() {
    ge("factoryDefaultsMsg").innerHTML = "Defaults Applied. Please wait for device reset...";
    websocket.send("factoryDefaultReset,1,");
}

function zeroElement(id) {
    ge(id).value = 0;
}

function zeroMessages() {

    var ubxMessages = document.querySelectorAll('input[id^=UBX_]'); //match all ids starting with UBX_
    for (let x = 0; x < ubxMessages.length; x++) {
        var messageName = ubxMessages[x].id;
        zeroElement(messageName);
    }
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

function resetToRTCMDefaults() {
    ge("UBX_RTCM_1005Base").value = 1;
    ge("UBX_RTCM_1074Base").value = 1;
    ge("UBX_RTCM_1077Base").value = 0;
    ge("UBX_RTCM_1084Base").value = 1;
    ge("UBX_RTCM_1087Base").value = 0;

    ge("UBX_RTCM_1094Base").value = 1;
    ge("UBX_RTCM_1097Base").value = 0;
    ge("UBX_RTCM_1124Base").value = 1;
    ge("UBX_RTCM_1127Base").value = 0;
    ge("UBX_RTCM_1230Base").value = 10;

    ge("UBX_RTCM_4072_0Base").value = 0;
    ge("UBX_RTCM_4072_1Base").value = 0;
}

function resetToLowBandwidthRTCM() {
    ge("UBX_RTCM_1005Base").value = 10;
    ge("UBX_RTCM_1074Base").value = 2;
    ge("UBX_RTCM_1077Base").value = 0;
    ge("UBX_RTCM_1084Base").value = 2;
    ge("UBX_RTCM_1087Base").value = 0;

    ge("UBX_RTCM_1094Base").value = 2;
    ge("UBX_RTCM_1097Base").value = 0;
    ge("UBX_RTCM_1124Base").value = 2;
    ge("UBX_RTCM_1127Base").value = 0;
    ge("UBX_RTCM_1230Base").value = 10;

    ge("UBX_RTCM_4072_0Base").value = 0;
    ge("UBX_RTCM_4072_1Base").value = 0;
}

function useECEFCoordinates() {
    ge("fixedEcefX").value = ecefX;
    ge("fixedEcefY").value = ecefY;
    ge("fixedEcefZ").value = ecefZ;
}
function useGeodeticCoordinates() {
    ge("fixedLatText").value = geodeticLat;
    ge("fixedLongText").value = geodeticLon;
    ge("fixedHAE_APC").value = geodeticAlt;

    $("input[name=markRadio][value=1]").prop('checked', true);
    $("input[name=markRadio][value=2]").prop('checked', false);

    adjustHAE();
}

function startNewLog() {
    websocket.send("startNewLog,1,");
}

function exitConfig() {
    hide("mainPage");
    show("resetInProcess");

    websocket.send("exitAndReset,1,");
    resetTimeout = setTimeout(exitConfig, 2000);
}

function resetComplete() {
    clearTimeout(resetTimeout);
    hide("mainPage");
    hide("resetInProcess");
    show("resetComplete");
}

//Called when the ESP32 has confirmed receipt of data over websocket from AP config page
function confirmDataReceipt() {
    //Determine which function sent the original data
    if (sendDataTimeout != null) {
        clearTimeout(sendDataTimeout);
        showSuccess('saveBtn', "All Saved!");
    }
    else {
        console.log("Unknown owner of confirmDataReceipt");
    }
}

function firmwareUploadWait() {
    var file = ge("submitFirmwareFile").files[0];
    var formdata = new FormData();
    formdata.append("submitFirmwareFile", file);
    var ajax = new XMLHttpRequest();
    ajax.open("POST", "/upload");
    ajax.send(formdata);

    ge("firmwareUploadMsg").innerHTML = "<br>Uploading, please wait...";
}

function firmwareUploadStatus(val) {
    ge("firmwareUploadMsg").innerHTML = val;
}

function firmwareUploadComplete() {
    show("firmwareUploadComplete");
    hide("mainPage");
}

function forgetPairedRadios() {
    ge("btnForgetRadiosMsg").innerHTML = "All radios forgotten.";
    ge("peerMACs").innerHTML = "None";
    websocket.send("forgetEspNowPeers,1,");
}

function btnResetProfile() {
    ge("resetProfileMsg").innerHTML = "Resetting profile.";
    websocket.send("resetProfile,1,");
}

document.addEventListener("DOMContentLoaded", (event) => {

    var radios = document.querySelectorAll('input[name=profileRadio]');
    for (var i = 0, max = radios.length; i < max; i++) {
        radios[i].onclick = function () {
            changeProfile();
        }
    }

    ge("measurementRateHz").addEventListener("change", function () {
        ge("measurementRateSec").value = 1.0 / ge("measurementRateHz").value;
    });

    ge("measurementRateSec").addEventListener("change", function () {
        ge("measurementRateHz").value = 1.0 / ge("measurementRateSec").value;
    });

    ge("baseTypeSurveyIn").addEventListener("change", function () {
        if (ge("baseTypeSurveyIn").checked) {
            show("surveyInConfig");
            hide("fixedConfig");
        }
    });

    ge("baseTypeFixed").addEventListener("change", function () {
        if (ge("baseTypeFixed").checked) {
            show("fixedConfig");
            hide("surveyInConfig");
        }
    });

    ge("fixedBaseCoordinateTypeECEF").addEventListener("change", function () {
        if (ge("fixedBaseCoordinateTypeECEF").checked) {
            show("ecefConfig");
            hide("geodeticConfig");
        }
    });

    ge("fixedBaseCoordinateTypeGeo").addEventListener("change", function () {
        if (ge("fixedBaseCoordinateTypeGeo").checked) {
            hide("ecefConfig");
            show("geodeticConfig");

            if (platformPrefix == "Facet") {
                ge("antennaReferencePoint").value = 61.4;
            }
            else if (platformPrefix == "Facet L-Band" || platformPrefix == "Facet L-Band Direct") {
                ge("antennaReferencePoint").value = 69.0;
            }
            else {
                ge("antennaReferencePoint").value = 0.0;
            }
        }
    });

    ge("enableNtripServer").addEventListener("change", function () {
        if (ge("enableNtripServer").checked) {
            show("ntripServerConfig");
        }
        else {
            hide("ntripServerConfig");
        }
    });

    ge("enableNtripClient").addEventListener("change", function () {
        if (ge("enableNtripClient").checked) {
            show("ntripClientConfig");
        }
        else {
            hide("ntripClientConfig");
        }
    });

    ge("enableFactoryDefaults").addEventListener("change", function () {
        if (ge("enableFactoryDefaults").checked) {
            ge("factoryDefaults").disabled = false;
        }
        else {
            ge("factoryDefaults").disabled = true;
        }
    });

    ge("dataPortChannel").addEventListener("change", function () {
        if (ge("dataPortChannel").value == 0) {
            show("dataPortBaudDropdown");
            hide("externalPulseConfig");
        }
        else if (ge("dataPortChannel").value == 1) {
            hide("dataPortBaudDropdown");
            show("externalPulseConfig");
        }
        else {
            hide("dataPortBaudDropdown");
            hide("externalPulseConfig");
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

    ge("enablePointPerfectCorrections").addEventListener("change", function () {
        if (ge("enablePointPerfectCorrections").checked) {
            show("ppSettingsConfig");
        }
        else {
            hide("ppSettingsConfig");
        }
    });

    ge("enableExternalPulse").addEventListener("change", function () {
        if (ge("enableExternalPulse").checked) {
            show("externalPulseConfigDetails");
        }
        else {
            hide("externalPulseConfigDetails");
        }
    });

    ge("radioType").addEventListener("change", function () {
        if (ge("radioType").value == 0) {
            hide("radioDetails");
        }
        else if (ge("radioType").value == 1) {
            show("radioDetails");
        }
    });

    ge("enableForgetRadios").addEventListener("change", function () {
        if (ge("enableForgetRadios").checked) {
            ge("btnForgetRadios").disabled = false;
        }
        else {
            ge("btnForgetRadios").disabled = true;
        }
    });

    ge("enableLogging").addEventListener("change", function () {
        if (ge("enableLogging").checked) {
            show("enableLoggingDetails");
        }
        else {
            hide("enableLoggingDetails");
        }
    });

    ge("enableARPLogging").addEventListener("change", function () {
        if (ge("enableARPLogging").checked) {
            show("enableARPLoggingDetails");
        }
        else {
            hide("enableARPLoggingDetails");
        }
    });

    ge("fixedAltitude").addEventListener("change", function () {
        adjustHAE();
    });

    ge("antennaHeight").addEventListener("change", function () {
        adjustHAE();
    });

    ge("antennaReferencePoint").addEventListener("change", function () {
        adjustHAE();
    });

    ge("fixedHAE_APC").addEventListener("change", function () {
        adjustHAE();
    });

    ge("autoIMUmountAlignment").addEventListener("change", function () {
        if (ge("autoIMUmountAlignment").checked) {
            ge("imuYaw").disabled = true;
            ge("imuPitch").disabled = true;
            ge("imuRoll").disabled = true;
        }
        else {
            ge("imuYaw").disabled = false;
            ge("imuPitch").disabled = false;
            ge("imuRoll").disabled = false;
        }
    });

})

function addECEF() {
    errorCount = 0;

    nicknameECEF.value = removeBadChars(nicknameECEF.value);

    checkElementString("nicknameECEF", 1, 49, "Must be 1 to 49 characters", "collapseBaseConfig");
    checkElementValue("fixedEcefX", -7000000, 7000000, "Must be -7000000 to 7000000", "collapseBaseConfig");
    checkElementValue("fixedEcefY", -7000000, 7000000, "Must be -7000000 to 7000000", "collapseBaseConfig");
    checkElementValue("fixedEcefZ", -7000000, 7000000, "Must be -7000000 to 7000000", "collapseBaseConfig");

    if (errorCount == 0) {
        //Check name against the list
        var index = 0;
        for (; index < recordsECEF.length; ++index) {
            var parts = recordsECEF[index].split(' ');
            if (ge("nicknameECEF").value == parts[0]) {
                recordsECEF[index] = nicknameECEF.value + ' ' + fixedEcefX.value + ' ' + fixedEcefY.value + ' ' + fixedEcefZ.value;
                break;
            }
        }
        if (index == recordsECEF.length)
            recordsECEF.push(nicknameECEF.value + ' ' + fixedEcefX.value + ' ' + fixedEcefY.value + ' ' + fixedEcefZ.value);
    }

    updateECEFList();
}

function deleteECEF() {

    var val = ge("StationCoordinatesECEF").value;
    if (val > "") {
        var parts = recordsECEF[val].split(' ');
        var nickName = parts[0];

        if (confirm("Delete location " + nickName + "?") == true) {
            recordsECEF.splice(val, 1);
        }
    }
    updateECEFList();
}

function loadECEF() {
    var val = ge("StationCoordinatesECEF").value;
    if (val > "") {
        var parts = recordsECEF[val].split(' ');
        ge("fixedEcefX").value = parts[1];
        ge("fixedEcefY").value = parts[2];
        ge("fixedEcefZ").value = parts[3];
        ge("nicknameECEF").value = parts[0];
        clearError("fixedEcefX");
        clearError("fixedEcefY");
        clearError("fixedEcefZ");
        clearError("nicknameECEF");
    }
}

//Based on recordsECEF array, update and monospace HTML list
function updateECEFList() {
    ge("StationCoordinatesECEF").length = 0;

    if (recordsECEF.length == 0) {
        hide("StationCoordinatesECEF");
        nicknameECEFText.innerHTML = "No coordinates stored";
    }
    else {
        show("StationCoordinatesECEF");
        nicknameECEFText.innerHTML = "Nickname: X/Y/Z";
        if (recordsECEF.length < 5)
            ge("StationCoordinatesECEF").size = recordsECEF.length;
    }

    for (let index = 0; index < recordsECEF.length; ++index) {
        var option = document.createElement('option');
        option.text = recordsECEF[index];
        option.value = index;
        ge("StationCoordinatesECEF").add(option);
    }

    $("#StationCoordinatesECEF option").each(function () {
        var parts = $(this).text().split(' ');
        var nickname = parts[0].substring(0, 15);
        $(this).text(nickname + ': ' + parts[1] + ' ' + parts[2] + ' ' + parts[3]).text;
    });
}

function addGeodetic() {
    errorCount = 0;

    nicknameGeodetic.value = removeBadChars(nicknameGeodetic.value);

    checkElementString("nicknameGeodetic", 1, 49, "Must be 1 to 49 characters", "collapseBaseConfig");
    checkLatLong();
    checkElementValue("fixedAltitude", -11034, 8849, "Must be -11034 to 8849", "collapseBaseConfig");
    checkElementValue("antennaHeight", -15000, 15000, "Must be -15000 to 15000", "collapseBaseConfig");
    checkElementValue("antennaReferencePoint", -200.0, 200.0, "Must be -200.0 to 200.0", "collapseBaseConfig");

    if (errorCount == 0) {
        //Check name against the list
        var index = 0;
        for (; index < recordsGeodetic.length; ++index) {
            var parts = recordsGeodetic[index].split(' ');
            if (ge("nicknameGeodetic").value == parts[0]) {
                recordsGeodetic[index] = nicknameGeodetic.value + ' ' + fixedLatText.value + ' ' + fixedLongText.value + ' ' + fixedAltitude.value + ' ' + antennaHeight.value + ' ' + antennaReferencePoint.value;
                break;
            }
        }
        if (index == recordsGeodetic.length)
            recordsGeodetic.push(nicknameGeodetic.value + ' ' + fixedLatText.value + ' ' + fixedLongText.value + ' ' + fixedAltitude.value + ' ' + antennaHeight.value + ' ' + antennaReferencePoint.value);
    }

    updateGeodeticList();
}

function deleteGeodetic() {
    var val = ge("StationCoordinatesGeodetic").value;
    if (val > "") {
        var parts = recordsGeodetic[val].split(' ');
        var nickName = parts[0];

        if (confirm("Delete location " + nickName + "?") == true) {
            recordsGeodetic.splice(val, 1);
        }
    }
    updateGeodeticList();
}

function adjustHAE() {

    var haeMethod = document.querySelector('input[name=markRadio]:checked').value;
    var hae;
    if (haeMethod == 1) {
        ge("fixedHAE_APC").disabled = false;
        ge("fixedAltitude").disabled = true;
        hae = Number(ge("fixedHAE_APC").value) - (Number(ge("antennaHeight").value) / 1000 + Number(ge("antennaReferencePoint").value) / 1000);
        ge("fixedAltitude").value = hae.toFixed(3);
    }
    else {
        ge("fixedHAE_APC").disabled = true;
        ge("fixedAltitude").disabled = false;
        hae = Number(ge("fixedAltitude").value) + (Number(ge("antennaHeight").value) / 1000 + Number(ge("antennaReferencePoint").value) / 1000);
        ge("fixedHAE_APC").value = hae.toFixed(3);
    }
}

function loadGeodetic() {
    var val = ge("StationCoordinatesGeodetic").value;
    if (val > "") {
        var parts = recordsGeodetic[val].split(' ');
        var numParts = parts.length;
        if (numParts >= 6) {
            var latParts = (numParts - 4) / 2;
            ge("nicknameGeodetic").value = parts[0];
            ge("fixedLatText").value = parts[1];
            if (latParts > 1) {
                for (let moreParts = 1; moreParts < latParts; moreParts++) {
                    ge("fixedLatText").value += ' ' + parts[moreParts + 1];
                }
            }
            ge("fixedLongText").value = parts[1 + latParts];
            if (latParts > 1) {
                for (let moreParts = 1; moreParts < latParts; moreParts++) {
                    ge("fixedLongText").value += ' ' + parts[moreParts + 1 + latParts];
                }
            }
            ge("fixedAltitude").value = parts[numParts - 3];
            ge("antennaHeight").value = parts[numParts - 2];
            ge("antennaReferencePoint").value = parts[numParts - 1];

            $("input[name=markRadio][value=1]").prop('checked', false);
            $("input[name=markRadio][value=2]").prop('checked', true);

            adjustHAE();

            clearError("nicknameGeodetic");
            clearError("fixedLatText");
            clearError("fixedLongText");
            clearError("fixedAltitude");
            clearError("antennaHeight");
            clearError("antennaReferencePoint");
        }
        else {
            console.log("stationGeodetic split error");
        }
    }
}

//Based on recordsGeodetic array, update and monospace HTML list
function updateGeodeticList() {
    ge("StationCoordinatesGeodetic").length = 0;

    if (recordsGeodetic.length == 0) {
        hide("StationCoordinatesGeodetic");
        nicknameGeodeticText.innerHTML = "No coordinates stored";
    }
    else {
        show("StationCoordinatesGeodetic");
        nicknameGeodeticText.innerHTML = "Nickname: Lat/Long/Alt";
        if (recordsGeodetic.length < 5)
            ge("StationCoordinatesGeodetic").size = recordsGeodetic.length;
    }

    for (let index = 0; index < recordsGeodetic.length; ++index) {
        var option = document.createElement('option');
        option.text = recordsGeodetic[index];
        option.value = index;
        ge("StationCoordinatesGeodetic").add(option);
    }

    $("#StationCoordinatesGeodetic option").each(function () {
        var parts = $(this).text().split(' ');
        var nickname = parts[0].substring(0, 15);

        if (parts.length >= 7) {
            $(this).text(nickname + ': ' + parts[1] + ' ' + parts[2] + ' ' + parts[3]
                + ' ' + parts[4] + ' ' + parts[5] + ' ' + parts[6]
                + ' ' + parts[7]).text;
        }
        else {
            $(this).text(nickname + ': ' + parts[1] + ' ' + parts[2] + ' ' + parts[3]).text;
        }

    });
}

function removeBadChars(val) {
    val = val.split(' ').join('');
    val = val.split(',').join('');
    val = val.split('\\').join('');
    return (val);
}

function getFileList() {
    if (showingFileList == false) {
        showingFileList = true;

        //If the tab was just opened, create table from scratch
        ge("fileManagerTable").innerHTML = "<table><tr align='left'><th>Name</th><th>Size</th><td><input type='checkbox' id='fileSelectAll' class='form-check-input fileManagerCheck' onClick='fileManagerToggle()'></td></tr></tr></table>";
        fileTableText = "";

        xmlhttp = new XMLHttpRequest();
        xmlhttp.open("GET", "/listfiles", false);
        xmlhttp.send();

        parseIncoming(xmlhttp.responseText); //Process CSV data into HTML

        ge("fileManagerTable").innerHTML += fileTableText;
    }
    else {
        showingFileList = false;
    }
}

function getMessageList() {
    if (obtainedMessageList == false) {
        obtainedMessageList = true;

        ge("messageList").innerHTML = "";
        messageText = "";

        xmlhttp = new XMLHttpRequest();
        xmlhttp.open("GET", "/listMessages", false);
        xmlhttp.send();

        parseIncoming(xmlhttp.responseText); //Process CSV data into HTML

        ge("messageList").innerHTML += messageText;
    }
}

function getMessageListBase() {
    if (obtainedMessageListBase == false) {
        obtainedMessageListBase = true;

        ge("messageListBase").innerHTML = "";
        messageText = "";

        xmlhttp = new XMLHttpRequest();
        xmlhttp.open("GET", "/listMessagesBase", false);
        xmlhttp.send();

        parseIncoming(xmlhttp.responseText); //Process CSV data into HTML

        ge("messageListBase").innerHTML += messageText;
    }
}

function fileManagerDownload() {
    selectedFiles = document.querySelectorAll('input[name=fileID]:checked');
    numberOfFilesSelected = document.querySelectorAll('input[name=fileID]:checked').length;
    fileNumber = 0;
    sendFile(); //Start first send
}

function sendFile() {
    if (fileNumber == numberOfFilesSelected) return;
    var urltocall = "/file?name=" + selectedFiles[fileNumber].id + "&action=download";
    console.log(urltocall);
    window.location.href = urltocall;

    fileNumber++;
}

function fileManagerToggle() {
    var checkboxes = document.querySelectorAll('input[name=fileID]');
    for (var i = 0, n = checkboxes.length; i < n; i++) {
        checkboxes[i].checked = ge("fileSelectAll").checked;
    }
}

function fileManagerDelete() {
    selectedFiles = document.querySelectorAll('input[name=fileID]:checked');

    if (confirm("Delete " + selectedFiles.length + " files?") == false) {
        return;
    }

    for (let x = 0; x < selectedFiles.length; x++) {
        var urltocall = "/file?name=" + selectedFiles[x].id + "&action=delete";
        xmlhttp = new XMLHttpRequest();

        xmlhttp.open("GET", urltocall, false);
        xmlhttp.send();
    }

    //Refresh file list
    showingFileList = false;
    getFileList();
}

function uploadFile() {
    var file = ge("file1").files[0];
    var formdata = new FormData();
    formdata.append("file1", file);
    var ajax = new XMLHttpRequest();
    ajax.upload.addEventListener("progress", progressHandler, false);
    ajax.addEventListener("load", completeHandler, false);
    ajax.addEventListener("error", errorHandler, false);
    ajax.addEventListener("abort", abortHandler, false);
    ajax.open("POST", "/");
    ajax.send(formdata);
}
function progressHandler(event) {
    var percent = (event.loaded / event.total) * 100;
    ge("progressBar").value = Math.round(percent);
    ge("uploadStatus").innerHTML = Math.round(percent) + "% uploaded...";
    if (percent >= 100) {
        ge("uploadStatus").innerHTML = "Please wait, writing file to filesystem";
    }
}
function completeHandler(event) {
    ge("uploadStatus").innerHTML = "Upload Complete";
    ge("progressBar").value = 0;

    //Refresh file list
    showingFileList = false;
    getFileList();

    document.getElementById("uploadStatus").innerHTML = "Upload Complete";
}
function errorHandler(event) {
    ge("uploadStatus").innerHTML = "Upload Failed";
}
function abortHandler(event) {
    ge("uploadStatus").innerHTML = "Upload Aborted";
}

function tcpClientBoxes() {
    if (ge("enablePvtClient").checked) {
        show("tcpClientSettingsConfig");
    }
    else {
        hide("tcpClientSettingsConfig");
        //ge("pvtClientPort").value = 2947;
    }
}

function tcpServerBoxes() {
    if (ge("enablePvtServer").checked) {
        show("tcpServerSettingsConfig");
    }
    else {
        hide("tcpServerSettingsConfig");
        //ge("pvtServerPort").value = 2947;
    }
}

function udpBoxes() {
    if (ge("enablePvtUdpServer").checked) {
        show("udpSettingsConfig");
    }
    else {
        hide("udpSettingsConfig");
        //ge("pvtUdpServerPort").value = 10110;
    }
}

function dhcpEthernet() {
    if (ge("ethernetDHCP").checked) {
        hide("fixedIPSettingsConfigEthernet");
    }
    else {
        show("fixedIPSettingsConfigEthernet");
    }
}

function networkCount() {
    var count = 0;

    var wifiNetworks = document.querySelectorAll('input[id^=wifiNetwork]' && 'input[id$=SSID]');
    for (let x = 0; x < wifiNetworks.length; x++) {
        if (wifiNetworks[x].value.length > 0)
            count++;
    }

    return (count);
}

function checkNewFirmware() {
    if (networkCount() == 0) {
        showMsgError('firmwareCheckNewMsg', "WiFi list is empty");
        return;
    }

    ge("btnCheckNewFirmware").disabled = true;
    showMsg('firmwareCheckNewMsg', "Connecting to WiFi", false);

    var settingCSV = "";

    //Send current WiFi SSID and PWs
    var clsElements = document.querySelectorAll('input[id^=wifiNetwork]');
    for (let x = 0; x < clsElements.length; x++) {
        settingCSV += clsElements[x].id + "," + clsElements[x].value + ",";
    }

    if (ge("enableRCFirmware").checked == true)
        settingCSV += "enableRCFirmware,true,";
    else
        settingCSV += "enableRCFirmware,false,";

    settingCSV += "checkNewFirmware,1,";

    console.log("firmware sending: " + settingCSV);
    websocket.send(settingCSV);

    checkNewFirmwareTimeout = setTimeout(checkNewFirmware, 2000);
}

function checkingNewFirmware() {
    clearTimeout(checkNewFirmwareTimeout);
    console.log("Clearing timeout for checkNewFirmwareTimeout");

    showMsg('firmwareCheckNewMsg', "Checking firmware version");
}

function newFirmwareVersion(firmwareVersion) {
    clearMsg('firmwareCheckNewMsg');
    if (firmwareVersion == "ERROR") {
        showMsgError('firmwareCheckNewMsg', "WiFi or Server not available");
        hide("divGetNewFirmware");
        ge("btnCheckNewFirmware").disabled = false;
        return;
    }
    else if (firmwareVersion == "CURRENT") {
        showMsg('firmwareCheckNewMsg', "Firmware is up to date");
        hide("divGetNewFirmware");
        ge("btnCheckNewFirmware").disabled = false;
        return;
    }

    ge("btnGetNewFirmware").innerHTML = "Update to v" + firmwareVersion;
    ge("btnGetNewFirmware").disabled = false;
    ge("firmwareUpdateProgressBar").value = 0;
    clearMsg('firmwareUpdateProgressMsg');
    show("divGetNewFirmware");
}

function getNewFirmware() {

    if (networkCount() == 0) {
        showMsgError('firmwareCheckNewMsg', "WiFi list is empty");
        hide("divGetNewFirmware");
        ge("btnCheckNewFirmware").disabled = false;
        return;
    }

    ge("btnGetNewFirmware").disabled = true;
    clearMsg('firmwareCheckNewMsg');
    showMsg('firmwareUpdateProgressMsg', "Getting new firmware");

    var settingCSV = "";

    //Send current WiFi SSID and PWs
    var clsElements = document.querySelectorAll('input[id^=wifiNetwork]');
    for (let x = 0; x < clsElements.length; x++) {
        settingCSV += clsElements[x].id + "," + clsElements[x].value + ",";
    }
    settingCSV += "getNewFirmware,1,";

    console.log("firmware sending: " + settingCSV);
    websocket.send(settingCSV);

    getNewFirmwareTimeout = setTimeout(getNewFirmware, 2000);
}

function gettingNewFirmware(val) {
    if (val == "1") {
        clearTimeout(getNewFirmwareTimeout);
    }
    else if (val == "ERROR") {
        hide("divGetNewFirmware");
        ge("btnCheckNewFirmware").disabled = false;
        showMsg('firmwareCheckNewMsg', "Error getting new firmware", true);
    }
}

function otaFirmwareStatus(percentComplete) {
    clearTimeout(getNewFirmwareTimeout);

    showMsg('firmwareUpdateProgressMsg', percentComplete + "% Complete");
    ge("firmwareUpdateProgressBar").value = percentComplete;

    if (percentComplete == 100) {
        resetComplete();
    }
}

//Given a user's string, try to identify the type and return the coordinate in DD.ddddddddd format
function identifyInputType(userEntry) {
    var coordinateInputType = CoordinateTypes.COORDINATE_INPUT_TYPE_INVALID_UNKNOWN;
    var dashCount = 0;
    var spaceCount = 0;
    var decimalCount = 0;
    var lengthOfLeadingNumber = 0;
    convertedCoordinate = 0.0; //Clear what is given to us

    //Scan entry for invalid chars
    //A valid entry has only numbers, -, ' ', and .
    for (var x = 0; x < userEntry.length; x++) {

        if (isdigit(userEntry[x])) {
            if (decimalCount == 0) lengthOfLeadingNumber++
        }
        else if (userEntry[x] == '-') dashCount++; //All good
        else if (userEntry[x] == ' ') spaceCount++; //All good
        else if (userEntry[x] == '.') decimalCount++; //All good
        else return (CoordinateTypes.COORDINATE_INPUT_TYPE_INVALID_UNKNOWN); //String contains invalid character
    }

    // Seven possible entry types
    // DD.dddddd
    // DDMM.mmmmmmm
    // DD MM.mmmmmmm
    // DD-MM.mmmmmmm
    // DDMMSS.ssssss
    // DD MM SS.ssssss
    // DD-MM-SS.ssssss

    if (decimalCount > 1) return (CoordinateTypes.COORDINATE_INPUT_TYPE_INVALID_UNKNOWN); //Just no. 40.09033470 is valid.
    if (spaceCount > 2) return (CoordinateTypes.COORDINATE_INPUT_TYPE_INVALID_UNKNOWN); //Only 0, 1, or 2 allowed. 40 05 25.2049 is valid.
    if (dashCount > 3) return (CoordinateTypes.COORDINATE_INPUT_TYPE_INVALID_UNKNOWN); //Only 0, 1, 2, or 3 allowed. -105-11-05.1629 is valid.
    if (lengthOfLeadingNumber > 7) return (CoordinateTypes.COORDINATE_INPUT_TYPE_INVALID_UNKNOWN); //Only 7 or fewer. -1051105.188992 (DDDMMSS or DDMMSS) is valid

    //console.log("userEntry: " + userEntry);
    //console.log("decimalCount: " + decimalCount);
    //console.log("spaceCount: " + spaceCount);
    //console.log("dashCount: " + dashCount);
    //console.log("lengthOfLeadingNumber: " + lengthOfLeadingNumber);

    var negativeSign = false;
    if (userEntry[0] == '-') {
        userEntry = setCharAt(userEntry, 0, ''); //Remove leading minus
        negativeSign = true;
        dashCount--; //Use dashCount as the internal dashes only, not the leading negative sign
    }

    if (spaceCount == 0 && dashCount == 0 && (lengthOfLeadingNumber == 7 || lengthOfLeadingNumber == 6)) //DDMMSS.ssssss
    {
        coordinateInputType = CoordinateTypes.COORDINATE_INPUT_TYPE_DDMMSS;

        var intPortion = Math.trunc(Number(userEntry)); //Get DDDMMSS
        var decimal = Math.trunc(intPortion / 10000); //Get DDD
        intPortion -= (decimal * 10000);
        var minutes = Math.trunc(intPortion / 100); //Get MM

        //Find '.'
        if (userEntry.indexOf('.') == -1)
            coordinateInputType = CoordinateTypes.COORDINATE_INPUT_TYPE_DDMMSS_NO_DECIMAL;

        var seconds = userEntry; //Get DDDMMSS.ssssss
        seconds -= (decimal * 10000); //Remove DDD
        seconds -= (minutes * 100); //Remove MM
        convertedCoordinate = decimal + (minutes / 60.0) + (seconds / 3600.0);
        if (negativeSign) convertedCoordinate *= -1;
    }
    else if (spaceCount == 0 && dashCount == 0 && (lengthOfLeadingNumber == 5 || lengthOfLeadingNumber == 4)) //DDMM.mmmmmmm
    {
        coordinateInputType = CoordinateTypes.COORDINATE_INPUT_TYPE_DDMM;

        var intPortion = Math.trunc(userEntry); //Get DDDMM
        var decimal = intPortion / 100; //Get DDD
        intPortion -= (decimal * 100);
        var minutes = userEntry; //Get DDDMM.mmmmmmm
        minutes -= (decimal * 100); //Remove DDD
        convertedCoordinate = decimal + (minutes / 60.0);
        if (negativeSign) convertedCoordinate *= -1.0;
    }

    else if (dashCount == 1) //DD-MM.mmmmmmm
    {
        coordinateInputType = CoordinateTypes.COORDINATE_INPUT_TYPE_DD_MM_DASH;

        var data = userEntry.split('-');
        var decimal = Number(data[0]); //Get DD
        var minutes = Number(data[1]); //Get MM.mmmmmmm
        convertedCoordinate = decimal + (minutes / 60.0);
        if (negativeSign) convertedCoordinate *= -1.0;
    }
    else if (dashCount == 2) //DD-MM-SS.ssss
    {
        coordinateInputType = CoordinateTypes.COORDINATE_INPUT_TYPE_DD_MM_SS_DASH;

        var data = userEntry.split('-');
        var decimal = Number(data[0]); //Get DD
        var minutes = Number(data[1]); //Get MM

        //Find '.'
        if (userEntry.indexOf('.') == -1)
            coordinateInputType = CoordinateTypes.COORDINATE_INPUT_TYPE_DD_MM_SS_DASH_NO_DECIMAL;

        var seconds = Number(data[2]); //Get SS.ssssss
        convertedCoordinate = decimal + (minutes / 60.0) + (seconds / 3600.0);
        if (negativeSign) convertedCoordinate *= -1.0;
    }
    else if (spaceCount == 0) //DD.ddddddddd
    {
        coordinateInputType = CoordinateTypes.COORDINATE_INPUT_TYPE_DD;
        convertedCoordinate = userEntry;
        if (negativeSign) convertedCoordinate *= -1.0;
    }
    else if (spaceCount == 1) //DD MM.mmmmmmm
    {
        coordinateInputType = CoordinateTypes.COORDINATE_INPUT_TYPE_DD_MM;

        var data = userEntry.split(' ');
        var decimal = Number(data[0]); //Get DD
        var minutes = Number(data[1]); //Get MM.mmmmmmm
        convertedCoordinate = decimal + (minutes / 60.0);
        if (negativeSign) convertedCoordinate *= -1.0;
    }
    else if (spaceCount == 2) //DD MM SS.ssssss
    {
        coordinateInputType = CoordinateTypes.COORDINATE_INPUT_TYPE_DD_MM_SS;

        var data = userEntry.split(' ');
        var decimal = Number(data[0]); //Get DD
        var minutes = Number(data[1]); //Get MM

        //Find '.'
        if (userEntry.indexOf('.') == -1)
            coordinateInputType = CoordinateTypes.COORDINATE_INPUT_TYPE_DD_MM_SS_NO_DECIMAL;

        var seconds = Number(data[2]); //Get SS.ssssss
        convertedCoordinate = decimal + (minutes / 60.0) + (seconds / 3600.0);
        if (negativeSign) convertedCoordinate *= -1.0;
    }

    //console.log("convertedCoordinate: " + Number(convertedCoordinate).toFixed(9));
    //console.log("Detected type: " + printableInputType(coordinateInputType));
    return (coordinateInputType);
}

//Given a coordinate and input type, output a string
//So DD.ddddddddd can become 'DD MM SS.ssssss', etc
function convertInput(coordinate, coordinateInputType) {
    var coordinateString = "";

    if (coordinateInputType == CoordinateTypes.COORDINATE_INPUT_TYPE_DD) {
        coordinate = Number(coordinate).toFixed(9);
        return (coordinate);
    }
    else if (coordinateInputType == CoordinateTypes.COORDINATE_INPUT_TYPE_DD_MM
        || coordinateInputType == CoordinateTypes.COORDINATE_INPUT_TYPE_DDMM
        || coordinateInputType == CoordinateTypes.COORDINATE_INPUT_TYPE_DD_MM_DASH
        || coordinateInputType == CoordinateTypes.COORDINATE_INPUT_TYPE_DD_MM_SYMBOL
    ) {
        var longitudeDegrees = Math.trunc(coordinate);
        coordinate -= longitudeDegrees;
        coordinate *= 60;
        if (coordinate < 1)
            coordinate *= -1;

        coordinate = coordinate.toFixed(7);

        if (coordinateInputType == CoordinateTypes.COORDINATE_INPUT_TYPE_DDMM)
            coordinateString = longitudeDegrees + "" + coordinate;
        else if (coordinateInputType == CoordinateTypes.COORDINATE_INPUT_TYPE_DD_MM_DASH)
            coordinateString = longitudeDegrees + "-" + coordinate;
        else if (coordinateInputType == CoordinateTypes.COORDINATE_INPUT_TYPE_DD_MM_SYMBOL)
            coordinateString = longitudeDegrees + "°" + coordinate + "'";
        else
            coordinateString = longitudeDegrees + " " + coordinate;
    }
    else if (coordinateInputType == CoordinateTypes.COORDINATE_INPUT_TYPE_DD_MM_SS
        || coordinateInputType == CoordinateTypes.COORDINATE_INPUT_TYPE_DDMMSS
        || coordinateInputType == CoordinateTypes.COORDINATE_INPUT_TYPE_DD_MM_SS_DASH
        || coordinateInputType == CoordinateTypes.COORDINATE_INPUT_TYPE_DD_MM_SS_SYMBOL
        || coordinateInputType == CoordinateTypes.COORDINATE_INPUT_TYPE_DDMMSS_NO_DECIMAL
        || coordinateInputType == CoordinateTypes.COORDINATE_INPUT_TYPE_DD_MM_SS_NO_DECIMAL
        || coordinateInputType == CoordinateTypes.COORDINATE_INPUT_TYPE_DD_MM_SS_DASH_NO_DECIMAL
    ) {
        var longitudeDegrees = Math.trunc(coordinate);
        coordinate -= longitudeDegrees;
        coordinate *= 60;
        if (coordinate < 1)
            coordinate *= -1;

        var longitudeMinutes = Math.trunc(coordinate);
        coordinate -= longitudeMinutes;
        coordinate *= 60;

        coordinate = coordinate.toFixed(6);

        if (coordinateInputType == CoordinateTypes.COORDINATE_INPUT_TYPE_DDMMSS)
            coordinateString = longitudeDegrees + "" + longitudeMinutes + "" + coordinate;
        else if (coordinateInputType == CoordinateTypes.COORDINATE_INPUT_TYPE_DD_MM_SS_DASH)
            coordinateString = longitudeDegrees + "-" + longitudeMinutes + "-" + coordinate;
        else if (coordinateInputType == CoordinateTypes.COORDINATE_INPUT_TYPE_DD_MM_SS_SYMBOL)
            coordinateString = longitudeDegrees + "°" + longitudeMinutes + "'" + coordinate + "\"";
        else if (coordinateInputType == CoordinateTypes.COORDINATE_INPUT_TYPE_DD_MM_SS)
            coordinateString = longitudeDegrees + " " + longitudeMinutes + " " + coordinate;
        else if (coordinateInputType == CoordinateTypes.COORDINATE_INPUT_TYPE_DDMMSS_NO_DECIMAL)
            coordinateString = longitudeDegrees + "" + longitudeMinutes + "" + Math.round(coordinate);
        else if (coordinateInputType == CoordinateTypes.COORDINATE_INPUT_TYPE_DD_MM_SS_NO_DECIMAL)
            coordinateString = longitudeDegrees + " " + longitudeMinutes + " " + Math.round(coordinate);
        else if (coordinateInputType == CoordinateTypes.COORDINATE_INPUT_TYPE_DD_MM_SS_DASH_NO_DECIMAL)
            coordinateString = longitudeDegrees + "-" + longitudeMinutes + "-" + Math.round(coordinate);
    }

    return (coordinateString);
}

function isdigit(c) { return /\d/.test(c); }

function setCharAt(str, index, chr) {
    if (index > str.length - 1) return str;
    return str.substring(0, index) + chr + str.substring(index + 1);
}

//Given an input type, return a printable string
function printableInputType(coordinateInputType) {
    switch (coordinateInputType) {
        default:
            return ("Unknown");
            break;
        case (CoordinateTypes.COORDINATE_INPUT_TYPE_DD):
            return ("DD.ddddddddd");
            break;
        case (CoordinateTypes.COORDINATE_INPUT_TYPE_DDMM):
            return ("DDMM.mmmmmmm");
            break;
        case (CoordinateTypes.COORDINATE_INPUT_TYPE_DD_MM):
            return ("DD MM.mmmmmmm");
            break;
        case (CoordinateTypes.COORDINATE_INPUT_TYPE_DD_MM_DASH):
            return ("DD-MM.mmmmmmm");
            break;
        case (CoordinateTypes.COORDINATE_INPUT_TYPE_DDMMSS):
            return ("DDMMSS.ssssss");
            break;
        case (CoordinateTypes.COORDINATE_INPUT_TYPE_DD_MM_SS):
            return ("DD MM SS.ssssss");
            break;
        case (CoordinateTypes.COORDINATE_INPUT_TYPE_DD_MM_SS_DASH):
            return ("DD-MM-SS.ssssss");
            break;
        case (CoordinateTypes.COORDINATE_INPUT_TYPE_DDMMSS_NO_DECIMAL):
            return ("DDMMSS");
            break;
        case (CoordinateTypes.COORDINATE_INPUT_TYPE_DD_MM_SS_NO_DECIMAL):
            return ("DD MM SS");
            break;
        case (CoordinateTypes.COORDINATE_INPUT_TYPE_DD_MM_SS_DASH_NO_DECIMAL):
            return ("DD-MM-SS");
            break;
    }
    return ("Unknown");
}
