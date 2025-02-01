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

function sendToServer(msg){
    console.log(msg);
    ws.send(msg);
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

function sendCommand(evt){
    let cmd = evt.target.getAttribute("data-cmd");
    console.log(cmd);

    if(cmd) {
        cmd = cmd.replace("%value%", evt.target.value);
        sendToServer(cmd)
    }
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



document.getElementById("btnCalibration").addEventListener("click", calibration);
document.getElementById("btnLedsOff").addEventListener("click", setLedsOff);
document.getElementById("btnStrobe").addEventListener("click", toggleStrobe);


document.getElementById("led0").addEventListener("change", sendCommand);
document.getElementById("led1").addEventListener("change", sendCommand);
document.getElementById("led2").addEventListener("change", sendCommand);
document.getElementById("led3").addEventListener("change", sendCommand);

document.getElementById("height").addEventListener("change", sendCommand);
document.getElementById("roll").addEventListener("change", sendCommand);

document.getElementById("btnShutdown").addEventListener("click", sendCommand);
document.getElementById("actions").addEventListener("click", sendCommand);
document.getElementById("emotions").addEventListener("click", sendCommand);

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