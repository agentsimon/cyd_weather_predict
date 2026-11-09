# cyd_weather_predict
CYD (JC2432W328) - METAR Weather Display
  --------------------------------------------------
  Fetches current conditions for a METAR station (default: VVDN, Da Nang
  Airport) from aviationweather.gov's free, keyless Data API. Shows a
  main menu; touch "Current Weather" for temperature, wind, pressure,
  calculated humidity, rain condition, and the current time, or
  "Ephemeris" for planetary positions. Touch anywhere to return to the
  menu. Data refetches every 30 minutes in the background.

  SKETCH LAYOUT: split across multiple files in this same folder -
  Arduino compiles them all together as one sketch.
    globals.h    - shared includes, constants, enums, extern globals,
                    and cross-file function prototypes (read this first)
    touch.h      - CST820 capacitive touch (I2C)
    wifi_setup.h - WiFiManager captive-portal connection
    location.h   - METAR station persistence + on-board web config form
    weather.h    - fetches/parses the METAR from aviationweather.gov
    ephemeris.h  - fetches planetary positions from FreeAstroAPI
    moon.h       - local moon phase calculation + icon
    kite_alert.h - wind-threshold email alert (Gmail SMTP)
    display.h    - all TFT drawing (menu, screens, weather icons)
    secrets.h    - station default, API keys, email settings (fill in)
  This file just defines the shared global objects/variables and holds
  setup()/loop().
