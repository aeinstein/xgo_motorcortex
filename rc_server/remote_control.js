const fs = require('fs');
const express = require('express');
const http = require('http');
const {WebSocketServer} = require("ws");
const WebSocket = require('ws');
const port = 1080;
const device_path = "/proc/XGORider";

const exec = require('child_process').exec;
/*
const keys = {
    //ca: fs.readFileSync('rootCA.crt'), included in fullchain
    cert: fs.readFileSync('fullchain.crt'),
    key: fs.readFileSync('privkey.key')
};*/

const app = express();
const server = http.createServer(app);


app.use("/", express.static('public'));
xgo_passthrough("battery");
xgo_passthrough("state");
xgo_passthrough("yaw");
server.listen(port, () => {
    console.log(`Server is listening on https://riderpi`);
});

let sendToClient = ()=>{};

function xgo_passthrough(path){
    console.log("register passthrough: " + path);

    app.get("/" + path, function (req, res) {
        fs.readFile(device_path + "/" + path, 'utf8', (error, data) => {
            if (error) {
                console.error('An error occurred while reading the file:', error);
                return;
            }
            console.log('File content:', data);
            res.send(data);
        });
    });
}

function writeData(path, data) {
    fs.appendFile(path, data, function (err) {
        if (err) throw err;
        console.log('written!');
    });
}

function refreshBattery(){
    fs.readFile(device_path + "/battery", 'utf8', (error, data) => {
        if (error) {
            console.error('An error occurred while reading the file:', error);
            return;
        }
        //console.log('File content:', data);
        sendToClient("Battery:" + data);
    });
}

setTimeout(refreshBattery, 10000);

const interval = setInterval(() => {
    wss.clients.forEach((client) => {
        if (client.isAlive === false) return client.terminate();

        client.isAlive = false;
        client.ping();
    });
}, 30000);

const wss = new WebSocket.Server({ server });

wss.on('connection', (ws) => {
    console.log('New client connected');

    ws.isAlive = true;

    // Sending a message to the client
    //ws.send('Welcome to the WebSocket server!');

    // Listening for messages from the client
    ws.on('message', (buffer) => {
        console.log(`Received message: ${buffer}`);

        const message = buffer.toString()
        console.log(message)

        const ar = message.split(':');
        switch (ar[0]) {
            case 'A':   // forward/backward
                writeData(device_path + "/action", ar[1])
                break;

            case 'X':   // forward/backward
                writeData(device_path + "/speed_x", ar[1])
                break;

            case "R":
                writeData(device_path + "/roll", ar[1])
                break;

            case "H":
                writeData(device_path + "/height", ar[1])
                break;

            case 'Z':
                writeData(device_path + "/speed_z", ar[1])
                break;

            case "S":
                writeData(device_path + "/shutdown", ar[1])
                break;

            case 'C':
                writeData(device_path + "/settings/calibration", ar[1])
                break;

            case 'led0':
            case 'led1':
            case 'led2':
            case 'led3':
                writeData(device_path + "/leds/" + ar[0].substring(3), ar[1])
                break;

            case "E":   // Emotion
                playEmotion(ar[1]);
                break;
        }

        // Echoing the message back to the client
        ws.send(`OK:${message}`);
    });

    ws.on('pong', () => {
        ws.isAlive = true;
    });

    // Handling client disconnection
    ws.on('close', () => {
        console.log('Client disconnected');
        // stop on connection lost
        writeData(device_path + "/speed_z", 128);
        writeData(device_path + "/speed_x", 128);
    });

    ws.on('error', (error) => {
        console.error(`WebSocket error: ${error.message}`);
    });

    sendToClient = (msg) => {
        console.log(`Sending message: ${msg}`);
        ws.send(msg);
    }
});

function playEmotion(emotion) {
    console.log(`Playing Emotion: ${emotion}`);

    const child = exec("./scripts/showEmotion emotions/" + emotion,
        (error, stdout, stderr) => {
            console.log('stdout: ' + stdout);
            console.log('stderr: ' + stderr);

            if (error !== null) {
                console.log('exec error: ' + error);
            }
        });
}
