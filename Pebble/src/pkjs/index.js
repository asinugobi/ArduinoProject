Pebble.addEventListener("appmessage", function(e) {
  // Parse the request type
  var message = JSON.stringify(e.payload);
  var message_parsed = JSON.parse(message);
  var request = message_parsed["0"];
  // Send the request to the server
  sendToServer(request);
});

function sendToServer(type) {
  var req = new XMLHttpRequest();
  var ipAddress = "10.0.1.2"; // Hard coded IP address
  var port = "3016"; // Same port specified as argument to server 
  var url = "http://" + ipAddress + ":" + port + "/" + type;
  var method = "GET";
  var async = true;
  console.log("URL: " + url);
  req.onload = function(e) {
    // see what came back
    var msg = "no response";
    var response = JSON.parse(req.responseText); 
    if (response && response.response_type) {
      var response_type = response.response_type;
      if(response_type == "getTemp"){
        msg = "It is " + response.temperature + " " + response.units + " in here";
      } else if(response_type == "changeUnits"){
        msg = "Changing the Units";
      }
      else msg = "noname";
    }
    // sends message back to pebble 
    Pebble.sendAppMessage({ "0": msg });
  };
  
  req.open(method, url, async);
  req.send(null);
  
}