let currentAction = "";

function startMovement(evt){
    console.log(evt);
    const action = evt.target.id;

    if(currentAction !== "") return;

    switch(action){
        case "forw":
            currentAction = "X";
            sendToServer("X:146");

            break;

        case "backw":
            currentAction = "X";
            sendToServer("X:110");
            break;

        case "left":
            currentAction = "Z";
            sendToServer("Z:146");
            break;

        case "right":
            currentAction = "Z";
            sendToServer("Z:110");
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
    sendToServer("H:" + evt.target.value);
}

function setRoll(evt){
    sendToServer("R:" + evt.target.value);
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
                        document.getElementById("state").value = "OK";
                        break;

                    case "1":
                        document.getElementById("state").value = "FALLEN";
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
