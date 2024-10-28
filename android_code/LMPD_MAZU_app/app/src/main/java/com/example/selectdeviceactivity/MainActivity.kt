package com.example.selectdeviceactivity

import android.Manifest
import android.app.Activity
import android.bluetooth.BluetoothAdapter
import android.bluetooth.BluetoothDevice
import android.content.Context
import android.content.pm.PackageManager
import android.os.Bundle
import android.util.Log
import android.widget.Toast
import android.widget.Button
import android.content.Intent
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.AdapterView
import android.widget.ArrayAdapter
import android.widget.ListView
import android.widget.TextView
import androidx.activity.result.contract.ActivityResultContracts

import androidx.appcompat.app.AppCompatActivity
import androidx.core.app.ActivityCompat
import androidx.core.splashscreen.SplashScreen.Companion.installSplashScreen
import com.google.firebase.auth.FirebaseAuth
import com.google.firebase.auth.ktx.auth
import com.google.firebase.ktx.Firebase

class MainActivity : AppCompatActivity() {

    private var m_bluetoothAdapter: BluetoothAdapter? = null
    private lateinit var m_pairedDevices: Set<BluetoothDevice>
    private lateinit var auth: FirebaseAuth
    private val REQUEST_ENABLE_BLUETOOTH = 1
    private val PERMISSION_REQUEST_BLUETOOTH = 1
    lateinit var userId: String
    private lateinit var buttonOut: Button




    companion object {
        val EXTRA_ADDRESS: String = "Device_address"
    }

    override fun onCreate(savedInstanceState: Bundle?) {

        super.onCreate(savedInstanceState)
        supportActionBar?.hide()
        //installSplashScreen()
        setContentView(R.layout.activity_main)
        userId = intent.getStringExtra("userId").toString()

        buttonOut = findViewById(R.id.btn_logout);
        auth = Firebase.auth

        buttonOut.setOnClickListener{
            auth.signOut()
            val intent = Intent(this, Login::class.java)
            startActivity(intent)
            finish()
        }


        m_bluetoothAdapter = BluetoothAdapter.getDefaultAdapter()
        if (m_bluetoothAdapter == null) {
            Toast.makeText(this, "Device doesn't support Bluetooth", Toast.LENGTH_LONG).show()
            return
        }
        if (!m_bluetoothAdapter!!.isEnabled) {
            val enableBluetoothIntent = Intent(Intent(BluetoothAdapter.ACTION_REQUEST_ENABLE))
            //startActivityForResult(enableBluetoothIntent, REQUEST_ENABLE_BLUETOOTH)
            //requestPermissions(applicationContext, arrayOf(Manifest.permission.REQUESTED_PERMISSION), REQUEST_CODE)
            requestPermissions(
                arrayOf(Manifest.permission.BLUETOOTH_CONNECT),
                REQUEST_ENABLE_BLUETOOTH
            )
            if (ActivityCompat.checkSelfPermission(
                    this,
                    Manifest.permission.BLUETOOTH_CONNECT
                ) != PackageManager.PERMISSION_GRANTED
            ) {
                // TODO: Consider calling
                //    ActivityCompat#requestPermissions
                // here to request the missing permissions, and then overriding
                //   public void onRequestPermissionsResult(int requestCode, String[] permissions,
                //                                          int[] grantResults)
                // to handle the case where the user grants the permission. See the documentation
                // for ActivityCompat#requestPermissions for more details.
                return
            }
        }
        val selectDeviceRefreshButton = findViewById<Button>(R.id.select_device_refresh)
        selectDeviceRefreshButton.setOnClickListener { pairedDeviceList() }

    }

        val requestPermissionLauncher =
        registerForActivityResult(
            ActivityResultContracts.RequestPermission()
        ) { isGranted: Boolean ->
            if (isGranted) {
                // Permission is granted. Continue the action or workflow in your
                // app.
            } else {
                // Explain to the user that the feature is unavailable because the
                // feature requires a permission that the user has denied. At the
                // same time, respect the user's decision. Don't link to system
                // settings in an effort to convince the user to change their
                // decision.
            }
        }

    private fun pairedDeviceList() {
        if (ActivityCompat.checkSelfPermission(this, Manifest.permission.BLUETOOTH_CONNECT) != PackageManager.PERMISSION_GRANTED) {
            ActivityCompat.requestPermissions(this, arrayOf(Manifest.permission.BLUETOOTH_CONNECT), REQUEST_ENABLE_BLUETOOTH)
            return
        }

        m_pairedDevices = m_bluetoothAdapter!!.bondedDevices
        val list: ArrayList<String> = ArrayList() // Stores device names

        if (m_pairedDevices.isNotEmpty()) {
            for (device: BluetoothDevice in m_pairedDevices) {
                Log.i("device", "Checking device: ${device.name}, Address: ${device.address}")

                if (device.name == "VARUNA5" || device.name == "MAZU2") {
                    if (isDeviceTurnedOn(device.address)) {
                        // Device is turned on, add its name to the list
                        Toast.makeText(this, "${device.name} Module Paired", Toast.LENGTH_SHORT).show()
                        list.add(device.name) // Add device name to the list
                    } else {
                        // Device is not turned on, show a toast and log
                        Toast.makeText(this, "${device.name} is not turned on", Toast.LENGTH_SHORT).show()
                        Log.i("device", "${device.name} is not turned on: ${device.address}")
                    }
                }
            }
        } else {
            Toast.makeText(this, "No paired devices found", Toast.LENGTH_SHORT).show()
        }

        // Set up ListView adapter
        val adapter = ArrayAdapter<String>(this, R.layout.list_item_device, R.id.device_name_text, list)
        val listView: ListView = findViewById(R.id.select_device_list)
        listView.adapter = adapter

        // Handle device selection
        listView.onItemClickListener = AdapterView.OnItemClickListener { _, _, position, _ ->
            val deviceName: String = list[position] // Get the device name from the list
            val device = m_pairedDevices.find { it.name == deviceName } // Find device by name
            val address: String = device?.address ?: ""

            // Determine which activity to start based on the device name
            val targetActivityClass = when (deviceName) {
                "VARUNA5" -> QualityActivity::class.java
                "MAZU2" -> SurfActivity::class.java
                else -> null // Handle other cases or unknown device names
            }

            // Start the target activity if determined
            targetActivityClass?.let {
                val intent = Intent(this, it)
                intent.putExtra(EXTRA_ADDRESS, address)
                intent.putExtra("userId", userId)
                startActivity(intent)
            } ?: run {
                // Handle the case where no matching activity is found for the device name
                Toast.makeText(this, "Unknown device selected", Toast.LENGTH_SHORT).show()
            }
        }
    }

    // Improved method to check if the device is turned on (bonded and Bluetooth is enabled)
    private fun isDeviceTurnedOn(deviceAddress: String): Boolean {
        val bluetoothAdapter = BluetoothAdapter.getDefaultAdapter()
        val device = bluetoothAdapter.getRemoteDevice(deviceAddress)

        // Ensure proper permissions
        return if (ActivityCompat.checkSelfPermission(this, Manifest.permission.BLUETOOTH) != PackageManager.PERMISSION_GRANTED) {
            ActivityCompat.requestPermissions(this, arrayOf(Manifest.permission.BLUETOOTH), PERMISSION_REQUEST_BLUETOOTH)
            false
        } else {
            // Check if the device is bonded and Bluetooth is enabled
            device != null && device.bondState == BluetoothDevice.BOND_BONDED && bluetoothAdapter.isEnabled
        }
    }


    override fun onActivityResult(requestCode: Int, resultCode: Int, data: Intent?) {
        super.onActivityResult(requestCode, resultCode, data)

        if (requestCode == REQUEST_ENABLE_BLUETOOTH) {
            if (resultCode == Activity.RESULT_OK) {
                if (m_bluetoothAdapter!!.isEnabled) {
                    Toast.makeText(this, "Bluetooth has been enable", Toast.LENGTH_SHORT).show()
                } else {
                    Toast.makeText(this, "Bluetooth has been disabled", Toast.LENGTH_SHORT).show()
                }
            } else if (resultCode == Activity.RESULT_CANCELED) {
                Toast.makeText(this, "Bluetooth enable has been canceled", Toast.LENGTH_SHORT)
                    .show()
            }
        }
    }

}