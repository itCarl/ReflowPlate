var gateway = `ws://${window.location.hostname}/ws`;
var websocket;
var time;
var nextBatteryReadTime;
var lastTime = 0;

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
    let string = "no data.";

    let data = JSON.parse(event.data);

    let battery = data.battery;
    let battery_voltage = battery.voltage > 0 ?  Math.round((battery.voltage + Number.EPSILON) * 100) / 100 + " v" : string;
    let battery_percent = battery.percent > 0 ? battery.percent + " %" : string;

    let airflow = data.airflow == 1  ? "High" : "none";

    // console.log(event);
    console.log(data);

    document.getElementById('battery_voltage').innerHTML = battery_voltage;
    document.getElementById('battery_level').innerHTML = battery_percent;
    document.getElementById('airflow').innerHTML = airflow;
    document.getElementById('timer').innerHTML = getTimerTime(data.startTimer, data.endTimer);

    nextBatteryReadTime = new Date().getTime() + battery.nextRead;
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
        
    var distance = nextBatteryReadTime - new Date().getTime();

    var minutes = Math.floor((distance % (1000 * 60 * 60)) / (1000 * 60));
    var seconds = Math.floor((distance % (1000 * 60)) / 1000);
        
    document.getElementById("countdown").innerHTML = "  (" + minutes + "m " + seconds + "s)";

    if (distance < 0) {
        clearInterval(x);
        document.getElementById("countdown").innerHTML = " ( time is up)";
    }
        
    }, 1000);
}

function getTimerTime(startTime, endTime) {
    if(startTime >= endTime || !endTime)
        return lastTime + " s";

    lastTime = ((endTime - startTime) / 1000);
    
    return lastTime + " s"; 
}