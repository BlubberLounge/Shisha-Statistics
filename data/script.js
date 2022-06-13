var gateway = `ws://${window.location.hostname}/ws`;
var websocket;
var time, nextBatteryReadTime;

window.addEventListener('load', onLoad);

function initWebSocket()
{
    console.log('Trying to open a WebSocket connection...');
    websocket = new WebSocket(gateway);
    websocket.onopen    = onOpen;
    websocket.onclose   = onClose;
    websocket.onmessage = onMessage;
}

function onOpen(event)
{
    console.log('Connection opened');
}

function onClose(event)
{
    console.log('Connection closed');
    setTimeout(initWebSocket, 2000);
}

function onMessage(event)
{
    let data = JSON.parse(event.data);

    // console.log(event);
    console.log(data);

    document.getElementById('battery_voltage').innerHTML = data.battery_voltage > 0 ?  Math.round((data.battery_voltage + Number.EPSILON) * 100) / 100 + " v" : "no data.";
    document.getElementById('battery_level').innerHTML = data.battery_level > 0 ? data.battery_level + " %" : "no data.";
    document.getElementById('airflow').innerHTML = data.airflow == 1  ? "High" : "none";

    time = data.time;
    nextBatteryReadTime = data.next_battery_read;
}


function onLoad(event)
{
    initWebSocket();
    initButton();
    updateCountdown();
}

function initButton()
{
    document.getElementById('update').addEventListener('click', update);
}

function update()
{
    websocket.send('update');
}

function updateCountdown()
{
    var x = setInterval(function() {
        
    var distance = nextBatteryReadTime - time;
        
    var minutes = Math.floor((distance % (1000 * 60 * 60)) / (1000 * 60));
    var seconds = Math.floor((distance % (1000 * 60)) / 1000);
        
    document.getElementById("countdown").innerHTML = "  (" + minutes + "m " + seconds + "s)";
        
    // If the count down is over, write some text 
    if (distance < 0) {
        clearInterval(x);
        document.getElementById("countdown").innerHTML = "EXPIRED";
    }
    }, 1000);
}