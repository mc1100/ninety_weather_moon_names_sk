var iconMap = {
    "01d" : 1,
    "01n" : 2,
    "02d" : 3,
    "02n" : 4,
    "03d" : 5,
    "03n" : 6,
    "04d" : 7,
    "04n" : 8,
    "09d" : 9,
    "09n" : 10,
    "10d" : 11,
    "10n" : 12,
    "11d" : 13,
    "11n" : 14,
    "13d" : 15,
    "13n" : 16,
    "50d" : 17,
    "50n" : 18
};

function iconFromWeatherIcon(weatherIcon) {
    var icon = iconMap[weatherIcon];
    return (icon) ? icon : 0;
}

function strTime(timestamp) {
    return new Date(timestamp * 1000).toTimeString().substr(0, 5);
}

function fetchWeather(latitude, longitude) {
    var req = new XMLHttpRequest();
    req.open("GET", "http://api.openweathermap.org/data/2.5/weather?lat=" + latitude + "&lon=" + longitude + "&units=metric", true);
    req.onload = function(e) {
        if (req.readyState == 4) {
            if (req.status == 200) {
                // console.log(req.responseText);
                var response = JSON.parse(req.responseText);
                if (response) {
                    Pebble.sendAppMessage({
                        "icon" : iconFromWeatherIcon(response.weather[0].icon),
                        "temperature" : Math.round(response.main.temp) + " \u00B0",
                        "sunrise" : strTime(response.sys.sunrise),
                        "sunset" : strTime(response.sys.sunset)
                    });
                }
            } else {
                // console.warn("XMLHttpRequest error");
            }
        }
    };
    req.send(null);
}

function locationSuccess(pos) {
    var coordinates = pos.coords;
    fetchWeather(coordinates.latitude, coordinates.longitude);
}

function locationError(err) {
    // console.warn("location error (" + err.code + "): " + err.message);
    Pebble.sendAppMessage({});
}

var locationOptions = {
    "timeout" : 15000,
    "maximumAge" : 60000
};

Pebble.addEventListener("ready", function(e) {
    // console.log("connect!" + e.ready);
    locationWatcher = window.navigator.geolocation.watchPosition(locationSuccess, locationError, locationOptions);
});

Pebble.addEventListener("appmessage", function(e) {
    window.navigator.geolocation.getCurrentPosition(locationSuccess, locationError, locationOptions);
    // console.log("message!");
});
