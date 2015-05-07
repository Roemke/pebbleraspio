var VERSION = "1.2";

var isReady = false;
var callbacks = []; //stack for callbacks
var options = {};

//convert object to object with keys
function prepareConfiguration(options) {
  //console.log("In prepare with " + serialized_settings);
  var result =  {
    KEY_ACCEL : options.accel ? 1 : 0,
    KEY_IP : options.ip
  };
  //need to append time-table to result, no can't handle that
  var dataCounter = 0; 
  return result;
}

function readyCallback(event) {
  isReady = true;
  var callback;
  //options = getOptions();
  //ok, this works as exepced
  
  //console.log("readyCallback 1, options: " + options + " Type:" + typeof(options));
  //console.log("options.ip: " + options.ip );
  //console.log("options.accel: " + options.accel );
  //options = JSON.stringify(options);
 
  //console.log("readyCallback 2, options: " + options + " Type:" + typeof(options));

  while (callbacks.length > 0) { //if callbacks on stack, process them
    callback = callbacks.shift();
    callback(event);
  }
}

function showConfiguration(event) {
  onReady(function() {
    var opts = getOptionsAsString(); //load from localStorage
    var url  = "http://zb42.de/pebble/enigmavu/configure.html";
    //var url = "http://192.168.2.54/roemke/pebble/enigmavu/configure.html";
    console.log("showConfiguration with string: " + url + "#v=" + encodeURIComponent(VERSION) + "&options=" + encodeURIComponent(opts));
    Pebble.openURL(url + "#v=" + encodeURIComponent(VERSION) + "&options=" + encodeURIComponent(opts));
  });
}

//response from configure.html
function webviewclosed(event) {
  var resp = event.response;
  console.log('configuration response: '+ resp + ' ('+ typeof resp +')');
  options = JSON.parse(resp); //store it in global object 
  if (typeof options.ip !== 'undefined')
  {
	  onReady(function() {	  	
	    //console.log('configuration response: '+ JSON.stringify(options) + ' ('+ typeof resp +')');
	    setOptionsAsString(resp); //store in local Storage as String 
	    //for version 1.2 send to pebble
	    Pebble.sendAppMessage(prepareConfiguration(options), function(event) {
		    console.log("delivered message");
  			}, logError);
	  });
  }
}
function logError(event) {
  console.log('Unable to deliver message with transactionId='+
              event.data.transactionId +' ; Error is'+ event.error.message);
}


// Retrieves stored configuration from localStorage.
function getOptionsAsString() {
  return localStorage.getItem("options") || ("{}");
}

// Retrieves stored configuration from localStorage, convert to object
function getOptions() {
  var options = JSON.parse(localStorage.getItem("options"));
  //console.log("read options from storage as object");
  //console.log("options.ip: " + options.ip);
  //console.log("options.accel: "  + options.accel);
  
  return options || {};
}

// Stores options in localStorage.
function setOptionsAsString(options) {
  localStorage.setItem("options", options);
}

// build an on stack for callback. 
//work is started if onReady event from watch set isReady to true
function onReady(callback) {
  if (isReady) {
    callback();
  }
  else {
    callbacks.push(callback);
  }
}

//we need three Listeners
Pebble.addEventListener("ready", readyCallback); //called when app is loaded on watch
Pebble.addEventListener("showConfiguration", showConfiguration);
Pebble.addEventListener("webviewclosed", webviewclosed);
//and one for message send from pebble
//ack is send by pebble js, we don't have to care for it 
//this could happen before app is ready - do we have to wait for app ready?
//we surely need the options, so try to read the options
//don't think that we need pebble ready because we don't need to send something to pebble

//just an experiment - if we are still sending we drop request from pebble
var eventListener = (function (e)
{
    var stillSending = false;
    //console.log("eventListener initialized");
    return function(e)
    {
      var payload = e.payload;
      //console.log("in eventListener");
      if (payload && !stillSending)
      {
        //console.log(JSON.stringify(payload));
        if (payload.KEY_CONTROL)   
        { 
        if (!options.ip)
          options = getOptions();
          //console.log("caller of getObject was eventhandler of appmessage");
          if (options.ip)
          {
            stillSending = true;
            var commandKey = payload.KEY_CONTROL;  
            var req = new XMLHttpRequest();
            //console.log('Send: GET on http://' + options.ip + '/web/remotecontrol?command='+commandKey);
            req.open('GET', 'http://' + options.ip + '/web/remotecontrol?command='+commandKey, true); //true async
            //does not work: 
            //req.open('GET', 'http://' + options.ip + '/cgi-bin/rc?'+commandKey, true); //true async
            req.onload = function(e) {//onload should always have state 4 ?
               if (req.readyState == 4 && req.status == 200) {
                 clearTimeout(xmlHttpTimeout); 
                  if(req.status == 200) {
                     stillSending = false;
                     //var response = req.responseText;
                     //console.log("It seems we have success with " + response);
                     } //else { console.log('Error'); }
                  } //status 200 
                  else
                  {
                    console.log("complete, but error, wrong command");
                  }
            };//onload
            req.send(null);
            // Timeout to abort in 5 seconds, property of XMLHttpRequest seems not to be supported
            var xmlHttpTimeout=setTimeout( 
               function (){
                 req.abort();
                 console.log("Request for vu+ timed out, maybe wrong ip: " +options.ip);
               },5000);
            }//eo options.ip found
            else
              Pebble.showSimpleNotificationOnPebble("Error", "Configuration of ip needed");
        } //payload has key_control         
      }//eof if payload
    }//eof inner function
  }//eof eventListener
)(); //first call initializes stillSending  
Pebble.addEventListener('appmessage', eventListener);
  //eventListener in closure, so of type of function which takes argument e, we don't need argument here
  //function(e) { //handle keys send from pebble, hard coded in c-file and appinfo.json
  //} //eof event listener
  

        

onReady(function(event) {
  options = getOptions();
  if (typeof options.ip === 'undefined')
  	showConfiguration();
  
  //var message = prepareConfiguration(getOptions());
  //transmitConfiguration(message); don't commit setting after loading js app
});

