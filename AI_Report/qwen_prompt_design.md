# Qwen AI Health Report Prompt Design

This file extracts and整理 the AI prompt logic from `HealthReportActivity.java`.

## Data input format

The App loads historical health data from MySQL through `JDBCUtil`, then passes it to the Qwen model.

Data columns:

```text
时间, 体温(℃), 心率(bpm), 血氧(%), 步数, 是否摔倒, 报警信息
```

## Prompt template

```text
你是一位专业健康管理师。请根据以下用户健康监测历史数据，分析其整体健康状况，并给出具体、实用的健康建议。

数据格式：时间, 体温(℃), 心率(bpm), 血氧(%), 步数, 是否摔倒, 报警信息

[这里拼接历史健康数据]

请用中文分点回答，包括：
⚫ 整体健康评估；
⚠ 异常情况总结；
📖 具体生活或医疗建议；
🏥 是否需要就医或进一步检查。

注意：不要做疾病确诊，不要建议用户自行调整药物剂量。本报告仅作为健康管理参考。
```

## API

The extracted Android code uses DashScope compatible OpenAI API endpoint:

```text
https://dashscope.aliyuncs.com/compatible-mode/v1/chat/completions
```

Model:

```text
qwen-plus
```

## Security note

The original PDF contained an API key in the code block. It has been removed and replaced with `YOUR_DASHSCOPE_API_KEY` in the extracted Java file.
Do not upload real API keys to GitHub.
