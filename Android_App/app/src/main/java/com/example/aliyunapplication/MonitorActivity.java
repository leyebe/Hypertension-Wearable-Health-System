package com.example.aliyunapplication;

import android.content.Intent;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.util.Log;
import android.view.View;
import android.widget.EditText;
import android.widget.RadioButton;
import android.widget.RadioGroup;
import android.widget.TextView;
import android.widget.Toast;

import androidx.appcompat.app.AppCompatActivity;

import org.eclipse.paho.client.mqttv3.IMqttDeliveryToken;
import org.eclipse.paho.client.mqttv3.MqttCallback;
import org.eclipse.paho.client.mqttv3.MqttClient;
import org.eclipse.paho.client.mqttv3.MqttConnectOptions;
import org.eclipse.paho.client.mqttv3.MqttException;
import org.eclipse.paho.client.mqttv3.MqttMessage;
import org.eclipse.paho.client.mqttv3.persist.MemoryPersistence;
import org.json.JSONException;
import org.json.JSONObject;

import java.nio.charset.StandardCharsets;
import java.util.HashMap;
import java.util.Map;
import java.util.concurrent.Executors;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.TimeUnit;

/**
 * MonitorActivity
 *
 * Extracted and cleaned from the project PDF.
 * Function:
 * 1. Connect Android App to Aliyun IoT MQTT broker.
 * 2. Subscribe to device data topic and receive JSON health data.
 * 3. Display temperature, heart rate, SpO2, step count, fall status and warning status.
 * 4. Publish threshold settings/control commands to device topic.
 *
 * Notes:
 * - Replace productKey/deviceName/deviceSecret/pub_topic/sub_topic before running.
 * - R.id and R.layout names must match your Android XML layout files.
 * - This file depends on AliyunIoTSignUtil.sign().
 */
public class MonitorActivity extends AppCompatActivity {

    private MqttClient client;
    private MqttConnectOptions options;
    private Handler handler;
    private ScheduledExecutorService scheduler;

    private RadioGroup radioGroup;
    private RadioButton selectedRadioButton;
    private int statusCode4 = 0;

    // TODO: replace these topics with your actual Aliyun IoT topics.
    private final String pub_topic = "/your_product_key/your_device_name/user/update";
    private final String sub_topic = "/your_product_key/your_device_name/user/get";

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.monitor);

        mqtt_init();
        start_reconnect();

        handler = new Handler() {
            @Override
            public void handleMessage(Message msg) {
                super.handleMessage(msg);
                switch (msg.what) {
                    case 1:
                        // Boot check feedback. Not used in this demo.
                        break;

                    case 2:
                        // Device feedback. Not used in this demo.
                        break;

                    case 3:
                        // MQTT message arrived.
                        String message = msg.obj.toString();
                        Log.d("MQTT", "Received MQTT message: " + message);

                        try {
                            JSONObject jsonObject = new JSONObject(message);

                            String temp = jsonObject.optString("temp");
                            String heart = jsonObject.optString("heart");
                            String spo2 = jsonObject.optString("spo2");
                            String step = jsonObject.optString("step");
                            int tumble = jsonObject.optInt("tumble");
                            int warning = jsonObject.optInt("warning");

                            TextView waterStatusTextView = findViewById(R.id.textViewWater);
                            TextView heartTextView = findViewById(R.id.textViewHeart);
                            TextView spo2StatusTextView = findViewById(R.id.textViewSpo2);
                            TextView stepStatusTextView = findViewById(R.id.textViewstep);
                            TextView tumbleStatusTextView = findViewById(R.id.Tumble);
                            TextView warningStatusTextView = findViewById(R.id.Warning);

                            String warningStatus = "正常";
                            switch (warning) {
                                case 0:
                                    warningStatus = "正常";
                                    break;
                                case 2:
                                    warningStatus = "温度过高";
                                    break;
                                case 3:
                                    warningStatus = "摔倒";
                                    break;
                                case 4:
                                    warningStatus = "心率过高";
                                    break;
                                case 5:
                                    warningStatus = "血氧过高";
                                    break;
                                default:
                                    warningStatus = "未知状态";
                                    break;
                            }

                            String tumbleStatus = (tumble == 1) ? "是" : "否";

                            waterStatusTextView.setText(temp + " ℃");
                            heartTextView.setText(heart);
                            spo2StatusTextView.setText(spo2);
                            stepStatusTextView.setText(step);
                            tumbleStatusTextView.setText(tumbleStatus);
                            warningStatusTextView.setText(warningStatus);
                        } catch (JSONException e) {
                            Log.e("JSON", "Failed to parse MQTT message: " + e.getMessage());
                            e.printStackTrace();
                        }
                        break;

                    case 30:
                        Toast.makeText(MonitorActivity.this, "MQTT 连接失败", Toast.LENGTH_SHORT).show();
                        break;

                    case 31:
                        Toast.makeText(MonitorActivity.this, "MQTT 连接成功", Toast.LENGTH_SHORT).show();
                        try {
                            client.subscribe(sub_topic, 1);
                        } catch (MqttException e) {
                            Log.e("MQTT", "Subscribe failed: " + e.getMessage());
                            e.printStackTrace();
                        }
                        break;

                    default:
                        break;
                }
            }
        };

        radioGroup = findViewById(R.id.radio_group);
        radioGroup.setOnCheckedChangeListener(new RadioGroup.OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(RadioGroup group, int checkedId) {
                selectedRadioButton = findViewById(checkedId);
                String selectedOption = selectedRadioButton.getText().toString();

                if ("主界面".equals(selectedOption)) {
                    statusCode4 = 0;
                } else if ("数据界面".equals(selectedOption)) {
                    statusCode4 = 1;
                } else if ("阈值设置".equals(selectedOption)) {
                    statusCode4 = 2;
                }
            }
        });
    }

    /**
     * Jump to health report page.
     */
    public void onStatisticsClicked(View view) {
        Intent intent = new Intent(this, HealthReportActivity.class);
        startActivity(intent);
    }

    /**
     * Publish control command or threshold settings to device.
     */
    public void onSendButtonClicked(View view) throws JSONException {
        EditText highestHeartEditText = findViewById(R.id.highestHeartEditText2);
        EditText highestSpo2EditText = findViewById(R.id.highestSpo2EditText2);
        EditText highestTemp1EditText1 = findViewById(R.id.highestTemp1EditText1);

        String heart = highestHeartEditText.getText().toString();
        String spo2 = highestSpo2EditText.getText().toString();
        String temp1 = highestTemp1EditText1.getText().toString();

        JSONObject jsonObject = new JSONObject();

        if (statusCode4 == 0) {
            jsonObject.put("mode", "0,");
        } else if (statusCode4 == 1) {
            jsonObject.put("mode", "1,");
        } else if (statusCode4 == 2) {
            jsonObject.put("mode", "2,");
            jsonObject.put("temp_max", temp1 + ",");
            jsonObject.put("heart_max", heart + ",");
            jsonObject.put("spo2_max", spo2 + ",");
        }

        publish_message(jsonObject.toString());
    }

    /**
     * Initialize MQTT client for Aliyun IoT platform.
     */
    private void mqtt_init() {
        try {
            // TODO: replace with your Aliyun IoT device triplet.
            String productKey = "your_product_key";
            String deviceName = "your_device_name";
            String deviceSecret = "your_device_secret";
            String clientId = "APPDUAN";

            Map<String, String> params = new HashMap<>(16);
            params.put("productKey", productKey);
            params.put("deviceName", deviceName);
            params.put("clientId", clientId);
            String timestamp = String.valueOf(System.currentTimeMillis());
            params.put("timestamp", timestamp);

            String hostUrl = "tcp://" + productKey + ".iot-as-mqtt.cn-shanghai.aliyuncs.com:1883";
            String clientIdWithParams = clientId
                    + "|securemode=2,signmethod=hmacsha1,timestamp=" + timestamp + "|";
            String userName = deviceName + "&" + productKey;
            String password = AliyunIoTSignUtil.sign(params, deviceSecret, "hmacsha1");

            Log.d("MQTT", "Broker: " + hostUrl);
            Log.d("MQTT", "Client ID: " + clientIdWithParams);

            client = new MqttClient(hostUrl, clientIdWithParams, new MemoryPersistence());

            options = new MqttConnectOptions();
            options.setCleanSession(false);
            options.setUserName(userName);
            options.setPassword(password.toCharArray());
            options.setConnectionTimeout(10);
            options.setKeepAliveInterval(60);

            client.setCallback(new MqttCallback() {
                @Override
                public void connectionLost(Throwable cause) {
                    Log.w("MQTT", "Connection lost: " + cause.getMessage());
                }

                @Override
                public void deliveryComplete(IMqttDeliveryToken token) {
                    Log.d("MQTT", "Delivery complete: " + token.isComplete());
                }

                @Override
                public void messageArrived(String topic, MqttMessage message) throws Exception {
                    Log.d("MQTT", "Message arrived, topic: " + topic);
                    Message msg = new Message();
                    msg.what = 3;
                    msg.obj = message.toString();
                    handler.sendMessage(msg);
                }
            });
        } catch (Exception e) {
            Log.e("MQTT", "MQTT init failed: " + e.getMessage());
            e.printStackTrace();
        }
    }

    /**
     * Connect MQTT in a background thread.
     */
    private void mqtt_connect() {
        new Thread(() -> {
            try {
                if (client != null && !client.isConnected()) {
                    client.connect(options);
                    handler.sendEmptyMessage(31);
                }
            } catch (Exception e) {
                Log.e("MQTT", "MQTT connect failed: " + e.getMessage());
                e.printStackTrace();
                handler.sendEmptyMessage(30);
            }
        }).start();
    }

    /**
     * Check connection status every 10 seconds and reconnect if needed.
     */
    private void start_reconnect() {
        scheduler = Executors.newSingleThreadScheduledExecutor();
        scheduler.scheduleAtFixedRate(() -> {
            if (client != null && !client.isConnected()) {
                mqtt_connect();
            }
        }, 0, 10, TimeUnit.SECONDS);
    }

    /**
     * Publish message to Aliyun IoT platform.
     */
    private void publish_message(String message) {
        if (client == null || !client.isConnected()) {
            Log.w("MQTT", "Client is not connected, cannot publish message.");
            return;
        }

        MqttMessage mqttMessage = new MqttMessage();
        mqttMessage.setPayload(message.getBytes(StandardCharsets.UTF_8));

        try {
            client.publish(pub_topic, mqttMessage);
            Log.d("MQTT", "Message published: " + message);
        } catch (MqttException e) {
            Log.e("MQTT", "Publish failed: " + e.getMessage());
            e.printStackTrace();
        }
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        try {
            if (scheduler != null) {
                scheduler.shutdownNow();
            }
            if (client != null && client.isConnected()) {
                client.disconnect();
            }
        } catch (Exception e) {
            Log.e("MQTT", "Resource release failed: " + e.getMessage());
        }
    }
}
