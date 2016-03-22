/*
 * hmm, ist ein pebble clean noetig bevor die JS-Datei neu uebertragen wird?
 * nach ewigem suchen - es gibt keine vernünftige Methode die meta-Daten hier 
 * einzubauen (build script, require patch usw waeren moeglich)
 * kopiere daher die appkeys hier herein
 */
  var appKeys = {
    "KEY_POWER": 116,
    "KEY_VOLUP": 115,
    "KEY_VOLDOWN": 114,
    "KEY_STATIONUP": 103,
    "KEY_STATIONDOWN": 108,
    "KEY_OK": 352,
    "KEY_EXIT": 1,
    "KEY_INFO": 358,
    "KEY_CONTROL": 1000,
    "KEY_VUIP": 1001,    
    "KEY_ACCEL": 1002,
    "KEY_RASPIO": 1003,
    "KEY_VU": 1004,   
    "KEY_RASPIP": 1005,
    "KEY_DEVICE" : 1006,
    "KEY_RASPSTATUS" : 1100,
    "KEY_RASPFULLSTATUS" : 1101
  };

var VERSION = "1.2";

var isReady = false;
var callbacks = []; //stack for callbacks
var options = {};

//convert object to object with keys
function prepareConfiguration(options) {
  //console.log("In prepare with " + serialized_settings);
  var result =  {
    KEY_ACCEL : options.accel ? 1 : 0,
    KEY_VUIP : options.vuip,
    KEY_RASPIP : options.raspip,
    KEY_VU : options.vu ? 1 : 0,
    KEY_RASPIO : options.raspio ? 1 : 0 ,
  };
  var dataCounter = 0; 
  return result;
}

function readyCallback(event) {
  isReady = true;
  var callback;
  //options = getOptions();
  //ok, this works as exepced
  
  console.log("readyCallback 1, options: " + options + " Type:" + typeof(options));
  //console.log("options.ip: " + options.ip );
  //console.log("options.accel: " + options.accel );
  //options = JSON.stringify(options);
 
  //console.log("readyCallback 2, options: " + options + " Type:" + typeof(options));

  while (callbacks.length > 0) { //if callbacks on stack, process them
    callback = callbacks.shift();
    callback(event);
  }
  options.actualDevice = appKeys.KEY_RASPIO; //will mit radio starten
}

function showConfiguration(event) {
  onReady(function() {
    var opts = getOptionsAsString(); //load from localStorage
    var url  = "http://zb42.de/pebble/raspio/configure.html";
    //var url = "http://192.168.2.54/roemke/pebble/enigmavu/configure.html";
    console.log("showConfiguration with string: " + url + "#v=" + encodeURIComponent(VERSION) + "&options=" + encodeURIComponent(opts));
    Pebble.openURL(url + "#v=" + encodeURIComponent(VERSION) + "&options=" + encodeURIComponent(opts));
  });
}

//response from configure.html
function webviewclosed(event) {
  var resp = event.response;
  //bedingung mal raus if ()
  
	onReady(function() {	  	
	  	console.log("WebView Closed:");
	  	options = JSON.parse(resp); //store it in global object 
		console.log("Options:");
	  	console.log(options);
	    console.log('configuration response (settings): '+ JSON.stringify(options) + ' ('+ typeof resp +')');
	    setOptionsString(resp); //store in local Storage as String 
	    //for version 1.2 send to pebble
	    Pebble.sendAppMessage(prepareConfiguration(options), function(event) {
		    console.log("delivered message");
			}, logError);
	});
  
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
  console.log("read options from storage as object");
  console.log(localStorage.getItem("options"));
  //console.log("options.ip: " + options.ip);
  //console.log("options.accel: "  + options.accel);
  
  return options || {};
}

// Stores options in localStorage.
function setOptionsString(options) {
  localStorage.setItem("options", options);
}

function setOptions(options)
{
	setOptionsString(JSON.stringify(options));
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
Pebble.addEventListener("ready", readyCallback); 
Pebble.addEventListener("showConfiguration", showConfiguration);
Pebble.addEventListener("webviewclosed", webviewclosed);
//and one for message send from pebble
//ack is send by pebble js, we don't have to care for it 
//this could happen before app is ready - do we have to wait for app ready?
//we surely need the options, so try to read the options
//don't think that we need pebble ready because we don't need to send something to pebble


//just an experiment - if we are still sending we drop request from pebble
//folgende Technik hatte ich mir mal angeschaut, jetzt nicht mehr parat :-(
//ein closure um stillSending einzubauen?
var eventListener = (function (e) //called if appmessage event - pebble is sending
{
    var stillSending = false;
    //console.log("eventListener initialized");
    //intern, nur hier gebraucht
    function translateRaspiKey(key)
    {
    	var result = "action=";
    	switch(key)
    	{
    		case appKeys.KEY_RASPSTATUS:
    			result += "status";
    		break;
    		case appKeys.KEY_RASPFULLSTATUS:
    			result += "completeState";
    		break;
    		case appKeys.KEY_VOLDOWN:
    			result += "volDown";
    		break;    		
    		case appKeys.KEY_VOLUP:
    			result += "volUp";
    		break;    		//todo aus liste auswaehlen
    		case appKeys.KEY_STATIONDOWN:
    		break;    		
    		case appKeys.KEY_STATIONUP:
    		break;    		    		
    	}
    	return result;
    }
    
    //zwei interne funktionen, moechte auf stillSending zugreifen
	//die folgende funktion findet er
	function handleCommandToRaspio(commandKey,options)
	{		
	    if (!options && !options.raspip)	    
	   		options = getOptions();
	   	if (!options.raspip)
	   		Pebble.showSimpleNotificationOnPebble("Error", "No IP of raspberry - configuration missing");
	   	else
	   	{
	   		//status abfrage (Lautstaerke)
	   		//voller status ( Sender und Liste?)
	   		//lautstaerke erhoehen / verrringern
	   		//sender hoch / runter
			var req = new XMLHttpRequest();
			var command = translateRaspiKey(commandKey);
			req.open('GET', 'http://' + options.vuip + '/web/remotecontrol?command='+commandKey, true); //true async
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
			   	 stillSending = false;
			     req.abort();
			     console.log("Request for timed out, maybe wrong ip: " + options.vuip);
			   },5000);
	   		
	   	}//ip is available
	   	   	
	}
	//diese nicht - das ist doch quatsch, funktion umbenannt zu hCTV -> wird gefunden
	//Funktion wieder zu handleCommandToVu geht auch wieder, d.h. das System hat bugs 
	function handleCommandToVu(commandKey,options)
	{		
	    if (!options && !options.vuip)
			options = getOptions();
		if (!options.vuip)
	   		Pebble.showSimpleNotificationOnPebble("Error", "No IP of VU + - configuration missing");
	    else
	    {
			var req = new XMLHttpRequest();
			//vuplus : sende direkt die keys an die vu, keys entsprechend definiert
			
			//console.log('Send: GET on http://' + options.ip + '/web/remotecontrol?command='+commandKey);
			req.open('GET', 'http://' + options.vuip + '/web/remotecontrol?command='+commandKey, true); //true async
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
			   	 stillSending = false;
			     req.abort();
			     console.log("Request for timed out, maybe wrong ip: " + options.vuip);
			   },5000);
		}//else ip is available   
	}

    
	//and the "constructor"     
    return function(e)  {
      var payload = e.payload;
      console.log("in eventListener mit payload " + JSON.stringify(payload ) + " stillSending is "+stillSending);
      if (payload)// && !stillSending) //todo ?
      {
        stillSending = true;
        //console.log(JSON.stringify(payload));
        if (!options.actualDevice)
        {
        	options = getOptions(); 
        }
        if (payload.KEY_DEVICE)
        {
          var deviceKey = payload.KEY_DEVICE;          	
       	  options.actualDevice = deviceKey;
      	  console.log("Device now " + deviceKey);
      	  setOptions(options);
      	  console.log("options: " + JSON.stringify(options));
      	  stillSending = false;
        }
        else if (payload.KEY_CONTROL)   
        { 
          //console.log("caller of getObject was eventhandler of appmessage");
          var commandKey = payload.KEY_CONTROL;  
          if (options.actualDevice == appKeys.KEY_VU)
          {          	
          	handleCommandToVu(commandKey,options);
          }//eo actual device is vu
          else if (options.actualDevice == appKeys.KEY_RASPIO)
          {
           	handleCommandToRaspio(commandKey,options); 	
          }
          else
              Pebble.showSimpleNotificationOnPebble("Error", "device unknown, ask developer");
        } //payload has key_control         
      }//eof if payload
    };//eof inner function
  }//eof eventListener
)(); //first call initializes stillSending  
  Pebble.addEventListener('appmessage', eventListener);
  //eventListener in closure, so of type of function which takes argument e, we don't need argument here
  //function(e) { //handle keys send from pebble, hard coded in c-file and appinfo.json
  //} //eof event listener
  

        
//todo (kr) mindestens nochmal schauen
onReady(function(event) {
  options = getOptions();
  if (typeof options.vu === 'undefined' && typeof options.raspio === 'undefined' )
  	showConfiguration();
  
  //var message = prepareConfiguration(getOptions());
  //transmitConfiguration(message); don't commit setting after loading js app
});

