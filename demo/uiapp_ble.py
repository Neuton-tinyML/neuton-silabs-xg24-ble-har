import pickle
import cv2
import matplotlib.pyplot as plt
import matplotlib.patches as patche
import numpy
from enum import Enum
from matplotlib import animation
from matplotlib import style
import serial 
import matplotlib.font_manager as fm
from datetime import datetime

import signal
import time
import asyncio
from bleak import BleakScanner, BleakClient
from threading import Thread

import sys
from PIL import Image, ImageDraw, ImageFont, ImageColor

import matplotlib.pyplot as plt

import os


class Activity(Enum):
    UNKNOWN = 1
    SCREWDRIVER = 2
    BRUSHING_HAIR = 3
    HAND_WAVING = 4
    WASHING_HANDS = 5
    CLAPPING = 6
    WIPING_TABLE = 7
    IDLE = 8

class State:
    def __init__(self, current_activity = Activity.UNKNOWN, update_image = True) -> None:
        self.current_activity = current_activity
        self.update_image = update_image

################################################################################################

def resize_icon_image(image, resize_percent = 45):
    # Calculate the new dimensions
    original_width, original_height = image.size
    new_width = int(original_width * resize_percent / 100)
    new_height = int(original_height * resize_percent / 100)

    return image.resize((new_width, new_height))

################################################################################################

dir_path = os.path.dirname(os.path.abspath(__file__))
material_path = os.path.join(dir_path, "assets")

print(material_path)

state_string_names = dict()
state_string_names[Activity.UNKNOWN]        = "UNKNOWN ACTIVITY"
state_string_names[Activity.SCREWDRIVER]    = "USING SCREWDRIVER"
state_string_names[Activity.BRUSHING_HAIR]  = "BRUSHIGN HAIR"
state_string_names[Activity.HAND_WAVING]    = "HAND WAVING"
state_string_names[Activity.WASHING_HANDS]  = "WASHING HANDS"
state_string_names[Activity.CLAPPING]       = "CLAPPING"
state_string_names[Activity.WIPING_TABLE]   = "WIPING TABLE"
state_string_names[Activity.IDLE]           = "NO MOVEMENTS"

main_img                = Image.open(os.path.join(material_path, "Background.png"))
icon_background_img       = Image.open(os.path.join(material_path, "IconBackground.png"))

activity_idle_img = resize_icon_image(Image.open(os.path.join(material_path, "Idle.png")))
activity_unknown_img = resize_icon_image(Image.open(os.path.join(material_path, "Unknown.png")))
activity_screwdriver_img = resize_icon_image(Image.open(os.path.join(material_path, "Screwdriver.png")))
activity_brushing_hair_img = resize_icon_image(Image.open(os.path.join(material_path, "BrushingHair.png")))
activity_waving_img = resize_icon_image(Image.open(os.path.join(material_path, "Waving.png")))
activity_washing_hands_img = resize_icon_image(Image.open(os.path.join(material_path, "WashingHands.png")))
activity_clapping_img = resize_icon_image(Image.open(os.path.join(material_path, "Clapping.png")))
activity_wiping_img = resize_icon_image(Image.open(os.path.join(material_path, "WipingTable.png")))

state = State()

# Initialize an empty list to store lines and timestamps
model_inferences_str_buffer = []

################################################################################################

def prepare_image(current_activity = Activity.UNKNOWN, update_image = False):

    if update_image == True:

        icon_with_background = icon_background_img.copy()
        images = dict()
        images[Activity.UNKNOWN]        = activity_unknown_img
        images[Activity.SCREWDRIVER]    = activity_screwdriver_img
        images[Activity.BRUSHING_HAIR]  = activity_brushing_hair_img
        images[Activity.HAND_WAVING]    = activity_waving_img
        images[Activity.WASHING_HANDS]  = activity_washing_hands_img
        images[Activity.CLAPPING]       = activity_clapping_img
        images[Activity.WIPING_TABLE]   = activity_wiping_img
        images[Activity.IDLE]           = activity_idle_img

        current_activity_image = images[current_activity]

        # Define the position where you want to place the icon on the background
        icon_x_position = 165
        icon_y_position = 20 

        # Paste the icon onto the background image at the specified position
        icon_with_background.paste(current_activity_image, (icon_x_position, icon_y_position), current_activity_image)

        background_x_position = 18
        background_y_position = 117

        main_img.paste(icon_with_background, (background_x_position, background_y_position), icon_with_background)

    return main_img

################################################################################################

def get_image(current_activity = Activity.UNKNOWN, update_image = False):
    prepared_image = prepare_image(current_activity, update_image)
    return prepared_image

################################################################################################

async def discover_services(device_name, characteristic_uuid):
    scanner = BleakScanner()

    # Scan for devices
    devices = await scanner.discover()
    for device in devices:
        if device.name == device_name:
            print("{0} device found".format(device_name))
            async with BleakClient(device) as client:
                await client.is_connected()
                print("Device Connected!")

                services = await client.get_services()

                for service in services:
                    for char in service.characteristics:
                        if char.uuid == characteristic_uuid:
                            print("Neuton characteristic found")
                            print("Ready to work")
                            await start_listening(client, char)
                            return

################################################################################################

async def start_listening(client, characteristic):
    def notification_handler(sender: int, data: bytearray):
        # This function will be called when notifications/indications are received
        # The received data will be available in the 'data' parameter.

        ble_str = data.decode('utf-8').strip()
        print(ble_str)

        predicted_class_str = str(ble_str.split(",")[0])
        probability_str = ble_str.split(",")[1].strip()

        class_label_raw = int(predicted_class_str) + 1 # Lable starts from 1 but on device starts from 0
        prob_percentage = int(probability_str)

        class_label = Activity(class_label_raw)
        class_name = state_string_names[class_label]

        print(class_label)

        state.update_image = False if state.current_activity == class_label and class_label == Activity.UNKNOWN else True

        state.current_activity = class_label

        process_device_output_message(f"{class_name}, {prob_percentage}%")


    # Subscribe to notifications/indications for the characteristic
    await client.start_notify(characteristic.uuid, notification_handler)

    # Keep the program running to continue listening for data
    while True:
        await asyncio.sleep(5)

################################################################################################


def thread_ble():
    asyncio.run(discover_services("Neuton-ble-ctrl", "5b026510-4088-c297-46d8-be6c736a087a"))

################################################################################################

def show_realtime_inference_labels():
    y_offset = 22 * 3
    # Display the last 3 lines and timestamps as text labels in reverse order
    for i, (line, timestamp) in enumerate(reversed(model_inferences_str_buffer)):
        ax.text(30, 508 + y_offset, f"({timestamp}) {line}", color='yellow', size=12)
        y_offset -= 22

################################################################################################

def update_realtime_inference_labels(new_inference_string):
    # Format the timestamp as minutes:seconds:milliseconds
    timestamp = datetime.now().strftime("%M:%S:%f")[:-3]

    if len(model_inferences_str_buffer) == 3:
        # If the list has 3 lines, remove the oldest line
        model_inferences_str_buffer.pop(0)

            # Add the new line to the list
    model_inferences_str_buffer.append((new_inference_string, timestamp))

################################################################################################

def process_device_output_message(read_str):
    update_realtime_inference_labels(read_str) 

################################################################################################

def animate(i):

    image_to_show = get_image(state.current_activity, state.update_image)
    open_cv_image = numpy.array(image_to_show) 
    ax.clear()
    ax.imshow(open_cv_image)

    if state.current_activity != None:
        lable_str = state_string_names[state.current_activity]
        ha = 'center'
        color = 'black'
        ax.text(305, 405, lable_str, ha=ha, color=color, size=16)


    show_realtime_inference_labels()

    plt.axis('off')

################################################################################################

fig, ax = plt.subplots(1, figsize = (15,10))
ax.axis('off')

plt.box(False)
plt.axis('off')

ani = animation.FuncAnimation(fig, animate, interval=50)

def signal_handler(sig, frame):
        sys.exit(0)

signal.signal(signal.SIGINT, signal_handler)

x = Thread(target=thread_ble)
x.start()

plt.show()
