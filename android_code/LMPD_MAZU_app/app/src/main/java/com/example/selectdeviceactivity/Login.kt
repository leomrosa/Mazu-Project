package com.example.selectdeviceactivity

import android.annotation.SuppressLint
import android.content.Intent
import androidx.appcompat.app.AppCompatActivity
import android.os.Bundle
import android.text.TextUtils
import android.util.Log
import android.view.View
import android.widget.Button
import android.widget.ProgressBar
import android.widget.TextView
import android.widget.Toast
import com.google.android.material.textfield.TextInputEditText
import com.google.firebase.auth.FirebaseAuth
import com.google.firebase.auth.ktx.auth
import com.google.firebase.database.DatabaseReference
import com.google.firebase.database.FirebaseDatabase
import com.google.firebase.database.ktx.database
import com.google.firebase.ktx.Firebase

class Login : AppCompatActivity() {

    private lateinit var editTextEmail: TextInputEditText
    private lateinit var editTextPassword: TextInputEditText
    private lateinit var buttonLog: Button
    private lateinit var buttonReg: Button
    private lateinit var progressBar: ProgressBar
    private lateinit var auth: FirebaseAuth
    private lateinit var database: DatabaseReference
    public lateinit var userId: String
// ...
    public override fun onStart() {
        super.onStart()
        // Check if user is signed in (non-null) and update UI accordingly.
        val currentUser = auth.currentUser

        if (currentUser != null) {
            userId = currentUser?.uid.toString()
            Log.d("IDUSER", " ${userId}")
            val intent = Intent(this, MainActivity::class.java)
            intent.putExtra("userId", userId)
            startActivity(intent)
            finish()
        }
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_login)

        database = FirebaseDatabase.getInstance().reference
        auth = Firebase.auth

        editTextEmail = findViewById(R.id.etEmail)

        editTextPassword = findViewById(R.id.etPassword)

        buttonLog = findViewById(R.id.btn_login)

        buttonReg = findViewById(R.id.btn_register)

        progressBar = findViewById(R.id.progressBar)

        fun writeNewUser(userId: String, email : String, password: String) {
            val user = User(email, password)
            Log.d("USER_INFO", "UserID: $userId")
            Log.d("USER_INFO", "email: $email")
            Log.d("USER_INFO", "pass: $password")
            database.child("users").child(userId).setValue(user)
        }


        buttonReg.setOnClickListener {
            progressBar.visibility = View.VISIBLE
            val email = editTextEmail.text.toString()
            val password = editTextPassword.text.toString()

            if (TextUtils.isEmpty(email)) {
                progressBar.visibility = View.GONE
                Toast.makeText(this@Login, "Enter Email", Toast.LENGTH_SHORT).show()
                return@setOnClickListener
            }
            if (TextUtils.isEmpty(password)) {
                progressBar.visibility = View.GONE
                Toast.makeText(this@Login, "Enter Password", Toast.LENGTH_SHORT).show()
                return@setOnClickListener
            }
            auth.createUserWithEmailAndPassword(email, password)
                .addOnCompleteListener(this) { task ->
                    if (task.isSuccessful) {
                        // Sign in success, update UI with the signed-in user's information
                        Log.d("Create", "createUserWithEmail:success")
                        val user = auth.currentUser
                        userId = user?.uid.toString() // Obtain the unique userId
                        if(userId != null)
                        {
                            Log.d("Create", "userID:success")
                            writeNewUser(userId, email, password)
                        }
                        val intent = Intent(this, MainActivity::class.java)
                        startActivity(intent)
                    } else {
                        // If sign in fails, display a message to the user.
                        Log.w("Create", "createUserWithEmail:failure", task.exception)
                        Toast.makeText(
                            baseContext,
                            "Authentication failed.",
                            Toast.LENGTH_SHORT,
                        ).show()
                    }
                }
        }
        buttonLog.setOnClickListener {
            progressBar.visibility = View.VISIBLE
            val email = editTextEmail.text.toString()
            val password = editTextPassword.text.toString()

            if (TextUtils.isEmpty(email)) {
                progressBar.visibility = View.GONE
                Toast.makeText(this@Login, "Enter Email", Toast.LENGTH_SHORT).show()
                return@setOnClickListener
            }
            if (TextUtils.isEmpty(password)) {
                progressBar.visibility = View.GONE
                Toast.makeText(this@Login, "Enter Password", Toast.LENGTH_SHORT).show()
                return@setOnClickListener
            }

            auth.signInWithEmailAndPassword(email, password)
                .addOnCompleteListener(this) { task ->
                    progressBar.visibility = View.VISIBLE
                    if (task.isSuccessful) {
                        val user = auth.currentUser
                        userId = user?.uid.toString() // Obtain the unique userId
                        // Sign in success, update UI with the signed-in user's information
                        Toast.makeText(baseContext, "Login Successful.", Toast.LENGTH_SHORT,)
                            .show()
                        val intent = Intent(this, MainActivity::class.java)
                        intent.putExtra("userId", userId)
                        startActivity(intent)
                        finish()
                    } else {
                        // If sign in fails, display a message to the user.
                        Toast.makeText(this@Login, "Authentication failed.", Toast.LENGTH_SHORT,)
                            .show()
                    }
                }
        }
    }
}