var gimbalState;
var settings;

console.log("touchscreen is", VirtualJoystick.touchScreenAvailable() ? "available" : "not available");

var btconnectedButton=document.getElementById("btconnected");
var settingsButton=document.getElementById("settings");
var powerButton=document.getElementById("power");
var modeButton=document.getElementById("mode");
var posResetButton=document.getElementById("posreset");
var posResetRevButton=document.getElementById("posresetrev");
var osdSpan=document.getElementById("osd");
var line1Span=document.getElementById("line1");
var line2Span=document.getElementById("line2");
var osdDiv=document.getElementById("osddiv");
var modal = document.getElementById('myModal');
var closeModal = document.getElementsByClassName("close")[0];
var statusSpan=document.getElementById("status");
var saveButton = document.getElementById("save");
var rebootButton = document.getElementById("reboot");
var factoryresetButton = document.getElementById("factoryreset");

function fancyTimeFormat(time) {
  // Hours, minutes and seconds
  var days = ~~(time / 86400);
  var hrs = ~~((time % 86400) / 3600);
  var mins = ~~((time % 3600) / 60);
  var secs = time % 60;

  // Output like "1:01" or "4:03:59" or "123:03:59"
  var ret = "";

  if (days > 0) {
      ret += "" + days + "." + (hrs < 10 ? "0" : "");
  }
  if (hrs > 0) {
      ret += "" + hrs + ":" + (mins < 10 ? "0" : "");
  }else {
      if(days > 0) {
          ret += ":";
      }
  }

  ret += "" + mins + ":" + (secs < 10 ? "0" : "");
  ret += "" + secs;
  return ret;
}

function formToJSON()
{
  var form = document.getElementById("settingsForm");
  var formData = new FormData(form);
  var object = {};
  for(var pair of formData.entries()) {
    console.log("key: "+pair[0]+" value: "+pair[1]); 
  }
  object["cmd"] = "SETTINGS_WRITE";
  object["wifi_opmode"] = parseInt(formData.get("wifi_opmode"));
  object["wifi_ssid"] = formData.get("wifi_ssid");
  object["wifi_secret"] = formData.get("wifi_secret");
  object["wifi_hostname"] = formData.get("wifi_hostname");
  object["gimbal_joystickmode"] = formData.get("gimbal_joystick_reverse_pitch")?1:0;
  object["gimbal_joystickmode"] |= formData.get("gimbal_joystick_reverse_pan")?4:0;
  
  return JSON.stringify(object);
}

function JSONToForm(form, json)
{
  settings = json;
  console.log(JSON.stringify(json));
  statusSpan.innerHTML="Version: "+json.version+" Uptime: "+fancyTimeFormat(json.uptime);
  switch(json.wifi_opmode) {
    case 0: document.getElementById("ap").checked=true; break;
    case 1: document.getElementById("sta").checked=true; break;
  }
  document.getElementsByName("wifi_ssid")[0].value=json.wifi_ssid;
  document.getElementsByName("wifi_secret")[0].value=json.wifi_secret;
  document.getElementsByName("wifi_hostname")[0].value=json.wifi_hostname;
  document.getElementsByName("gimbal_joystick_reverse_pitch")[0].checked=json.gimbal_joystickmode&1?true:false;
  document.getElementsByName("gimbal_joystick_reverse_pan")[0].checked=json.gimbal_joystickmode&4?true:false;
}

function send(msg) {
  var myInt = setInterval(function () {
    if (websocket.bufferedAmount == 0) {
      websocket.send(msg);
      clearInterval(myInt);
    }
  }, 50);
}

function cmd(cmd, value1 = 0, value2 = 0, repeat = 1) {
  var obj = { cmd: cmd, value1: value1, value2: value2, repeat: repeat };
  var json = JSON.stringify(obj);
//  console.log("json: " + json);
  send(json);
}

powerButton.onclick     = function()    {cmd("POWER", !gimbalState.powerState);};
modeButton.onclick      = function()    {cmd("MODE", (gimbalState.mode+1)%3);};
posResetButton.onclick  = function()    {cmd("POSITIONRESET", 0);};
posResetRevButton.onclick  = function() {cmd("POSITIONRESET", 1);};
settingsButton.onclick  = function()    {cmd("SETTINGS");modal.style.display = "block";};
closeModal.onclick = function()         {
  document.onkeydown = checkKeycode;
  document.onkeyup = checkKeycode;  
  modal.style.display = "none"; 
};
saveButton.onclick = function()         {
  document.onkeydown = checkKeycode;
  document.onkeyup = checkKeycode;  
  modal.style.display = "none"; 
  jsonStr=formToJSON();
  console.log(jsonStr);
  send(jsonStr);
  cmd("REBOOT");
};
rebootButton.onclick = function()       {
  document.onkeydown = checkKeycode;
  document.onkeyup = checkKeycode;  
  modal.style.display = "none"; 
  cmd("REBOOT");
};
factoryresetButton.onclick = function () {
  document.onkeydown = checkKeycode;
  document.onkeyup = checkKeycode;  
  modal.style.display = "none"; 
  cmd("FACTORY_RESET");
}
window.onclick = function(event) {
  if (event.target == modal) {
      document.onkeydown = checkKeycode;
      document.onkeyup = checkKeycode;  
      modal.style.display = "none";
  }
}

var joystick1 = new VirtualJoystick({
  container: document.getElementById('container1'),
  mouseSupport: true,
  stationaryBase: false,
  baseX: document.getElementById("container1").offsetWidth / 2,
  baseY: document.getElementById("container1").offsetHeight / 2
});

var joystick2 = new VirtualJoystick({
  container: document.getElementById('container2'),
  mouseSupport: true,
  stationaryBase: false,
  baseX: document.getElementById("container2").offsetWidth / 2,
  baseY: document.getElementById("container2").offsetHeight / 2
});

var websocket;
function wsConnect() {
  if(window.location.href.indexOf("http") > -1) {
    websocket = new WebSocket('ws://'+location.hostname+'/');
  }
  else{  
    websocket = new WebSocket('ws://192.168.4.1/');
  }
  websocket.onopen = function (evt) {
    console.log('WebSocket connection opened');
    cmd("SETTINGS");
  }

  websocket.onmessage = function (evt) {
    var msg = evt.data;
    gimbalState=JSON.parse(evt.data);
    if(gimbalState.cmd == 'STATUS') {
      if(gimbalState.connected==1) {
        btconnectedButton.style.background='#0071bb';
        btconnectedButton.style.color='white';
      } else {
        btconnectedButton.style.color='white';
        btconnectedButton.style.background='#a0a0a0';
      }
      if(gimbalState.powerState==1) {
        powerButton.style.background='#008000';
      } else {
        powerButton.style.background='#a0a0a0';
      }
      switch(gimbalState.mode) {
        case 0: modeButton.style.background='#008000'; modeButton.innerText="Pan Follow"; break;
        case 1: modeButton.style.background='#008000'; modeButton.innerText="Locking"; break;
        case 2: modeButton.style.background='#008000'; modeButton.innerText="Follow"; break;
        default: modeButton.style.background='#a0a0a0'; break;
      }
      cellFullVoltage = 4.2;
      cellEmptyVoltage = 3.3;
      if(gimbalState.batteryLevel) {
        numOfcells=Math.floor(gimbalState.batteryLevel/cellEmptyVoltage);
        battPercent=(((gimbalState.batteryLevel/numOfcells)-cellEmptyVoltage)/(cellFullVoltage-cellEmptyVoltage))*100;
        battPercent=Math.round(battPercent<0?0:battPercent>100?100:battPercent);
      } else {
        battPercent = 0;
      }
      version="N/A";
      if(settings) {
        version=settings.version;
      }
      line1Span.innerHTML = " Version: "+version+" Battery: "+gimbalState.batteryLevel+"V ("+battPercent+"%)";
      line2Span.innerHTML = "Pitch: "+gimbalState.pitch+"° Roll: "+gimbalState.roll+"° Pan: "+gimbalState.pan+"°";
    } else if(gimbalState.cmd == 'SETTINGS') {
      JSONToForm("settingsForm", gimbalState);
    }
  }

  websocket.onclose = function (evt) {
    console.log('Websocket connection closed');
//    document.getElementById("test").innerHTML = "WebSocket closed";
    gimbalState.connected=0;
    gimbalState.powerState=0;
    gimbalState.mode=-1;
    gimbalState.batteryLevel=0;
    setTimeout(function () { wsConnect() }, 3000);
  }

  websocket.onerror = function (evt) {
    console.log('Websocket error: ' + evt);
    gimbalState.connected=0;
    gimbalState.powerState=0;
    gimbalState.mode=-1;
    gimbalState.batteryLevel=0;
//    document.getElementById("test").innerHTML = "WebSocket error????!!!1!!";
  }
}

var haveEvents = 'GamepadEvent' in window;
var haveWebkitEvents = 'WebKitGamepadEvent' in window;
var controllers = {};
var rAF = window.mozRequestAnimationFrame ||
  window.webkitRequestAnimationFrame ||
  window.requestAnimationFrame;

function connecthandler(e) {
  addgamepad(e.gamepad);
}

function addgamepad(gamepad) {
  controllers[gamepad.index] = gamepad; 
  rAF(updateStatus);
}

function disconnecthandler(e) {
  removegamepad(e.gamepad);
}

function removegamepad(gamepad) {
  delete controllers[gamepad.index];
}

function updateStatus() {
  scangamepads();
  rAF(updateStatus);
}

function scangamepads() {
  var gamepads = navigator.getGamepads ? navigator.getGamepads() : (navigator.webkitGetGamepads ? navigator.webkitGetGamepads() : []);
  for (var i = 0; i < gamepads.length; i++) {
    if (gamepads[i]) {
      if (!(gamepads[i].index in controllers)) {
        addgamepad(gamepads[i]);
      } else {
        controllers[gamepads[i].index] = gamepads[i];
      }
    }
  }
}

if (haveEvents) {
  window.addEventListener("gamepadconnected", connecthandler);
  window.addEventListener("gamepaddisconnected", disconnecthandler);
} else if (haveWebkitEvents) {
  window.addEventListener("webkitgamepadconnected", connecthandler);
  window.addEventListener("webkitgamepaddisconnected", disconnecthandler);
} else {
  setInterval(scangamepads, 500);
}

function moveGimbal(scaledx,scaledy)
{
  var startx = 300;
  var starty = 300;
  if(settings) {
    scaledy*=(settings.gimbal_joystickmode&1)?-1.0:1.0;
  }
  if(scaledx != 0 || scaledy != 0) {
    console.log("scaledx "+scaledx+" scaledy: "+scaledy);
  }
  scaledx *= 2048-startx;
  scaledy *= 2048-starty;
  if(scaledx) {
    if(scaledx > 0) {
      x=scaledx+2048+startx;
    } else {
      x=scaledx+2048-startx;
    }
  } else {
    x=2048;
  }
  if(scaledy) {
    if(scaledy > 0) {
      y=scaledy+2048+starty;
    } else {
      y=scaledy+2048-starty;
    }
  }
  else {
    y=2048;
  }
  
  x=x<0?0:x;
  x=x>4095?4095:x;

  y=y<0?0:y;
  y=y>4095?4095:y;

  if(x!=2048) {
    cmd("PAN", x);
//    cmd("ROLL", x);
  } 
  if(y!=2048) {
    cmd("PITCH", y);
  }
  
}

function zoomCamera(scalezw, scalezh) {
  if(scalezh) {
    console.log("scalezw: "+scalezw+" scalezh: "+scalezh);
  }
  scalezw = 10*scalezw;
  scalezh = 10*scalezh;

  if(scalezh != 0) {
    if(scalezh<0) {
      cmd("IR_SONY", 0x00006c9a, 15, Math.abs(scalezh));
    } else {
      cmd("IR_SONY", 0x00006c9b, 15, Math.abs(scalezh));
    }
  }
}

function visualizeMovement(x,y,zh,zw) {
  var radians = Math.atan2(y, x);
  var degrees = radians * ( 180 / Math.PI );
  var show=0;
  osdSpan.innerHTML='';

  if(Math.abs(x)>0.1 || Math.abs(y)>0.1) {
    osdSpan.innerHTML='&#x21e8;';
    show = 1;
  } 
  if(zh>0.1) {
    osdSpan.innerHTML+=' &#x25A2;';
    show = 1;
  }
  if(zh<-0.1) {
    osdSpan.innerHTML+=' &#x25A0;';
    show = 1;
  }
  
  if(show) {    
    osdSpan.style.visibility="visible";
    osdSpan.style.transform='rotate('+degrees+'deg)';
  } else {
    osdSpan.style.visibility="hidden";
  }
//  osdSpan.innerHTML="degrees: "+degrees;
}

var cmdStr = "";
var oldCmdStr = "";
var cmdSpeed = 0;
function Timer() {
  xw = document.getElementById("container1").offsetWidth;
  yh = document.getElementById("container1").offsetHeight;
  x = joystick1.deltaX()*2/xw;
  y = joystick1.deltaY()*2/yh;
  
  zw = document.getElementById("container2").offsetWidth;
  zh = document.getElementById("container2").offsetHeight;
  zw = joystick2.deltaX()*2/zw;
  zh = joystick2.deltaY()*2/zh;

  for (j in controllers) {
    var controller = controllers[j];
    for (var i=0; i<controller.buttons.length; i++) {
      var val = controller.buttons[i];
      if (typeof(val) == "object") {
        pressed = val.pressed;
        val = val.value;
      }

      if(pressed) {
        console.log("button: "+i+" pressed: "+pressed+" value: "+val);
        switch(i) {
          case 0: cmd("POWER", !gimbalState.powerState); break;
          case 1: cmd("POSITIONRESET", 0); break;
          case 2: cmd("POSITIONRESET", 1); break;
          case 3: cmd("MODE", (gimbalState.mode+1)%3); break;
          default: 
        }
      }

    }
    x+=controller.axes[0];
    y+=controller.axes[1];
    zw+=controller.axes[2];
    zh+=controller.axes[3];
  }

  visualizeMovement(x,y,zh,zw);

  if(Math.abs(x)>0.1 || Math.abs(y)>0.1 ) {    
    moveGimbal(x, y);
  }
  if(Math.abs(zh)>0.1 ) {
   zoomCamera(zw, zh);
  }

  setTimeout(Timer, 100);
}

function checkKeycode(event) {
  // handling Internet Explorer stupidity with window.event
  // @see http://stackoverflow.com/a/3985882/517705
  var keyDownEvent = event || window.event,
      keycode = (keyDownEvent.which) ? keyDownEvent.which : keyDownEvent.keyCode;

  console.log("key pressed: "+keycode);
  LEFT = 37;
  UP = 38;
  RIGHT = 39;
  DOWN = 40;
  PLUS = 187;
  MINUS = 189;

  var x=0,y=0,zh=0,zw=0;

  switch(keycode) {
    case LEFT:  x=-0.7; break;
    case UP:    y=-0.7; break;
    case RIGHT: x=0.7; break;
    case DOWN:  y=0.7; break;
    case PLUS: zh=1; break;
    case MINUS: zh=-1; break;
    default:
  }
  visualizeMovement(x,y,zh,zw);

  if(Math.abs(x)>0.1 || Math.abs(y)>0.1 ) { 
    moveGimbal(x, y);
  }
 
  return false;
}

document.onkeydown = checkKeycode;
document.onkeyup = checkKeycode;

var hTimer = setTimeout(Timer, 10);

wsConnect();

