const BleAdapter = require("../lib/binding.js");
const assert = require("assert");

assert(BleAdapter, "The expected module is undefined");

let instances = BleAdapter.list();

if(process.platform !== "win32") {
    assert(instances.length === 0, "Module should ignore non windows platforms");
}

console.log(instances);

console.log("Tests passed - everything looks OK!");