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
  var ipAddress = "158.130.169.59"; // Hard coded IP address
  var port = "3001"; // Same port specified as argument to server 
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
      } else if(response_type == "StandBy"){
        msg = "Standby Mode";
      } else if(response_type == "StandByExiting"){
        msg = "Exiting Standby Mode";
      } else if(response_type == "arduinoNotConnected"){
        msg = "Arduino Not Connected";
      } else if(response_type == "temperatureReportAvg"){
        msg = "Average: " + response.temperature_avg + " " + response.units;
      } else if(response_type == "temperatureReportMin"){
        msg = "Low: " + response.temperature_avg + " " + response.units;
      } else if(response_type == "temperatureReportMax"){
        msg = "High: " + response.temperature_avg + " " + response.units;
      }
      else msg = "Arduino error";
    }
    // sends message back to pebble 
    Pebble.sendAppMessage({ "0": msg });
  };
  
  req.open(method, url, async);
  req.send(null);
  
}