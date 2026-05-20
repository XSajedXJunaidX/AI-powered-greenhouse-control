import streamlit as st
import serial
import serial.tools.list_ports
import json
import time
import anthropic

# ------------------------------------------------------------------ #
#  Page Config
# ------------------------------------------------------------------ #
st.set_page_config(
    page_title="Smart Plant Monitor",
    page_icon="🌱",
    layout="wide"
)
st.title("🌱 Smart Plant Monitor")

# ------------------------------------------------------------------ #
#  AI Plant Advisor — runs once when plant type is submitted
# ------------------------------------------------------------------ #
def get_plant_requirements(plant_name: str) -> dict:
    """Ask Claude for optimal growing conditions for the given plant."""
    client = anthropic.Anthropic()

    prompt = f"""You are a plant care expert. For the plant "{plant_name}", 
return ONLY a JSON object with no extra text, no markdown, no explanation.
The JSON must have exactly these keys:
{{
  "temp_min": <minimum optimal temperature in Celsius, integer>,
  "temp_max": <maximum optimal temperature in Celsius, integer>,
  "humidity_min": <minimum optimal humidity percentage, integer>,
  "humidity_max": <maximum optimal humidity percentage, integer>,
  "soil_min": <minimum optimal soil moisture percentage, integer>,
  "soil_max": <maximum optimal soil moisture percentage, integer>,
  "notes": "<one sentence care tip>"
}}"""

    message = client.messages.create(
        model="claude-sonnet-4-20250514",
        max_tokens=256,
        messages=[{"role": "user", "content": prompt}]
    )

    raw = message.content[0].text.strip()
    return json.loads(raw)


def evaluate(value, min_val, max_val, label, unit,
             low_msg, high_msg):
    """Return a status string and Streamlit status level."""
    if value < min_val:
        return "warning", f"⚠️ {label} too low ({value}{unit}) — {low_msg}"
    elif value > max_val:
        return "warning", f"⚠️ {label} too high ({value}{unit}) — {high_msg}"
    else:
        return "success", f"✅ {label} is optimal ({value}{unit})"


# ------------------------------------------------------------------ #
#  Sidebar — Plant Selection
# ------------------------------------------------------------------ #
with st.sidebar:
    st.header("🌿 Plant Settings")
    plant_input = st.text_input("Enter plant type", placeholder="e.g. Tomato")
    plant_button = st.button("Get Requirements")

    if plant_button and plant_input:
        with st.spinner(f"Looking up {plant_input} requirements..."):
            try:
                reqs = get_plant_requirements(plant_input)
                st.session_state.plant_name = plant_input
                st.session_state.requirements = reqs
                st.success("Requirements loaded!")
            except Exception as e:
                st.error(f"Could not get requirements: {e}")

    if "requirements" in st.session_state:
        reqs = st.session_state.requirements
        st.markdown("---")
        st.markdown(f"**Plant:** {st.session_state.plant_name}")
        st.markdown(f"🌡️ Temp: {reqs['temp_min']}–{reqs['temp_max']} °C")
        st.markdown(f"💧 Humidity: {reqs['humidity_min']}–{reqs['humidity_max']} %")
        st.markdown(f"🪴 Soil: {reqs['soil_min']}–{reqs['soil_max']} %")
        st.markdown(f"📝 {reqs['notes']}")

# ------------------------------------------------------------------ #
#  Main Panel — Live Sensor Data
# ------------------------------------------------------------------ #
ports = [p.device for p in serial.tools.list_ports.comports()]

if not ports:
    st.error("No COM ports found. Make sure HC-05 is paired.")
    st.stop()

selected_port = st.selectbox("Select Bluetooth COM Port", ports)

col1, col2, col3 = st.columns(3)
temp_display  = col1.empty()
humi_display  = col2.empty()
soil_display  = col3.empty()

advice_box    = st.empty()
status_box    = st.empty()

if st.button("Start Monitoring"):
    try:
        ser = serial.Serial(selected_port, 9600, timeout=3)
        status_box.success(f"Connected on {selected_port}")

        while True:
            try:
                line = ser.readline().decode("utf-8").strip()
                if not line:
                    status_box.warning("Waiting for data...")
                    continue

                data = json.loads(line)
                temp = data["temp"]
                humi = data["humidity"]
                soil = data["soil"]

                # --- Metrics ---
                temp_display.metric("🌡️ Temperature", f"{temp} °C")
                humi_display.metric("💧 Humidity",    f"{humi} %")
                soil_display.metric("🪴 Soil Moisture", f"{soil} %")

                # --- Plant Advice ---
                if "requirements" in st.session_state:
                    reqs = st.session_state.requirements
                    advices = []

                    for level, msg in [
                        evaluate(temp, reqs["temp_min"], reqs["temp_max"],
                                 "Temperature", "°C",
                                 "warm up the environment",
                                 "cool down the environment"),
                        evaluate(humi, reqs["humidity_min"], reqs["humidity_max"],
                                 "Humidity", "%",
                                 "increase humidity",
                                 "improve ventilation"),
                        evaluate(soil, reqs["soil_min"], reqs["soil_max"],
                                 "Soil moisture", "%",
                                 "water your plant",
                                 "reduce watering"),
                    ]:
                        advices.append((level, msg))

                    with advice_box.container():
                        st.markdown(f"### Advice for {st.session_state.plant_name}")
                        for level, msg in advices:
                            if level == "success":
                                st.success(msg)
                            else:
                                st.warning(msg)
                else:
                    advice_box.info(
                        "Enter a plant type in the sidebar to get care advice.")

                status_box.success(
                    f"Last updated: {time.strftime('%H:%M:%S')}")

            except json.JSONDecodeError:
                continue

    except serial.SerialException as e:
        st.error(f"Could not open port: {e}")