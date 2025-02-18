// var gateway = `ws://${window.location.hostname}/ws`;
var gateway = `ws://192.168.178.81/ws`;
var websocket;
var start;
var myChart;
var cfg = {
    profiles: [{}],
    idleSafetyTemperature: 0.0,
    version: "1.0.0",
    buildTime: "unkown",
};

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
    getConfig();
    start = new Date();
}

function onClose(event)
{
    console.log('Connection closed');
    setTimeout(initWebSocket, 2000);
}

function enrichData(data)
{
    if (!data || !data.length) return [];

    const firstTimestamp = data[data.length - 1].timestamp;
    const startTime = start;
    const currentTime = new Date();

    return data.map(entry => {
        const timeDifference = firstTimestamp - entry.timestamp;
        const entryTime = new Date(currentTime.getTime() - timeDifference);

        return {
            time: entryTime.toLocaleTimeString(),
            temp: entry.temp,
            setpoint: entry.setpoint,
            timestamp: entry.timestamp,
            pwr: entry.setpoint <= 15 ? entry.pwr : 0,
        };
    });
}

function onMessage(event)
{
    let data = JSON.parse(event.data);
    // console.log(event);
    // console.log(data.temp);

    if(data.cfg) {
        // process static config
        let dcfg = data.cfg;
        cfg.version = dcfg.version;
        cfg.buildTime = dcfg.buildTime;
        document.getElementById('version').innerHTML = cfg.version;
        document.getElementById('buildTime').innerHTML = cfg.buildTime;
    }

    if(data.meta) {
        // process Metadata
        let metaData = data.meta;
        document.getElementById('inP').placeholder = metaData.kp;
        document.getElementById('inI').placeholder = metaData.ki;
        document.getElementById('inD').placeholder = metaData.kd;
    }

    if(data.temp) {
        // process Temperature data
        let tempData = enrichData(leftPadArray(data.temp, {
            temp: null,
            setpoint: null,
            timestamp: 0,
        }, 150));

        // console.log(tempData);

        const mostRecentData = tempData[tempData.length - 1];
        document.getElementById('currentTemp').innerHTML = mostRecentData.temp;
        document.getElementById('setpointTemp').innerHTML = mostRecentData.setpoint;
        document.getElementById('pwr').innerHTML = mostRecentData.pwr;

        myChart.setOption({
            xAxis: {
                // data: Array.from({ length: tempData.length }, (_, i) => i)
                // data: tempData.map(item => item.timestamp)
                data: tempData.map(item => item.time)
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
                    data: tempData.map(item => item.temp),
                    type: 'line',
                    smooth: true
                }
            ]
        });
    }
    // document.getElementById('battery_voltage').innerHTML = battery_voltage;
    // document.getElementById('battery_level').innerHTML = battery_percent;
    // document.getElementById('airflow').innerHTML = airflow;
    // document.getElementById('timer').innerHTML = getTimerTime(data.startTimer, data.endTimer);
}

/**
 * Left pads an array with a specified pad object until it reaches 150 elements.
 * If the array is longer than 150 elements, it returns the last 150 elements.
 *
 * @param {Array} arr - The array to be padded.
 * @param {Object} padObj - The object to use for padding.
 * @param {number} [targetLength=150] - The desired array length.
 * @returns {Array} - The padded (or trimmed) array.
 */
function leftPadArray(arr, padObj, targetLength = 150)
{
    if (!Array.isArray(arr))
        throw new TypeError("The first argument must be an array.");

    if (arr.length >= targetLength)
        return arr.slice(-targetLength);

    const padCount = targetLength - arr.length;
    const padArray = Array.from({ length: padCount }, () => ({ ...padObj }));
    return padArray.concat(arr);
}

document.addEventListener("DOMContentLoaded", function()
{
    var data = [];
    var chartDom = document.getElementById('mainChart');

    myChart = echarts.init(chartDom, 'dark', { // 'dark'
        renderer: 'canvas',
        useDirtyRect: false
    });

    data = enrichData(leftPadArray(data, {
        temp: 15,
        setpoint: 15,
        timestamp: 0,
    }, 150));

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
            left: 20,
            top: 25,
            right: 20,
            bottom: 25,
            // left: '3%',
            // right: '4%',
            // bottom: '3%',
            containLabel: true
        },
        xAxis: {
            type: 'category',
            data: [...Array(data.length).keys()]
        },
        yAxis: {
            type: 'value',
            max: 250,
            axisLabel: {
                formatter: '{value} °C'
            },
            axisLine: {
                show: true,
            },
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

    document.getElementById('chartSkeleton').classList.add('loaded');
    document.getElementById('mainChart').style.display = 'block';
    window.addEventListener('resize', myChart.resize);
});

function onLoad(event)
{
    initWebSocket();
}

function sendMessage(msg)
{
    websocket.send(JSON.stringify(msg));
}

function getConfig()
{
    sendMessage({
        cmd: "getCfg"
    });
}
