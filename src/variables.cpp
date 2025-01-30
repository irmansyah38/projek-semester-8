#include <Arduino.h>
#include <pgmspace.h>
#include "variables.h" // Sertakan file header

int condition_main = 0;

// wifi
String ip;

// oled
String last_sentance;

// telegram
String bot_token;
String chat_id;

// disable sleep
unsigned long last_time;

// website
bool web_condition = true;

// nomor handphone
String nomor_handphone;

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">

<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Document</title>
</head>

<style>
  body {
    margin: auto;
    font-family: -apple-system, BlinkMacSystemFont, sans-serif;
    overflow: auto;
    background: linear-gradient(315deg, rgba(101, 0, 94, 1) 3%, rgba(60, 132, 206, 1) 38%, rgba(48, 238, 226, 1) 68%, rgba(255, 25, 25, 1) 98%);
    animation: gradient 15s ease infinite;
    background-size: 400% 400%;
    background-attachment: fixed;
  }

  @keyframes gradient {
    0% {
      background-position: 0% 0%;
    }

    50% {
      background-position: 100% 100%;
    }

    100% {
      background-position: 0% 0%;
    }
  }

  .wave {
    background: rgb(255 255 255 / 25%);
    border-radius: 1000% 1000% 0 0;
    position: fixed;
    width: 200%;
    height: 12em;
    animation: wave 10s -3s linear infinite;
    transform: translate3d(0, 0, 0);
    opacity: 0.8;
    bottom: 0;
    left: 0;
    z-index: -1;
  }

  .wave:nth-of-type(2) {
    bottom: -1.25em;
    animation: wave 18s linear reverse infinite;
    opacity: 0.8;
  }

  .wave:nth-of-type(3) {
    bottom: -2.5em;
    animation: wave 20s -1s reverse infinite;
    opacity: 0.9;
  }

  @keyframes wave {
    2% {
      transform: translateX(1);
    }

    25% {
      transform: translateX(-25%);
    }

    50% {
      transform: translateX(-50%);
    }

    75% {
      transform: translateX(-25%);
    }

    100% {
      transform: translateX(1);
    }
  }

  .kotak {
    width: 300px;
    position: absolute;
    top: 50%;
    left: 50%;
    transform: translate(-50%, -50%);
    background-color: rgba(255, 255, 255, 0.8);
    padding: 20px;
    border-radius: 10px;
    box-shadow: 0 0 10px rgba(0, 0, 0, 0.4);
  }

  .kotak h2 {
    text-align: center;
  }

  .alert {
    display: none;
    border-radius: 25px;
    padding: 20px;
    background-color: #f44336;
    color: white;
  }

  .closebtn {
    margin-left: 15px;
    color: white;
    font-weight: bold;
    float: right;
    font-size: 22px;
    line-height: 20px;
    cursor: pointer;
    transition: 0.3s;
  }

  .closebtn:hover {
    color: black;
  }

  /* From Uiverse.io by Satwinder04 */
  /* Input container */
  .input-container {
    position: relative;
    margin: 20px;
  }

  /* Input field */
  .input-field {
    display: block;
    width: 100%;
    padding: 10px;
    font-size: 16px;
    border: none;
    border-bottom: 2px solid #ccc;
    outline: none;
    background-color: transparent;
  }

  /* Input label */
  .input-label {
    position: absolute;
    top: 0;
    left: 0;
    font-size: 16px;
    color: rgba(204, 204, 204, 0);
    pointer-events: none;
    transition: all 0.3s ease;
  }

  /* Input highlight */
  .input-highlight {
    position: absolute;
    bottom: 0;
    left: 0;
    height: 2px;
    width: 0;
    background-color: #007bff;
    transition: all 0.3s ease;
  }

  /* Input field:focus styles */
  .input-field:focus+.input-label {
    top: -20px;
    font-size: 12px;
    color: #007bff;
  }

  .input-field:focus+.input-label+.input-highlight {
    width: 100%;
  }



  /* From Uiverse.io by cssbuttons-io */
  button {
    appearance: button;
    background-color: #1899D6;
    border: solid transparent;
    border-radius: 16px;
    border-width: 0 0 4px;
    box-sizing: border-box;
    color: #FFFFFF;
    cursor: pointer;
    display: inline-block;
    font-size: 15px;
    font-weight: 700;
    letter-spacing: .8px;
    line-height: 20px;
    margin: 0;
    outline: none;
    overflow: visible;
    padding: 13px 19px;
    text-align: center;
    text-transform: uppercase;
    touch-action: manipulation;
    transform: translateZ(0);
    transition: filter .2s;
    user-select: none;
    -webkit-user-select: none;
    vertical-align: middle;
    white-space: nowrap;
  }

  button:after {
    background-clip: padding-box;
    background-color: #1CB0F6;
    border: solid transparent;
    border-radius: 16px;
    border-width: 0 0 4px;
    bottom: -4px;
    content: "";
    left: 0;
    position: absolute;
    right: 0;
    top: 0;
    z-index: -1;
  }

  button:main,
  button:focus {
    user-select: auto;
  }

  button:hover:not(:disabled) {
    filter: brightness(1.1);
  }

  button:disabled {
    cursor: auto;
  }

  button:active:after {
    border-width: 0 0 0px;
  }

  button:active {
    padding-bottom: 10px;
  }
</style>

<body>
  <div>
    <div class="wave"></div>
    <div class="wave"></div>
    <div class="wave"></div>
  </div>

  <div class="kotak">
    <h2>Tugas Akhir</h2>
    <div class="alert">
      <span class="closebtn" onclick="this.parentElement.style.display='none';">&times;</span>
      <span id="text"></span>
    </div>
    <br>
    <form class="form">
      <!-- /* From Uiverse.io by Satwinder04 */ -->
      <div class="input-container d">
        <input placeholder="Token" class="input-field" type="text" id="botToken">
        <label for="input-field" class="input-label">Token</label>
        <span class="input-highlight"></span>
      </div>

      <div class="input-container d1">
        <input placeholder="Chat ID" class="input-field" type="text" id="chatID">
        <label for="input-field" class="input-label">Chat ID</label>
        <span class="input-highlight"></span>
      </div>

      <div class="input-container">
        <input placeholder="Nomor handphone" class="input-field" type="text" required id="nomorHandphone">
        <label for="input-field" class="input-label">Nomor handphone</label>
        <span class="input-highlight"></span>
      </div>

      <!-- /* From Uiverse.io by micaelgomestavares */ -->
      <div style="margin-left: 10px; margin-bottom: 5px;">
        <input type="checkbox" id="onlyHandphone" onclick="displayNone()">
        <label for="onlyHandphone">Hanya menggunakan No handphone</label>
      </div>
      <div style="margin-left: 10px; margin-bottom: 5px;">
        <input type="checkbox" id="save">
        <label for="save" id="keterangan">Simpan no hp dan token</label>
      </div>
      <br>
      <div style="text-align: center;">
        <button type="submit">Submit</button>
      </div>
    </form>
  </div>
</body>

<script>

  const onlyHandphone1 = document.getElementById("onlyHandphone")

  function alertMessage(bool, text) {
    const alertMessageDisplay = document.querySelector(".alert")
    alertMessageDisplay.style.display = "block"
    document.getElementById("text").textContent = text
    if (bool) {
      alertMessageDisplay.style.backgroundColor = " #04AA6D"
    } else {
      alertMessageDisplay.style.backgroundColor = "#f44336"
    }
  }

  function displayNone() {
    if (onlyHandphone1.checked) {
      document.getElementById("keterangan").innerText = "Simpan no hp"
      document.querySelector(".d").style.display = "none"
      document.querySelector(".d1").style.display = "none"

    } else {
      document.getElementById("keterangan").innerText = "Simpan no hp dan token"
      document.querySelector(".d").style.display = "block"
      document.querySelector(".d1").style.display = "block"
    }
  }

  document.querySelector(".form").addEventListener("submit", async function (event) {
    event.preventDefault();

    const nomorHandphone1 = document.getElementById("nomorHandphone").value
    const remember1 = document.querySelector("#save").checked

    if (nomorHandphone1 == "") {
      alertMessage(false, "Minimal isi nomor handphone dan centang hanya no telpon")
      return
    }

    let sendData
    if (onlyHandphone1.checked) {
      sendData = `botToken=&chatID=&remember=${encodeURIComponent(remember1)}&onlyHandphone=${encodeURIComponent(onlyHandphone1.checked)}&nomorHandphone=${encodeURIComponent(nomorHandphone1)}`

    } else {
      const botToken1 = document.getElementById("botToken").value
      const chatID1 = document.getElementById("chatID").value
      sendData = `botToken=${encodeURIComponent(botToken1)}&chatID=${encodeURIComponent(chatID1)}&remember=${encodeURIComponent(remember1)}&onlyHandphone=${encodeURIComponent(onlyHandphone1.checked)}&nomorHandphone=${encodeURIComponent(nomorHandphone1)}`
    }

    const options = {
      method: 'POST',
      headers: {
        'Content-Type': 'application/x-www-form-urlencoded',

      },
      body: sendData
    };

    console.log(sendData)

    try {
      const response = await fetch(`http://${window.location.hostname}:90/submit`, options)
      const data = await response.json();
      console.log(data)
      if (data.status == "1") {
        alertMessage(true, "Nomor handphone ada dan disimpan")
      } else if (data.status == "2") {
        alertMessage(true, "Nomor handphone ada tetapi gagal disimpan")
      } else if (data.status == "3") {
        alertMessage(true, "Nomor handphone sudah ada di alat")
      } else if (data.status == "4") {
        alertMessage(true, "Token dan chatid ada dan disimpan")
      } else if (data.status == "5") {
        alertMessage(true, "Token dan chatid ada tetapi gagal disimpan")
      } else if (data.status == "6") {
        alertMessage(true, "Token dan chatid ada")
      }
      else {
        alertMessage(false, "Ada yang salah token atau chatid")
      }
    } catch (error) {
      console.log(error)
      alertMessage(false, "ada kesalahan alat tolong coba lagi atau restart")
    }
  });
</script>

</html>
)rawliteral";
