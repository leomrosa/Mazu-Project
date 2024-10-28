package com.example.selectdeviceactivity

import android.Manifest
import android.app.Activity
import android.app.AlertDialog
import android.app.ProgressDialog
import android.bluetooth.BluetoothAdapter
import android.bluetooth.BluetoothDevice
import android.bluetooth.BluetoothSocket
import android.content.Context
import android.content.Intent
import android.content.pm.PackageManager
import android.os.AsyncTask
import android.os.Bundle
import android.os.Handler
import android.util.Log
import android.widget.Button
import android.widget.CompoundButton
import android.widget.ImageButton
import android.widget.ProgressBar
import android.widget.TextView
import android.widget.Toast
import android.widget.ToggleButton
import androidx.appcompat.app.AppCompatActivity
import androidx.core.app.ActivityCompat
import kotlinx.coroutines.*
import android.net.ConnectivityManager
import android.content.BroadcastReceiver
import android.content.IntentFilter
import androidx.core.content.ContextCompat
import kotlinx.coroutines.*
import java.text.SimpleDateFormat;



import com.example.selectdeviceactivity.R
import com.google.android.material.switchmaterial.SwitchMaterial
import com.google.firebase.database.DataSnapshot
import com.google.firebase.database.DatabaseError
import com.google.firebase.database.DatabaseReference
import com.google.firebase.database.FirebaseDatabase
import com.google.firebase.database.ValueEventListener
import java.io.IOException
import java.util.Date
import java.util.Locale
import java.util.UUID
import kotlin.math.round


class QualityActivity : AppCompatActivity() {

    companion object {
        val EXTRA_ADDRESS: String = "Device_address"
        lateinit var userId: String
        private lateinit var m_address: String
        private val m_myUUID: UUID = UUID.fromString("00001101-0000-1000-8000-00805F9B34FB")
        private var m_bluetoothSocket: BluetoothSocket? = null
        private lateinit var m_progress: ProgressDialog
        private lateinit var m_bluetoothAdapter: BluetoothAdapter
        private var m_isConnected: Boolean = false
        private var sentCommand: String = ""
        lateinit var database: DatabaseReference

        private var commandHandler: Handler? = null
        private var commandRunnable: Runnable? = null

        private var networkCommandHandler: Handler? = null
        private var networkCommandRunnable: Runnable? = null

        var isSurfMode: Boolean = false

        lateinit var parameterLib: Parameters // Create an instance of Parameters class



        private lateinit var toggleButton_mode: ToggleButton

        lateinit var currentTime_surf: String

    }
    private lateinit var currentTime: String

    private lateinit var valueEventListener: ValueEventListener
    private lateinit var toggleButton: ToggleButton
    private lateinit var onlinebutton: Button
    private lateinit var watertype: Button



    private var currentIndex: Int = 0
    val parameters = listOf("Temperature", "Potential Hydrogen", "TDS", "Dissolved Oxygen", "Turbidity")
    val waterType = "Water Type"
    val parameterValues = Array(5) { "" }

    var isCommandSendingEnabled = true // Flag to control command sending


    val minTemperature = 0 // Minimum  value
    val maxTemperature = 35 // Maximum  value


    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_quality)
        database = FirebaseDatabase.getInstance().reference

        val currentTimeMillis = System.currentTimeMillis()
        val sdf = java.text.SimpleDateFormat("yyyy-MM-dd HH:mm", Locale.getDefault())
        currentTime = sdf.format(Date(currentTimeMillis))



        onlinebutton = findViewById(R.id.modeoffon)
        watertype = findViewById(R.id.water_type)

        val arrowRight = findViewById<ImageButton>(R.id.arrowRight)
        val arrowLeft = findViewById<ImageButton>(R.id.arrowLeft)

        val waterParameter = findViewById<TextView>(R.id.waterParameter)

        m_address = intent.getStringExtra(MainActivity.EXTRA_ADDRESS).toString()
        userId = intent.getStringExtra("userId").toString()

        parameterLib = Parameters() // Initialize the Parameters instance


        Log.d("IDUSER", " ${userId}")


        arrowRight.setOnClickListener {
            currentIndex = (currentIndex + 1) % parameters.size
            waterParameter.text = parameters[currentIndex]
            updateSensorValues(parameterValues[currentIndex])

        }

        arrowLeft.setOnClickListener {
            currentIndex = if (currentIndex - 1 < 0) {
                parameters.size - 1
            } else {
                (currentIndex - 1) % parameters.size
            }
            waterParameter.text = parameters[currentIndex]
            updateSensorValues(parameterValues[currentIndex])

        }


        // Set initial button text color based on network connectivity status
        updateButtonTextColor(isNetworkConnected())


        onlinebutton.setOnClickListener {
            if (isNetworkConnected()) {
                // Device is currently offline (Flush mode)
                // Show a popup dialog or perform specific action
                showFlushConfirmationDialog()
            }
            else{
                Toast.makeText(this, "Your device is offline", Toast.LENGTH_SHORT).show()
            }
        }

        // Update button text color whenever network connectivity changes
        registerReceiver(networkReceiver, IntentFilter(ConnectivityManager.CONNECTIVITY_ACTION))


        ConnectToDevice(this).execute()

        val sendDisconnectButton: Button = findViewById(R.id.disconnect)

        sendDisconnectButton.setOnClickListener { disconnect() }

        updateTextView("CALIBRATING")
        startCommandSending()
    }

    private fun restartQualityActivity() {
        val intent = Intent(this, QualityActivity::class.java)
        intent.flags = Intent.FLAG_ACTIVITY_NEW_TASK or Intent.FLAG_ACTIVITY_CLEAR_TASK
        startActivity(intent)
    }



    private fun showFlushConfirmationDialog() {
        val alertDialog = AlertDialog.Builder(this)
        alertDialog.setTitle("You sure you want to Flush?")
        alertDialog.setPositiveButton("Yes") { dialog, which ->
            // User clicked Yes, handle the flush action here
            // Trigger Bluetooth command to ESP32 to send data to Firebase
            // Example:
            //isCommandSendingEnabled = false; IMPORTANT TO USE SD CARD FLUSH

        }
        alertDialog.setNegativeButton("No") { dialog, which ->
            // User clicked No, handle the cancellation here
            // Do nothing or provide feedback to the user
        }
        alertDialog.show()
    }



    fun updateSensorValues(value: String?) {

        val waterParameter = findViewById<TextView>(R.id.waterParameter)
        waterParameter.text = parameters[currentIndex]

        // Query the database for the specific child based on the currentIndex
        val sensorValueRef = database.child("users").child(userId)
            .child("sensorValues")
            .child(currentTime)
            .child(parameters[currentIndex])

        if (value != null) {
            val cleanedValue = value.replace("T", "").replace("P", "") // Clean the value


            val sensorUnityTextView = findViewById<TextView>(R.id.sensorUnity) // Get the sensor unity text view

            // Update progress bar and sensor unity text view based on the current index
            when (currentIndex) {
                0 -> { // Temperature

                    updateTextView(parameterLib.temperature.toString()) // Update the text view with the cleaned value
                    Log.d("BarProgress", "Progress: $currentIndex")
                    sensorUnityTextView.text = "ºC"
                }
                1 -> { // Potential Hydrogen (pH)

                    // No need to update progress bar for pH, only update the sensor unity text view
                    updateTextView(parameterLib.pH.toString()) // Update the text view with the cleaned value
                    Log.d("BarProgress", "Progress: $currentIndex")
                    sensorUnityTextView.text = "pH"
                }

                2 -> { // TDS (ppm/NTU)

                    // No need to update progress bar for pH, only update the sensor unity text view
                    updateTextView(parameterLib.tds.toString()) // Update the text view with the cleaned value
                    Log.d("BarProgress", "Progress: $currentIndex")
                    sensorUnityTextView.text = "ppm"
                }
                3 -> { // DO (mg/mol)

                    // No need to update progress bar for pH, only update the sensor unity text view
                    updateTextView(parameterLib.Do.toString()) // Update the text view with the cleaned value
                    Log.d("BarProgress", "Progress: $currentIndex")
                    sensorUnityTextView.text = "mg/L"
                }
                4 -> { // DO (mg/mol)

                    // No need to update progress bar for pH, only update the sensor unity text view
                    updateTextView(parameterLib.turb.toString()) // Update the text view with the cleaned value
                    Log.d("BarProgress", "Progress: $currentIndex")
                    sensorUnityTextView.text = ""
                }
                // Add more cases for other sensor types if needed
            }
        }
    }



    private fun updateTextView(value: String) {
        val sensorValueTextView = findViewById<TextView>(R.id.sensorValue)

        // Check if the value is "Calibrating" to apply specific styling
        if (value == "CALIBRATING") {
            sensorValueTextView.textSize = 20f // Set text size in pixels (not dp)
            sensorValueTextView.text = value

        } else {
            // Reset text size to default for other values
            sensorValueTextView.textSize = 30f // Set default text size in pixels
            sensorValueTextView.text = value
        }
    }

    override fun onDestroy() {
        super.onDestroy()
        // Remove the ValueEventListener only if it has been initialized
        if (::valueEventListener.isInitialized) {
            val sensorValueRef = database.child("users").child(userId)
                .child("sensorValues").child(currentTime)
                .child(parameters[currentIndex])
            sensorValueRef.removeEventListener(valueEventListener)
        }
        unregisterReceiver(networkReceiver)
    }

    private fun isNetworkConnected(): Boolean {
        val connectivityManager = getSystemService(Context.CONNECTIVITY_SERVICE) as? ConnectivityManager
        val networkInfo = connectivityManager?.activeNetworkInfo
        return networkInfo != null && networkInfo.isConnected
    }


    private val networkReceiver = object : BroadcastReceiver() {
        override fun onReceive(context: Context?, intent: Intent?) {
            if (intent?.action == ConnectivityManager.CONNECTIVITY_ACTION) {
                // Update button text color when connectivity changes
                updateButtonTextColor(isNetworkConnected())
            }
        }
    }

    private fun updateButtonTextColor(isConnected: Boolean) {
        val colorResId = if (isConnected) android.R.color.holo_green_dark else android.R.color.holo_red_dark
        val text = if (isConnected) "Online" else "Flush"
        onlinebutton.setTextColor(ContextCompat.getColor(this, colorResId))
        onlinebutton.text = text
    }



    private fun disconnect() {
        if (QualityActivity.m_bluetoothSocket != null) {
            try {
                QualityActivity.m_bluetoothSocket!!.close()
                QualityActivity.m_bluetoothSocket = null
                QualityActivity.m_isConnected = false
                stopCommandSending()


                isSurfMode = false // Update the isSurfMode flag to false
                val intent = Intent(this, MainActivity::class.java)
                intent.putExtra("userId", userId)
                startActivity(intent)

            } catch (e: IOException) {
                e.printStackTrace()
            }
        }
        finish()
    }


    public fun sendCommand(input: String, timestamp: String) {
        if (QualityActivity.m_bluetoothSocket != null) {
            try {
                QualityActivity.m_bluetoothSocket!!.outputStream.write(input.toByteArray())
                QualityActivity.sentCommand = input

                Log.i("response", "RecieveResponse")
                receiveResponse(timestamp)
            } catch (e: IOException) {
                e.printStackTrace()
            }
        }
    }


    public fun sendCommand_NET(input: String) {
        if (QualityActivity.m_bluetoothSocket != null) {
            try {
                QualityActivity.m_bluetoothSocket!!.outputStream.write(input.toByteArray())
                QualityActivity.sentCommand = input
            } catch (e: IOException) {
                e.printStackTrace()
            }
        }
    }

    public fun sendCommand_TIME(input: String) {
        if (QualityActivity.m_bluetoothSocket != null) {
            try {
                QualityActivity.m_bluetoothSocket!!.outputStream.write(input.toByteArray())
                QualityActivity.sentCommand = input
            } catch (e: IOException) {
                e.printStackTrace()
            }
        }
    }


    private fun receiveResponse(timestamp: String) {

        fun Float.roundTo(decimalPlaces: Int): Double {
            var multiplier = 1.0
            repeat(decimalPlaces) { multiplier *= 10 }
            return round(this * multiplier) / multiplier
        }

        val buffer = ByteArray(2048)
        var bytes: Int

        try {
            bytes = QualityActivity.m_bluetoothSocket!!.inputStream.read(buffer)
            val response = String(buffer, 0, bytes)
            val sensorUnityTextView = findViewById<TextView>(R.id.sensorUnity) // Get the sensor unity text view


            val currentTimeMillis = System.currentTimeMillis()
            val sdf = java.text.SimpleDateFormat("yyyy-MM-dd HH:mm", Locale.getDefault())
            //currentTime = sdf.format(Date(currentTimeMillis))

            val sdf_surf = java.text.SimpleDateFormat("yyyy-MM-dd HH:mm:ss", Locale.getDefault())
            currentTime_surf = sdf_surf.format(Date(currentTimeMillis))



            val sensorValuesRef = QualityActivity.database.child("users").child(QualityActivity.userId).child("sensorValues").child(timestamp)
            val sensorValuesFlush = QualityActivity.database.child("users").child(QualityActivity.userId).child("sensorValues")

            val surfValuesRef = QualityActivity.database.child("users").child(QualityActivity.userId).child("AccelValues").child(currentTime_surf)

            val progressBar = findViewById<ProgressBar>(R.id.progressBar) // Get the progress bar

            val waterParameter = findViewById<TextView>(R.id.waterParameter)
            val sensorValues = HashMap<String, Any>()

            Log.i("response", response)


            when (QualityActivity.sentCommand.trim()) {
                "T" -> {
                    if(response.contains("T")) {
                        val (floatValue, progressValue) = Parameters.processValues(response, "T", minTemperature, maxTemperature)
                        parameterLib.updateTemperature(floatValue, progressValue)

                        // Round the DO value to 2 decimal places
                        val roundedTValue = parameterLib.temperature.roundTo(1)

                        // Update sensor values with the rounded value
                        updateSensorValues(roundedTValue.toString())

                        // Set the rounded value in Firebase
                        sensorValuesRef.child("Temperature").setValue(roundedTValue)

                        // Log the rounded value
                        Log.i("T", roundedTValue.toString())
                    }
                }
                "P" -> {
                    if(response.contains("P")) {

                        val (floatValue, progressValue) = Parameters.processValues(response, "P", minTemperature, maxTemperature)
                        parameterLib.updatePH(floatValue, progressValue)

                        // Round the DO value to 2 decimal places
                        val roundedPHValue = parameterLib.pH.roundTo(1)

                        // Update sensor values with the rounded value
                        updateSensorValues(roundedPHValue.toString())

                        // Set the rounded value in Firebase
                        sensorValuesRef.child("Potential Hydrogen").setValue(roundedPHValue)

                        // Log the rounded value
                        Log.i("PH", roundedPHValue.toString())

                    }
                }
                "S" -> {
                    if(response.contains("S")) {
                        val (floatValue, progressValue) = Parameters.processValues(response, "S", minTemperature, maxTemperature)
                        parameterLib.updateTDS(floatValue.toInt(), progressValue)
                        updateSensorValues(parameterLib.tds.toString())
                        sensorValuesRef.child("Total Dissolved Solids").setValue(parameterLib.tds)
                        Log.i("tds", parameterLib.tds.toString())
                    }
                }


                "O" -> {
                    if(response.contains("O")) {
                        val (floatValue, progressValue) = Parameters.processValues(response, "O", minTemperature, maxTemperature)
                        parameterLib.updateDO(floatValue, progressValue)

                        // Round the DO value to 2 decimal places
                        val roundedDoValue = parameterLib.Do.roundTo(2)

                        // Update sensor values with the rounded value
                        updateSensorValues(roundedDoValue.toString())

                        // Set the rounded value in Firebase
                        sensorValuesRef.child("Dissolved Oxygen").setValue(roundedDoValue)

                        // Log the rounded value
                        Log.i("Do", roundedDoValue.toString())
                    }
                }


                "B" -> {
                    if(response.contains("B")) {
                        val trimmedResponse = response.replace("B", "") // Remove all occurrences of 'H'
                        parameterLib.updateTURB(trimmedResponse)
                        updateSensorValues(parameterLib.turb)
                        sensorValuesRef.child("Turbidity").setValue(parameterLib.turb)
                        Log.i("Turb", parameterLib.turb)
                    }
                }
                "C" -> {
                    if(response.contains("C")) {
                        val trimmedResponse = response.replace("C", "")
                        //progressBar.progress = trimmedResponse.toInt()
                        Log.i("BATTERY", trimmedResponse)
                    }
                }
                "H" -> {
                    if(response.contains("H")) {
                        val trimmedResponse = response.replace("H", "") // Remove all occurrences of 'H'

                        // Set the trimmed response text on the button
                        watertype.text = trimmedResponse

                        // Set the trimmed response value in Firebase
                        sensorValuesRef.child("Water Type").setValue(trimmedResponse)

                        // Log the trimmed response
                        Log.i("Type", trimmedResponse)
                    }
                }
                "F" -> {
                    if (response.contains("Z")) {
                        isCommandSendingEnabled = true
                    }
                    if (response.contains("FLUSHED_data")) {
                        updateTextView("FLUSH")
                        sensorUnityTextView.text = ""
                        // Extract the dataString before "FLUSHED_data" marker
                        val dataString = response.substringBefore("FLUSHED_data").trim()

                        // Extract the Timestamp value from the dataString
                        val timestampIndex = dataString.indexOf("Timestamp:")
                        val timestamp: String? = if (timestampIndex != -1) {
                            // Timestamp is found, extract the timestamp value
                            val timestampStart = timestampIndex + "Timestamp:".length
                            dataString.substring(timestampStart).trim()
                        } else {
                            null // Timestamp not found
                        }

                        if (timestamp != null) {
                            // Split the dataString into parameter name and value pairs
                            val parameterValuePairs = dataString.split(",")

                            // Map to store parameter-value pairs
                            val parameterMap = mutableMapOf<String, String>()

                            // Parse each parameter-value pair and populate the parameterMap
                            for (pair in parameterValuePairs) {
                                if (pair.contains(":")) {
                                    val components = pair.split(":")
                                    if (components.size == 2) {
                                        val parameterName = components[0].trim()
                                        val parameterValue = components[1].trim()
                                        parameterMap[parameterName] = parameterValue
                                    }
                                }
                            }

                            // Log the parsed parameter-value pairs using Log.i
                            parameterMap.forEach { (parameter, value) ->
                                Log.i("ParsedData", "$parameter -> $value")
                            }

                            // Log the extracted Timestamp value
                            Log.i("ParsedData", "Timestamp -> $timestamp")

                            // Save sensor values to Firebase under the extracted timestamp
                            saveSensorValuesToFirebase(timestamp, parameterMap)
                        } else {
                            // Handle case where Timestamp was not found in the response
                            Log.e("ParsedData", "Timestamp not found in the response: $response")
                        }
                    }
                }

                else -> println("Command Not Recognized")
            }
        } catch (e: IOException) {
            e.printStackTrace()
        }
    }


    private fun saveSensorValuesToFirebase(timestamp: String, sensorValues: Map<String, String>) {
        try {
            // Update Firebase with the sensor values under the specified timestamp
            val userId = QualityActivity.userId
            val database = QualityActivity.database

            val sensorValuesRef = database
                .child("users")
                .child(userId)
                .child("sensorValues")
                .child(timestamp) // Use specified timestamp as the child node

            // Update Firebase with each sensor value under its corresponding child node
            for ((sensorName, value) in sensorValues) {
                sensorValuesRef.child(sensorName).setValue(value)
                Log.i("Firebase", "Updated $sensorName to Firebase at $timestamp: $value")
            }

            Log.i("Firebase", "Sensor values updated to Firebase at $timestamp")
        } catch (e: Exception) {
            e.printStackTrace()
        }
    }



    private var commandIndex = 0
    private var isFirstCommandSent = false
    private var isSecondCommandSent = false
    private var isThirdCommandSent = false
    private var isFourthCommandSent = false
    private var isFifthCommandSent = false
    private var isSixthCommandSent = false


    var checkNetwork = true

    private var shouldIgnoreCallbacks = false


    private var commandHandler: Handler? = null

    private fun startCommandSending() {
        commandHandler = Handler()
        val commandRunnable = object : Runnable {
            private var cycleCount = 0
            override fun run() {
                Log.i("kilf", "$currentTime")

                if (isCommandSendingEnabled) {

                    if (cycleCount == 0) {
                        val currentTimeMillis = System.currentTimeMillis()
                        val sdf = SimpleDateFormat("yyyy-MM-dd HH:mm", Locale.getDefault())
                        currentTime = sdf.format(Date(currentTimeMillis))
                    }

                    if (!isSurfMode) {
                        when (cycleCount) {
                            0 -> {
                                // First cycle
                                sendCommand("T", currentTime)
                                isFirstCommandSent = true
                                commandHandler?.postDelayed(this, 2000)
                            }
                            1 -> {
                                sendCommand("P", currentTime)
                                isSecondCommandSent = true
                                commandHandler?.postDelayed(this, 2000)
                            }
                            2 -> {
                                sendCommand("S", currentTime)
                                isThirdCommandSent = true
                                commandHandler?.postDelayed(this, 2000)
                            }
                            3 -> {
                                sendCommand("O", currentTime)
                                isFourthCommandSent = true
                                commandHandler?.postDelayed(this, 2000)
                            }
                            4 -> {
                                sendCommand("B", currentTime)
                                isFifthCommandSent = true
                                commandHandler?.postDelayed(this, 2000)
                                // Start 30-second wait and check network
                            }
                            5 -> {
                                checkNetworkStatus()
                                sendCommand("H", currentTime)
                                sendCommand_TIME("DATE:$currentTime")
                                isSixthCommandSent = true
                                // Start 30-second wait and check network
                                checkNetwork = true
                                startWaiting()
                            }
                            else -> {
                                // All commands sent, reset state for next cycle
                                resetState()
                                cycleCount = -1 // Reset to -1 to start over
                                commandHandler?.postDelayed(this, 10000) // Next cycle after 10 seconds
                            }
                        }
                        cycleCount++
                    } else {
                        // In "surf" mode, send "A" command
                        sendCommand("A", currentTime)
                        commandHandler?.postDelayed(this, 5000) // Next cycle after 5 seconds
                    }
                } else {
                    // When command sending is disabled, send "F" every 1 second
                    sendCommand("F", currentTime)
                }

            }

            private fun startWaiting() {
                // Wait for 30 seconds and check network during the wait
                for (i in 1..32) {
                    commandHandler?.postDelayed({
                        if (!shouldIgnoreCallbacks) {
                            checkNetworkStatus()
                        }
                    }, i * 2000L)
                }
                // After 30 seconds, reset state and trigger next cycle
                resetState()
                commandHandler?.postDelayed(this, 60000) // Next cycle after 10 seconds
            }

            fun resetState() {
                isFirstCommandSent = false
                isSecondCommandSent = false
                isThirdCommandSent = false
                isFourthCommandSent = false
                isFifthCommandSent = false
                isSixthCommandSent = false
                checkNetwork = false
            }
        }

        // Start the first cycle
        commandHandler?.postDelayed(commandRunnable, 10000) // Start after 10 seconds
    }

    private fun checkNetworkStatus() {
        if(isCommandSendingEnabled) {
            if (isNetworkConnected()) {
                sendCommand_NET("N")
                sendCommand("C", currentTime)
            } else {
                sendCommand_NET("M")
                sendCommand("C", currentTime)
            }
        }
        else
        {
            sendCommand("F", currentTime)
        }
    }


    private fun startNetworkStatusCommands() {
        networkCommandHandler = Handler()
        networkCommandRunnable = object : Runnable {
            override fun run() {
                if (isNetworkConnected()) {
                    // If network is connected, send "DB"
                    sendCommand_NET("N")

                } else {
                    // If network is not connected, send "SD"
                    sendCommand_NET("M")
                }
                // Schedule the next network status check after a delay
                networkCommandHandler?.postDelayed(this, 5000) // Adjust delay as needed
            }
        }
        // Start sending network status commands immediately
        networkCommandHandler?.post(networkCommandRunnable!!)
    }

    private fun stopCommandSending() {

        isCommandSendingEnabled = false
        shouldIgnoreCallbacks = true // Set the flag to ignore callbacks
        QualityActivity.commandRunnable?.let { QualityActivity.commandHandler?.removeCallbacks(it) }
        QualityActivity.commandHandler = null
        QualityActivity.commandRunnable = null
    }

    private class ConnectToDevice(c: Context) : AsyncTask<Void, Void, String>() {
        private var connectSuccess: Boolean = true
        private val context: Context

        init {
            this.context = c
        }

        override fun onPreExecute() {
            super.onPreExecute()
            QualityActivity.m_progress = ProgressDialog.show(context, "Connecting...", "Please Wait")
        }

        override fun doInBackground(vararg p0: Void?): String? {
            try {
                if (QualityActivity.m_bluetoothSocket == null || !QualityActivity.m_isConnected) {
                    QualityActivity.m_bluetoothAdapter = BluetoothAdapter.getDefaultAdapter()
                    val device: BluetoothDevice = QualityActivity.m_bluetoothAdapter.getRemoteDevice(
                        QualityActivity.m_address
                    )

                    if (QualityActivity.m_bluetoothSocket != null && QualityActivity.m_bluetoothSocket!!.isConnected) {
                        // Socket is already open, no need to connect again
                        return null
                    }
                    if (ActivityCompat.checkSelfPermission(
                            context as Activity,
                            Manifest.permission.BLUETOOTH
                        ) != PackageManager.PERMISSION_GRANTED ||
                        ActivityCompat.checkSelfPermission(
                            context,
                            Manifest.permission.BLUETOOTH_ADMIN
                        ) != PackageManager.PERMISSION_GRANTED
                    ) {
                        return ""
                    }
                    QualityActivity.m_bluetoothSocket = device.createInsecureRfcommSocketToServiceRecord(
                        QualityActivity.m_myUUID
                    )
                    QualityActivity.m_bluetoothSocket!!.connect()
                }

            } catch (e: IOException) {
                connectSuccess = false
                (context as Activity).runOnUiThread {
                    Toast.makeText(context, "Turn on LIMPID device", Toast.LENGTH_SHORT).show()
                }
                e.printStackTrace()
            }
            return null
        }

        override fun onPostExecute(result: String?) {
            super.onPostExecute(result)
            if (!connectSuccess) {
                val intent = Intent(context, MainActivity::class.java)
                context.startActivity(intent)
            } else {
                QualityActivity.m_isConnected = true
            }
            QualityActivity.m_progress.dismiss()
            //isSurfMode = QualityActivity.m_isConnected

        }
    }

    interface AccelValueListener {
        fun onAccelValueChanged(value: String)
    }
}

