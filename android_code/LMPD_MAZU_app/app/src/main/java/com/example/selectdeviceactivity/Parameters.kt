package com.example.selectdeviceactivity

class Parameters {
    companion object {
        fun <T : Number> processValues(response: String, command: String, minTemperature: T, maxTemperature: T): Pair<Float, Int> {
            val cleaned = response.replace(command, "")
            val floatValue = cleaned.toFloatOrNull() ?: 0.0f
            val minTemp = minTemperature.toFloat()
            val maxTemp = maxTemperature.toFloat()
            val progressValue = ((floatValue - minTemp) * 100 / (maxTemp - minTemp)).toInt()
            return Pair(floatValue, progressValue)
        }
    }
    var temperature: Float = 0.0f
    var pH: Float = 0.0f
    var tds: Int = 0
    var Do: Float = 0.0f
    lateinit var type: String
    lateinit var turb: String

    var temperatureProgress: Int = 0
    var pHProgress: Int = 0
    var tdsProgress: Int = 0
    var DoProgress: Int = 0
    var turbProgress: Int = 0

    // Add properties for other parameters and their progress values as needed

    fun updateTemperature(value: Float, progress: Int) {
        temperature = value
        temperatureProgress = progress
    }

    fun updatePH(value: Float, progress: Int) {
        pH = value
        pHProgress = progress
    }

    fun updateTDS(value: Int, progress: Int) {
        tds = value
        tdsProgress = progress
    }

    fun updateDO(value: Float, progress: Int) {
        Do = value
        DoProgress = progress
    }

    fun updateTURB(value: String) {
        turb = value
    }

    fun updateType(value: String) {
        type = value
    }
    // Add methods to update other parameters and their progress values as needed
}
