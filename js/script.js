const homeBtn = document.querySelectorAll('.icon__home')
const activityBtn = document.querySelectorAll('.icon__activity')
const personalBtn = document.querySelectorAll('.icon__account')
const startBtn = document.getElementById('startBtn')
const stopBtn = document.getElementById('stopBtn')
const todaysArrow = document.getElementById('todays_arrow')
const todaysHomeArrow = document.getElementById('arrow__icon')

document.addEventListener('DOMContentLoaded', () => {
    const submitBtn = document.getElementById('submitBtn');
    
    // Check if the submitBtn exists
    if (submitBtn) {
        submitBtn.addEventListener('click', () => {
            submit(); // Call the submit function
        });
    } else {
        console.error('Submit button not found.');
    }
});


$(document).ready(function() {
    $("#home").show();
    $("#daily__activity").hide();
    $('#current__activity').hide();
    $("#day__activity").hide();
    $('#personal').hide();

    loadExerciseData();
    loadPersonalData();

});

todaysArrow.addEventListener('click', event => {
    $("#daily__activity").show();
    $("#home").hide();
    $('#current__activity').hide();
    $("#day__activity").hide();
    $('#personal').hide();
});
todaysHomeArrow.addEventListener('click', event => {
    $("#home").show();
    $("#daily__activity").hide();
    $('#current__activity').hide();
    $("#day__activity").hide();
    $('#personal').hide();
});
homeBtn.forEach(el => el.addEventListener('click', event => {
    $("#home").show();
    $("#daily__activity").hide();
    $('#current__activity').hide();
    $("#day__activity").hide();
    $('#personal').hide();
}));

activityBtn.forEach(el => el.addEventListener('click', event => {
    $("#daily__activity").show();
    $("#home").hide();
    $('#current__activity').hide();
    $("#day__activity").hide();
    $('#personal').hide();
}));

personalBtn.forEach(el => el.addEventListener('click', event => {
    $("#personal").show();
    $("#daily__activity").hide();
    $("#home").hide();
    $('#current__activity').hide();
    $("#day__activity").hide();

}));

startBtn.addEventListener('click', event => {
    $("#home").hide();
    $("#daily__activity").hide();
    $("#current__activity").show();
    $("#day__activity").hide();
});

stopBtn.addEventListener('click', event => {
    $("#home").hide();
    $("#current__activity").hide();
    $("#daily__activity").show();
    $("#day__activity").hide();
});

function showDayActivity(day) {
    var DayCountRef = firebase.database().ref(day + '/exercise');
    DayCountRef.on('value', (snapshot) => {
        var data = snapshot.val();
        Object.keys(data)
            .forEach(function eachKey(key) {
                exercise_key = "#day__" + key + "__count";
                exercise_value = data[key][key];
                $(exercise_key).text(exercise_value);
                window[key] = exercise_value;
            });
    });
    day = day.charAt(0).toUpperCase() + day.slice(1);
    $("#day_name").html(day + ' Activity <label> Stats</label>')
    $("#home").hide();
    $("#current__activity").hide();
    $("#daily__activity").hide();
    $("#day__activity").show();
}


//----------Bluetooth-------------------------//
var bluetoothDevice;

var  int ,exerciseValue;
var startValue;
var stopValue;
var resetValue;

var armcircles = 0;
var pushups = 0;
var squats = 0;


function connect() {
    var state = true;
    navigator.bluetooth.getDevices()
        .then(devices => {
            console.log("inside")
            for (const device of devices) {
                console.log('  > ' + device.name + ' (' + device.id + ')');
                if (device.name == "Get-Fit" || device.name == "Arduino" || device.id == "e2D6KSLwZCC8CzY+W+5FjA==") {
                    state = false;
                    connectToBluetoothDevice(device)
                }
            }

        }).catch(error => {
            log('Argh! ' + error);
        });

}

function connectToBluetoothDevice(device) {

    let serviceUuid = "e267751a-ae76-11eb-8529-0242ac130003";
    if (serviceUuid.startsWith('0x')) {
        serviceUuid = parseInt(serviceUuid);
    }

    const abortController = new AbortController();

    device.addEventListener('advertisementreceived', (event) => {
        console.log('> Received advertisement from "' + device.name + '"...');

        abortController.abort();
        console.log('Connecting to GATT Server from "' + device.name + '"...');
        return device.gatt.connect()
            .then(server => {
                console.log('Getting Service...');
                return server.getPrimaryService(serviceUuid);
            })
            .then(
                service => {
                    console.log('Getting Characteristic...');
                    return service.getCharacteristics();
                }
            )
            .then(characteristic => {
                exerciseValue = characteristic[0];
                startValue = characteristic[1];
                stopValue = characteristic[2];
                resetValue = characteristic[3];
            })

        .catch(error => {
            console.log('Argh! ' + error);
        });
    }, { once: true });

    console.log('Watching advertisements from "' + device.name + '"...');
    device.watchAdvertisements({ signal: abortController.signal })
        .catch(error => {
            console.log('Argh! ' + error);
        });
}

// function requestDevice() {

//     let serviceUuid = "e267751a-ae76-11eb-8529-0242ac130003";
//     if (serviceUuid.startsWith('0x')) {
//         serviceUuid = parseInt(serviceUuid);
//     }

//     let characteristicUuid = "00002a19-0000-1000-8000-00805f9b34fb";
//     if (characteristicUuid.startsWith('0x')) {
//         characteristicUuid = parseInt(characteristicUuid);
//     }

//     navigator.bluetooth.requestDevice({ filters: [{ services: [serviceUuid] }] })
//         .then(async device => {
//             console.log("Chosen device: " + device.name);
            
//                 // try {
//                 //     console.log("connect");
//                 //     const server = await device.gatt.connect();
//                 //     console.log('Connected to GATT Server:', server);
                    
//                 //     return server;
//                 // } catch (error) {
//                 //     console.error('Error connecting to GATT Server:', error);
//                 // }
            
            
//             return connect()
//         })
// }

async function requestDevice() {
    let serviceUuid = "e267751a-ae76-11eb-8529-0242ac130003";
    if (serviceUuid.startsWith('0x')) {
        serviceUuid = parseInt(serviceUuid);
    }

    try {
        const device = await navigator.bluetooth.requestDevice({
            filters: [{ services: [serviceUuid] }],
            optionalServices: ['battery_service', 'device_information']
        });
        console.log("Chosen device: " + device.name);

        connectToBluetoothDevice(device);
    } catch (error) {
        console.error('Error requesting device:', error);
    }
}

async function connectToBluetoothDevice(device) {
    let serviceUuid = "e267751a-ae76-11eb-8529-0242ac130003";
    if (serviceUuid.startsWith('0x')) {
        serviceUuid = parseInt(serviceUuid);
    }

    try {
        console.log('Connecting to GATT Server from "' + device.name + '"...');
        const server = await device.gatt.connect();
        console.log('Connected to GATT Server:', server);

        console.log('Getting Service...');
        const service = await server.getPrimaryService(serviceUuid);

        console.log('Getting Characteristics...');
        const characteristics = await service.getCharacteristics();

        console.log('Characteristics:', characteristics);

        // Assuming these are the characteristics you need
         exerciseValue = characteristics[0];
         startValue = characteristics[1];
         stopValue = characteristics[2];
         resetValue = characteristics[3];

        document.getElementById('deviceInfo').textContent = `Connected to ${device.name}`;
    } catch (error) {
        console.error('Error connecting to device:', error);
    }
}

function start() {
    console.log('hi how ')
    var arr = new Int8Array([21, 31]);
    console.log(startValue)
    return startValue.writeValueWithResponse(arr).then(response => {
        console.log(arr)
        console.log(exerciseValue)
        return exerciseValue.startNotifications().then(_ => {
            console.log('> Notifications started');
            exerciseValue.addEventListener('characteristicvaluechanged', handleNotifications);
        });
    });

}


function handleNotifications(event) {
    let value = event.target.value.getInt8();
    console.log(value);
    if (value == 0) {
        armcircles = armcircles + 1;
        firebase.database().ref(today + '/exercise/armcircles').update({
            armcircles: parseInt(armcircles),
        });
    } else if (value == 1) {
        squats = squats + 1;
        firebase.database().ref(today + '/exercise/squats').update({
            squats: parseInt(squats),
        });
    } else if (value == 2) {
        pushups = pushups + 1;
        firebase.database().ref(today + '/exercise/pushups').update({
            pushups: parseInt(pushups),
        });
    }
}

function stop() {
    var arr = new Int8Array([21, 31]);
    const today_pushups = document.getElementById("squats__count_1");
    console.log(today_pushups);

    today_pushups.innerText = squats.toString();
    $("#todays_squats").text(squats.toString());
    $("#todays_armcircles").text(armcircles.toString());

    return stopValue.writeValueWithResponse(arr).then(response => {
        return exerciseValue.stopNotifications()
            .then(_ => {
                console.log("exit");
                console.log(squats);
            })
            .catch(error => {
                console.log('Argh! ' + error);
            });
    });
}


function reset() {
    var arr = new Int8Array([21, 31]);
    return resetValue.writeValueWithResponse(arr).then(response => {
        console.log();
    });

}



//----------Firebase------------------------//
const d = new Date();
let day = d.getDay();
weekday = { 0: 'sunday', 1: 'monday', 2: 'tuesday', 3: 'wednesday', 4: 'thursday', 5: 'friday', 6: 'saturday' }
today = weekday[day]

const firebaseConfig = {
    apiKey: "AIzaSyBQQS2n8bUfNla2ITB2NghHVDZ-wPPuxvo",
    authDomain: "pro435-37a9b.firebaseapp.com",
    databaseURL: "https://pro435-37a9b-default-rtdb.asia-southeast1.firebasedatabase.app",
    projectId: "pro435-37a9b",
    storageBucket: "pro435-37a9b.appspot.com",
    messagingSenderId: "85520392493",
    appId: "1:85520392493:web:030d72e18b4d28a617b3a4"
  };
firebase.initializeApp(firebaseConfig);

// Generate a unique user/session ID
const userId = localStorage.getItem('userId') || generateUserId();
localStorage.setItem('userId', userId);

function generateUserId() {
    return 'user_' + Math.random().toString(36).substr(2, 9);
}

var CountRef = firebase.database().ref(today + `users/${userId}/exercise`);
CountRef.on('child_changed', (snapshot) => {
    console.log('Child changed:', snapshot.key, snapshot.val());
    const data = snapshot.val();
    exercise = Object.keys(data)[0];
    count = data[exercise];
    $('#lds-ripple').hide();
    $('#current__info').show();
    $('#exercise').text(exercise.toUpperCase());
    $('#count').text(count);
});


CountRef.on('value', (snapshot) => {
    console.log('Value changed:', snapshot.val());
    var data = snapshot.val();
    Object.keys(data)
        .forEach(function eachKey(key) {
            exercise_key = "#" + key + "__count";
            exercise_value = data[key][key];
            $(exercise_key).text(exercise_value);
            window[key] = exercise_value;
        });
    $("#todays_pushups").text(pushups);
    $("#todays_squats").text(squats);
    $("#todays_armcircles").text(armcircles)
    met_pushups = 0.8;
    met_squats = 0.8;
    met_armcircles = 0.6;
    calories = (((armcircles * 4 / 60) * met_armcircles * 3.5 * 65) / 200) + (((pushups * 2 / 60) * met_pushups * 3.5 * 65) / 200) + (((squats * 4 / 60) * met_squats * 3.5 * 65) / 200)
    calories = parseFloat(calories).toFixed(2);
    $("#todays_calorie").text(calories)
});

function submit() {
    console.log('Submit function is called');
    submitPersonalData(); // Call existing function
    showSuccessMessage(); // Display the message
}

function submitPersonalData() {
    const personalRef = firebase.database().ref(`users/${userId}/personal`);
    const name = $("#name_value").val();
    const gender = $("#gender_value").val();
    const weight = $("#weight_value").val();
    const height = $("#height_value").val();
    console.log('Submitting personal data:', { name, gender, weight, height });
    personalRef.set({ name, gender, weight, height })
        .then(() => console.log('Personal data submitted successfully'))
        .catch(error => console.error('Error submitting personal data:', error));
}

function showSuccessMessage() {
    messageDiv.textContent = 'Personal data submitted successfully!';
    messageDiv.style.display = 'block'; // Show the message
    setTimeout(() => {
        messageDiv.style.display = 'none'; // Hide the message after 3 seconds
    }, 3000);
}


function loadPersonalData() {
    const personalRef = firebase.database().ref(`users/${userId}/personal`);
    personalRef.once('value', (snapshot) => {
        const data = snapshot.val();
        if (data) {
            $("#name_value").val(data.name || '');
            $("#gender_value").val(data.gender || '');
            $("#weight_value").val(data.weight || '');
            $("#height_value").val(data.height || '');
        } else {
            console.log("No personal data found");
        }
    }).catch(error => {
        console.error("Error loading personal data:", error);
    });
}



function updateExerciseData() {
    const exerciseRef = firebase.database().ref(`users/${userId}/exercise`);
    exerciseRef.set({
        armcircles: armcircles,
        pushups: pushups,
        squats: squats
    });
}

function resetExerciseData() {
    const exerciseRef = firebase.database().ref(`users/${userId}/exercise`);
    exerciseRef.set({
        armcircles: 0,
        pushups: 0,
        squats: 0
    });
}

function loadExerciseData() {
    const exerciseRef = firebase.database().ref(`users/${userId}/exercise`);
    exerciseRef.once('value', (snapshot) => {
        const data = snapshot.val();
        if (data) {
            armcircles = data.armcircles || 0;
            pushups = data.pushups || 0;
            squats = data.squats || 0;
            $("#todays_armcircles").text(armcircles);
            $("#todays_pushups").text(pushups);
            $("#todays_squats").text(squats);
        } else {
            console.log("No exercise data found");
        }
    }).catch(error => {
        console.error("Error loading exercise data:", error);
    });
}


