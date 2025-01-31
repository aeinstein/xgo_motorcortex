let currentAction = "";
let calibration_mode = false;
let strobe_pid = -1;

function startMovement(evt){
    console.log(evt);
    const action = evt.target.id;

    if(currentAction !== "") return;

    let speed = 128;
    let step = Number(document.getElementById("move_speed").value);

    switch(action){
        case "forw":
            currentAction = "X";
            sendToServer("X:" + (speed + step));
            break;

        case "backw":
            currentAction = "X";
            sendToServer("X:" + (speed - step));
            break;

        case "left":
            currentAction = "Z";
            sendToServer("Z:" + (speed + step));
            break;

        case "right":
            currentAction = "Z";
            sendToServer("Z:" + (speed - step));
            break;
    }

}

function stopMovement(evt){
    console.log(evt);

    sendToServer(currentAction + ":128");
    currentAction = "";
}

function setLed(evt){
    const led = evt.target.id;

    sendToServer(led + ":" + evt.target.value);
}

function sendToServer(msg){
    console.log(msg);
    ws.send(msg);
}

function setHeight(evt){
    let value = Number(evt.target.value);
    value += 128;
    sendToServer("H:" + value);
}

function setRoll(evt){
    let value = Number(evt.target.value);
    value += 128;
    sendToServer("R:" + value);
}

function shutdown(evt){
    sendToServer("S:0");
}

function calibration(evt){
    if(calibration_mode) {
        evt.target.style.backgroundColor = "#00FF00";
        sendToServer("C:0");
    } else {
        evt.target.style.backgroundColor = "initial";
        sendToServer("C:1");
    }
}

function setAction(evt){
    let button = evt.target.id.substring(9);
    sendToServer("A:" + button);
}

function setLedsOff(){
    sendToServer("led0:#000000");
    sendToServer("led1:#000000");
    sendToServer("led2:#000000");
    sendToServer("led3:#000000");

    document.getElementById("led0").value = "#000000";
    document.getElementById("led1").value = "#000000";
    document.getElementById("led2").value = "#000000";
    document.getElementById("led3").value = "#000000";
}

function startStrobe(){
    strobe_pid = setInterval(rotateStrobe, 100);
}

function toggleStrobe(){
    if(strobe_pid === -1){
        startStrobe();
    } else {
        stopStrobe();
    }
}

function stopStrobe(){
    clearInterval(strobe_pid);
    strobe_pid = -1;
    setLedsOff();
}


let currentLed= 0;
function rotateStrobe(){
    sendToServer("led" + currentLed + ":#000000");
    document.getElementById("led" + currentLed).value = "#000000";
    currentLed++;
    if(currentLed === 4) currentLed = 0;
    sendToServer("led" + currentLed + ":#0000FF");
    document.getElementById("led" + currentLed).value = "#0000FF";
}

// EventHandler
document.getElementById("movement_tab").addEventListener("mousedown", startMovement);
document.getElementById("movement_tab").addEventListener("mouseup", stopMovement);
document.getElementById("height").addEventListener("change", setHeight);
document.getElementById("roll").addEventListener("change", setRoll);
document.getElementById("led0").addEventListener("change", setLed);
document.getElementById("led1").addEventListener("change", setLed);
document.getElementById("led2").addEventListener("change", setLed);
document.getElementById("led3").addEventListener("change", setLed);
document.getElementById("actions").addEventListener("click", setAction);
document.getElementById("btnCalibration").addEventListener("click", calibration);
document.getElementById("btnShutdown").addEventListener("click", shutdown);
document.getElementById("btnLedsOff").addEventListener("click", setLedsOff);
document.getElementById("btnStrobe").addEventListener("click", toggleStrobe);

function request(path){
    const xhttp = new XMLHttpRequest();
    xhttp.onreadystatechange = function() {
        if (this.readyState == 4 && this.status == 200) {
            switch(path){
                case "battery":
                    document.getElementById("battery").value =this.responseText;
                    break;

                case "state":
                    switch(this.responseText) {
                    case "0":
                        document.getElementById("state").value = "Fallen";
                        break;

                    case "1":
                        document.getElementById("state").value = "OK";
                        break;

                    default:
                        document.getElementById("state").value = this.responseText;
                        break;
                    }
                    break;

                case "yaw":
                    document.getElementById("yaw").value =this.responseText;
                    break;
            }
        }
    };
    xhttp.open("GET", path, true);
    xhttp.send();
}

function getProcData(){
    request("battery");
    request("yaw");
    request("state");
}

dataloader = window.setInterval(getProcData, 1000);


const ws = new WebSocket('wss://riderpi:1080');

ws.onmessage = (event) => {
    console.log(`Message from server: ${event.data}`);
};

ws.onclose = (evt) => {
    console.log('Connection closed', evt);
};

ws.onerror= (evt) => {
    console.log('error', evt);
};

ws.onopen = () => {
    console.log('Connected to server');
    ws.send('Hello, Server!');
};

const videoElement = document.getElementById('mainvideo');

const rtmp_player = webrtmpjs.createWebRTMP();

rtmp_player.attachMediaElement(videoElement);

rtmp_player.open("riderpi", 9001).then(()=>{   // Host, Port of WebRTMP Proxy
    rtmp_player.connect("demo").then(()=>{                  // Application name
        rtmp_player.play("live").then(()=>{      // Stream name
            console.log("playing");
        })
    })
})