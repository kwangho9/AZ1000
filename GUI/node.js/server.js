//const path = require('path');
//const fs = require('fs');

const path = require('path');
const fs = require('fs');
const express = require('express')
const app = express()
const open = require('open');
const serialPort = require('serialport');
const Readline = require('@serialport/parser-readline');

const multer = require('multer');
const storage = multer.memoryStorage();
const upload = multer({storage: storage});

//const port = 9988;
var myPort = null;
var result = "";
var comport = "";
var deviceIdx = 3;
const deviceName = ["PS1030", "PS1060", "PS1120", "NoDevice"];


app.set('port', process.env.PORT || 9988);

/* 모든 request(요청)을 받아서 처리하지는 않고, next()로 다음 미들웨어에 넘긴다. */
app.use((req, res, next) => {
    res.cookie('device', deviceIdx.toString(), {
        httpOnly: false,
        signed: false,
        maxAge: 10000
    });
    next();
});

/* root directory('/')를 server의 'static' directory로 연결한다.
 * main 페이지('/')가 "index.html"이면, 자동으로 파일을 찾아서 전송한다.
 * client의 요청(request) URL에 기술된 파일을 검색해서 전송한다.
 * 요청(..../img/a.png)라고 하면, 'static' 폴더내에 'img'폴더와 'a.png' 파일이 있어야 한다.
 */
app.use('/', express.static(path.join(__dirname, 'static')));
//app.use('/', express.static(__dirname + '/static'));

function processkill(){
    if( deviceIdx == 3 ) {
        console.log('exit()');
        server.close();
        process.exit();
    } else {
        open(`http://localhost:${app.get('port')}/`);
    }
}

let processDone = async ()=>{
//  await setInterval(processkill,1000)
  await setTimeout(processkill,500)
}

// COM 포트를 검색하고, 각각의 포트에 대해서 Device 정보를 검색하도록 한다.
// Device 정보를 먼저 가져온다.
// 만약, Device정보가 없다면, Server를 실행하지 않는다.

var Target = "ESP8266";

function detectDevice(com) {
    const Port = new serialPort(com, {baudRate: 57600}, false);
//    const parser = myPort.pipe(new Readline({delimiter: '\n'}));
    const parser = Port.pipe(new Readline({delimiter: '\n'}));
    Port.open( function() {
        console.log(`${com} open.`);
        Port.write("getDevice\n", function(err, results) { });
    });
    Port.on('error', function(err) {
//        console.log('Serial port error : ' + err);
    });

    parser.on('data', (data) => {
        console.log(`${com} rx : ${data}`);

        var header = data.split(':');
//        console.log(`split -> ${header[0]}, ${header[1]}`);
        if( (header[0] != undefined) && (header[0] == 'device') ) {
            if( header[1].substring(0,7) == 'ESP8266' ) {
//                console.log("detect");
                comport = com;
                myPort = Port;
                deviceIdx = 2;
            } else {
//                console.log("not detect");
            }
        } else if( header[0] == 'switch' ) {
        } else if( header[0] == 'info' ) {
            console.log(header[1]);
        } else {
            console.log(`${data} received`);
            comport = '';
            deviceIdx = 3;              // NoDevice
        }
    });
}

serialPort.list().then(async ports => {
    var list = '';
    ports.sort(function(a, b) {         // COM 포트를 오름차순으로 정렬한다.
        return a.path < b.path ? -1 : a.path > b.path ? 1 : 0;
    });
    ports.forEach(async function(port) {
        list += port.path + ' ';
//        console.log(`${port.path} connected.`);
        detectDevice(port.path);
    });
    console.log('port list : ' + list);
});

//processDone()
//process.exit(1);              // Node.js를 강제 종료시킨다.

// http request 처리함수를 작성한다.
/*
app.get('/', function(req, res) {
    console.log(__dirname);
    res.sendFile(path.join(__dirname, '/static/PS1xxx UI.html'));
//    if( myPort != null ) {
//        console.log(myPort);
//        console.log(myPort.path);
//        console.log(myPort.settings.baudRate);
//    }
//    processDone()
})
*/

app.get('/get_device', function(req, res) {
    if( myPort != null ) {
//        var str = "0x55 0xAA\n";
        var str = "getDevice\n";
        console.log("get_device");
        detectDevice(myPort.path);
        deviceIdx = 0;
        res.send(deviceIdx.toString());
//        myPort.write(str, function(err, results) {});
//        result = '';
//        console.log("get_device : " + result);
    }
});


app.get('/Send', function(req, res) {
    res.status(204).send("");

//    console.log(req.url);
//    console.log(req.query);
    var str = req.query.Ctrl + ' ' + req.query.Addr + ' ' + req.query.Data + '\n';
    console.log("Received Command : "+str);
    if( myPort != null ) {
        myPort.write(str, function(err, results) {});
    } else {
        console.log("COM Port Not Available.");
    }
});

app.post('/uploadFile', upload.single("datafile"), function(req, res, next) {
    const file = req.file;

    if( !file ) {
        const error = new Error("Please upload a file");
        return next(error);
    }

    const multerText = Buffer.from(file.buffer).toString("utf-8");
    console.log(multerText);
    if( myPort != null ) {
        myPort.write(multerText, function(err, results) {});
    } else {
        console.log("COM Port Not Available.");
    }

    res.status(204).send("");
    console.log("Received File\n");
});

// http server 실행.
const server = app.listen(app.get('port'), () => {
    console.log(`Web Server listening at http://localhost:${app.get('port')}`);
});


open(`http://localhost:${app.get('port')}/`);
