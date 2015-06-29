var API_ADDRESS = 'http://steenman.me:3000/';
var myToken = undefined;

var apiRequest = function (action, type, callback) {
    var url = API_ADDRESS + action;
    var xhr = new XMLHttpRequest();
    xhr.onload = function () {
        callback(this.responseText);
    };
    xhr.open(type, url);
    xhr.send();
};

getCurrentInstalls = function() {
    var url = API_ADDRESS + 'current';
    apiRequest('current', 'GET', function(count) {
        console.log('Got installs: ' + count);
        var data = {
          'KEY_CURRENT_COUNT': +count
        }
        Pebble.sendAppMessage(data, function(e) {
            console.log('Sent succesfully!');
        }, function(e) {
          console.log('Error sending data');
        })
    });
}

var subscribePins = function() {
    if (!myToken) {
        console.log('Token not yet known');
        Pebble.getTimelineToken(function (token) {
            console.log('Got token: ' + token);
            myToken = token; 
            subscribePins();
        }, function (error) {
            console.log('Error getting timeline token: ' + error);
        });
    } else {
        console.log('Token known, requesting');
        apiRequest('pin_subscribe/' + myToken, 'GET', function(result) {
            console.log('Result ' + result);
        });
    }
}

// Listen for when the watchface is opened
Pebble.addEventListener('ready', 
    function(e) {
        console.log('PebbleKit JS ready!');
        subscribePins();
        getCurrentInstalls();
    }
);

// Listen for when an AppMessage is received
Pebble.addEventListener('appmessage',
    function(e) {
        console.log('AppMessage received!');
        getCurrentInstalls();
        subscribePins();
    }                     
);