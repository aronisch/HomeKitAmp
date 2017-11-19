var path = require('path');
var storage = require('node-persist');
var uuid = require('./HAP-NodeJS').uuid;
var Accessory = require('./HAP-NodeJS').Accessory;
var accessoryLoader = require('./HAP-NodeJS/lib/AccessoryLoader');

console.log("HAP-NodeJS starting...");

// Initialize our storage system
storage.initSync();

// Our Accessories will each have their own HAP server; we will assign ports sequentially
var targetPort = 51826;

// Load up all accessories in the /accessories folder
var dir = path.join(__dirname, "HomeKitAmpAccessory");
var accessories = accessoryLoader.loadDirectory(dir);

// Publish them all separately (as opposed to BridgedCore which publishes them behind a single Bridge accessory)
accessories.forEach(function (accessory) {

    // To push Accessories separately, we'll need a few extra properties
    if (!accessory.username)
        throw new Error("Username not found on accessory '" + accessory.displayName +
            "'. Core.js requires all accessories to define a unique 'username' property.");

    if (!accessory.pincode)
        throw new Error("Pincode not found on accessory '" + accessory.displayName +
            "'. Core.js requires all accessories to define a 'pincode' property.");

    // publish this Accessory on the local network
    accessory.publish({
        port: targetPort++,
        username: accessory.username,
        pincode: accessory.pincode
    });
});