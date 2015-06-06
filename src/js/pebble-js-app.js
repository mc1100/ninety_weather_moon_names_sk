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
    return (timestamp) ? new Date(timestamp * 1000).toTimeString().substr(0, 5) : "--:--";
}

function fetchWeather(latitude, longitude) {
    var req = new XMLHttpRequest();
    req.open("GET", "http://api.openweathermap.org/data/2.5/weather?lat=" + latitude + "&lon=" + longitude + "&units=metric&APPID=7ea2c73b4157ffa54303ee1565edc8be", true);
    req.onload = function(e) {
        if (req.readyState == 4) {
            if (req.status == 200) {
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
                onError(e.error.message);
            }
        }
    };
    req.send(null);
}

function locationSuccess(pos) {
    var coordinates = pos.coords;
    fetchWeather(coordinates.latitude, coordinates.longitude);
}

function locationError(message) {
    console.log("failed to obtain location. Error: " + message);
    onError(message);
}

function onError(message) {
    Pebble.sendAppMessage({
        "icon" : 0,
        "temperature" : "- \u00B0",
        "sunrise" : strTime(null),
        "sunset" : strTime(null)
    });
}

var locationOptions = {
    "timeout" : 15000,
    "maximumAge" : 60000
};

Pebble.addEventListener("ready", function(e) {
    console.log("ready");
    window.navigator.geolocation.watchPosition(locationSuccess, locationError, locationOptions);
});

Pebble.addEventListener("showConfiguration", function(e) {
    var options = JSON.parse(window.localStorage.getItem("options"));
    var uri = "https://rawgit.com/mc1100/ninety_weather_moon_names_sk/sdk_2.0/html/configuration.html";
    if (options === null) {
        console.log("options have not been set");
    } else {
        var so = JSON.stringify(options);
        console.log("read options: " + so);
        var uri += "?calendar=" + encodeURIComponent(options["calendar"]);
    }

    Pebble.openURL(uri);
});

Pebble.addEventListener("webviewclosed", function(e) {
    console.log("configuration closed");
    if (e.response) {
        var options = JSON.parse(decodeURIComponent(e.response));
        var stroptions = JSON.stringify(options);
        console.log("storing options: " + stroptions);
        window.localStorage.setItem("options", stroptions);
        Pebble.sendAppMessage(options,
            function(e) {
                console.log("successfully sent options to pebble");
            },
            function(e) {
                console.log("failed to send options to pebble. Error: " + e.error.message);
            }
        );
    } else {
        console.log("no options received");
    }
});
