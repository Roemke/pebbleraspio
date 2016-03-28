/*
 * hmm, ist ein pebble clean noetig bevor die JS-Datei neu uebertragen wird?
 * nach ewigem suchen - es gibt keine vernünftige Methode die meta-Daten hier 
 * einzubauen (build script, require patch usw waeren moeglich)
 * kopiere daher die appkeys hier herein
 */
  var appKeys = {
  	"KEY_NOTSET": 0,
    "KEY_VU": 1,   
    "KEY_RASPIO": 2,  	
  	"KEY_APPREADY": 10,
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
    "KEY_RASPIP": 1005,
    "KEY_DEVICE" : 1006,
    "KEY_RASPVOLUME" : 1100,
    "KEY_RASPFULLSTATUS" : 1101,
    "KEY_RASPSTATIONLIST" :  1102,
    "KEY_RASPACTUALSTAION" : 1103,
    "KEY_SWITCHSTATION" : 1104
  };

var VERSION = "1.2";

var isReady = false;
var callbacks = []; //stack for callbacks
var options = getOptions();


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

//doc says: The watchapp has been launched and the PebbleKit JS component is now ready to receive events. 
function readyCallback(event) {
  isReady = true;
  var callback;
  console.log("readyCallback 1, options: " + JSON.stringify(options) + " Type:" + typeof(options));
  while (callbacks.length > 0) { //if callbacks on stack, process them
    callback = callbacks.shift();
    callback(event);
  }
  Pebble.sendAppMessage({'KEY_APPREADY': 1}); //inform watch
}

function showConfiguration(event) {
    var opts = getOptionsAsString(); //load from localStorage
    var url  = "http://zb42.de/pebble/raspio/configure.html";
    //var url = "http://192.168.2.54/roemke/pebble/enigmavu/configure.html";
    console.log("showConfiguration with string: " + url + "#v=" + encodeURIComponent(VERSION) + "&options=" + encodeURIComponent(opts));
    Pebble.openURL(url + "#v=" + encodeURIComponent(VERSION) + "&options=" + encodeURIComponent(opts));
}

//response from configure.html
function webviewclosed(event) {
  var resp = event.response;
	console.log("WebView Closed:");
	if (resp)
	{
	  	options = JSON.parse(resp); //store it in global object 
		console.log("Options:");
		console.log(options);
		console.log('configuration response (settings): '+ JSON.stringify(options) + ' ('+ typeof resp +')');
		setOptionsString(resp); //store in local Storage as String 
		if (options.raspio)
			options.actualDevice = appKeys.KEY_RASPIO;
		else
			options.actualDevice = appKeys.KEY_VU;
		
		Pebble.sendAppMessage(prepareConfiguration(options), function(event) {
		    console.log("delivered message");
				}, logError);
	}	
	//else works
	//console.log("cancel in config");
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
  if (options)
  {
	  console.log("read options from storage as object");
	  console.log(localStorage.getItem("options"));
	  //console.log("options.ip: " + options.ip);
	  //console.log("options.accel: "  + options.accel);
  }
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
    		case appKeys.KEY_RASPVOLUME:
    			result += "status";
    		break;
    		case appKeys.KEY_RASPFULLSTATUS:
    			result += "completeState";
    		break;
    		case appKeys.KEY_VOLDOWN:
    			result += "volumeDown";
    		break;    		
    		case appKeys.KEY_VOLUP:
    			result += "volumeUp";
    		break;    		
    		case appKeys.KEY_SWITCHSTATION:
    			options=getOptions();
				result += "switch&station=" + (options.actualStation+1);
    		break;    		
    		case appKeys.KEY_RASPSTATIONLIST :
    			result += "liste";
    		break;  		
    	}
    	return result;
    }

/* answer bei completeState
 * Response = {"infoText":"completeState","result":
 *   [{"response":null,"status":"OK","values":
 *      {"volume":"85","repeat":"0","random":"0","single":"0","consume":"0","playlist":"115","playlistlength":"13",
 *       "mixrampdb":"0.000000","state":"play","song":"0","songid":"14","time":"2439:0","elapsed":"2439.498",
 *       "bitrate":"128","audio":"48000:24:2","nextsong":"1","nextsongid":"15"}},
 *    {"response":null,"status":"OK",
 *       "values":{"file":"http:\/\/wdr-5.akacast.akamaistream.net\/7\/41\/119439\/v1\/gnl.akacast.akamaistream.net\/wdr-5",
 *       "Title":"WDR 5 Hotline - 0221 56789 555","Name":"WDR 5","Pos":"0","Id":"14"}}
 *       ,"2"],"state":0}
 *  -> modifiziere raspberry radio, so dass er im infotext das kommando mit sendet -> duerfte im Webinterface
 *     kein Problem geben, getan, 
 */
	function sendAnswerFromRaspi (answer) //send Answer to pebble
	{
		
		//console.log("Response = " + answer);
		var ansO = JSON.parse(answer);
		var send = "";	
		switch (ansO.infoText)
		{
			case "completeState" :
			//vorsicht volumen nicht korrigiert
			console.log("complete state with " + JSON.stringify(ansO));
			 if (!ansO.result[1].values.Title)
			 	ansO.result[1].values.Title="keine Information des Senders";
			 send = ansO.result[1].values.Name + "|" + ansO.result[1].values.Title + "|" + ansO.result[0].values.volume ; 
			 //wieso hab ich die Antwort so kompliziert gemacht?
			   Pebble.sendAppMessage({'KEY_RASPFULLSTATUS': send});
			break;
			case "status" : //dann nur volume
			//hatte in meinem web-interface eine volumenkorrektur eingebaut, der MPD ist bei 50% nicht hoerbar.
			//vielleicht hätte ich die besser serverseitig bei der Mpd-Klasse eingebaut, naja, korrigiere auch hier			
			  send = parseInt(ansO.result.values.volume) ; 
			  send = (send - 50)*2;
			  send = (send < 0) ? 0 : send;  
			  console.log("send volume " + send );
			  Pebble.sendAppMessage({'KEY_RASPVOLUME': send});
			break;
			case "volumeUp": //dann bekomme ich den aktuellen Status zurueck, diesen an pebble senden
			  send = parseInt(ansO.result.values.volume) ; 
			  send = (send - 50)*2;
			  send = (send < 0) ? 0 : send;  
			  console.log("send aktualisiertes Volume " + send );
			  Pebble.sendAppMessage({'KEY_RASPVOLUME': send});
			break;
			case "volumeDown": //dann bekomme ich den aktuellen Status zurueck, diesen an pebble senden
			  send = parseInt(ansO.result.values.volume) ; 
			  send = (send - 50)*2;
			  send = (send < 0) ? 0 : send;  
			  console.log("send aktualisiertes Volume " + send );
			  Pebble.sendAppMessage({'KEY_RASPVOLUME': send});
			break;
			case "liste" :
				var stationArray = ansO.result;
				var actualPos = parseInt(ansO.actualPos);
				console.log("got liste with actual pos " + actualPos);
				if (stationArray.length > 0)
				{
					var send = stationArray[0].name;
					for (var i = 1; i < stationArray.length; ++i)
					{
						send += ("|" + stationArray[i].name);
					}
					//first send actual and if successful send the list
					Pebble.sendAppMessage({"KEY_RASPACTUALSTAION" : actualPos} ,
						function(){Pebble.sendAppMessage({"KEY_RASPSTATIONLIST" : send});});
				}
			break;
		}
		console.log("Send to Pebble: " + send); 
	}    
    //zwei interne funktionen
	function handleCommandToRaspio(commandKey,options)
	{		
	    if (!options && !options.raspip)	    
	   		options = getOptions();
	   	if (!options.raspip)
	   		Pebble.showSimpleNotificationOnPebble("Error", "No IP of raspberry - configuration missing");
	   	else
	   	{
			// Timeout to abort in 5 seconds, property of XMLHttpRequest seems not to be supported
			var xmlHttpTimeout=setTimeout( 
			   function (){
			   	 stillSending = false;
			     req.abort();
			     console.log("Request for timed out, maybe wrong ip: " + options.raspip);
			   },5000);
			var req = new XMLHttpRequest();
			var command = translateRaspiKey(commandKey);
			console.log("request is http://" + options.raspip + '/radio/php/ajaxSender.php?'+command);
			req.open('GET', 'http://' + options.raspip + '/radio/php/ajaxSender.php?'+command, true); //true async
			req.onload = function(e) {//onload should always have state 4 ?
			   if (req.readyState == 4 && req.status == 200) {
			     clearTimeout(xmlHttpTimeout); 
			      if(req.status == 200) {
			         sendAnswerFromRaspi(this.responseText);
			         stillSending = false;
			         } //else { console.log('Error'); }
			      } //status 200 
			      else
			      {
			        console.log("complete, but error, wrong command");
			      }
			};//onload
			req.send(null);
	   		
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
			// Timeout to abort in 5 seconds, property of XMLHttpRequest seems not to be supported
			var xmlHttpTimeout=setTimeout( 
			   function (){
			   	 stillSending = false;
			     req.abort();
			     console.log("Request for timed out, maybe wrong ip: " + options.vuip);
			   },5000);
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
        if (payload.hasOwnProperty('KEY_DEVICE'))//payload.KEY_DEVICE) //doch keine so gute Idee das device fuer die VU auf 0 zu setzen :-)
        {
          var deviceKey = payload.KEY_DEVICE;          	
       	  options.actualDevice = deviceKey;
      	  console.log("Device now " + deviceKey);
      	  setOptions(options);
      	  console.log("options: " + JSON.stringify(options));
      	  stillSending = false;
        }
        else if (payload.hasOwnProperty('KEY_SWITCHSTATION'))
        {
        	options.actualStation = payload.KEY_SWITCHSTATION;
        	setOptions(options);
        	console.log ("set options after switch station, now : " 
        		+ JSON.stringify(options));
        	if (options.actualDevice == appKeys.KEY_RASPIO)
        		handleCommandToRaspio(appKeys.KEY_SWITCHSTATION,options);
        }
        else if (payload.KEY_CONTROL)   
        { 
          //console.log("caller of getObject was eventhandler of appmessage");
          var commandKey = payload.KEY_CONTROL;  
          console.log("options: " + JSON.stringify(options));
          if (options.actualDevice == appKeys.KEY_VU)
          {          	
          	handleCommandToVu(commandKey,options);
          }//eo actual device is vu
          else if (options.actualDevice == appKeys.KEY_RASPIO)
          {
           	handleCommandToRaspio(commandKey,options); 	
          }
          else
              Pebble.showSimpleNotificationOnPebble("Error","No actual device,Configuration missing!");
        } //payload has key_control         
      }//eof if payload
    };//eof inner function
  }//eof eventListener
)(); //first call initializes stillSending 
 
Pebble.addEventListener('appmessage', eventListener);
  //eventListener in closure, so of type of function which takes argument e, we don't need argument here
  //function(e) { //handle keys send from pebble, hard coded in c-file and appinfo.json
  //} //eof event listener
  

        
onReady(function(event) {
  options = getOptions();
  if (! options.hasOwnProperty('vu') && ! options.hasOwnProperty('raspio'))
  {
    Pebble.showSimpleNotificationOnPebble("First start", "Configuration missing!"); 
   	showConfiguration();
  }
  else 
  {
  	 if (options.raspio)
  	 	options.actualDevice = appKeys.KEY_RASPIO;
  	 else
  	 	options.actualDevice = appKeys.KEY_VU;
  }
});

