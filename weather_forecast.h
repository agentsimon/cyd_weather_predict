#ifndef WEATHER_FORECAST_H
#define WEATHER_FORECAST_H

#include "globals.h"

/*
  weather_forecast.h
  ------------------
  Fetches the next WEATHER_FORECAST_HOURS hours (starting with the
  current hour) of predicted hourly rain, humidity, pressure and wind
  from Open-Meteo's free, keyless forecast API in a single call, so
  tapping any of the four stat rows on the Current Conditions screen can
  show that metric's predicted trend as a point/line graph.
  `forecast_days=2` guarantees the window is covered even late in the
  (UTC) day, when today's remaining hours alone would run out.

  Open-Meteo's hourly timestamps default to UTC, which matches the
  board's own NTP-synced clock directly - no timezone conversion needed
  to find "now" in the data, only to format the on-screen time labels
  as Da Nang local time.

  Rain comes back in mm and is stored in cm (mm / 10) to match the
  Current Conditions screen's units; humidity, pressure and wind speed
  are stored as Open-Meteo returns them (%, hPa, km/h).
*/

bool fetchWeatherForecast() {
  Serial.println("=== fetchWeatherForecast() ===");
  weatherForecastCount = 0;

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi not connected, attempting reconnect...");
    WiFi.reconnect();
    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < 10000) {
      delay(300);
    }
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("Still not connected, aborting weather forecast fetch.");
      return false;
    }
  }

  struct tm timeinfo;
  if (!getLocalTime(&timeinfo, 2000)) {
    Serial.println("NTP time not available yet - aborting weather forecast fetch.");
    return false;
  }

  // The current hour, UTC, in the same "YYYY-MM-DDTHH:00" format
  // Open-Meteo's hourly.time array uses - this is what we search for
  // below to find "now" in the response.
  char nowKey[18];
  snprintf(nowKey, sizeof(nowKey), "%04d-%02d-%02dT%02d:00",
           timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday, timeinfo.tm_hour);

  HTTPClient http;
  String url = "https://api.open-meteo.com/v1/forecast?latitude=" + String(WEATHER_LAT) +
               "&longitude=" + String(WEATHER_LON) +
               "&hourly=precipitation,relative_humidity_2m,pressure_msl,wind_speed_10m" +
               "&forecast_days=2";

  Serial.print("Requesting URL: ");
  Serial.println(url);

  http.begin(secureClient, url);
  int httpCode = http.GET();
  Serial.printf("HTTP response code: %d\n", httpCode);

  if (httpCode != 200) {
    Serial.println("Non-200 response, dumping body for debugging:");
    Serial.println(http.getString());
    http.end();
    return false;
  }

  String payload = http.getString();
  http.end();

  Serial.printf("Payload length: %d bytes\n", payload.length());

  JSONVar doc = JSON.parse(payload);
  if (JSON.typeof(doc) == "undefined") {
    Serial.println("Weather forecast JSON parse FAILED");
    return false;
  }

  JSONVar hourly    = doc["hourly"];
  JSONVar times     = hourly["time"];
  JSONVar precip    = hourly["precipitation"];
  JSONVar humid     = hourly["relative_humidity_2m"];
  JSONVar pressure  = hourly["pressure_msl"];
  JSONVar wind      = hourly["wind_speed_10m"];

  if (JSON.typeof(times) != "array" || JSON.typeof(precip) != "array" ||
      JSON.typeof(humid) != "array" || JSON.typeof(pressure) != "array" ||
      JSON.typeof(wind) != "array") {
    Serial.println("Weather forecast JSON missing one or more hourly arrays.");
    return false;
  }

  int n = times.length();

  // Find the index matching the current UTC hour.
  int nowIdx = -1;
  for (int i = 0; i < n; i++) {
    JSONVar tVar = times[i];
    String t = (JSON.typeof(tVar) == "string") ? (const char*)tVar : "";
    if (t == nowKey) { nowIdx = i; break; }
  }

  if (nowIdx < 0) {
    Serial.println("Could not find the current hour in Open-Meteo's hourly data.");
    return false;
  }

  // The forecast window: the current hour plus the following
  // (WEATHER_FORECAST_HOURS - 1) hours, clipped to what the API returned.
  int endIdx = nowIdx + (WEATHER_FORECAST_HOURS - 1);
  if (endIdx > n - 1) endIdx = n - 1;

  for (int i = nowIdx; i <= endIdx && weatherForecastCount < WEATHER_FORECAST_MAX_POINTS; i++) {
    JSONVar tVar = times[i];
    String t = (JSON.typeof(tVar) == "string") ? (const char*)tVar : "";

    // t is "YYYY-MM-DDTHH:00" in UTC - convert the hour to Da Nang local
    // for the axis label (the date part doesn't matter for a short window).
    int hourUTC = (t.length() >= 13) ? t.substring(11, 13).toInt() : 0;
    int hourLocal = ((hourUTC + DANANG_UTC_OFFSET_MIN / 60) % 24 + 24) % 24;
    char label[6];
    snprintf(label, sizeof(label), "%02d:00", hourLocal);

    double mm = (double)precip[i];

    rainForecastCm[weatherForecastCount]      = (float)(mm / 10.0);
    humidityForecastPct[weatherForecastCount] = (float)(double)humid[i];
    pressureForecastHpa[weatherForecastCount] = (float)(double)pressure[i];
    windForecastKmh[weatherForecastCount]     = (float)(double)wind[i];
    weatherForecastTime[weatherForecastCount] = String(label);
    weatherForecastCount++;
  }

  Serial.printf("Weather forecast points parsed: %d\n", weatherForecastCount);
  return weatherForecastCount > 0;
}

#endif // WEATHER_FORECAST_H
