package io.github.yuubinnkyoku.hexagonfluid

import android.content.Intent
import android.os.Build
import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.compose.foundation.horizontalScroll
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.verticalScroll
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.text.selection.SelectionContainer
import androidx.compose.material3.Button
import androidx.compose.material3.Card
import androidx.compose.material3.FilterChip
import androidx.compose.material3.LinearProgressIndicator
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.OutlinedButton
import androidx.compose.material3.Scaffold
import androidx.compose.material3.Surface
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableIntStateOf
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.rememberCoroutineScope
import androidx.compose.runtime.setValue
import androidx.compose.ui.Modifier
import androidx.compose.ui.unit.dp
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext
import org.json.JSONArray
import org.json.JSONObject
import java.text.SimpleDateFormat
import java.util.Date
import java.util.Locale
import java.util.TimeZone

private object FluidNative {
    init { System.loadLibrary("hfs_jni") }
    external fun run(size: Int, maxIterations: Int, tolerance: Double): String
}

private data class BenchmarkReport(val description: String, val json: String)

private fun measure(size: Int, maxIterations: Int, repeats: Int): BenchmarkReport {
    // First run is unrecorded warmup. Numerical RHS construction is not timed.
    val samples = JSONArray()
    val times = mutableListOf<Double>()
    var converged = true
    var residual = Double.NaN
    var error = Double.NaN
    var iterations = 0
    repeat(repeats + 1) { index ->
        val result = JSONObject(FluidNative.run(size, maxIterations, 1e-4))
        if (index != 0) {
            samples.put(result)
            times.add(result.getDouble("solver_wall_time_ms"))
            converged = converged && result.getBoolean("converged")
            residual = result.getDouble("relative_residual")
            error = result.getDouble("max_abs_pressure_error")
            iterations = result.getInt("iterations")
        }
    }
    val sorted = times.sorted()
    val middle = sorted.size / 2
    val median = if (sorted.size % 2 == 1) sorted[middle]
                 else (sorted[middle - 1] + sorted[middle]) / 2.0
    val timestamp = SimpleDateFormat("yyyy-MM-dd'T'HH:mm:ss'Z'", Locale.US).run {
        timeZone = TimeZone.getTimeZone("UTC")
        format(Date())
    }
    val device = JSONObject()
        .put("manufacturer", Build.MANUFACTURER)
        .put("model", Build.MODEL)
        .put("device", Build.DEVICE)
        .put("android_sdk", Build.VERSION.SDK_INT)
        .put("android_release", Build.VERSION.RELEASE)
        .put("soc_model", if (Build.VERSION.SDK_INT >= 31) Build.SOC_MODEL else "unknown")
        .put("abi", Build.SUPPORTED_ABIS.firstOrNull() ?: "unknown")
    val report = JSONObject()
        .put("project", "hexagon-fluid-simulation")
        .put("timestamp_utc", timestamp)
        .put("device", device)
        .put("backend", "cpu_jacobi")
        .put("grid", size)
        .put("max_iterations", maxIterations)
        .put("relative_tolerance", 1e-4)
        .put("warmups", 1)
        .put("runs", repeats)
        .put("median_solver_wall_time_ms", median)
        .put("all_converged", converged)
        .put("samples", samples)
    fun display(value: Double) = String.format(Locale.US, "%.6g", value)
    val description = buildString {
        appendLine("CPU Jacobi / " + size + " × " + size)
        appendLine("測定回数: " + repeats + "回（事前に1回実行）")
        appendLine("実行時間の中央値: " + display(median) + " ms")
        appendLine("最短: " + display(sorted.first()) + " ms")
        appendLine("最長: " + display(sorted.last()) + " ms")
        appendLine("反復回数: " + iterations)
        appendLine("最後の相対残差: " + display(residual))
        appendLine("最後の圧力最大誤差: " + display(error))
        appendLine("全測定で収束: " + (if (converged) "はい" else "いいえ"))
        if (!converged) {
            appendLine()
            appendLine("注意: 収束していません。同じ精度での速度比較には使えません。")
        }
        appendLine()
        appendLine("処理時間は端末内C++の計算時間です。描画や起動は含みません。")
    }
    return BenchmarkReport(description, report.toString(2))
}

class MainActivity : ComponentActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContent {
            MaterialTheme {
                Surface {
                    BenchScreen(onShare = { report ->
                        val send = Intent(Intent.ACTION_SEND).apply {
                            type = "application/json"
                            putExtra(Intent.EXTRA_TEXT, report)
                        }
                        startActivity(Intent.createChooser(send, "測定結果を共有"))
                    })
                }
            }
        }
    }
}

@Composable
private fun BenchScreen(onShare: (String) -> Unit) {
    var size by remember { mutableIntStateOf(32) }
    var maxIterations by remember { mutableIntStateOf(10000) }
    var repeats by remember { mutableIntStateOf(5) }
    var busy by remember { mutableStateOf(false) }
    var message by remember { mutableStateOf("測定を開始すると、ここに結果が表示されます。") }
    var json by remember { mutableStateOf<String?>(null) }
    val scope = rememberCoroutineScope()

    Scaffold { padding ->
        Column(
            modifier = Modifier.fillMaxSize().padding(padding)
                .verticalScroll(rememberScrollState()).padding(20.dp),
            verticalArrangement = Arrangement.spacedBy(14.dp)
        ) {
            Text("Hexagon Fluid Lab", style = MaterialTheme.typography.headlineMedium)
            Text("圧力ポアソン方程式の性能測定", style = MaterialTheme.typography.titleMedium)
            Card {
                Column(Modifier.padding(16.dp), verticalArrangement = Arrangement.spacedBy(8.dp)) {
                    Text("現在の計算装置: CPU（C++ / FP32）")
                    Text("HVX・HMXは未実装です。ここで得る値を比較基準にします。")
                }
            }
            Text("格子の大きさ", style = MaterialTheme.typography.titleMedium)
            ChoiceRow(listOf(16, 32, 64, 128), size, !busy) { size = it }
            Text("反復回数の上限", style = MaterialTheme.typography.titleMedium)
            ChoiceRow(listOf(1000, 5000, 10000, 20000), maxIterations, !busy) {
                maxIterations = it
            }
            Text("測定回数", style = MaterialTheme.typography.titleMedium)
            ChoiceRow(listOf(3, 5, 10), repeats, !busy) { repeats = it }

            Button(
                onClick = {
                    busy = true
                    json = null
                    message = "計算中…"
                    val runSize = size
                    val runIterations = maxIterations
                    val runRepeats = repeats
                    scope.launch {
                        try {
                            val result = withContext(Dispatchers.Default) {
                                measure(runSize, runIterations, runRepeats)
                            }
                            message = result.description
                            json = result.json
                        } catch (e: Exception) {
                            message = "測定エラー: " + (e.message ?: e.javaClass.simpleName)
                        } finally {
                            busy = false
                        }
                    }
                },
                enabled = !busy,
                modifier = Modifier.fillMaxWidth()
            ) {
                Text(if (busy) "測定中" else "CPU性能を測定")
            }
            if (busy) LinearProgressIndicator(modifier = Modifier.fillMaxWidth())
            Card {
                Column(Modifier.padding(16.dp)) {
                    Text("測定結果", style = MaterialTheme.typography.titleMedium)
                    Spacer(Modifier.height(10.dp))
                    SelectionContainer { Text(message) }
                }
            }
            OutlinedButton(
                onClick = { json?.let(onShare) },
                enabled = !busy && json != null,
                modifier = Modifier.fillMaxWidth()
            ) {
                Text("JSON形式で結果を共有")
            }
            Text(
                "現在は流体の描画を行いません。圧力計算だけの時間と精度を測定する試作アプリです。",
                style = MaterialTheme.typography.bodySmall
            )
        }
    }
}

@Composable
private fun ChoiceRow(
    choices: List<Int>,
    selected: Int,
    enabled: Boolean,
    onSelected: (Int) -> Unit
) {
    Row(
        Modifier.horizontalScroll(rememberScrollState()),
        horizontalArrangement = Arrangement.spacedBy(8.dp)
    ) {
        choices.forEach { value ->
            FilterChip(
                selected = value == selected,
                onClick = { onSelected(value) },
                label = { Text(value.toString()) },
                enabled = enabled
            )
        }
    }
}
