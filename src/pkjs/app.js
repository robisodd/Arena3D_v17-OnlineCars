/*jshint esversion: 6 */ // <-- stops CloudPebble from highlighting supported things like "const"
//(function () {
  //'use strict';
  
//----------------------------------------------------------------------------------------------------------//
// Includes and Globals
//----------------------------------------------------------------------------------------------------------//
  Pebble.connected = false;

  var watch_info = require('watch_info');  // make sure to have this before your Pebble READY event listener so watch_info is filled out when its called.
  
  watch_info.onDetect(function() {
    console.log('watch_info = ' + JSON.stringify(watch_info));
    if(watch_info.emulator)
      console.log("JS Emulator Detected: " + watch_info.model + " (" + watch_info.platform + ")");
    else
      console.log("JS Detected Pebble: " + watch_info.model + " (" + watch_info.platform + ")");
    
    
    if(watch_info.emulator) {
      console.log("Disabling Online Connection During Development");
    } else {
      init_entities();
      create_username();
      connect_to_server();
    }
  });



//----------------------------------------------------------------------------------------------------------//
// Game
//----------------------------------------------------------------------------------------------------------//
  var entities = new Array(256);
  var username = "";
  var id = 0;  // Get ID from server, if 0, can't do anything.

  function init_entities() {
    for (var i=0; i<255; i++)
      entities[i] = {x: 0, y: 0, facing: 0, sprite: 0, id: 0, color: 0, enabled: false};
  }



  
//----------------------------------------------------------------------------------------------------------//
// WebSocket -- Phone to Server Communication
//----------------------------------------------------------------------------------------------------------//
  var socket = null;
  //var serverURL = "ws://192.168.33.103:8080";                 // local, without encryption
  var serverURL = 'wss://arena3d-robisodd.rhcloud.com:8443/';   // with encryption
  //var serverURL = 'ws://arena3d-robisodd.rhcloud.com:8000/';  // without encryption


  // WebSocket Packet Types
  const PACKET_JOIN_SUCCESSFUL = 128+0;  // 
  const PACKET_POSITION = 128+1;         // Receive a User's Position from server or Send our position to server
  //const PACKET_MOVE = 128+5;             // Move ME (Could use PACKET_POSITION, but just to be sure we don't jump around)
  //const PACKET_REQUEST_INFO = 128+2;  //
  const PACKET_NEW_USER_JOINED = 128+3;
  const PACKET_USER_DISCONNECTD = 128+4;



  function received_string_from_server(message) {
    console.log("Received String: '" + message + "'");
  }


  function received_JSON_from_server(JSONobj) {
    switch(JSONobj.type) {
      case "msg":
        var usr = JSONobj.user || "someone";
        var say = JSONobj.says || "something";
        console.log(usr + ': ' + say);
      break;

      default:
        console.log("unknown message type: " + JSONobj.type);
    }
  }


  function receive_data_from_server(data) {
    var msg8 = new Uint8Array(data);
    var msg16 = new Int16Array(data);
    var uid;
    //console.log('DATA8:  [' + msg8 + ']  DATA16: [' + msg16 + ']');
    switch(msg8[0]) {
      case PACKET_JOIN_SUCCESSFUL:
        console.log("Successfuly Joined Game");
        id = msg16[1];
        entities[id] = {x: msg16[2], y: msg16[3], facing: msg16[4], sprite: 0, id: msg16[1], color: msg16[5], enabled: true};
        //console.log("Server Positioned You: " + JSON.stringify(entities[id]));
        send_position_to_pebble(id);
      break;

      case PACKET_NEW_USER_JOINED:
        uid = msg16[1];
        console.log("New User: " + uid);
        if(entities[uid].enabled) {
          console.log("ERROR: User " + uid + " already exists!");
          // return; // meh, let it override anyway.
        }
        entities[uid] = {x: msg16[2], y: msg16[3], facing: msg16[4], sprite: 0, id: msg16[1], color: msg16[5], enabled: true};
        console.log("New user: " + JSON.stringify(entities[uid]));
        send_position_to_pebble(uid);
      break;

      case PACKET_USER_DISCONNECTD:
        uid = msg16[1];
        console.log("User " + uid + " disconnected.");
        if(!entities[uid].enabled) {
          console.log("ERROR: User " + uid + " already doesn't exist!");
        }
        entities[uid].exists = false;
        send_user_disconnected_to_pebble(uid);
      break;

      case PACKET_POSITION:  // Another player has moved
        uid = msg16[1];
        entities[uid].x      = msg16[2];
        entities[uid].y      = msg16[3];
        entities[uid].facing = msg16[4];
        send_position_to_pebble(uid, entities);
      break;

      default:
        console.log("unknown data type: " + msg8[0]);
        console.log('DATA: [' + msg8 + ']');
    }
  }


  function send_position_to_server(x, y, facing) {
    if(socket && socket.readyState === socket.OPEN) {
      var message = new Int16Array(4);
      message[0] = PACKET_POSITION;
      message[1] = x;
      message[2] = y;
      message[3] = facing;
      //console.log("SendPos: " + message);
      socket.send(message);
    } else {
      //console.log("SendPos: Server Not Connected");
    }
  }



  function send_text_message_to_server(msg) {
    if(socket && socket.readyState === socket.OPEN)
      socket.send(JSON.stringify({"says":msg}));
    else
      send_error_message_to_pebble("[Not Connected]");
  }

  
  function join_game() {
    console.log('Requesting to Join Game');
    if(socket && socket.readyState === socket.OPEN) {
      //SOCKET EXISTS and is OPEN
      if(id > 0) {
        console.log("You already already joined!  ID: " + id);
        return;
      }

      // Send request to join, give a username.  Server *should* respond with an ID
      console.log("Requesting To Join as '" + username + "'");
      socket.send(JSON.stringify({"type":"join", "name":username}));
    }
  }





  function close_connection() {
    switch(socket.readyState) {
      case socket.OPEN: 
        //send_text_message_to_server("Goodbye everybody!");
      case socket.CONNECTING:
        console.log("Closing connection...");
        //sendSystemMessagetoPebble("Closing connection...");
        socket.close();
        return;
    }
    console.log("Connection has already been closed.");
    //send_error_message_to_pebble("ALREADY CLOSED");
  }



  function connect_to_server() {
    console.log("Connecting to server...");
    try {
      switch(socket.readyState) {
        case socket.OPEN:
          console.log("Server already connected!");
        return;
        case socket.CONNECTING:
          console.log("Still trying to connect...");
        return;
        default:  // CLOSING or CLOSED
      }
    } catch(err) { }


    try {
      //socket = new WebSocket(serverURL, 'echo-protocol');
      socket = new WebSocket(serverURL);//, 'echo-protocol');
      socket.binaryType = "arraybuffer";
      socket.onopen     = function(evt) { onOpen(evt);    };
      socket.onclose    = function(evt) { onClose(evt);   };
      socket.onmessage  = function(evt) { onMessage(evt); };
      socket.onerror    = function(evt) { onError(evt);   };
    } catch(err) {
      console.log("WebSocket Creation Error: " + err);
      socket = null;
    }
  }











  function onOpen(evt) {
    console.log('CONNECTED');
    join_game();
    //sendSystemMessagetoPebble("CONNECTED");
    //socket.send(JSON.stringify({"user":username}));
  }

  function onClose(evt) {
    //send_system_message_to_pebble("CONNECTION CLOSED");
    socket = null; // = {};
    init_entities();
    id = 0;
  }

  function onMessage(evt) {
    if(typeof evt.data == "string") {
      var JSONobj;
      try {
        JSONobj = JSON.parse(evt.data);
      } catch (e) {
        received_string_from_server(evt.data);
        return;
      }

      if(!JSONobj.hasOwnProperty("type"))
        console.log("JSON Object missing type! " + evt.data);
      else
        received_JSON_from_server(JSONobj);

    } else if(evt.data instanceof ArrayBuffer) {  // jshint ignore:line
      //if(typeof evt.data == "object") {
      if(evt.data.byteLength>0) {
        receive_data_from_server(evt.data);
      } else {
        console.log("Empty ArrayBuffer!");
      }
    }
  }


  function onError(err) {
    console.log('FLAGRANT ERROR: ' + err.data);
    console.log("socket error - err=" + JSON.stringify(err));
    send_error_message_to_pebble("Socket Error");
    socket = null; //  = {};
  }
































//----------------------------------------------------------------------------------------------------------//
// PebbleKit JS -- Pebble to Phone Communcation
//----------------------------------------------------------------------------------------------------------//
  
  const COMMAND_OPEN_CONNECTION = 0;
  const COMMAND_CLOSE_CONNECTION = 1;
  
  const STATUS_PEBBLEKIT_JS_READY = 2;

  
  
//   function sendUserMessagetoPebble(usr, msg) {
//     Pebble.sendAppMessage({"userkey":usr, "message": msg}, messageSuccessHandler, messageFailureHandler);
//   }

//   function send_message_to_pebble(msg) {
//     Pebble.sendAppMessage({"message": msg}, messageSuccessHandler, messageFailureHandler);
//   }

  function send_position_to_pebble(index) {
    var msg = {'U': [index, 0,
                     entities[index].x & 0xff, (entities[index].x >> 8) & 0xff,
                     entities[index].y & 0xff, (entities[index].y >> 8) & 0xff,
                     entities[index].facing & 0xff, (entities[index].facing >> 8) & 0xff]};
    Pebble.sendAppMessage(msg, messageSuccessHandler, messageFailureHandler);
  }

  function send_user_disconnected_to_pebble(uid) {
    var msg = {'D': uid};
    Pebble.sendAppMessage(msg, messageSuccessHandler, messageFailureHandler);
  }

  function send_status_to_pebble(status) {
    Pebble.sendAppMessage({"status": status}, messageSuccessHandler, messageFailureHandler);
  }

  function send_system_message_to_pebble(msg) {
    //Pebble.sendAppMessage({"system_message": msg}, messageSuccessHandler, messageFailureHandler);
  }
  
  function send_error_message_to_pebble(msg) {
    Pebble.sendAppMessage({"err": msg}, messageSuccessHandler, messageFailureHandler);
  }

  function messageSuccessHandler() {
    //console.log("Message send succeeded.");
  }
  
  function messageFailureHandler(data, error) {
    console.log("Message from phone-to-Pebble failed: " + error);
  }
  

  // -------------------------------------------------------------------------------- //
  

  function create_username() {
    username = Pebble.getAccountToken();
    username = "Pebble " + username.slice(username.length - 5);
    console.log("Username = " + username);
  }

  
  // -------------------------------------------------------------------------------- //
  
  
  Pebble.addEventListener("ready", function(e) {
    Pebble.connected = true;
    console.log("JS is ready!");    
    send_system_message_to_pebble("PebbleKit JS Ready");
    
    send_status_to_pebble(STATUS_PEBBLEKIT_JS_READY);
    
    // Note: Another READY listener is above with "watch_info.onDetect"
  });


  // -------------------------------------------------------------------------------- //
  
  
  Pebble.addEventListener("appmessage", function(e) {
    if (typeof e.payload.message !== 'undefined') {
      console.log("Received Text from Pebble C: " + e.payload.message);
      send_text_message_to_server(e.payload.message);
    }
  
    // Received Status/Command from Pebble
    if (typeof e.payload.status !== 'undefined') {
      if(e.payload.status === COMMAND_OPEN_CONNECTION) {
        console.log("Task: Create connection...");
        connect_to_server();
      } else if(e.payload.status === COMMAND_CLOSE_CONNECTION) {
        console.log("Task: Close connection...");
        close_connection();
      } else {
        console.log("Received Unknown Status from Pebble C: " + e.payload.status);
      }
    }
    
    // Received Our Position from Pebble, save in JS then send to the world
    if (typeof e.payload.X !== 'undefined') {
      //     console.log("Received Status from Pebble C: (" + e.payload.x + ", " + e.payload.y + ", " + e.payload.f +")");
      entities[id].x      = e.payload.X;
      entities[id].y      = e.payload.Y;
      entities[id].facing = e.payload.F;
      send_position_to_server(e.payload.X, e.payload.Y, e.payload.F);
    }
  });










//}());







  /*
  function send_ws_bytes() {
    if(socket && socket.readyState === socket.OPEN) {
      //var message = new Uint8Array([0, 1, 1, 2, 3, 5, 8, 13, 21, 34, 55, 89]);
      //socket.send(message);
      socket.send(new Uint8Array([0, 1, 1, 2, 3, 5, 8, 13, 21, 34, 55, 89]));
    } else
      send_error_message_to_pebble("[No WS Connection]");
  }
  */

  /*  
  function send_ws_tap() {
    if(socket.readyState === socket.OPEN)
      socket.send(new Uint8Array([tapID]));

    //   if(establish_connection()) {
    //     var message = new Uint8Array([1]);
    //     socket.send(message);
    //   }
    //else send_error_message_to_pebble("[No WS Connection]");
  }
  */




  /*
  function userexists(userid) {
    try {
      if(entities[userid].enabled)
        return true;
      else
        return false;
    } catch (e) {
      entities[userid] = {x: 0, y: 0, facing: 0, sprite: 0, id: 0, color: 0, enabled: false};
      return false;
    }
  }
  */




  /*  
    if(!socket) {
      console.log('Cannot Join Game: Disconnected from Server');
      return;
    }

    // websocket exists
    switch(socket.readyState) {
      case socket.CONNECTING:
        console.log('Cannot Join: Socket Still Connecting...');
        return;
      case socket.CLOSED:
        console.log('Cannot Join: Socket Closed');
        return;
      case socket.CLOSING:
        console.log('Cannot Join: Socket Closing');
        return;
      default:
        console.log('join_game(): Unknown websocket readyState: ' + socket.readyState);
        return;
      case socket.OPEN:
      case 1:
    }
  */
    

  /*
    socket.onmessage = function(msg) {
//       console.log("onmessage = " + JSON.stringify(msg.data));
//       console.log(JSON.parse(JSON.stringify(msg)));
//       console.log("onmessage = " + JSON.stringify(msg));
      if(typeof msg.data == "string") {
        try {
          var obj = JSON.parse(msg.data);
          //if (typeof obj.user !== 'undefined')
          var usr = obj.user || "someone";
          var say = obj.says || "something";
          sendUserMessagetoPebble(usr, say);
        } catch (e) {
          sendMessagetoPebble(msg.data);
        }
      }

      if(msg.data instanceof ArrayBuffer) {
        //if(typeof msg.data == "object") {
        var array = new Uint8Array(msg.data);
        //console.log("Array length = " + array.length);
        //sendMessagetoPebble("DATA: ["+array+"]");
        var bytes = [];
        for(var i = 0; i<array.length; i++)
          bytes.push(array[i]);
        console.log("Data Message: [" + bytes + "]");
        //sendUserMessagetoPebble("DATA MESSAGE", "["+bytes+"]");
        if(array[0]!==tapID)
          sendStatustoPebble(1);
        //sendMessagetoPebble("DATA: ["+bytes+"]");
      }
    };
*/