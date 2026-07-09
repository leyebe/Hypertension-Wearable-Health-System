package com.example.aliyunapplication;

import android.os.AsyncTask;
import android.os.Bundle;
import android.widget.TableLayout;
import android.widget.TableRow;
import android.widget.TextView;
import android.widget.Toast;

import androidx.appcompat.app.AppCompatActivity;

import com.google.gson.Gson;

import java.io.IOException;
import java.util.List;

import okhttp3.MediaType;
import okhttp3.OkHttpClient;
import okhttp3.Request;
import okhttp3.RequestBody;
import okhttp3.Response;

/**
 * HealthReportActivity
 *
 * Extracted and cleaned from the project PDF.
 * Function:
 * 1. Load health monitoring history from MySQL through JDBCUtil.
 * 2. Display historical data in a table.
 * 3. Call DashScope Qwen API to generate an AI health report.
 *
 * Security note:
 * - The API key found in the PDF has been removed and replaced with a placeholder.
 * - In a real project, do not store API keys directly in Android source code.
 *   Use a backend service, encrypted storage, or BuildConfig with proper protection.
 */
@SuppressWarnings("deprecation")
public class HealthReportActivity extends AppCompatActivity {

    private TableLayout tableLayout;
    private TextView analysisTextView;

    // TODO: Replace with a safe key loading mechanism. Do not hardcode real keys in production.
    private static final String API_KEY = "YOUR_DASHSCOPE_API_KEY";
    private static final String BASE_URL = "https://dashscope.aliyuncs.com/compatible-mode/v1/chat/completions";

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_health_report);

        tableLayout = findViewById(R.id.data_table);
        analysisTextView = findViewById(R.id.analysis_text);

        new LoadDataTask().execute();
    }

    /**
     * Load health log data from database and render table.
     */
    private class LoadDataTask extends AsyncTask<Void, Void, List<String[]>> {
        @Override
        protected List<String[]> doInBackground(Void... voids) {
            try {
                // In the original PDF this method is named getPetLogData().
                // For this health project, it is better to rename it to getHealthLogData().
                return JDBCUtil.getInstance().getPetLogData();
            } catch (Exception e) {
                e.printStackTrace();
                return null;
            }
        }

        @Override
        protected void onPostExecute(List<String[]> data) {
            if (data == null || data.isEmpty()) {
                Toast.makeText(HealthReportActivity.this, "暂无健康数据", Toast.LENGTH_SHORT).show();
                analysisTextView.setText("暂无数据，无法生成分析报告。");
                return;
            }

            tableLayout.removeAllViews();

            TableRow headerRow = new TableRow(HealthReportActivity.this);
            String[] headers = {"时间", "体温(℃)", "心率(bpm)", "血氧(%)", "步数", "是否摔倒", "报警信息"};
            for (String header : headers) {
                TextView tv = new TextView(HealthReportActivity.this);
                tv.setText(header);
                tv.setPadding(8, 8, 8, 8);
                tv.setBackgroundResource(R.drawable.table_header_bg);
                tv.setTextColor(getResources().getColor(android.R.color.white));
                tv.setMinWidth(100);
                headerRow.addView(tv);
            }
            tableLayout.addView(headerRow);

            for (String[] row : data) {
                TableRow tableRow = new TableRow(HealthReportActivity.this);
                for (String cell : row) {
                    TextView tv = new TextView(HealthReportActivity.this);
                    tv.setText(cell);
                    tv.setPadding(8, 8, 8, 8);
                    tv.setBackgroundResource(R.drawable.table_cell_bg);
                    tv.setMinWidth(100);
                    tableRow.addView(tv);
                }
                tableLayout.addView(tableRow);
            }

            new GenerateAIHealthReportTask().execute(data);
        }
    }

    /**
     * Call DashScope Qwen model and generate AI health suggestions.
     */
    private class GenerateAIHealthReportTask extends AsyncTask<List<String[]>, Void, String> {
        @Override
        protected void onPreExecute() {
            analysisTextView.setText("正在分析健康数据，生成 AI 健康建议...");
        }

        @Override
        protected String doInBackground(List<String[]>... params) {
            List<String[]> data = params[0];

            StringBuilder promptBuilder = new StringBuilder();
            promptBuilder.append("你是一位专业健康管理师。请根据以下用户健康监测历史数据，")
                    .append("分析其整体健康状况，并给出具体、实用的健康建议。\n\n");
            promptBuilder.append("数据格式：时间, 体温(℃), 心率(bpm), 血氧(%), 步数, 是否摔倒, 报警信息\n");

            for (String[] row : data) {
                if (row.length >= 7) {
                    promptBuilder.append(String.join(", ", row)).append("\n");
                }
            }

            promptBuilder.append("\n请用中文分点回答，包括：\n");
            promptBuilder.append("⚫ 整体健康评估；\n");
            promptBuilder.append("⚠ 异常情况总结；\n");
            promptBuilder.append("📖 具体生活或医疗建议；\n");
            promptBuilder.append("🏥 是否需要就医或进一步检查。\n");
            promptBuilder.append("\n注意：不要做疾病确诊，不要建议用户自行调整药物剂量。本报告仅作为健康管理参考。\n");

            String prompt = promptBuilder.toString();

            String jsonBody = "{"
                    + "\"model\": \"qwen-plus\","
                    + "\"messages\": [{\"role\": \"user\", \"content\": \"" + escapeJson(prompt) + "\"}],"
                    + "\"max_tokens\": 600"
                    + "}";

            OkHttpClient client = new OkHttpClient.Builder()
                    .connectTimeout(10, java.util.concurrent.TimeUnit.SECONDS)
                    .readTimeout(25, java.util.concurrent.TimeUnit.SECONDS)
                    .writeTimeout(10, java.util.concurrent.TimeUnit.SECONDS)
                    .build();

            MediaType JSON = MediaType.get("application/json; charset=utf-8");
            RequestBody body = RequestBody.create(jsonBody, JSON);
            Request request = new Request.Builder()
                    .url(BASE_URL)
                    .addHeader("Authorization", "Bearer " + API_KEY)
                    .addHeader("Content-Type", "application/json")
                    .post(body)
                    .build();

            try (Response response = client.newCall(request).execute()) {
                if (response.isSuccessful() && response.body() != null) {
                    String responseBody = response.body().string();
                    Gson gson = new Gson();
                    ChatResponse chatResponse = gson.fromJson(responseBody, ChatResponse.class);
                    if (chatResponse.choices != null && chatResponse.choices.length > 0) {
                        return chatResponse.choices[0].message.content.trim();
                    }
                } else {
                    return "AI 服务返回错误：" + response.code() + " - " + response.message();
                }
            } catch (IOException e) {
                e.printStackTrace();
                return "网络请求失败：" + e.getMessage();
            }

            return "未能生成 AI 健康建议，请稍后重试。";
        }

        @Override
        protected void onPostExecute(String suggestion) {
            analysisTextView.setText(suggestion);
        }

        private String escapeJson(String input) {
            if (input == null) return "";
            return input.replace("\\", "\\\\")
                    .replace("\"", "\\\"")
                    .replace("\n", "\\n")
                    .replace("\r", "\\r")
                    .replace("\t", "\\t");
        }
    }

    /**
     * Response model for DashScope compatible OpenAI API.
     */
    static class ChatResponse {
        Choice[] choices;

        static class Choice {
            Message message;
        }

        static class Message {
            String content;
        }
    }
}
