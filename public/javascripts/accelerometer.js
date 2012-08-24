$(function () {

  var outputDiv = document.getElementById('output');

  window.ondeviceorientation = function(event) {
  // event.alpha
  // event.beta
  // event.gamma
    // console.log(event);
  };
  window.ondevicemotion = function(event) {
  // event.accelerationIncludingGravity.x
  // event.accelerationIncludingGravity.y
   // console.log(event.accelerationIncludingGravity.z);
   outputDiv.innerHTML = event.accelerationIncludingGravity.z;
  };

})
