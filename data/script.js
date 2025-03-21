var d = document;
// var gateway = `ws://${window.location.hostname}/ws`;
var gateway = `ws://192.168.178.81/ws`;
var websocket;
var start;
var myChart;
var cfg = {
    states: {
        // all possible element states will be saved here
    }
    // will be filled on connection
};
var s = t => t/1000;
var isEmpty = str => !str?.length;
var byId = id => d.getElementById(id);
var upt = (id, val) => { if(byId(id).innerHTML.trim() != val) byId(id).innerHTML = val };
var onClick = (id, cb) => byId(id).addEventListener('click', cb);
window.addEventListener('load', onLoad);

function clearProfiles()
{
    let profilesContainer = byId('profiles');
    profilesContainer.replaceChildren();
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
            timestamp: entry.timestamp
        };
    });
}

function onLoad(event)
{
    initWebSocket();
}

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
    clearProfiles();
    getConfig();
    start = new Date();
}

function onClose(event)
{
    console.log('Connection closed');
    setTimeout(initWebSocket, 2000);
    clearProfiles();
}

function onMessage(event)
{
    let data = JSON.parse(event.data);
    // console.log(event);
    // console.log(data.temp);

    if(data.cfg) {
        // process static config
        let cfg = data.cfg;

        byId('version').innerHTML = cfg.version;
        byId('buildTime').innerHTML = cfg.buildTime;

        let profilesContainer = byId('profiles');
        cfg.profiles.forEach(e => {
            let btn = d.createElement('button');
            btn.innerHTML = e.name;
            btn.classList.add('button-outline');
            btn.dataset.prid = e.id;
            btn.title = `max. Temperature: ${e.maxTemp}°C\ntotal time: ${s(e.totalTime)}s\n--------------------\npreheat: ${s(e.preheatTime)}s@${e.preheatTemp}°C\nsoak: ${s(e.soakTime)}s@${e.soakTemp}°C\nreflow: ${s(e.reflowTime)}s@${e.reflowTemp}°C\ncooldown: ${s(e.coolTime)}s@${e.coolTemp}°C`;
            btn.addEventListener('click', el => {
                sendMessage({
                    cmd: "selProf",
                    data: e.id
                });
            });
            profilesContainer.append(btn);
        });
    }

    if(data.metaB) {
        upt('pwr', data.metaB.pwr);
    }

    if(data.meta) {
        // process Metadata
        let metaData = data.meta;
        upt('mode', metaData.mode);
        upt('connectedClients', metaData.cc);

        byId('inP').placeholder = cfg.states.kp = metaData.kp;
        byId('inI').placeholder = cfg.states.ki = metaData.ki;
        byId('inD').placeholder = cfg.states.kd = metaData.kd;

        cfg.states.selectedProfile = metaData.sprof;

        let profiles = byId('profiles');
        profiles.querySelector(`[data-prid="${metaData.sprof}"]`).classList.remove('button-outline');
        profiles.querySelectorAll(`[data-prid]:not([data-prid="${metaData.sprof}"])`).forEach(profile => {
            if (!profile.classList.contains('button-outline')) {
                profile.classList.add('button-outline');
            }
        });

        cfg.states.controlsLocked = metaData.controlsLocked;

        const btnLock = byId('btnLock').firstChild;
        var btnLockClass = btnLock.classList;
        btnLockClass.remove('fa-spinner');
        btnLockClass.remove('fa-pulse');
        btnLockClass.remove('fa-fw');
        if(cfg.states.controlsLocked) {
            btnLockClass.remove('fa-lock-open');
            btnLockClass.remove('unlocked');
            btnLockClass.add('fa-lock', 'locked');
        } else {
            btnLockClass.remove('fa-lock');
            btnLockClass.remove('locked');
            btnLockClass.add('fa-lock-open', 'unlocked');
        }
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
        byId('currentTemp').innerHTML = mostRecentData.temp;
        byId('setpointTemp').innerHTML = mostRecentData.setpoint;

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

function sendMessage(msg)
{
    websocket.send(JSON.stringify(msg));
}

function update()
{
    sendMessage({
        cmd: "upt",
        states: cfg.states
    });
}

function getConfig()
{
    sendMessage({
        cmd: "getCfg"
    });
}

function toggleLockBtn(e)
{
    let el = e.currentTarget.firstChild;
    el.classList.remove('fa-lock');
    el.classList.remove('fa-lock-open');
    el.classList.remove('unlocked');
    el.classList.remove('locked');
    el.classList.add('fa-spinner', 'fa-pulse', 'fa-fw');
    cfg.states.controlsLocked = !cfg.states.controlsLocked;
    update();
}

function UpdatePIDBtn(e)
{
    cfg.states.kp = parseFloat(byId('inP').value || cfg.states.kp);
    cfg.states.ki = parseFloat(byId('inI').value || cfg.states.ki);
    cfg.states.kd = parseFloat(byId('inD').value || cfg.states.kd);

    byId('inP').value = "";
    byId('inI').value = "";
    byId('inD').value = "";
    update();
}


d.addEventListener("DOMContentLoaded", function()
{
    var data = [];
    var chartDom = byId('mainChart');

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

    byId('chartSkeleton').classList.add('loaded');
    byId('mainChart').style.display = 'block';
    onClick('btnLock', toggleLockBtn);
    onClick('btnRestart', e => {
        sendMessage({
            cmd: "rstCntlr"
        })
    });
    onClick('btnStart', e => {
        sendMessage({
            cmd: "start"
        })
    });
    onClick('btnStop', e => {
        sendMessage({
            cmd: "stop"
        })
    });
    onClick('btnUpdatePID', UpdatePIDBtn);

    window.addEventListener('resize', myChart.resize);
});
