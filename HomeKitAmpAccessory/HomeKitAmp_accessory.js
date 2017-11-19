const worker = require("../NodeJSAddon/node_modules/streaming-worker");
const path = require("path");

var addon_path = path.join(__dirname, "../NodeJSAddon/build/Release/HomeKitAmp");

const hkamp = worker(addon_path);

var Accessory = require('../node_modules/hap-nodejs').Accessory;
var Service = require('../node_modules/hap-nodejs').Service;
var Characteristic = require('../node_modules/hap-nodejs').Characteristic;
var uuid = require('../node_modules/hap-nodejs').uuid;
var err = null; // in case there were any problems

hkamp.from.on('status', function (value) {
    var isOn = value == "I" ? true : false;
    AMPLIFIER.powerOn = isOn;
});

hkamp.from.on('delay', function (value) {
    console.log("Safety Delay :");
    console.log(value);
});

// here's a fake hardware device that we'll expose to HomeKit
var AMPLIFIER = {
    setPowerOn: function (on) {
        console.log("Turning the amplifier %s!...", on ? "on" : "off");
        if (on) {
            hkamp.to.emit("Instructions", 'I');
            AMPLIFIER.powerOn = true;
            if (err) { return console.log(err); }
            console.log("...amplifier is now on.");
        } else {
            hkamp.to.emit("Instructions", 'O');
            AMPLIFIER.powerOn = false;
            if (err) { return console.log(err); }
            console.log("...amplifier is now off.");
        }
    },
    identify: function () {
        console.log("Identify the ampifier.");
    }
}

// Generate a consistent UUID for our outlet Accessory that will remain the same even when
// restarting our server. We use the `uuid.generate` helper function to create a deterministic
// UUID based on an arbitrary "namespace" and the accessory name.
var outletUUID = uuid.generate('hap-nodejs:accessories:Outlet');

// This is the Accessory that we'll return to HAP-NodeJS that represents our fake light.
var outlet = exports.accessory = new Accessory('Outlet', outletUUID);

// Add properties for publishing (in case we're using Core.js and not BridgedCore.js)
outlet.username = "1A:2B:3C:4D:5D:FF";
outlet.pincode = "031-45-154";

// set some basic properties (these values are arbitrary and setting them is optional)
outlet
    .getService(Service.AccessoryInformation)
    .setCharacteristic(Characteristic.Manufacturer, "AR")
    .setCharacteristic(Characteristic.Model, "V1.0")
    .setCharacteristic(Characteristic.SerialNumber, "A1S2NASF88EW");

// listen for the "identify" event for this Accessory
outlet.on('identify', function (paired, callback) {
    AMPLIFIER.identify();
    callback(); // success
});

// Add the actual outlet Service and listen for change events from iOS.
// We can see the complete list of Services and Characteristics in `lib/gen/HomeKitTypes.js`
outlet
    .addService(Service.Outlet, "Amplifier") // services exposed to the user should have "names" like "Fake Light" for us
    .getCharacteristic(Characteristic.On)
    .on('set', function (value, callback) {
        AMPLIFIER.setPowerOn(value);
        callback(); // Our fake Outlet is synchronous - this value has been successfully set
    });

// We want to intercept requests for our current power state so we can query the hardware itself instead of
// allowing HAP-NodeJS to return the cached Characteristic.value.
outlet
    .getService(Service.Outlet)
    .getCharacteristic(Characteristic.On)
    .on('get', function (callback) {

        // this event is emitted when you ask Siri directly whether your light is on or not. you might query
        // the light hardware itself to find this out, then call the callback. But if you take longer than a
        // few seconds to respond, Siri will give up.

        var err = null; // in case there were any problems

        if (AMPLIFIER.powerOn) {
            console.log("Are we on? Yes.");
            callback(err, true);
        }
        else {
            console.log("Are we on? No.");
            callback(err, false);
        }
    }); 