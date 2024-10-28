package com.example.selectdeviceactivity

import android.Manifest
import android.app.Activity
import android.app.ProgressDialog
import android.bluetooth.BluetoothAdapter
import android.bluetooth.BluetoothDevice
import android.bluetooth.BluetoothSocket
import android.content.Context
import android.content.Intent
import android.content.pm.PackageManager
import android.os.AsyncTask
import android.os.Bundle
import android.util.Log
import android.view.View
import android.widget.*
import androidx.appcompat.app.AppCompatActivity
import androidx.core.app.ActivityCompat
import com.google.firebase.database.*
import java.io.IOException
import java.text.SimpleDateFormat
import java.util.*

class SurfActivity : AppCompatActivity() {

    companion object {
        const val EXTRA_ADDRESS: String = "Device_address"
        private val m_myUUID: UUID = UUID.fromString("00001101-0000-1000-8000-00805F9B34FB")
        private var m_bluetoothSocket: BluetoothSocket? = null
        private lateinit var m_bluetoothAdapter: BluetoothAdapter
        private var m_isConnected: Boolean = false
        private var isReceiving: Boolean = false
        private lateinit var m_progress: ProgressDialog
        private lateinit var m_address: String
    }

    private val biomechanicParameters = listOf("Session Duration", "Maximum Speed", "Number of Waves", "Total Distance", "Max Wave Distance")
    private val biomechanicUnits = listOf("", "m/s", "", "m", "m") // Corresponding units for each parameter
    private var currentParameterIndex = 0

    // Variables to store the last values of each parameter
    private var lastMspeed: String = "0"
    private var lastDuration: String = "0"
    private var lastNwaves: String = "0"
    private var lastTdistance: String = "0"
    private var lastTemperature: String = "0"
    private var mwaveDistance: String = "0"

    // Declare the temperature TextView as a class-level property
    private lateinit var temperatureParameterValue: TextView
    private lateinit var biomechanicParameterValue: TextView
    private lateinit var biomechanicParameterUnit: TextView // For unit display
    private lateinit var userId: String
    private lateinit var database: DatabaseReference

    private var sessionDataListener: ValueEventListener? = null // To remove listener on destroy

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_surf)

        userId = intent.getStringExtra("userId") ?: run {
            Log.e("SurfActivity", "User ID is missing.")
            finish()
            return
        }

        database = FirebaseDatabase.getInstance().reference

        // Initialize views
        val disconnectButton = findViewById<ToggleButton>(R.id.switch_disconnect)
        val switchModeButton = findViewById<Button>(R.id.switch_mode)
        biomechanicParameterValue = findViewById(R.id.biomech_value)
        biomechanicParameterUnit = findViewById(R.id.biomech_unit)
        temperatureParameterValue = findViewById(R.id.temp)
        val arrowLeft = findViewById<ImageButton>(R.id.arrowLeft)
        val arrowRight = findViewById<ImageButton>(R.id.arrowRight)

        retrieveLatestSessionData()

        updateParameterDisplay()

        arrowLeft.setOnClickListener {
            currentParameterIndex = if (currentParameterIndex > 0) {
                currentParameterIndex - 1
            } else {
                biomechanicParameters.size - 1
            }
            updateParameterDisplay()
        }

        arrowRight.setOnClickListener {
            currentParameterIndex = if (currentParameterIndex < biomechanicParameters.size - 1) {
                currentParameterIndex + 1
            } else {
                0
            }
            updateParameterDisplay()
        }

        disconnectButton.setOnClickListener {
            disconnect()
        }

        switchModeButton.setOnClickListener {
            if (switchModeButton.text == "Sync") {
                sendCommand("W")
            }
        }

        m_address = intent.getStringExtra(EXTRA_ADDRESS).toString()
        ConnectToDevice(this, disconnectButton, switchModeButton).execute()
    }

    override fun onStart() {
        super.onStart()
        retrieveLatestSessionData()
    }

    override fun onDestroy() {
        super.onDestroy()
        disconnect()
        sessionDataListener?.let {
            database.removeEventListener(it)
        }
    }

    private fun disconnect() {
        try {
            m_bluetoothSocket?.close()
            m_bluetoothSocket = null
            m_isConnected = false
            Toast.makeText(this, "Bluetooth Disconnected", Toast.LENGTH_SHORT).show()
        } catch (e: IOException) {
            e.printStackTrace()
        }
        finish() // Destroy SurfActivity and navigate back
    }


    private fun retrieveLatestSessionData() {
        val currentDate = SimpleDateFormat("yyyy-MM-dd", Locale.getDefault()).format(Date())
        val sessionsRef = database.child("users").child(userId).child("Surf Sessions").child(currentDate)

        sessionDataListener?.let { sessionsRef.removeEventListener(it) }

        sessionDataListener = sessionsRef.orderByKey().limitToLast(1).addValueEventListener(object : ValueEventListener {
            override fun onDataChange(snapshot: DataSnapshot) {
                if (snapshot.exists()) {
                    Toast.makeText(this@SurfActivity, "Data found for today", Toast.LENGTH_SHORT).show()

                    // Since we're limiting to the last one, this loop should only run once
                    for (sessionSnapshot in snapshot.children) {
                        val sessionData = sessionSnapshot.value as? Map<String, Any> ?: continue

                        val mspeed = sessionData["maxSpeed"]?.toString() ?: "0"
                        val nwaves = sessionData["numberOfWaves"]?.toString() ?: "0"
                        val tdistance = sessionData["totalWaveDistance"]?.toString() ?: "0"
                        val duration = sessionData["sessionDuration"]?.toString() ?: "0"
                        val temperature = sessionData["temperature"]?.toString() ?: "0"
                        val mwdistance = sessionData["maxWaveDistance"]?.toString() ?: "0"

                        updateParameterValues(mspeed, nwaves, tdistance, duration, temperature, mwdistance)
                    }
                } else {
                    Toast.makeText(this@SurfActivity, "No sessions found for today", Toast.LENGTH_SHORT).show()
                    Log.d("Firebase", "No sessions found for today.")
                }
            }

            override fun onCancelled(error: DatabaseError) {
                Log.e("Firebase", "Error retrieving session data: ${error.message}")
                Toast.makeText(this@SurfActivity, "Error retrieving data", Toast.LENGTH_SHORT).show()
            }
        })
    }


    private fun updateParameterValues(
        mspeed: String, nwaves: String, tdistance: String, duration: String, temperature: String, mwdistance: String
    ) {
        lastMspeed = mspeed
        lastNwaves = nwaves
        lastTdistance = tdistance
        lastDuration = duration
        lastTemperature = temperature
        mwaveDistance = mwdistance

        temperatureParameterValue.text = lastTemperature
        findViewById<TextView>(R.id.biomech_value).text = getCurrentParameterValue()
    }

    private fun getCurrentParameterValue(): String {
        return when (biomechanicParameters[currentParameterIndex]) {
            "Maximum Speed" -> {
                try {
                    "%.1f".format(lastMspeed.toDouble())
                } catch (e: NumberFormatException) {
                    "0.0" // Default to 0.0 if parsing fails
                }
            }
            "Total Distance" -> {
                try {
                    "%.0f".format(lastTdistance.toDouble()) // No decimal places
                } catch (e: NumberFormatException) {
                    "0" // Default to 0 if parsing fails
                }
            }
            "Max Wave Distance" -> {
                try {
                    "%.0f".format(mwaveDistance.toDouble()) // No decimal places
                } catch (e: NumberFormatException) {
                    "0" // Default to 0 if parsing fails
                }
            }
            "Session Duration" -> lastDuration
            "Number of Waves" -> lastNwaves
            else -> "N/A"
        }
    }


    private fun updateParameterDisplay() {
        // Update parameter name, value, and unit
        val biomechanicParameterName = findViewById<TextView>(R.id.BiomechanicParameter)
        biomechanicParameterName.text = biomechanicParameters[currentParameterIndex]
        biomechanicParameterValue.text = getCurrentParameterValue()
        biomechanicParameterUnit.text = biomechanicUnits[currentParameterIndex]
    }

    private fun sendCommand(command: String) {
        if (command == "W" && !isReceiving) {
            startDataStream()
        } else if (m_bluetoothSocket != null && m_bluetoothSocket!!.isConnected) {
            try {
                m_bluetoothSocket!!.outputStream.write(command.toByteArray())
                Log.d("BluetoothCommand", "Command Sent: $command")
            } catch (e: IOException) {
                Log.e("BluetoothCommand", "Error sending command", e)
            }
        } else {
            Log.w("BluetoothCommand", "Not connected to device")
        }
    }

    private fun startDataStream() {
        if (m_bluetoothSocket != null && m_bluetoothSocket!!.isConnected) {
            isReceiving = true
            Thread {
                try {
                    while (isReceiving && m_bluetoothSocket!!.isConnected) {
                        m_bluetoothSocket!!.outputStream.write("W".toByteArray())
                        Log.d("BluetoothCommand", "Command Sent: W")
                        receiveContinuousResponse()
                        Thread.sleep(500)
                    }
                } catch (e: IOException) {
                    Log.e("BluetoothStream", "Error in continuous data stream", e)
                    isReceiving = false
                }
            }.start()
        } else {
            Log.w("BluetoothStream", "Not connected to device")
        }
    }

    private fun receiveContinuousResponse() {
        try {
            val inputStream = m_bluetoothSocket!!.inputStream
            val buffer = ByteArray(1024)

            while (inputStream.available() > 0) {
                val bytesRead = inputStream.read(buffer)
                if (bytesRead <= 0) {
                    Log.w("BluetoothResponse", "No data received, stopping flush")
                    isReceiving = false
                    break
                }

                val response = String(buffer, 0, bytesRead).trim()
                Log.d("BluetoothResponse", "Response: $response")

                // Check if the response contains "End" to stop receiving data
                if (response.contains("End", ignoreCase = true)) {
                    Log.i("BluetoothResponse", "End message received, stopping flush")
                    isReceiving = false
                    break
                }

                // Check if the response is empty or null, and stop if it is
                if (response.isEmpty()) {
                    Log.w("BluetoothResponse", "Received empty response, stopping flush")
                    isReceiving = false
                    break
                }

                // Process the response data line
                processResponseData(response)
            }

            // Show a toast message after flush is over
            runOnUiThread {
                Toast.makeText(this, "Sync completed", Toast.LENGTH_SHORT).show()
            }

        } catch (e: IOException) {
            Log.e("BluetoothResponse", "Error receiving response", e)
            isReceiving = false  // Reset the flag to allow new transmissions if disconnected
        }
    }


    private fun processResponseData(response: String) {
        val dataParts = response.split(";")
        if (dataParts.size >= 8) {
            val date = dataParts[0].trim()
            val time = dataParts[1].trim()
            val duration = dataParts[2].trim()
            val nwaves = dataParts[4].trim()
            val mwaveDistance = dataParts[6].trim()
            val tdistance = dataParts[7].trim()
            val mspeed = dataParts[9].trim()
            val temperature = dataParts[10].trim()
            sendDataToFirebase(date, time, duration, nwaves, mwaveDistance, tdistance, mspeed, temperature)
        } else {
            Log.w("processResponseData", "Unexpected data format: $response")
        }
    }

    private fun sendDataToFirebase(
        date: String,
        time: String,
        duration: String,
        nwaves: String,
        mwaveDistance: String,
        tdistance: String,
        mspeed: String,
        temperature: String
    ) {
        val dateRef = database.child("users").child(userId).child("Surf Sessions").child(date)
        val sessionKey = time  // Use only the time as the session key

        val data = mapOf(
            "time" to time,
            "sessionDuration" to duration,
            "numberOfWaves" to nwaves,
            "maxWaveDistance" to mwaveDistance,
            "totalWaveDistance" to tdistance,
            "maxSpeed" to mspeed,
            "temperature" to temperature
        )

        dateRef.child(sessionKey).setValue(data)
            .addOnSuccessListener {
                Log.d("Firebase", "Session data saved successfully for $sessionKey")
            }
            .addOnFailureListener { e ->
                Log.e("Firebase", "Failed to save session data", e)
            }
    }


    private class ConnectToDevice(
        private val context: Context,
        private val disconnectButton: ToggleButton,
        private val switchModeButton: Button
    ) : AsyncTask<Void, Void, String>() {
        private var connectSuccess: Boolean = true

        override fun onPreExecute() {
            super.onPreExecute()
            SurfActivity.m_progress = ProgressDialog.show(context, "Connecting...", "Please Wait")
        }

        override fun doInBackground(vararg p0: Void?): String? {
            try {
                if (SurfActivity.m_bluetoothSocket == null || !SurfActivity.m_isConnected) {
                    SurfActivity.m_bluetoothAdapter = BluetoothAdapter.getDefaultAdapter()
                    val device: BluetoothDevice = SurfActivity.m_bluetoothAdapter.getRemoteDevice(
                        SurfActivity.m_address
                    )

                    if (SurfActivity.m_bluetoothSocket != null && SurfActivity.m_bluetoothSocket!!.isConnected) {
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
                    SurfActivity.m_bluetoothSocket = device.createInsecureRfcommSocketToServiceRecord(
                        SurfActivity.m_myUUID
                    )
                    SurfActivity.m_bluetoothSocket!!.connect()
                }
            } catch (e: IOException) {
                connectSuccess = false
                (context as Activity).runOnUiThread {
                    Toast.makeText(context, "Turn on the device", Toast.LENGTH_SHORT).show()
                }
                e.printStackTrace()
            }
            return null
        }

        override fun onPostExecute(result: String?) {
            super.onPostExecute(result)
            SurfActivity.m_progress.dismiss()

            if (connectSuccess) {
                SurfActivity.m_isConnected = true
                disconnectButton.visibility = View.VISIBLE
                switchModeButton.text = "Sync"
            } else {
                SurfActivity.m_isConnected = false
                disconnectButton.visibility = View.GONE
                switchModeButton.text = "Offline"
                Toast.makeText(context, "Running in offline mode.", Toast.LENGTH_SHORT).show()
            }
        }
    }
}
