// var gateway = `ws://${window.location.hostname}/ws`;
var gateway = `ws://192.168.178.81/ws`;
var websocket;
var time;
var lastTime = 0;
var myChart;

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
    update();
}

function onClose(event)
{
    console.log('Connection closed');
    setTimeout(initWebSocket, 2000);
}

function onMessage(event)
{
    let data = JSON.parse(event.data);
    let metaData = data.meta;
    let tempData = data.temp;

    // console.log(event);
    // console.log(data.temp);

    myChart.setOption({
        grid: {
            left: "5%",
            top: 30,
            right: "5%",
            bottom: 30
        },
        xAxis: {
            // data: Array.from({ length: tempData.length }, (_, i) => i)
            data: tempData.map(item => item.timestamp)
        },
        series: [
            {
                name: 'dataSetpoint',
                data: tempData.map(item => item.setpoint),
                type: 'line',
                smooth: true,
                areaStyle: {}
            },
            {
                name: 'dataInput',
                data: tempData.map(item => item.input),
                type: 'line',
                smooth: true
            }
        ]
    });

    // document.getElementById('battery_voltage').innerHTML = battery_voltage;
    // document.getElementById('battery_level').innerHTML = battery_percent;
    // document.getElementById('airflow').innerHTML = airflow;
    // document.getElementById('timer').innerHTML = getTimerTime(data.startTimer, data.endTimer);
}


document.addEventListener("DOMContentLoaded", function()
{
    // window.setInterval(update, 1000);

    var data = [];
    var chartDom = document.getElementById('main');
    myChart = echarts.init(chartDom, 'dark', { // 'dark'
        renderer: 'canvas',
        useDirtyRect: false
    });
    var option = {
        tooltip: {
            trigger: 'axis',
            axisPointer: {
                type: 'cross',
                label: {
                    backgroundColor: '#6a7985'
                }
            }
        },
        grid: {
            left: "5%",
            top: 30,
            right: "5%",
            bottom: 30
            // left: '3%',
            // right: '4%',
            // bottom: '3%',
            // containLabel: true
        },
        xAxis: {
            type: 'category',
            data: [...Array(data.length).keys()]
        },
        yAxis: {
            type: 'value',
            max: 250
        },
        series: [  
            {
                name: 'dataSetpoint',
                data: data,
                type: 'line',
                smooth: true,
                showSymbol: false,
                areaStyle: {}
            },        
            {
                name: 'dataInput',
                data: data,
                type: 'line',
                showSymbol: false,
                smooth: true
            }
        ]
    };

    if (option && typeof option === 'object') {
        myChart.setOption(option);
    }
    // option && myChart.setOption(option);

    window.addEventListener('resize', myChart.resize);
});

function onLoad(event)
{
    initWebSocket();
}

// function initButton()
// {
//     document.getElementById('update').addEventListener('click', update);
// }

function update()
{
    websocket.send('update');
}

// function getReadings(){
//     websocket.send("getReadings");
// }

// function updateCountdown()
// {
//     var x = setInterval(function() {
        
//     var distance = nextBatteryReadTime - new Date().getTime();

//     var minutes = Math.floor((distance % (1000 * 60 * 60)) / (1000 * 60));
//     var seconds = Math.floor((distance % (1000 * 60)) / 1000);
        
//     document.getElementById("countdown").innerHTML = "  (" + minutes + "m " + seconds + "s)";

//     if (distance < 0) {
//         clearInterval(x);
//         document.getElementById("countdown").innerHTML = " ( time is up)";
//     }
        
//     }, 1000);
// }

// function getTimerTime(startTime, endTime) {
//     if(startTime >= endTime || !endTime)
//         return lastTime + " s";

//     lastTime = ((endTime - startTime) / 1000);
    
//     return lastTime + " s"; 
// }