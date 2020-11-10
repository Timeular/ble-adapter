let BLEAdapter = {
    list: function() {
        return [];
    }
}

if(process.platform === "win32") {
    BLEAdapter = require('../build/Release/ble-adapter-native.node');
}

module.exports = BLEAdapter;
