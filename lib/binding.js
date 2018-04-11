let BLEAdapter = {
    list: function() {
        return [];
    }
}

if(process.platform === "win32") {
    BLEAdapter = require('bindings')('ble-adapter-native.node');
} 

module.exports = BLEAdapter;