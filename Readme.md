# BLE Adapter

Get a list of installed Bluetooth adapters and if they support Bluetooth 4.0 (BLE).   
*Note: windows only, returns an empty list on other platforms*

## Doc

```typescript
interface  Adapter { name: string, manufacturer: string, bleCapable: boolean }
// returns a list of available bluetooth adapters
BleAdapter.list() : Array< Adapter >   
```

## Example

```javascript
const BLEAdapter = require('ble-adapter');
const bluethoothAdapter = BleAdapter.list();
```